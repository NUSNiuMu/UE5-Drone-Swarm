// AStarPathFinderComponent.cpp
#include "AStarPathFinderComponent.h"
#include "DrawDebugHelpers.h"
#include "PathModifierComponent.h"

// Helper struct for A* algorithm
struct FAStarNode
{
    FVector Position;      // World position
    int GridX, GridY, GridZ; // Grid coordinates
    float GScore;          // Cost from start
    float FScore;          // Total cost (GScore + Heuristic)
    FAStarNode* Parent;    // Parent node
    
    FAStarNode(FVector InPos, int X, int Y, int Z)
        : Position(InPos), GridX(X), GridY(Y), GridZ(Z),
          GScore(FLT_MAX), FScore(FLT_MAX), Parent(nullptr)
    {
    }
    
    bool operator==(const FAStarNode& Other) const
    {
        return GridX == Other.GridX && GridY == Other.GridY && GridZ == Other.GridZ;
    }
};

// Node comparison operator for priority queue
struct FNodeCompare
{
    bool operator()(const FAStarNode& A, const FAStarNode& B) const
    {
        return A.FScore > B.FScore; // Lower FScore has higher priority
    }
};

// 添加最大搜索步数和超时时间常量
const int32 MAX_SEARCH_STEPS = 100000;
const float MAX_SEARCH_TIME = 1.0f; // 1秒超时

// 定义静态成员变量
TMap<int32, FDroneReservation> UAStarPathFinderComponent::ReservationTable;

// 实现静态方法
void UAStarPathFinderComponent::AddReservation(int32 DroneID, const FDroneReservation& Reservation)
{
    ReservationTable.Add(DroneID, Reservation);
    UE_LOG(LogTemp, Warning, TEXT("Static ReservationTable - Added DroneID: %d, Total Entries: %d"), DroneID, ReservationTable.Num());
}

const TMap<int32, FDroneReservation>& UAStarPathFinderComponent::GetReservationTable()
{
    return ReservationTable;
}

void UAStarPathFinderComponent::ClearReservationTable()
{
    ReservationTable.Empty();
    UE_LOG(LogTemp, Warning, TEXT("Static ReservationTable - Cleared"));
}

UAStarPathFinderComponent::UAStarPathFinderComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f;
    ProgramStartTime = FPlatformTime::Seconds();
}

