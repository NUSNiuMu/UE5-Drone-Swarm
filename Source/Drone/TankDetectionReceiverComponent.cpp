#include "TankDetectionReceiverComponent.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "DroneImageCaptureComponent.h"
#include "DroneActor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneCaptureComponent2D.h"
#include "EngineUtils.h"
#include <thread>
#include "Async/Async.h"

UTankDetectionReceiverComponent::UTankDetectionReceiverComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    ListenSocket = nullptr;
    bShouldListen = true;
}

void UTankDetectionReceiverComponent::BeginPlay()
{
    // Super::BeginPlay();
    // ListenSocket = FUdpSocketBuilder(TEXT("TankDetectionSocket"))
    //     .AsNonBlocking()
    //     .AsReusable()
    //     .BoundToPort(12345)
    //     .WithReceiveBufferSize(2 * 1024 * 1024);
    // if (ListenSocket)
    // {
    //     UE_LOG(LogTemp, Log, TEXT("[TankDetectionReceiver] UDP Socket created and bound to port 12345"));
    // }
    // else
    // {
    //     UE_LOG(LogTemp, Error, TEXT("[TankDetectionReceiver] Failed to create UDP Socket!"));
    // }
    // bShouldListen = true;
    // ListenStdThread = std::thread([this]() { ListenForUDP(); });
    // UE_LOG(LogTemp, Log, TEXT("[TankDetectionReceiver] Listen thread started"));
}

void UTankDetectionReceiverComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bShouldListen = false;
    if (ListenStdThread.joinable())
    {
        ListenStdThread.join();
        UE_LOG(LogTemp, Log, TEXT("[TankDetectionReceiver] Listen thread joined"));
    }
    if (ListenSocket)
    {
        ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
        ListenSocket = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[TankDetectionReceiver] UDP Socket closed and destroyed"));
    }
    Super::EndPlay(EndPlayReason);
}

void UTankDetectionReceiverComponent::ListenForUDP()
{
    UE_LOG(LogTemp, Log, TEXT("[TankDetectionReceiver] ListenForUDP thread started"));
    TArray<uint8> ReceivedData;
    uint32 Size;
    
    while (bShouldListen)
    {
        if (ListenSocket && ListenSocket->HasPendingData(Size))
        {
            ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));
            int32 Read = 0;
            ListenSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
            FString NewMessage = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())), Read);
            // UE_LOG(LogTemp, Warning, TEXT("[TankDetectionReceiver] Raw UDP message (len=%d): %s"), NewMessage.Len(), *NewMessage);

            // 只保留第二个]之前的内容
            int32 FirstBracket = -1;
            int32 SecondBracket = -1;
            int32 SearchStart = 0;
            for (int i = 0; i < 2; ++i)
            {
                int32 Found = NewMessage.Find(TEXT("]"), ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchStart);
                if (Found != INDEX_NONE)
                {
                    if (i == 0)
                        FirstBracket = Found;
                    else
                        SecondBracket = Found;
                    SearchStart = Found + 1;
                }
                else
                {
                    break;
                }
            }
            if (SecondBracket != -1)
            {
                // 截断到第二个]（包含第二个]）
                NewMessage = NewMessage.Left(SecondBracket + 1);
            }

            if (!NewMessage.IsEmpty())
            {
                AsyncTask(ENamedThreads::GameThread, [this, NewMessage]() {
                    HandleDetectionMessage(NewMessage);
                });
            }
        
        }
        FPlatformProcess::Sleep(0.01f);
    }
    UE_LOG(LogTemp, Log, TEXT("[TankDetectionReceiver] ListenForUDP thread exiting"));
}

