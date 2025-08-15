#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DroneImageCaptureComponent.h"
#include <thread>
#include "TankDetectionReceiverComponent.generated.h"

USTRUCT(BlueprintType)
struct FTankDetection
{
    GENERATED_BODY();
    UPROPERTY(BlueprintReadOnly)
    int32 DroneId;
    UPROPERTY(BlueprintReadOnly)
    FString Label;
    UPROPERTY(BlueprintReadOnly)
    FVector2D PixelBoxMin;
    UPROPERTY(BlueprintReadOnly)
    FVector2D PixelBoxMax;
    UPROPERTY(BlueprintReadOnly)
    float Confidence;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DRONE_API UTankDetectionReceiverComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UTankDetectionReceiverComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    void ListenForUDP();
    void HandleDetectionMessage(const FString& Message);
    TArray<FTankDetection> LastDetections;
    class FSocket* ListenSocket;
    FThreadSafeBool bShouldListen;
    std::thread ListenStdThread;
    

    void UpdateDroneTarget(int32 DroneId, const FVector& WorldTarget);
public:
    UFUNCTION(BlueprintCallable)
    const TArray<FTankDetection>& GetLastDetections() const { return LastDetections; }
}; 