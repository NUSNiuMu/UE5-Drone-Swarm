#include "DroneImageCaptureComponent.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Logging/LogMacros.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"
#include "DroneActor.h"
#include "Async/Async.h"

UDroneImageCaptureComponent::UDroneImageCaptureComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UDroneImageCaptureComponent::BeginPlay()
{
    // Super::BeginPlay();
    // if (!bEnableImageCapture)
    // {
    //     UE_LOG(LogTemp, Log, TEXT("[ImageCapture] Component disabled by bEnableImageCapture=false"));
    //     return;
    // }
    // UE_LOG(LogTemp, Log, TEXT("[ImageCapture] BeginPlay called for %s"), *GetName());
    //
    // // 递归清空存储图片的文件夹内容，但不删除文件夹本身
    // FString CleanDirectory = "D:/data/nm/picture cam/BP_DroneActor_C_0";
    // if (!CleanDirectory.IsEmpty())
    // {
    //     TArray<FString> FilesAndDirs;
    //     IFileManager::Get().FindFilesRecursive(FilesAndDirs, *CleanDirectory, TEXT("*"), true, true, true);
    //     for (const FString& Path : FilesAndDirs)
    //     {
    //         if (IFileManager::Get().DirectoryExists(*Path))
    //         {
    //             IFileManager::Get().DeleteDirectory(*Path, false, true);
    //         }
    //         else
    //         {
    //             IFileManager::Get().Delete(*Path, false, true);
    //         }
    //     }
    //     UE_LOG(LogTemp, Log, TEXT("[ImageCapture] Cleared contents of save directory: %s"), *CleanDirectory);
    // }
    //
    // FindSceneCaptureComponent();
    // ImageIndex = 1;
    //
    // if (SceneCapture)
    // {
    //     // 设置采集分辨率和投影参数
    //     int32 ImageWidth = 512;
    //     int32 ImageHeight = 512;
    //     float FOV = 90.0f;
    //     float NearPlane = 10.0f;
    //     float FarPlane = 10000.0f;
    //
    //     // RGB RenderTarget
    //     UTextureRenderTarget2D* NewRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    //     NewRenderTarget->InitAutoFormat(ImageWidth, ImageHeight);
    //     NewRenderTarget->UpdateResourceImmediate(true);
    //     SceneCapture->TextureTarget = NewRenderTarget;
    //     
    //     // 深度RenderTarget - 使用PF_R32_FLOAT格式，专门用于深度捕获
    //     DepthRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    //     DepthRenderTarget->InitCustomFormat(ImageWidth, ImageHeight, PF_R32_FLOAT, false);
    //     DepthRenderTarget->RenderTargetFormat = RTF_R32f;
    //     DepthRenderTarget->ClearColor = FLinearColor::Black;
    //     DepthRenderTarget->UpdateResourceImmediate(true);
    //     
    //     // 只设置FOVAngle
    //     SceneCapture->FOVAngle = FOV;
    //     // 不设置bUseCustomProjectionMatrix和ProjectionMatrix
    //     
    //     // 优化场景捕获设置
    //     SceneCapture->bCaptureEveryFrame = false;
    //     SceneCapture->bCaptureOnMovement = false;
    //     
    //     GetWorld()->GetTimerManager().SetTimer(
    //         CaptureTimerHandle, this, &UDroneImageCaptureComponent::CaptureAndSaveImage, CaptureInterval, true
    //     );
    // }
}

void UDroneImageCaptureComponent::FindSceneCaptureComponent()
{
    AActor* Owner = GetOwner();
    if (Owner)
    {
        TArray<USceneCaptureComponent2D*> Components;
        Owner->GetComponents<USceneCaptureComponent2D>(Components);
        if (Components.Num() > 0)
        {
            SceneCapture = Components[0];
        }
    }
}

// 获取无人机ID（如果Owner是ADroneActor，否则返回-1）
int32 UDroneImageCaptureComponent::GetDroneIDFromOwner() const
{
    if (AActor* Owner = GetOwner())
    {
        if (const ADroneActor* Drone = Cast<ADroneActor>(Owner))
        {
            return Drone->GetDroneID();
        }
    }
    return -1;
}

// 获取唯一存储目录
FString UDroneImageCaptureComponent::GetDroneSaveDirectory() const
{
    int32 DroneID = GetDroneIDFromOwner();
    if (DroneID >= 0)
    {
        return SaveDirectory / FString::Printf(TEXT("BP_DroneActor_C_%d"), DroneID);
    }
    return SaveDirectory / (GetOwner() ? GetOwner()->GetName() : TEXT("UnknownDrone"));
}

