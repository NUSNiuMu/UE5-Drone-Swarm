#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneSwarmManagerComponent.h"
#include "DroneActor.h"
#include "GridMapComponent.h"
#include "DroneSwarmTestActor.generated.h"

USTRUCT(BlueprintType)
struct FDroneFormationConfig
{
    GENERATED_BODY()

    // 队形的起始位置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Formation")
    FVector StartLocation = FVector::ZeroVector;

    // 队形的朝向（用于确定Y轴方向）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Formation")
    FRotator FormationRotation = FRotator::ZeroRotator;

    // 队形的宽度（沿Y轴）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Formation")
    float FormationWidth = 1200.0f;
};

USTRUCT(BlueprintType)
struct FDroneTargetConfig
{
    GENERATED_BODY()

    // 目标区域的中心位置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Target")
    FVector TargetCenter = FVector(900.0f, 1000.0f, 200.0f);

    // 目标区域的大小
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Target")
    FVector TargetAreaSize = FVector(1000.0f, 500.0f, 100.0f);

    // 是否使用随机目标位置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Target")
    bool bUseRandomTargets = true;

    // 如果不使用随机目标，每个无人机的固定目标位置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone Target", meta = (EditCondition = "!bUseRandomTargets"))
    TArray<FVector> FixedTargetLocations;
};

UCLASS()
class DRONE_API ADroneSwarmTestActor : public AActor
{
    GENERATED_BODY()

public:
    ADroneSwarmTestActor();

    virtual void BeginPlay() override;

    // 开始群体测试
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void StartSwarmTest();

    // 无人机预制体类
    UPROPERTY(EditDefaultsOnly, Category = "Drone")
    TSubclassOf<ADroneActor> DroneClass;

    // 要生成的无人机数量
    UPROPERTY(EditAnywhere, Category = "Drone")
    int32 NumDrones = 6;

    // 无人机之间的间距（厘米）
    UPROPERTY(EditAnywhere, Category = "Drone")
    float DroneSpacing = 300.0f;

    // 无人机生成高度
    UPROPERTY(EditAnywhere, Category = "Drone")
    float DroneHeight = 200.0f;

    // 无人机移动速度（厘米/秒）
    UPROPERTY(EditAnywhere, Category = "Drone", meta = (ClampMin = "0.0"))
    float DroneSpeed = 200.0f;

    // 无人机生成区域大小
    UPROPERTY(EditAnywhere, Category = "Drone")
    FVector SpawnAreaSize = FVector(1000.0f, 1000.0f, 300.0f);

    // 起始队形配置
    UPROPERTY(EditAnywhere, Category = "Drone")
    FDroneFormationConfig FormationConfig;

    // 目标区域配置
    UPROPERTY(EditAnywhere, Category = "Drone")
    FDroneTargetConfig TargetConfig;

    // 是否使用信标系统
    UPROPERTY(EditAnywhere, Category = "Beacon System")
    bool bUseBeaconSystem = true;

    // 起点信标Tag前缀（用于在场景中查找起点信标）
    UPROPERTY(EditAnywhere, Category = "Beacon System", meta = (EditCondition = "bUseBeaconSystem"))
    FString StartBeaconTagPrefix = "DroneStartBeacon_";

    // 终点信标Tag前缀（用于在场景中查找终点信标）
    UPROPERTY(EditAnywhere, Category = "Beacon System", meta = (EditCondition = "bUseBeaconSystem"))
    FString EndBeaconTagPrefix = "DroneEndBeacon_";

    // 地图外扩边距（厘米）
    UPROPERTY(EditAnywhere, Category = "Beacon System", meta = (EditCondition = "bUseBeaconSystem"))
    float MapMargin = 100.0f;

    // 网格地图原点
    UPROPERTY(EditAnywhere, Category = "GridMap")
    FVector GridMapOrigin = FVector(1000.0f, 1000.0f, 500.0f);

    // 网格地图大小
    UPROPERTY(EditAnywhere, Category = "GridMap")
    FVector GridMapSize = FVector(5000.0f, 5000.0f, 1000.0f);

    // 网格分辨率
    UPROPERTY(EditAnywhere, Category = "GridMap")
    float GridResolution = 50.0f;

    UPROPERTY(EditAnywhere, Category = "GridMap")
    float LocalGridResolution = 50.0f;

protected:
    // 群组管理器组件
    UPROPERTY()
    UDroneSwarmManagerComponent* SwarmManager;

    // 网格地图组件
    UPROPERTY()
    UGridMapComponent* GridMap;

    // 初始化网格地图
    void InitializeGridMap();

    // 添加圆柱形障碍物
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void AddObstacle(const FVector& Location, float Radius, float Height);

    // 在指定区域内获取随机位置
    FVector GetRandomPositionInArea(const FVector& Center, const FVector& Size);

private:
    // 存储生成的无人机
    UPROPERTY()
    TArray<ADroneActor*> SpawnedDrones;
}; 