// AStarPathFinderComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "GridMapComponent.h"
#include "AStarPathFinderComponent.generated.h"

// 定义碰撞检测回调函数类型
DECLARE_DELEGATE_RetVal_OneParam(bool, FCollisionCheckDelegate, const FVector&);

USTRUCT(BlueprintType)
struct FSpaceTimePoint
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PathPlanning")
    FVector Position;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PathPlanning")
    float AbsTime; // 绝对时间（相对于程序启动）

    FSpaceTimePoint() : Position(FVector::ZeroVector), AbsTime(0.0f) {}
    FSpaceTimePoint(const FVector& InPos, float InAbsTime) : Position(InPos), AbsTime(InAbsTime) {}
};

USTRUCT(BlueprintType)
struct FDroneReservation
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PathPlanning")
    TArray<FSpaceTimePoint> PathPoints;

    FDroneReservation() {}
    FDroneReservation(const TArray<FSpaceTimePoint>& InPoints) : PathPoints(InPoints) {}
};

UCLASS(ClassGroup=(PathPlanning), meta=(BlueprintSpawnableComponent))
class DRONE_API UAStarPathFinderComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // 设置默认值
    UAStarPathFinderComponent();
    
    // 寻找从起点到终点的路径
    UFUNCTION(BlueprintCallable, Category="PathPlanning|AStar")
    bool FindPath(const FVector& Start, const FVector& Goal, TArray<FVector>& OutPath, int32 DroneID);
    
    // 获取保存的路径
    UFUNCTION(BlueprintCallable, Category="PathPlanning|AStar")
    TArray<FVector> GetSearchedPath();
    
    // 可视化路径
    UFUNCTION(BlueprintCallable, Category="PathPlanning|AStar")
    void VisualizePath(float Duration = 5.0f);
    
    // 设置路径平滑度
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PathPlanning|AStar")
    float PathSmoothingFactor = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PathPlanning|AStar")
    float DroneSpeed = 100.0f;
    
    // 设置路径点间距
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PathPlanning|AStar")
    float PathPointSpacing = 100.0f;

    // 设置无人机尺寸
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PathPlanning|AStar")
    float DroneRadius = 10.0f;  // 无人机半径（厘米）
    
    // 设置安全距离系数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PathPlanning|AStar", meta=(ClampMin="1.0", ClampMax="3.0"))
    float SafetyFactor = 1.1f;  // 安全距离系数，实际安全距离 = DroneRadius * SafetyFactor

    // 设置GridMap引用
    UFUNCTION(BlueprintCallable, Category="PathPlanning|AStar")
    void SetGridMap(UGridMapComponent* InGridMap);

    // 设置碰撞检测回调
    void SetCollisionCheckCallback(const FCollisionCheckDelegate& Callback) { CollisionCheckCallback = Callback; }

    // 清除碰撞检测回调
    void ClearCollisionCheckCallback() { CollisionCheckCallback.Unbind(); }

    // 添加静态访问方法
    static void AddReservation(int32 DroneID, const FDroneReservation& Reservation);
    static const TMap<int32, FDroneReservation>& GetReservationTable();
    static void ClearReservationTable();

    // 生成预约表可视化
    UFUNCTION(BlueprintCallable, Category="PathPlanning|AStar")
    static void VisualizeReservationTable();

    // 获取预约表的字符串表示
    UFUNCTION(BlueprintCallable, Category="PathPlanning|AStar")
    static FString GetReservationTableString();

    UGridMapComponent* GetGridMap() const { return GridMap; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // 网格地图组件的引用
    UPROPERTY()
    UGridMapComponent* GridMap;
    
    // 存储的路径
    TArray<FVector> StoredPath;
    TArray<FVector> CurrentPath;
    
    // 启发式函数
    float GetDiagonalHeuristic(int X1, int Y1, int Z1, int X2, int Y2, int Z2);
    float GetManhattanHeuristic(int X1, int Y1, int Z1, int X2, int Y2, int Z2);
    float GetEuclideanHeuristic(int X1, int Y1, int Z1, int X2, int Y2, int Z2);
    
    // 计算两个节点间的距离
    float Distance(struct FAStarNode* A, struct FAStarNode* B);
    
    // 获取周围的临近节点（优化版本）
    void GetNeighborsOptimized(struct FAStarNode* Node, TArray<struct FAStarNode*>& OutNeighbors, int32 GoalX, int32 GoalY, int32 GoalZ);
    
    // 重建路径
    void ReconstructPath(struct FAStarNode* GoalNode, TArray<FVector>& OutPath);

    // 检查路径点是否安全
    bool IsPathPointSafe(const FVector& Point) const;

    // 获取路径点的安全距离
    float GetSafetyDistance() const { return DroneRadius * SafetyFactor; }

    // 检查两点之间是否可以直线到达
    bool CanReachDirectly(const FVector& Start, const FVector& Goal);

    // 平滑路径
    void SmoothPath(TArray<FVector>& InOutPath);

    FVector GetObstacleRepulsionForce(const FVector& Point);

    // 碰撞检测回调
    FCollisionCheckDelegate CollisionCheckCallback;

    // 检查点是否发生碰撞
    bool CheckCollision(const FVector& Point) const
    {
        return CollisionCheckCallback.IsBound() ? CollisionCheckCallback.Execute(Point) : false;
    }

    bool IsSpaceTimeConflict(const FVector& Position, float AbsTime, int32 SelfDroneID) const;
    
    // 将 ReservationTable 改为静态成员
    static TMap<int32, FDroneReservation> ReservationTable;

    float ProgramStartTime = 0.0f;

    // 辅助函数：将时间转换为字符串
    static FString TimeToString(float AbsTime);
    // 辅助函数：将位置转换为字符串
    static FString PositionToString(const FVector& Position);
};