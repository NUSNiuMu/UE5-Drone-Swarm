#include "DroneSwarmManagerComponent.h"
#include "DrawDebugHelpers.h"
#include "InputCoreTypes.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"


UDroneSwarmManagerComponent::UDroneSwarmManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UDroneSwarmManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 获取GridMap组件引用
    AActor* Owner = GetOwner();
    if (Owner)
    {
        GridMap = Owner->FindComponentByClass<UGridMapComponent>();
        if (GridMap)
        {
            UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Successfully found GridMap component"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[SwarmManager] Failed to find GridMap component"));
        }
        // 绑定M键输入
        if (Owner->InputComponent == nullptr)
        {
            Owner->EnableInput(GetWorld()->GetFirstPlayerController());
        }
        if (Owner->InputComponent)
        {
            Owner->InputComponent->BindKey(EKeys::M, IE_Pressed, this, &UDroneSwarmManagerComponent::ToggleAllDronesMovement);
            Owner->InputComponent->BindKey(EKeys::L, IE_Pressed, this, &UDroneSwarmManagerComponent::ReplayAllDronesPath);
        }
    }
}

void UDroneSwarmManagerComponent::ToggleAllDronesMovement()
{
    UE_LOG(LogTemp, Error, TEXT("[SwarmManager] ToggleAllDronesMovement"));
    if (bIsSwarmPaused)
    {
        // 恢复所有无人机
        for (FDronePathTask& Task : DroneTasks)
        {
            if (Task.DroneActor)
            {
                Task.DroneActor->ResumeMovementFromCurrentPosition();
            }
        }
        bIsSwarmPaused = false;
        UE_LOG(LogTemp, Log, TEXT("[SwarmManager] All drones resumed"));
    }
    else
    {
        // 暂停所有无人机
        for (FDronePathTask& Task : DroneTasks)
        {
            if (Task.DroneActor)
            {
                Task.DroneActor->StopMovement();
            }
        }
        bIsSwarmPaused = true;
        UE_LOG(LogTemp, Log, TEXT("[SwarmManager] All drones paused"));
    }
}

void UDroneSwarmManagerComponent::AddDrone(ADroneActor* Drone, const FVector& GoalLocation)
{
    if (!Drone)
    {
        UE_LOG(LogTemp, Error, TEXT("[SwarmManager] Attempted to add null drone"));
        return;
    }

    FDronePathTask NewTask;
    NewTask.DroneActor = Drone;
    NewTask.StartLocation = Drone->GetActorLocation();
    NewTask.GoalLocation = GoalLocation;
    NewTask.StartTime = -1.0f;  // 将在规划时设置
    NewTask.EstimatedDuration = -1.0f;  // 将在规划时计算

    // 设置无人机的目标位置
    Drone->SetGoalLocation(GoalLocation);

    DroneTasks.Add(NewTask);
    
    UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Added Drone %d to swarm. Start: %s, Goal: %s"), 
        Drone->GetDroneID(), *NewTask.StartLocation.ToString(), *NewTask.GoalLocation.ToString());
}

void UDroneSwarmManagerComponent::StartSwarmPathPlanning()
{
    if (!GridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("[SwarmManager] Cannot start path planning: GridMap component not found!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Starting path planning for %d drones"), DroneTasks.Num());

    // 获取优先级排序后的任务索引
    TArray<int32> PrioritizedIndices = GetPrioritizedTaskIndices();

    // 按优先级顺序为每个无人机规划路径
    float CurrentStartTime = 0.0f;
    for (int32 TaskIndex : PrioritizedIndices)
    {
        FDronePathTask& Task = DroneTasks[TaskIndex];
        Task.StartTime = CurrentStartTime;

        UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Planning path for Drone %d (Priority Index: %d)"), 
            Task.DroneActor->GetDroneID(), TaskIndex);

        if (PlanPathForDrone(Task))
        {
            // 更新下一个无人机的开始时间
            CurrentStartTime += Task.EstimatedDuration;
            UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Successfully planned path for Drone %d. Start Time: %.2f, Duration: %.2f"), 
                Task.DroneActor->GetDroneID(), Task.StartTime, Task.EstimatedDuration);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[SwarmManager] Failed to plan path for Drone %d"), 
                Task.DroneActor->GetDroneID());
        }
    }

    // 显示预约表
    // UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Displaying reservation table..."));
    // UAStarPathFinderComponent::VisualizeReservationTable();
    
    // 输出预约表到日志
    // FString ReservationTableStr = UAStarPathFinderComponent::GetReservationTableString();
    // UE_LOG(LogTemp, Log, TEXT("\n%s"), *ReservationTableStr);
}