void UDroneImageCaptureComponent::CaptureAndSaveImage()
{
    if (!SceneCapture || !DepthRenderTarget)
    {
        UE_LOG(LogTemp, Error, TEXT("[ImageCapture] SceneCapture or DepthRenderTarget is null!"));
        return;
    }

    FString DroneSaveDirectory = GetDroneSaveDirectory();
    IFileManager::Get().MakeDirectory(*DroneSaveDirectory, true);

    int32 ThisImageIndex = ImageIndex;
    int32 DroneID = GetDroneIDFromOwner();
    FString FileName = FString::Printf(TEXT("%s/Capture_%d.png"), *DroneSaveDirectory, ThisImageIndex);
    FString DepthFileName = DroneSaveDirectory / TEXT("depth") / FString::Printf(TEXT("Depth_%d.bin"), ThisImageIndex);
    // UE_LOG(LogTemp, Warning, TEXT("[ImageCapture] DroneID=%d Save ImageIndex=%d Path=%s"), DroneID, ThisImageIndex, *FileName);
    // UE_LOG(LogTemp, Warning, TEXT("[ImageCapture] DroneID=%d Save DepthIndex=%d Path=%s"), DroneID, ThisImageIndex, *DepthFileName);

    // 捕获深度
    SceneCapture->TextureTarget = DepthRenderTarget;
    SceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
    SceneCapture->bCaptureEveryFrame = false;
    SceneCapture->bCaptureOnMovement = false;
    SceneCapture->CaptureScene();
    FTextureRenderTargetResource* DepthResource = DepthRenderTarget->GameThread_GetRenderTargetResource();
    if (DepthResource)
    {
        TArray<FLinearColor> DepthPixels;
        FReadSurfaceDataFlags ReadFlags;
        bool bReadSuccess = DepthResource->ReadLinearColorPixels(DepthPixels, ReadFlags);
        if (!bReadSuccess)
        {
            TArray<FColor> RawPixels;
            DepthResource->ReadPixels(RawPixels, ReadFlags);
            if (RawPixels.Num() > 0)
            {
                UE_LOG(LogTemp, Log, TEXT("[ImageCapture] Successfully read %d raw pixels"), RawPixels.Num());
                DepthPixels.SetNum(RawPixels.Num());
                for (int32 i = 0; i < RawPixels.Num(); ++i)
                {
                    DepthPixels[i] = FLinearColor(RawPixels[i]);
                }
            }
        }
        if (DepthPixels.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("[ImageCapture] Failed to read any depth pixels!"));
        }
        DepthData.SetNum(DepthPixels.Num());
        for (int32 i = 0; i < DepthPixels.Num(); ++i)
        {
            DepthData[i] = DepthPixels[i].R;
        }
        float MinDepth = FLT_MAX, MaxDepth = -FLT_MAX;
        int32 ValidDepthCount = 0;
        int32 ZeroDepthCount = 0;
        int32 ExtremeDepthCount = 0;
        for (float D : DepthData)
        {
            if (FMath::IsFinite(D))
            {
                if (D == 0.0f) { ZeroDepthCount++; }
                else if (D > 0.0f && D < 1000.0f)
                {
                    ValidDepthCount++;
                    if (D < MinDepth) MinDepth = D;
                    if (D > MaxDepth) MaxDepth = D;
                }
                else { ExtremeDepthCount++; }
            }
        }
        // UE_LOG(LogTemp, Log, TEXT("[ImageCapture] ValidDepthCount: %d, ZeroDepthCount: %d, ExtremeDepthCount: %d, MinDepth: %.4f, MaxDepth: %.4f"), 
        //     ValidDepthCount, ZeroDepthCount, ExtremeDepthCount, MinDepth, MaxDepth);
    }
    // 捕获RGB图像
    UTextureRenderTarget2D* RGBRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    RGBRenderTarget->InitAutoFormat(512, 512);
    SceneCapture->TextureTarget = RGBRenderTarget;
    SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    SceneCapture->CaptureScene();
    FTextureRenderTargetResource* RenderTarget = SceneCapture->TextureTarget->GameThread_GetRenderTargetResource();
    if (!RenderTarget) return;
    TArray<FColor> OutBMP;
    FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
    RenderTarget->ReadPixels(OutBMP, ReadPixelFlags);
    int32 Width = SceneCapture->TextureTarget->SizeX;
    int32 Height = SceneCapture->TextureTarget->SizeY;
    TArray<uint8, FDefaultAllocator64> CompressedPNG;
    FImageUtils::PNGCompressImageArray(Width, Height, OutBMP, CompressedPNG);
    if (CompressedPNG.Num() == 0) return;
    // 保存图片和深度数据都用异步
    TArray<uint8, FDefaultAllocator64> PNGCopy = CompressedPNG;
    TArray<float> DepthCopy = DepthData;
    FString DepthSaveDir = DroneSaveDirectory / TEXT("depth");
    IFileManager::Get().MakeDirectory(*DepthSaveDir, true);
    FString DepthFileNameFull = FString::Printf(TEXT("%s/Depth_%d.bin"), *DepthSaveDir, ThisImageIndex);
    FString FileNameFull = FileName;
    Async(EAsyncExecution::ThreadPool, [PNGCopy, FileNameFull]() {
        FFileHelper::SaveArrayToFile(PNGCopy, *FileNameFull);
    });
    Async(EAsyncExecution::ThreadPool, [DepthCopy, DepthFileNameFull]() {
        FArchive* Writer = IFileManager::Get().CreateFileWriter(*DepthFileNameFull);
        if (Writer)
        {
            Writer->Serialize((void*)DepthCopy.GetData(), DepthCopy.Num() * sizeof(float));
            Writer->Close();
            delete Writer;
        }
    });
    ImageIndex++;
}