// 新接口，带DroneID
bool UAStarPathFinderComponent::FindPath(const FVector& Start, const FVector& Goal, TArray<FVector>& OutPath, int32 DroneID)
{

    if (!GridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("AStarPathFinder: GridMap is not set"));
        return false;
    }

    StoredPath.Empty();
    OutPath.Empty();
    // Convert start and goal to grid coordinates
    int StartX, StartY, StartZ;
    int GoalX, GoalY, GoalZ;
    
    if (!GridMap->WorldToGrid(Start, StartX, StartY, StartZ))
    {
        UE_LOG(LogTemp, Warning, TEXT("AStarPathFinder: Start position is outside the grid"));
        return false;
    }
    
    if (!GridMap->WorldToGrid(Goal, GoalX, GoalY, GoalZ))
    {
        UE_LOG(LogTemp, Warning, TEXT("AStarPathFinder: Goal position is outside the grid"));
        return false;
    }
    
    // Check if start or goal is occupied
    if (GridMap->IsOccupied(Start))
    {
        UE_LOG(LogTemp, Warning, TEXT("AStarPathFinder: Start position is occupied"));
        return false;
    }
    
    if (GridMap->IsOccupied(Goal))
    {
        UE_LOG(LogTemp, Warning, TEXT("AStarPathFinder: Goal position is occupied"));
        return false;
    }
    // 使用优先队列替代TArray
    TArray<FAStarNode*> OpenSet;
    TArray<FAStarNode*> ClosedSet;
    TMap<int32, FAStarNode*> NodeMap; // 用于快速查找节点
    
    // 创建起始节点
    FAStarNode* StartNode = new FAStarNode(Start, StartX, StartY, StartZ);
    StartNode->GScore = 0;
    float InitialHScore = GetDiagonalHeuristic(StartX, StartY, StartZ, GoalX, GoalY, GoalZ);
    StartNode->FScore = InitialHScore;
    
    // 添加到开放集
    OpenSet.Add(StartNode);
    float SearchStartTime = GetWorld()->GetTimeSeconds();
    int32 Steps = 0;
    bool bFound = false;
    FAStarNode* GoalNode = nullptr;
    while (OpenSet.Num() > 0)
    {
        if (GetWorld()->GetTimeSeconds() - SearchStartTime > MAX_SEARCH_TIME)
        {
            for (FAStarNode* Node : OpenSet) delete Node;
            for (FAStarNode* Node : ClosedSet) delete Node;
            return false;
        }
        
        // 检查步数限制
        if (++Steps > MAX_SEARCH_STEPS)
        {
            UE_LOG(LogTemp, Warning, TEXT("AStarPathFinder: Exceeded max steps %d"), MAX_SEARCH_STEPS);
            // 清理内存
            for (FAStarNode* Node : OpenSet) delete Node;
            for (FAStarNode* Node : ClosedSet) delete Node;
            return false;
        }
        
        // 获取F值最小的节点
        int32 BestIndex = 0;
        float BestFScore = OpenSet[0]->FScore;
        for (int32 i = 1; i < OpenSet.Num(); i++)
        {
            if (OpenSet[i]->FScore < BestFScore)
            {
                BestFScore = OpenSet[i]->FScore;
                BestIndex = i;
            }
        }
        
        FAStarNode* Current = OpenSet[BestIndex];
        OpenSet.RemoveAt(BestIndex);
        if(Steps%10000==0)
        {UE_LOG(LogTemp, Log, TEXT("Step %d: Current node at grid (%d, %d, %d), world (%.2f, %.2f, %.2f), FScore=%.2f"), 
            Steps, Current->GridX, Current->GridY, Current->GridZ, 
            Current->Position.X, Current->Position.Y, Current->Position.Z, Current->FScore);}
        
        // 检查是否到达目标
        const float GoalTolerance = 1.0f; // 允许1个网格单位的误差
        float DistanceToGoal = FVector::Dist(
            FVector(Current->GridX, Current->GridY, Current->GridZ),
            FVector(GoalX, GoalY, GoalZ)
        );
        
        if (DistanceToGoal <= GoalTolerance)
        {
            // 如果非常接近目标，直接使用目标点作为终点
            if (DistanceToGoal > 0.0f)
            {
                FVector GoalWorldPos = GridMap->GridToWorld(GoalX, GoalY, GoalZ);
                Current->Position = GoalWorldPos;
                Current->GridX = GoalX;
                Current->GridY = GoalY;
                Current->GridZ = GoalZ;
            }
            GoalNode = Current;
            bFound = true;
            break;
        }
        
        // 添加到关闭集
        ClosedSet.Add(Current);
        
        // 获取邻居节点
        TArray<FAStarNode*> Neighbors;
        GetNeighborsOptimized(Current, Neighbors, GoalX, GoalY, GoalZ);
        
        // 处理邻居节点
        for (FAStarNode* Neighbor : Neighbors)
        {
            float SegmentDistance = FVector::Dist(Current->Position, Neighbor->Position);
            float RelativeTime = Current->GScore / DroneSpeed + SegmentDistance / DroneSpeed;
            float AbsTime = ProgramStartTime + RelativeTime;
            // 使用时空冲突检测
            if (IsSpaceTimeConflict(Neighbor->Position, AbsTime, DroneID))
            {
                delete Neighbor;
                continue;
            }
            // 检查是否在关闭集中
            bool InClosedSet = false;
            for (FAStarNode* ClosedNode : ClosedSet)
            {
                if (*Neighbor == *ClosedNode)
                {
                    InClosedSet = true;
                    break;
                }
            }
            
            if (InClosedSet)
            {
                delete Neighbor;
                continue;
            }
            
            // 检查是否在开放集中
            bool InOpenSet = false;
            FAStarNode* OpenNode = nullptr;
            for (FAStarNode* Node : OpenSet)
            {
                if (*Node == *Neighbor)
                {
                    InOpenSet = true;
                    OpenNode = Node;
                    break;
                }
            }
            float TentativeGScore = Current->GScore + SegmentDistance;
            if (!InOpenSet)
            {
                Neighbor->GScore = TentativeGScore;
                float NeighborHScore = GetDiagonalHeuristic(Neighbor->GridX, Neighbor->GridY, Neighbor->GridZ, GoalX, GoalY, GoalZ);
                Neighbor->FScore = TentativeGScore + NeighborHScore;
                Neighbor->Parent = Current;
                OpenSet.Add(Neighbor);
            }
            else if (TentativeGScore < OpenNode->GScore)
            {
                OpenNode->GScore = TentativeGScore;
                float OpenNodeHScore = GetDiagonalHeuristic(OpenNode->GridX, OpenNode->GridY, OpenNode->GridZ, GoalX, GoalY, GoalZ);
                OpenNode->FScore = TentativeGScore + OpenNodeHScore;
                OpenNode->Parent = Current;
                delete Neighbor;
            }
            else
            {
                delete Neighbor;
            }
        }
    }
    if (bFound)
    {
        ReconstructPath(GoalNode, OutPath);
        for (FAStarNode* Node : OpenSet) delete Node;
        for (FAStarNode* Node : ClosedSet) delete Node;
        SmoothPath(OutPath);
        StoredPath = OutPath;

        // 记录预约
        TArray<FSpaceTimePoint> NewReservation;
        float AccumTime = 0.0f;
        for (int32 i = 0; i < OutPath.Num() - 1; ++i)
        {
            float Segment = FVector::Dist(OutPath[i], OutPath[i+1]);
            float SegmentTime = Segment / DroneSpeed;
            NewReservation.Add(FSpaceTimePoint(OutPath[i], ProgramStartTime + AccumTime));
            AccumTime += SegmentTime;
        }
        if (OutPath.Num() > 0)
            NewReservation.Add(FSpaceTimePoint(OutPath.Last(), ProgramStartTime + AccumTime));
        ReservationTable.Remove(DroneID);
        ReservationTable.Add(DroneID, FDroneReservation(NewReservation));
        // UE_LOG(LogTemp, Warning, TEXT("Current Reservations:"));
        // for (const auto& Elem : GetReservationTable())
        // {
        //     UE_LOG(LogTemp, Warning, TEXT("DroneID: %d, PathPoints: %d"), Elem.Key, Elem.Value.PathPoints.Num());
        // }
        return true;
    }
    for (FAStarNode* Node : OpenSet) delete Node;
    for (FAStarNode* Node : ClosedSet) delete Node;
    return false;
}

