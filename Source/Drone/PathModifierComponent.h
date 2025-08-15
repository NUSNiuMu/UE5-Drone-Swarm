#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PathModifierComponent.generated.h"

// 声明路径更新的委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPathModified, const TArray<FVector>&, NewPath);

class UGridMapComponent;
class UAStarPathFinderComponent;
class ADroneActor;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DRONE_API UPathModifierComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPathModifierComponent();

    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 设置原始路径（由A*设置）
    UFUNCTION(BlueprintCallable, Category = "Path")
    void SetPath(const TArray<FVector>& InPath);

    // 检查路径是否可行并自动修正
    UFUNCTION(BlueprintCallable)
    void CheckAndModifyPath();

    // 设置GridMap引用
    UFUNCTION(BlueprintCallable, Category="PathPlanning")
    void SetGridMap(UGridMapComponent* InGridMap);

    // 获取当前路径
    UFUNCTION(BlueprintCallable, Category="PathPlanning")
    const TArray<FVector>& GetCurrentPath() const { return CurrentPath; }

    // 路径修改时的委托
    UPROPERTY(BlueprintAssignable, Category="PathPlanning")
    FOnPathModified OnPathModified;

protected:
    // 当前路径
    TArray<FVector> CurrentPath;

    // 网格地图组件引用
    UPROPERTY()
    UGridMapComponent* GridMap;

    // A*寻路组件引用
    UPROPERTY()
    class UAStarPathFinderComponent* AStar;

    // 拥有者无人机引用
    UPROPERTY()
    class ADroneActor* OwnerDrone;

    // 无人机ID
    int32 DroneID;

    // 临时停止的计时器句柄
    FTimerHandle StopTimerHandle;

    // 临时停止的持续时间（秒）
    const float StopDuration = 0.5f;

    UFUNCTION()
    void OnGridUpdated(const FVector& UpdatedLocation);  // 接收网格更新通知

private:
    int32 CurrentPathIndex = 0;  // 添加当前路径索引变量
    bool bNeedsReplanning = false;  // 添加重规划标志
};
