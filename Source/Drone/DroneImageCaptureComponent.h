#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DroneImageCaptureComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DRONE_API UDroneImageCaptureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDroneImageCaptureComponent();
    
    UFUNCTION(BlueprintCallable, Category="Image Capture")
    FVector PixelToWorldWithDepth(const FVector2D& PixelCoord, int32 DroneID, const AActor* DroneActor) const;
    
    UFUNCTION(BlueprintCallable, Category="Image Capture")
    float GetDepthAtPixel(int32 X, int32 Y) const;
    
    UFUNCTION(BlueprintCallable, Category="Image Capture")
    void SaveDepthData(int32 ThisImageIndex);

    int32 GetDroneIDFromOwner() const;
    FString GetDroneSaveDirectory() const;

    static bool LoadDepthDataForDrone(int32 DroneID, int32 ImageIndex, TArray<float>& OutDepthData, int32& OutWidth, int32& OutHeight, const FString& SaveDirectory);

protected:
    virtual void BeginPlay() override;

    UPROPERTY()
    class USceneCaptureComponent2D* SceneCapture;

    UPROPERTY()
    class UTextureRenderTarget2D* DepthRenderTarget;  // 新增：深度RenderTarget

    UPROPERTY(EditAnywhere, Category="Image Capture")
    FString SaveDirectory = TEXT("D:/data/nm/picture cam");

    UPROPERTY(EditAnywhere, Category="Image Capture")
    float CaptureInterval = 1.0f; // 每0.5秒保存一次

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Image Capture")
    bool bEnableImageCapture = false;

    FTimerHandle CaptureTimerHandle;

    int32 ImageIndex = 0;

    TArray<float> DepthData;  // 新增：深度数据数组

    void CaptureAndSaveImage();

    void FindSceneCaptureComponent();
}; 