TArray<FVector> UAStarPathFinderComponent::GetSearchedPath()
{
    return StoredPath;
}

void UAStarPathFinderComponent::VisualizePath(float Duration)
{
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("VisualizePath: No valid world found!"));
        return;
    }
    // FlushPersistentDebugLines(GetWorld());
    if (StoredPath.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("VisualizePath: Not enough path points! Current points: %d"), StoredPath.Num());
        return;
    }
    // UE_LOG(LogTemp, Log, TEXT("VisualizePath: Starting visualization with %d points"), StoredPath.Num());
    // 在起点绘制红色小球
    // DrawDebugSphere(
    //     GetWorld(),
    //     StoredPath[0],  // 起点位置
    //     20.0f,          // 球体半径
    //     12,             // 球体分段数
    //     FColor::Red,    // 红色
    //     false,          // 不持久化
    //     Duration,
    //     0,
    //     2.0f           // 线条粗细
    // );
    
    // 在终点绘制绿色小球
    DrawDebugSphere(
        GetWorld(),
        StoredPath.Last(),  // 终点位置
        20.0f,              // 球体半径
        12,                 // 球体分段数
        FColor::Red,      // 绿色
        false,              // 不持久化
        Duration,
        0,
        2.0f               // 线条粗细
    );
    
    // Draw lines between path points
    // for (int i = 0; i < StoredPath.Num() - 1; i++)
    // {
    //     DrawDebugLine(
    //         GetWorld(),
    //         StoredPath[i],
    //         StoredPath[i+1],
    //         FColor::White,  // 白色线条
    //         false,
    //         Duration,
    //         0,
    //         1.0f  // 细线条
    //     );
    // }
}

