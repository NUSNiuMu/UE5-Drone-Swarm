// DroneActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AStarPathFinderComponent.h"
#include "ObstacleScannerComponent.h"
#include "PathModifierComponent.h"
#include "DroneImageCaptureComponent.h"
#include "DroneActor.generated.h"

UCLASS()
class DRONE_API ADroneActor : public AActor
{
    GENERATED_BODY()

public:
    ADroneActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 获取无人机ID
    UFUNCTION(BlueprintCallable, Category = "Drone")
    int32 GetDroneID() const { return DroneID; }

    // 设置无人机ID
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void SetDroneID(int32 NewID) { DroneID = NewID; }

    // 设置无人机速度
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void SetDroneSpeed(float NewSpeed) { DroneSpeed = NewSpeed; }

    // 获取路径规划组件
    UFUNCTION(BlueprintCallable, Category = "Drone")
    UAStarPathFinderComponent* GetPathFinder() const { return PathFinder; }

    // 获取障碍扫描组件
    UFUNCTION(BlueprintCallable, Category = "Drone")
    UObstacleScannerComponent* GetObstacleScanner() const { return ObstacleScanner; }

    // 获取路径修改组件
    UFUNCTION(BlueprintCallable, Category = "Drone")
    UPathModifierComponent* GetPathModifier() const { return PathModifier; }

    // 设置目标位置
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void SetGoalLocation(const FVector& NewGoal);

    // 获取目标位置
    UFUNCTION(BlueprintCallable, Category = "Drone")
    FVector GetGoalLocation() const { return GoalLocation; }

    // 开始沿路径移动
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void StartMovement();

    // 停止移动
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void StopMovement();

    // 从当前位置继续移动
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void ResumeMovementFromCurrentPosition();

    // 获取当前路径索引
    UFUNCTION(BlueprintCallable, Category = "Drone")
    int32 GetCurrentPathIndex() const { return CurrentPathIndex; }

    // 设置路径
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void SetPath(const TArray<FVector>& NewPath);

    // 处理路径更新
    UFUNCTION()
    void OnPathModified(const TArray<FVector>& NewPath);

    // 设置所有组件的GridMap引用
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void SetupGridMapReferences(UGridMapComponent* InGridMap);

    // 获取是否正在移动
    UFUNCTION(BlueprintCallable, Category = "Drone")
    bool IsMoving() const { return bIsMoving; }

    // 获取当前路径
    UFUNCTION(BlueprintCallable, Category = "Drone")
    const TArray<FVector>& GetCurrentPath() const { return CurrentPath; }

    // 获取无人机路径颜色
    FColor GetDronePathColor() const;

    // 切换无人机运动状态
    void SaveDonePathToFile();
    bool LoadDonePathFromFile();

    void ClearDrawnPath();

    TArray<FVector> DonePath;
    int32 CurrentPathIndex = 0;
    bool bIsMoving = false;
    TArray<FVector> CurrentPath;

    // 标记是否已保存到达终点的路径，防止重复保存
    UPROPERTY()
    bool bHasSavedDonePath = false;

protected:
    // 路径规划组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
    UAStarPathFinderComponent* PathFinder;

    // 障碍扫描组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
    UObstacleScannerComponent* ObstacleScanner;

    // 路径修改组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
    UPathModifierComponent* PathModifier;

    // 无人机网格体组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
    UStaticMeshComponent* DroneMesh;

    // 无人机ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
    int32 DroneID;

    // 目标位置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
    FVector GoalLocation;

    // 无人机速度（厘米/秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
    float DroneSpeed;

    // 路径线宽
    UPROPERTY(EditAnywhere, Category = "Path Visualization")
    float PathLineThickness = 5.0f;

    // 路径持续时间（秒，-1表示永久显示）
    UPROPERTY(EditAnywhere, Category = "Path Visualization")
    float PathDisplayDuration = -1.0f;

    // 图像捕获组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UDroneImageCaptureComponent* ImageCaptureComponent;

private:
    // 绘制已走过的路径
    void DrawDonePath();

    // 更新无人机位置
    void UpdateDronePosition(float DeltaTime);

    // 上一个记录的路径点
    FVector LastRecordedPoint;

}; 