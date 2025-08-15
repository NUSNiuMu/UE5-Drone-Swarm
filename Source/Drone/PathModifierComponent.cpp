#include "CoreMinimal.h"
#include "PathModifierComponent.h"
#include "GridMapComponent.h"
#include "AStarPathFinderComponent.h"
#include "DroneActor.h"
#include "EngineUtils.h"

UPathModifierComponent::UPathModifierComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    GridMap = nullptr;
    AStar = nullptr;
}

void UPathModifierComponent::BeginPlay()
{
    Super::BeginPlay();

    // 查找AStar组件和GridMap组件
    AActor* Owner = GetOwner();
    if (Owner)
    {
        AStar = Owner->FindComponentByClass<UAStarPathFinderComponent>();
        if (AStar)
        {
            UE_LOG(LogTemp, Log, TEXT("[PathModifier] AStar component found"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[PathModifier] AStar component not found"));
        }

        // 缓存无人机指针
        OwnerDrone = Cast<ADroneActor>(Owner);
        if (OwnerDrone)
        {
            UE_LOG(LogTemp, Log, TEXT("[PathModifier] Cached OwnerDrone with ID: %d"), OwnerDrone->GetDroneID());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[PathModifier] Owner is not a valid ADroneActor"));
        }

        // 订阅GridMap更新事件
        if (GridMap)
        {
            GridMap->OnGridMapUpdated.AddDynamic(this, &UPathModifierComponent::OnGridUpdated);
            UE_LOG(LogTemp, Log, TEXT("[PathModifier] Successfully subscribed to GridMap updates"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[PathModifier] GridMap not found, will try to subscribe when it's set"));
        }
    }
}

void UPathModifierComponent::SetPath(const TArray<FVector>& InPath)
{
    CurrentPath = InPath;
    // UE_LOG(LogTemp, Warning, TEXT("[PathModifier] SetPath 被调用，路径点数: %d"), CurrentPath.Num());
}

void UPathModifierComponent::CheckAndModifyPath()
{
    if (!GridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("[PathModifier] GridMap component not found"));
        return;
    }

    if (!AStar)
    {
        UE_LOG(LogTemp, Error, TEXT("[PathModifier] AStar component not found"));
        return;
    }

    // 检查路径的每个点
    bNeedsReplanning = false;  // 使用类成员变量
    for (const FVector& Point : CurrentPath)
    {
        if (GridMap->IsOccupied(Point))
        {
            bNeedsReplanning = true;
            // UE_LOG(LogTemp, Warning, TEXT("[PathModifier] 检测到路径点被占用: %s"), *Point.ToString());
            break;
        }
    }

    // 如果需要重新规划
    if (bNeedsReplanning)
    {
        TArray<FVector> NewPath;
        if (AStar && CurrentPath.Num() >= 2)
        {
            // 使用当前位置和原始目标点重新规划路径
            FVector StartPoint = GetOwner()->GetActorLocation();  // 使用当前位置作为起点
            if (AStar->FindPath(StartPoint, OwnerDrone->GetGoalLocation(), NewPath, DroneID))
            {
                CurrentPath = NewPath;
                OnPathModified.Broadcast(CurrentPath);
            }
        }
    }
}

void UPathModifierComponent::SetGridMap(UGridMapComponent* InGridMap)
{
    if (!InGridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("[PathModifier] Trying to set null GridMap"));
        return;
    }

    // 如果已经有GridMap，先取消订阅
    if (GridMap)
    {
        GridMap->OnGridMapUpdated.RemoveDynamic(this, &UPathModifierComponent::OnGridUpdated);
    }

    GridMap = InGridMap;

    // 订阅新的GridMap更新事件
    GridMap->OnGridMapUpdated.AddDynamic(this, &UPathModifierComponent::OnGridUpdated);
    UE_LOG(LogTemp, Log, TEXT("[PathModifier] GridMap set and subscribed to updates"));
}

void UPathModifierComponent::OnGridUpdated(const FVector& UpdatedLocation)
{
    CheckAndModifyPath();
}

void UPathModifierComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !AStar) return;

    // 获取当前位置和时间
    FVector CurrentLocation = GetOwner()->GetActorLocation();
    float CurrentTime = GetWorld()->GetTimeSeconds();

    // 检查当前位置是否与其他无人机位置冲突
    bool bHasConflict = false;
    ADroneActor* ConflictingDrone = nullptr;

    // 遍历场景中的所有无人机
    for (TActorIterator<ADroneActor> It(GetWorld()); It; ++It)
    {
        ADroneActor* OtherDrone = *It;
        if (!OtherDrone || OtherDrone == OwnerDrone) continue;

        // 计算两架无人机之间的距离
        float Distance = FVector::Dist(CurrentLocation, OtherDrone->GetActorLocation());
        
        // 如果距离小于安全阈值，认为发生冲突
        if (Distance < 110.0f)
        {
            bHasConflict = true;
            ConflictingDrone = OtherDrone;
            
            // 计算两个无人机的移动方向
            FVector OurDirection = FVector::ZeroVector;
            FVector TheirDirection = FVector::ZeroVector;

            // 获取我方无人机的移动方向
            if (CurrentPath.Num() >= 2)
            {
                for (int32 i = 1; i < CurrentPath.Num(); ++i)
                {
                    if (FVector::Dist(CurrentLocation, CurrentPath[i]) > 50.0f)
                    {
                        OurDirection = (CurrentPath[i] - CurrentLocation).GetSafeNormal();
                        break;
                    }
                }
            }

            // 获取对方无人机的移动方向
            const TArray<FVector>& TheirPath = ConflictingDrone->GetCurrentPath();
            FVector TheirLocation = ConflictingDrone->GetActorLocation();
            if (TheirPath.Num() >= 2)
            {
                for (int32 i = 1; i < TheirPath.Num(); ++i)
                {
                    if (FVector::Dist(TheirLocation, TheirPath[i]) > 50.0f)
                    {
                        TheirDirection = (TheirPath[i] - TheirLocation).GetSafeNormal();
                        break;
                    }
                }
            }

            // 如果两个无人机都有有效的移动方向
            if (!OurDirection.IsZero() && !TheirDirection.IsZero())
            {
                // 计算相对位置向量
                FVector RelativePos = ConflictingDrone->GetActorLocation() - CurrentLocation;
                RelativePos.Normalize();

                // 计算两个无人机与相对位置向量的点积
                float OurDot = FVector::DotProduct(OurDirection, RelativePos);
                float TheirDot = FVector::DotProduct(TheirDirection, -RelativePos);

                // 判断谁在后面：如果我方点积大于0且对方点积小于0，说明我方在后面
                bool bWeAreFollowing = (OurDot > 0.f && TheirDot < -0.1f);

                // 如果我方在后面，我方停止
                if (!bWeAreFollowing)
                {
                    break;  // 跳出循环，继续执行后面的停止逻辑
                }
                else
                {
                    // 如果我方不在后面，继续检查其他无人机
                    bHasConflict = false;
                    ConflictingDrone = nullptr;
                    continue;
                }
            }
            break;
        }
    }

    // 如果检测到冲突且冲突的无人机正在移动，则临时停止并调整时间戳
    if (bHasConflict && ConflictingDrone && ConflictingDrone->IsMoving())
    {
        if (OwnerDrone && CurrentPath.Num() >= 2)
        {
            // 停止移动
            OwnerDrone->StopMovement();
            // UE_LOG(LogTemp, Warning, TEXT("[PathModifier] DroneID: %d 因冲突临时停止移动"), DroneID);

            // 获取并修改当前无人机的预约表
            TMap<int32, FDroneReservation>& MutableReservationTable = const_cast<TMap<int32, FDroneReservation>&>(AStar->GetReservationTable());
            if (MutableReservationTable.Contains(DroneID))
            {
                FDroneReservation& Reservation = MutableReservationTable[DroneID];
                TArray<FSpaceTimePoint>& PathPoints = Reservation.PathPoints;
                
                // 从当前时间开始，将所有后续路径点的时间戳增加0.5秒
                for (int32 i = 0; i < PathPoints.Num(); ++i)
                {
                    if (PathPoints[i].AbsTime >= CurrentTime)
                    {
                        PathPoints[i].AbsTime += StopDuration;
                    }
                }
            }

            // 设置定时器在0.5秒后恢复移动
            GetWorld()->GetTimerManager().SetTimer(
                StopTimerHandle,
                [this]()
                {
                    if (OwnerDrone)
                    {
                        OwnerDrone->ResumeMovementFromCurrentPosition();
                        UE_LOG(LogTemp, Log, TEXT("[PathModifier] DroneID: %d 恢复移动"), DroneID);
                    }
                },
                StopDuration,
                false  // 不循环
            );
        }
    }
}