float UAStarPathFinderComponent::Distance(FAStarNode* A, FAStarNode* B)
{
    // Euclidean distance
    return FVector::Dist(A->Position, B->Position);
}

// 优化后的邻居节点生成函数
void UAStarPathFinderComponent::GetNeighborsOptimized(FAStarNode* Node, TArray<FAStarNode*>& OutNeighbors, int32 GoalX, int32 GoalY, int32 GoalZ)
{
    if (!Node || !GridMap) return;
    // 获取网格分辨率
    const float GridResolution = GridMap->GetResolution();
    
    // 定义可能的移动方向（26个方向：6个面 + 12个边 + 8个角）
    static const int32 Directions[][3] = {
        // 面
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
        // 边
        {1, 1, 0}, {1, -1, 0}, {-1, 1, 0}, {-1, -1, 0},
        {1, 0, 1}, {1, 0, -1}, {-1, 0, 1}, {-1, 0, -1},
        {0, 1, 1}, {0, 1, -1}, {0, -1, 1}, {0, -1, -1},
        // 角
        {1, 1, 1}, {1, 1, -1}, {1, -1, 1}, {1, -1, -1},
        {-1, 1, 1}, {-1, 1, -1}, {-1, -1, 1}, {-1, -1, -1}
    };

    // 遍历所有可能的移动方向
    for (const auto& Dir : Directions)
    {
        int32 NewX = Node->GridX + Dir[0];
        int32 NewY = Node->GridY + Dir[1];
        int32 NewZ = Node->GridZ + Dir[2];

        // 检查新位置是否在地图范围内
        if (NewX < 0 || NewX >= GridMap->GetGridDimX() ||
            NewY < 0 || NewY >= GridMap->GetGridDimY() ||
            NewZ < 0 || NewZ >= GridMap->GetGridDimZ())
        {
            continue;
        }

        // 获取世界坐标
        FVector WorldPos = GridMap->GridToWorld(NewX, NewY, NewZ);

        // 检查是否被占用
        if (GridMap->IsOccupied(WorldPos))
            continue;

        // 创建新节点
        FAStarNode* NewNode = new FAStarNode(WorldPos, NewX, NewY, NewZ);
        OutNeighbors.Add(NewNode);
    }
}

void UAStarPathFinderComponent::ReconstructPath(FAStarNode* GoalNode, TArray<FVector>& OutPath)
{
    // Clear path
    OutPath.Empty();
    // Trace back from goal to start
    FAStarNode* Current = GoalNode;
    while (Current)
    {
        OutPath.Insert(Current->Position, 0);
        Current = Current->Parent;
    }
}

void UAStarPathFinderComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 确保 ReservationTable 存在
    UE_LOG(LogTemp, Warning, TEXT("BeginPlay - Current ReservationTable entries: %d"), ReservationTable.Num());
    ProgramStartTime = GetWorld()->GetTimeSeconds();
}

void UAStarPathFinderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    if (StoredPath.Num()>0)
    {
        VisualizePath(1.0f);
    }
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// 添加新的启发式函数
float UAStarPathFinderComponent::GetDiagonalHeuristic(int X1, int Y1, int Z1, int X2, int Y2, int Z2)
{
    if (!GridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("GetDiagonalHeuristic: GridMap is not set"));
        return 0.0f;
    }
    
    float DX = FMath::Abs(X2 - X1);
    float DY = FMath::Abs(Y2 - Y1);
    float DZ = FMath::Abs(Z2 - Z1);
    
    float H = 0.0f;
    float Diag = FMath::Min3(DX, DY, DZ);
    DX -= Diag;
    DY -= Diag;
    DZ -= Diag;
    
    if (DX == 0)
    {
        H = 1.0f * FMath::Sqrt(3.0f) * Diag + FMath::Sqrt(2.0f) * FMath::Min(DY, DZ) + 1.0f * FMath::Abs(DY - DZ);
    }
    else if (DY == 0)
    {
        H = 1.0f * FMath::Sqrt(3.0f) * Diag + FMath::Sqrt(2.0f) * FMath::Min(DX, DZ) + 1.0f * FMath::Abs(DX - DZ);
    }
    else if (DZ == 0)
    {
        H = 1.0f * FMath::Sqrt(3.0f) * Diag + FMath::Sqrt(2.0f) * FMath::Min(DX, DY) + 1.0f * FMath::Abs(DX - DY);
    }
    
    return H * GridMap->GetResolution();
}