void UDroneSwarmManagerComponent::StartAllDrones()
{
    UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Starting movement for all drones"));
    
    int32 SuccessCount = 0;
    for (FDronePathTask& Task : DroneTasks)
    {
        if (Task.DroneActor && Task.PlannedPath.Num() > 0)
        {
            // 设置规划好的路径
            Task.DroneActor->SetPath(Task.PlannedPath);
            // 开始移动
            Task.DroneActor->StartMovement();
            SuccessCount++;
            
            UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Started Drone %d with path length: %d"), 
                Task.DroneActor->GetDroneID(), Task.PlannedPath.Num());
        }
        else
        {
            if (!Task.DroneActor)
            {
                UE_LOG(LogTemp, Error, TEXT("[SwarmManager] Invalid drone actor in task"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[SwarmManager] Drone %d has no planned path"), 
                    Task.DroneActor->GetDroneID());
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Successfully started %d/%d drones"), 
        SuccessCount, DroneTasks.Num());
}

void UDroneSwarmManagerComponent::StopAllDrones()
{
    UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Stopping all drones"));
    int32 StoppedCount = 0;
    for (FDronePathTask& Task : DroneTasks)
    {
        if (Task.DroneActor)
        {
            Task.DroneActor->StopMovement();
            StoppedCount++;
            UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Stopped Drone %d at location: %s"), 
                Task.DroneActor->GetDroneID(), *Task.DroneActor->GetActorLocation().ToString());
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Successfully stopped %d/%d drones"), 
        StoppedCount, DroneTasks.Num());
}

bool UDroneSwarmManagerComponent::PlanPathForDrone(FDronePathTask& DroneTask)
{
    if (!DroneTask.DroneActor || !GridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("[SwarmManager] Cannot plan path: Invalid drone or GridMap"));
        return false;
    }

    UAStarPathFinderComponent* PathFinder = DroneTask.DroneActor->GetPathFinder();
    if (!PathFinder)
    {
        UE_LOG(LogTemp, Error, TEXT("[SwarmManager] PathFinder component not found for Drone %d"), DroneTask.DroneActor->GetDroneID());
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Planning path for Drone %d from %s to %s"), DroneTask.DroneActor->GetDroneID(), *DroneTask.StartLocation.ToString(), *DroneTask.GoalLocation.ToString());


    if (PathFinder->FindPath(DroneTask.StartLocation, DroneTask.GoalLocation, DroneTask.PlannedPath, DroneTask.DroneActor->GetDroneID()))
    {
        UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Successfully planned path for Drone %d: %d waypoints, duration: %.2f"), DroneTask.DroneActor->GetDroneID(), DroneTask.PlannedPath.Num(), DroneTask.EstimatedDuration);
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("[SwarmManager] Failed to find path for Drone %d"), DroneTask.DroneActor->GetDroneID());
    return false;
}



TArray<int32> UDroneSwarmManagerComponent::GetPrioritizedTaskIndices()
{
    // 创建索引数组
    TArray<int32> Indices;
    for (int32 i = 0; i < DroneTasks.Num(); ++i)
    {
        Indices.Add(i);
    }

    // 按照路径长度排序（较长的路径优先）
    Indices.Sort([this](int32 A, int32 B) {
        float DistanceA = FVector::Distance(DroneTasks[A].StartLocation, DroneTasks[A].GoalLocation);
        float DistanceB = FVector::Distance(DroneTasks[B].StartLocation, DroneTasks[B].GoalLocation);
        return DistanceA > DistanceB;
    });

    return Indices;
}

void UDroneSwarmManagerComponent::UpdateTaskPriorities()
{
    // 根据路径长度更新优先级
    for (FDronePathTask& Task : DroneTasks)
    {
        float PathLength = FVector::Distance(Task.StartLocation, Task.GoalLocation);
        Task.Priority = FMath::FloorToInt(PathLength / 100.0f);  // 每100厘米增加1点优先级
    }
}

void UDroneSwarmManagerComponent::ReplayAllDronesPath()
{
    UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Replaying all drones' saved paths"));
    for (FDronePathTask& Task : DroneTasks)
    {
        if (Task.DroneActor)
        {
            Task.DroneActor->bHasSavedDonePath = false;
        }
        if (Task.DroneActor && Task.DroneActor->LoadDonePathFromFile() && Task.DroneActor->DonePath.Num() > 1)
        {
            Task.DroneActor->ClearDrawnPath();
            Task.DroneActor->DonePath.Empty();
            Task.DroneActor->DonePath.Add(Task.DroneActor->CurrentPath[0]);
            Task.DroneActor->SetActorLocation(Task.DroneActor->CurrentPath[0]);
            Task.DroneActor->SetPath(Task.DroneActor->CurrentPath);
            Task.DroneActor->CurrentPathIndex = 0;
            Task.DroneActor->bIsMoving = true;
            UE_LOG(LogTemp, Log, TEXT("[SwarmManager] Drone %d start replaying path, points: %d"), Task.DroneActor->GetDroneID(), Task.DroneActor->CurrentPath.Num());
        }
        else if (Task.DroneActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SwarmManager] Drone %d failed to load path or path too short!"), Task.DroneActor->GetDroneID());
        }
    }
}

