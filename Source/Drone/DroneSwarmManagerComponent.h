// DroneSwarmManagerComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DroneActor.h"
#include "GridMapComponent.h"
#include "DroneSwarmManagerComponent.generated.h"

// 单个无人机的路径规划任务
USTRUCT(BlueprintType)
struct FDronePathTask
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
    ADroneActor* DroneActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
    FVector StartLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
    FVector GoalLocation;

    UPROPERTY()
    TArray<FVector> PlannedPath;

    UPROPERTY()
    float StartTime;

    UPROPERTY()
    float EstimatedDuration;

    UPROPERTY()
    int32 Priority;

    FDronePathTask()
        : DroneActor(nullptr)
        , StartLocation(FVector::ZeroVector)
        , GoalLocation(FVector::ZeroVector)
        , StartTime(-1.0f)
        , EstimatedDuration(-1.0f)
        , Priority(0)
    {}
};


UCLASS(ClassGroup=(PathPlanning), meta=(BlueprintSpawnableComponent))
class DRONE_API UDroneSwarmManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDroneSwarmManagerComponent();

    // 添加一个无人机到群组
    UFUNCTION(BlueprintCallable, Category = "DroneSwarm")
    void AddDrone(ADroneActor* Drone, const FVector& GoalLocation);

    // 开始群体路径规划
    UFUNCTION(BlueprintCallable, Category = "DroneSwarm")
    void StartSwarmPathPlanning();

    // 开始所有无人机的运动
    UFUNCTION(BlueprintCallable, Category = "DroneSwarm")
    void StartAllDrones();

    // 停止所有无人机
    UFUNCTION(BlueprintCallable, Category = "DroneSwarm")
    void StopAllDrones();

    // 切换所有无人机运动状态
    UFUNCTION(BlueprintCallable, Category = "DroneSwarm")
    void ToggleAllDronesMovement();

    // 当前是否处于暂停状态
    bool bIsSwarmPaused = false;

    void ReplayAllDronesPath();

protected:
    virtual void BeginPlay() override;

private:
    // 存储所有无人机的任务
    TArray<FDronePathTask> DroneTasks;


    // 网格地图组件引用
    UPROPERTY()
    UGridMapComponent* GridMap;

    // Plan path for a single drone
    bool PlanPathForDrone(FDronePathTask& DroneTask);

    // Get prioritized task indices
    TArray<int32> GetPrioritizedTaskIndices();

    // Update task priorities
    void UpdateTaskPriorities();

}; 