float UAStarPathFinderComponent::GetManhattanHeuristic(int X1, int Y1, int Z1, int X2, int Y2, int Z2)
{
    if (!GridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("GetManhattanHeuristic: GridMap is not set"));
        return 0.0f;
    }
    
    float DX = FMath::Abs(X2 - X1);
    float DY = FMath::Abs(Y2 - Y1);
    float DZ = FMath::Abs(Z2 - Z1);
    return (DX + DY + DZ) * GridMap->GetResolution();
}

float UAStarPathFinderComponent::GetEuclideanHeuristic(int X1, int Y1, int Z1, int X2, int Y2, int Z2)
{
    if (!GridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("GetEuclideanHeuristic: GridMap is not set"));
        return 0.0f;
    }
    
    float DX = X2 - X1;
    float DY = Y2 - Y1;
    float DZ = Z2 - Z1;
    return FMath::Sqrt(DX*DX + DY*DY + DZ*DZ) * GridMap->GetResolution();
}



bool UAStarPathFinderComponent::IsPathPointSafe(const FVector& Point) const
{
    if (!GridMap)
    {
        return false;
    }
    
    // 获取安全距离
    float SafetyDistance = GetSafetyDistance();
    
    // 检查点周围的安全距离内是否有障碍物
    TArray<FVector> CheckPoints;
    int32 NumChecks = 8;  // 在水平面上检查8个方向
    float AngleStep = 2.0f * PI / NumChecks;
    
    for (int32 i = 0; i < NumChecks; i++)
    {
        float Angle = i * AngleStep;
        FVector Offset(
            FMath::Cos(Angle) * SafetyDistance,
            FMath::Sin(Angle) * SafetyDistance,
            0.0f
        );
        CheckPoints.Add(Point + Offset);
    }
    
    // 检查上下方向
    CheckPoints.Add(Point + FVector(0, 0, SafetyDistance));
    CheckPoints.Add(Point + FVector(0, 0, -SafetyDistance));
    
    // 检查所有点
    for (const FVector& CheckPoint : CheckPoints)
    {
        if (GridMap->IsOccupied(CheckPoint))
        {
            return false;
        }
    }
    
    return true;
}

FVector UAStarPathFinderComponent::GetObstacleRepulsionForce(const FVector& Point)
{
    const float SampleDist = 10.f; // 周围采样范围
    const float InfluenceRadius = 100.f; // 斥力有效距离
    const float RepulsionStrength = 100.f; // 斥力系数

    FVector Gradient = FVector::ZeroVector;
    const FVector Directions[6] = {
        FVector(1, 0, 0), FVector(-1, 0, 0),
        FVector(0, 1, 0), FVector(0, -1, 0),
        FVector(0, 0, 1), FVector(0, 0, -1),
    };

    for (const FVector& Dir : Directions)
    {
        FVector SamplePoint = Point + Dir * SampleDist;
        if (!IsPathPointSafe(SamplePoint))
        {
            float Dist = FMath::Max(KINDA_SMALL_NUMBER, FVector::Dist(Point, SamplePoint));
            float Force = RepulsionStrength * (1.0f - FMath::Clamp(Dist / InfluenceRadius, 0.0f, 1.0f));
            Gradient -= Dir * Force;
        }
    }

    return Gradient;
}