void UDroneImageCaptureComponent::SaveDepthData(int32 ThisImageIndex)
{
    // 已被异步保存替代，如需兼容可保留空实现
}

bool UDroneImageCaptureComponent::LoadDepthDataForDrone(int32 DroneID, int32 ImageIndex, TArray<float>& OutDepthData, int32& OutWidth, int32& OutHeight, const FString& SaveDirectory)
{
    FString DroneDir = SaveDirectory / FString::Printf(TEXT("BP_DroneActor_C_%d/depth"), DroneID);
    FString DepthFile = FString::Printf(TEXT("%s/Depth_%d.bin"), *DroneDir, ImageIndex);
    TArray<uint8> RawData;
    if (!FFileHelper::LoadFileToArray(RawData, *DepthFile))
        return false;
    int32 NumFloats = RawData.Num() / sizeof(float);
    OutDepthData.SetNumUninitialized(NumFloats);
    FMemory::Memcpy(OutDepthData.GetData(), RawData.GetData(), RawData.Num());
    OutWidth = 512;
    OutHeight = 512;
    // INSERT_YOUR_CODE
    // 统计有效值（大于0且小于1000的深度）
    int32 ValidCount = 0;
    for (float Depth : OutDepthData)
    {
        if (Depth > 0.0f && Depth < 1000.0f)
        {
            ++ValidCount;
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[ImageCapture] Loaded depth data for DroneID=%d ImageIndex=%d: Total=%d, Valid(0<d<1000)=%d"), DroneID, ImageIndex, OutDepthData.Num(), ValidCount);


    return true;
}

float UDroneImageCaptureComponent::GetDepthAtPixel(int32 X, int32 Y) const
{
    if (DepthData.Num() == 0 || X < 0 || Y < 0 || X >= 512 || Y >= 512)
        return -1.0f;
    int32 Index = Y * 512 + X;
    if (Index < DepthData.Num())
        return DepthData[Index];
        
    return -1.0f;
}

FVector UDroneImageCaptureComponent::PixelToWorldWithDepth(const FVector2D& PixelCoord, int32 DroneID, const AActor* DroneActor) const
{
    if (!SceneCapture || !SceneCapture->TextureTarget)
        return FVector::ZeroVector;
    int32 Width = SceneCapture->TextureTarget->SizeX;
    int32 Height = SceneCapture->TextureTarget->SizeY;
    float Depth = -1.0f;
    {
        int32 ThisImageIndex = ImageIndex - 1;
        FString DepthFileName = GetDroneSaveDirectory() / TEXT("depth") / FString::Printf(TEXT("Depth_%d.bin"), ThisImageIndex);
        UE_LOG(LogTemp, Warning, TEXT("[ImageCapture] DroneID=%d Read DepthIndex=%d Path=%s"), DroneID, ThisImageIndex, *DepthFileName);
        TArray<float> LoadedDepthData;
        if (LoadDepthDataForDrone(DroneID, ThisImageIndex, LoadedDepthData, Width, Height, SaveDirectory))
        {
            int32 X = FMath::RoundToInt(PixelCoord.X);
            int32 Y = FMath::RoundToInt(PixelCoord.Y);
            if (X >= 0 && Y >= 0 && X < Width && Y < Height)
            {
                int32 Index = Y * Width + X;
                if (LoadedDepthData.IsValidIndex(Index))
                {
                    Depth = LoadedDepthData[Index];
                    UE_LOG(LogTemp, Warning, TEXT("[ImageCapture] DroneID=%d Read Depth=%f"), DroneID, Depth)
                }
            }
        }
    }
    if (Depth <= 0.0f)
        return FVector::ZeroVector;
    float FOV = SceneCapture->FOVAngle;
    float fx = (Width / 2.0f) / FMath::Tan(FMath::DegreesToRadians(FOV / 2.0f));
    float fy = fx;
    float cx = Width / 2.0f;
    float cy = Height / 2.0f;
    float x = Depth;
    float y = (PixelCoord.X - cx) / fx * Depth;
    float z = -(PixelCoord.Y - cy) / fy * Depth;
    FVector CameraSpacePoint(x, y, z);
    FTransform CamTransform = SceneCapture->GetComponentTransform();
    FVector WorldPos = CamTransform.TransformPosition(CameraSpacePoint);
    FVector CamLocation = CamTransform.GetLocation();
    FRotator CamRotation = CamTransform.GetRotation().Rotator();
    UE_LOG(LogTemp, Warning, TEXT("  Final WorldPos: (%.2f, %.2f, %.2f)"), WorldPos.X, WorldPos.Y, WorldPos.Z);
    UE_LOG(LogTemp, Warning, TEXT("  Camera Location: (%.2f, %.2f, %.2f), Rotation: (Pitch=%.2f, Yaw=%.2f, Roll=%.2f)"), 
        CamLocation.X, CamLocation.Y, CamLocation.Z, CamRotation.Pitch, CamRotation.Yaw, CamRotation.Roll);
    return WorldPos;
}