void UTankDetectionReceiverComponent::HandleDetectionMessage(const FString& Message)
{
    // 重新处理消息，适配新的消息格式：[{"drone_id": 1, "label": "tank", "box": [274.87, 232.46, 350.87, 266.17]}]
    FString CleanMessage = Message;
    TArray<TSharedPtr<FJsonValue>> DetectionsArray;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CleanMessage);
    if (!FJsonSerializer::Deserialize(Reader, DetectionsArray))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TankDetectionReceiver] Failed to parse JSON array message: %s"), *CleanMessage);
        return;
    }

    LastDetections.Empty();
    for (const TSharedPtr<FJsonValue>& DetectionVal : DetectionsArray)
    {
        const TSharedPtr<FJsonObject>* DetectionObj;
        if (!DetectionVal->TryGetObject(DetectionObj)) continue;
        FTankDetection Detection;
        // drone_id
        if ((*DetectionObj)->HasField(TEXT("drone_id")))
        {
            Detection.DroneId = (*DetectionObj)->GetIntegerField(TEXT("drone_id"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[TankDetectionReceiver] No 'drone_id' in detection object"));
            continue;
        }
        // label
        if ((*DetectionObj)->HasField(TEXT("label")))
        {
            Detection.Label = (*DetectionObj)->GetStringField(TEXT("label"));
        }
        else
        {
            Detection.Label = TEXT("");
        }
        // box
        const TArray<TSharedPtr<FJsonValue>>* BoxArray;
        if ((*DetectionObj)->TryGetArrayField(TEXT("box"), BoxArray) && BoxArray->Num() == 4)
        {
            Detection.PixelBoxMin = FVector2D(
                static_cast<float>((*BoxArray)[0]->AsNumber()),
                static_cast<float>((*BoxArray)[1]->AsNumber())
            );
            Detection.PixelBoxMax = FVector2D(
                static_cast<float>((*BoxArray)[2]->AsNumber()),
                static_cast<float>((*BoxArray)[3]->AsNumber())
            );
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[TankDetectionReceiver] 'box' array missing or invalid for drone_id %d"), Detection.DroneId);
            continue;
        }
        // 新格式没有confidence字段，设为1.0
        Detection.Confidence = 1.0f;
        LastDetections.Add(Detection);
    }
    
    // 找到最近的坦克，使用深度信息更新对应无人机目标
    for (const FTankDetection& Detection : LastDetections)
    {
        FVector2D PixelCenter = (Detection.PixelBoxMin + Detection.PixelBoxMax) * 0.5f;
        FVector TankWorldPos = FVector::ZeroVector;
        bool bFoundDepth = false;
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            AActor* Actor = *It;
            if (UDroneImageCaptureComponent* ImageCapture = Actor->FindComponentByClass<UDroneImageCaptureComponent>())
            {
                TankWorldPos = ImageCapture->PixelToWorldWithDepth(PixelCenter, Detection.DroneId, Actor);
                if (TankWorldPos != FVector::ZeroVector)
                {
                    bFoundDepth = true;
                    UE_LOG(LogTemp, Log, TEXT("[TankDetectionReceiver] Using depth info for DroneId=%d: PixelCenter=(%.1f,%.1f) -> WorldPos=(%.1f,%.1f,%.1f)"),
                        Detection.DroneId, PixelCenter.X, PixelCenter.Y, TankWorldPos.X, TankWorldPos.Y, TankWorldPos.Z);
                    break;
                }
            }
        }
        if (!bFoundDepth)
        {
            UE_LOG(LogTemp, Warning, TEXT("[TankDetectionReceiver] No depth info found for DroneId=%d, skip updating target."), Detection.DroneId);
            continue;
        }
        UpdateDroneTarget(Detection.DroneId, TankWorldPos);
    }
    // 处理完后清空LastDetections，防止残留影响后续消息
    LastDetections.Empty();
    CleanMessage.Empty();
}

void UTankDetectionReceiverComponent::UpdateDroneTarget(int32 DroneId, const FVector& WorldTarget)
{
    bool bFound = false;
    for (TActorIterator<ADroneActor> It(GetWorld()); It; ++It)
    {
        ADroneActor* Drone = *It;
        if (Drone && Drone->GetDroneID() == DroneId)
        {
            Drone->SetGoalLocation(WorldTarget);
            UE_LOG(LogTemp, Log, TEXT("[TankDetectionReceiver] SetGoalLocation for DroneId=%d to (%.1f, %.1f, %.1f)"),
                DroneId, WorldTarget.X, WorldTarget.Y, WorldTarget.Z);
            bFound = true;
            break;
        }
    }
    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TankDetectionReceiver] No drone found with DroneId=%d to update target"), DroneId);
    }
}