// 添加平滑函数
void UAStarPathFinderComponent::SmoothPath(TArray<FVector>& InOutPath)
{
    if (InOutPath.Num() < 3) return;  // 至少需要3个点才能平滑
    const float SmoothingWeight = 0.5f;  // 平滑权重
    const int32 Iterations = 5;  // 平滑迭代次数
    const float ObstacleWeight = 0.05f; // 新增惩罚项权重
    
    TArray<FVector> SmoothedPath = InOutPath;
    TArray<FVector> OriginalPath = InOutPath;  // 保存原始路径
    bool bIsPathSafe = true;
    
    for (int32 Iter = 0; Iter < Iterations && bIsPathSafe; Iter++)
    {
        TArray<FVector> TempPath = SmoothedPath;
        
        // 保持起点和终点不变，只平滑中间点
        for (int32 i = 1; i < SmoothedPath.Num() - 1; i++)
        {
            // 使用前一个点和后一个点的加权平均来平滑当前点
            FVector PrevPoint = TempPath[i - 1];
            FVector NextPoint = TempPath[i + 1];
            FVector CurrentPoint = TempPath[i];
            
            // 计算平滑后的点
            FVector SmoothTerm = (PrevPoint + NextPoint) * 0.5f - CurrentPoint;
            FVector ObstacleForce = GetObstacleRepulsionForce(CurrentPoint);

            FVector SmoothedPoint = CurrentPoint
                + SmoothingWeight * SmoothTerm
                + ObstacleWeight * ObstacleForce;
            
            // 检查平滑后的点是否安全
            if (!IsPathPointSafe(SmoothedPoint))
            {
                bIsPathSafe = false;
                break;
            }

            // 检查到前一个点和后一个点的路径是否安全
            if (!CanReachDirectly(SmoothedPoint, PrevPoint) || !CanReachDirectly(SmoothedPoint, NextPoint))
            {
                bIsPathSafe = false;
                break;
            }

            SmoothedPath[i] = SmoothedPoint;
        }

        // 如果发现不安全的点，回退到原始路径
        if (!bIsPathSafe)
        {
            // UE_LOG(LogTemp, Warning, TEXT("SmoothPath: 检测到不安全的平滑路径，保持原始路径"));
            SmoothedPath = OriginalPath;
            break;
        }
    }
    
    // 最后一次检查整条路径的安全性
    for (int32 i = 0; i < SmoothedPath.Num() - 1; i++)
    {
        if (!CanReachDirectly(SmoothedPath[i], SmoothedPath[i + 1]))
        {
            UE_LOG(LogTemp, Warning, TEXT("SmoothPath: 最终路径安全性检查失败，保持原始路径"));
            SmoothedPath = OriginalPath;
            break;
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("SmoothPath: 平滑后的路径点数: %d"), SmoothedPath.Num());
    InOutPath = SmoothedPath;
}

bool UAStarPathFinderComponent::CanReachDirectly(const FVector& Start, const FVector& Goal)
{
    if (!GridMap)
        return false;
       
    // 计算直线路径上的点
    FVector Direction = (Goal - Start).GetSafeNormal();
    float Distance = FVector::Dist(Start, Goal);
    float StepSize = GridMap->GetResolution();
   
    // 检查直线路径上是否有障碍物
    for (float Dist = 0; Dist < Distance; Dist += StepSize)
    {
        FVector CheckPoint = Start + Direction * Dist;
        if (GridMap->IsOccupied(CheckPoint))
            return false;
    }
   
    return true;
}

void UAStarPathFinderComponent::SetGridMap(UGridMapComponent* InGridMap)
{
    if (!InGridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("AStarPathFinder: Trying to set null GridMap"));
        return;
    }
    GridMap = InGridMap;
    UE_LOG(LogTemp, Log, TEXT("AStarPathFinder: GridMap set successfully"));
}

// 检查时空冲突：同一时刻，距离小于1米（100cm）算冲突
bool UAStarPathFinderComponent::IsSpaceTimeConflict(const FVector& Position, float AbsTime, int32 SelfDroneID) const
{
    for (const auto& Elem : ReservationTable)
    {
        if (Elem.Key == SelfDroneID) continue;
        for (const FSpaceTimePoint& Point : Elem.Value.PathPoints)
        {
            if (FVector::Dist(Point.Position, Position) < 160.0f) // 允许50ms误差
            {
                if (FMath::Abs(Point.AbsTime - AbsTime) < 0.04f)
                {
                    // UE_LOG(LogTemp, Warning, TEXT("SpaceTimeConflict: DroneID: %d, Position: %s, AbsTime: %f, SelfDroneID: %d"), Elem.Key, *Position.ToString(), AbsTime, SelfDroneID);
                    return true;
                }
            }
        }
    }
    return false;
}

// 将时间转换为字符串
FString UAStarPathFinderComponent::TimeToString(float AbsTime)
{
    int32 Seconds = FMath::FloorToInt(AbsTime);
    int32 Milliseconds = FMath::RoundToInt((AbsTime - Seconds) * 1000);
    return FString::Printf(TEXT("%02d.%03d"), Seconds, Milliseconds);
}

// 将位置转换为字符串
FString UAStarPathFinderComponent::PositionToString(const FVector& Position)
{
    return FString::Printf(TEXT("(%.0f, %.0f, %.0f)"), Position.X, Position.Y, Position.Z);
}

// 获取预约表的字符串表示
FString UAStarPathFinderComponent::GetReservationTableString()
{
    FString TableStr = TEXT("时空预约表:\n");
    TableStr += TEXT("============================================\n");
    
    // 按DroneID排序
    TArray<int32> DroneIDs;
    ReservationTable.GetKeys(DroneIDs);
    DroneIDs.Sort();

    for (int32 DroneID : DroneIDs)
    {
        const FDroneReservation& Reservation = ReservationTable[DroneID];
        TableStr += FString::Printf(TEXT("\n无人机 %d 的预约:\n"), DroneID);
        TableStr += TEXT("--------------------------------------------\n");
        TableStr += TEXT("时间点\t\t位置\t\t\t安全区域\n");
        
        for (const FSpaceTimePoint& Point : Reservation.PathPoints)
        {
            TableStr += FString::Printf(TEXT("%s\t%s\t半径160cm\n"),
                *TimeToString(Point.AbsTime),
                *PositionToString(Point.Position));
        }
    }
    
    return TableStr;
}

// 生成预约表可视化
void UAStarPathFinderComponent::VisualizeReservationTable()
{
    // 获取预约表字符串
    FString TableStr = GetReservationTableString();
    
    // 输出到日志
    UE_LOG(LogTemp, Log, TEXT("\n%s"), *TableStr);
    
    // 在世界中可视化预约点和安全区域
    UWorld* World = GEngine->GetWorld();
    if (!World) return;

    // 为每个无人机使用不同的颜色
    const TArray<FColor> DroneColors = {
        FColor::Red,
        FColor::Green,
        FColor::Blue,
        FColor::Yellow,
        FColor::Magenta,
        FColor::Cyan,
        FColor::Orange,
        FColor::Purple,
        FColor::Turquoise,
        FColor::White
    };

    // 按DroneID排序
    TArray<int32> DroneIDs;
    ReservationTable.GetKeys(DroneIDs);
    DroneIDs.Sort();

    // 可视化每个无人机的预约点
    for (int32 Index = 0; Index < DroneIDs.Num(); ++Index)
    {
        int32 DroneID = DroneIDs[Index];
        const FDroneReservation& Reservation = ReservationTable[DroneID];
        FColor DroneColor = DroneColors[Index % DroneColors.Num()];

        // 绘制路径点和安全区域
        for (int32 i = 0; i < Reservation.PathPoints.Num(); ++i)
        {
            const FSpaceTimePoint& Point = Reservation.PathPoints[i];
            
            // 绘制点
            DrawDebugPoint(
                World,
                Point.Position,
                10.0f,  // 点大小
                DroneColor,
                false,  // 持久化
                5.0f    // 显示时间
            );

            // 绘制安全区域（半透明球体）
            DrawDebugSphere(
                World,
                Point.Position,
                160.0f,  // 安全距离半径
                16,      // 球体分段数
                DroneColor,
                false,   // 持久化
                5.0f,    // 显示时间
                0,       // DepthPriority
                1.0f     // 线条粗细
            );

            // 如果不是最后一个点，绘制到下一个点的连线
            if (i < Reservation.PathPoints.Num() - 1)
            {
                const FSpaceTimePoint& NextPoint = Reservation.PathPoints[i + 1];
                DrawDebugLine(
                    World,
                    Point.Position,
                    NextPoint.Position,
                    DroneColor,
                    false,   // 持久化
                    5.0f,    // 显示时间
                    0,       // DepthPriority
                    2.0f     // 线条粗细
                );
            }

            // 显示时间标签
            DrawDebugString(
                World,
                Point.Position,
                TimeToString(Point.AbsTime),
                nullptr,
                DroneColor,
                5.0f    // 显示时间
            );
        }
    }
}