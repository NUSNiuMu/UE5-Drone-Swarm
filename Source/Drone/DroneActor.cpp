#include "DroneActor.h"
#include "DrawDebugHelpers.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

ADroneActor::ADroneActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建无人机网格体组件
    DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
    RootComponent = DroneMesh;

    // 创建路径规划组件
    PathFinder = CreateDefaultSubobject<UAStarPathFinderComponent>(TEXT("PathFinder"));

    // 创建障碍扫描组件
    ObstacleScanner = CreateDefaultSubobject<UObstacleScannerComponent>(TEXT("ObstacleScanner"));

    // 创建路径修改组件
    PathModifier = CreateDefaultSubobject<UPathModifierComponent>(TEXT("PathModifier"));

    PathFinder = CreateDefaultSubobject<UAStarPathFinderComponent>(TEXT("AStarPathFinder"));

    // 创建图像捕获组件
    ImageCaptureComponent = CreateDefaultSubobject<UDroneImageCaptureComponent>(TEXT("ImageCaptureComponent"));

    // 初始化默认值
    DroneID = -1;
    DroneSpeed = 50.0f; // 默认速度：100厘米/秒
    CurrentPathIndex = 0;
    bIsMoving = false;
    LastRecordedPoint = FVector::ZeroVector;

    UE_LOG(LogTemp, Log, TEXT("[Drone] 无人机Actor创建完成，等待初始化..."));
}

void ADroneActor::BeginPlay()
{
    Super::BeginPlay();

    // 初始化LastRecordedPoint为当前位置
    LastRecordedPoint = GetActorLocation();
    // 添加初始位置到DonePath
    DonePath.Add(LastRecordedPoint);

    // 添加调试日志
    FVector Location = GetActorLocation();
    UE_LOG(LogTemp, Error, TEXT("[Drone Debug] Actor Class: %s, Actor Name: %s, DroneID: %d, Location: X=%f, Y=%f, Z=%f"),
        *GetClass()->GetName(), *GetName(), DroneID, Location.X, Location.Y, Location.Z);

    // 设置碰撞
    if (DroneMesh)
    {
        // 启用物理模拟
        DroneMesh->SetSimulatePhysics(true);
        // 设置碰撞响应
        DroneMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        DroneMesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
        // 对所有通道启用碰撞
        DroneMesh->SetCollisionResponseToAllChannels(ECR_Block);
        // 设置物理材质参数
        if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(DroneMesh))
        {
            PrimComp->SetLinearDamping(2.0f);  // 增加线性阻尼
            PrimComp->SetAngularDamping(2.0f);  // 增加角度阻尼
            PrimComp->SetMassScale(NAME_None, 10.0f);  // 增加质量
        }
        // 设置默认的无人机网格体大小
        DroneMesh->SetWorldScale3D(FVector(1.0f, 1.0f, 0.5f));
        UE_LOG(LogTemp, Log, TEXT("[Drone %d] 初始化完成 位置: %s"), 
            DroneID, *GetActorLocation().ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Drone %d] DroneMesh组件未找到!"), DroneID);
    }

    // 订阅路径修改事件
    if (PathModifier)
    {
        PathModifier->OnPathModified.AddDynamic(this, &ADroneActor::OnPathModified);
        UE_LOG(LogTemp, Log, TEXT("[Drone %d] 已订阅路径修改事件"), DroneID);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Drone %d] PathModifier组件未找到!"), DroneID);
    }
}

void ADroneActor::SetupGridMapReferences(UGridMapComponent* InGridMap)
{
    if (!InGridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("[Drone %d] 设置GridMap失败: 无效的GridMap引用"), DroneID);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Drone %d] 开始设置GridMap引用..."), DroneID);

    // 设置路径规划组件的GridMap引用
    if (PathFinder)
    {
        PathFinder->SetGridMap(InGridMap);
        UE_LOG(LogTemp, Log, TEXT("InGridMap: %s"), *InGridMap->GetName());
        UE_LOG(LogTemp, Log, TEXT("[Drone %d] PathFinder组件已设置GridMap"), DroneID);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Drone %d] PathFinder组件未找到"), DroneID);
    }

    // 设置障碍扫描组件的GridMap引用
    if (ObstacleScanner)
    {
        ObstacleScanner->SetGridMap(InGridMap);
        UE_LOG(LogTemp, Log, TEXT("InGridMap: %s"), *InGridMap->GetName());
        UE_LOG(LogTemp, Log, TEXT("[Drone %d] ObstacleScanner组件已设置GridMap"), DroneID);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Drone %d] ObstacleScanner组件未找到"), DroneID);
    }

    // 设置路径修改组件的GridMap引用
    if (PathModifier)
    {
        PathModifier->SetGridMap(InGridMap);
        UE_LOG(LogTemp, Log, TEXT("[Drone %d] PathModifier组件已设置GridMap"), DroneID);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Drone %d] PathModifier组件未找到"), DroneID);
    }

    if (PathFinder)
    {
        PathFinder->SetGridMap(InGridMap);
    }
    
}

void ADroneActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsMoving)
    {
        UpdateDronePosition(DeltaTime);
    }

    // 每帧绘制路径
    DrawDonePath();
}

FColor ADroneActor::GetDronePathColor() const
{
    // 预定义一组不同的颜色
    static const TArray<FColor> DroneColors = {
        FColor(255, 0, 0),    // 红色
        FColor(0, 255, 0),    // 绿色
        FColor(0, 0, 255),    // 蓝色
        FColor(255, 255, 0),  // 黄色
        FColor(255, 0, 255),  // 紫色
        FColor(0, 255, 255),  // 青色
        FColor(255, 128, 0),  // 橙色
        FColor(128, 0, 255),  // 深紫色
        FColor(0, 255, 128),  // 浅绿色
        FColor(255, 0, 128)   // 粉色
    };

    // 使用DroneID来选择颜色，如果ID超出颜色数组范围，则循环使用
    int32 ColorIndex = DroneID >= 0 ? DroneID % DroneColors.Num() : 0;
    return DroneColors[ColorIndex];
}

void ADroneActor::DrawDonePath()
{
    if (DonePath.Num() < 2) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // 获取当前无人机的路径颜色
    FColor CurrentPathColor = GetDronePathColor();

    // 绘制路径线段
    for (int32 i = 0; i < DonePath.Num() - 1; ++i)
    {
        DrawDebugLine(
            World,
            DonePath[i],
            DonePath[i + 1],
            CurrentPathColor,
            false,  // 持久性
            PathDisplayDuration,  // 持续时间
            0,  // DepthPriority
            PathLineThickness  // 线宽
        );
    }
}

void ADroneActor::ClearDrawnPath()
{
    if (UWorld* World = GetWorld())
    {
        FlushPersistentDebugLines(World);
    }
}

void ADroneActor::SetGoalLocation(const FVector& NewGoal)
{
    // 校验目标点是否在地图范围内
    bool bGoalInsideGrid = true;
    UGridMapComponent* GridMap = nullptr;
    if (PathFinder && PathFinder->GetGridMap())
    {
        GridMap = PathFinder->GetGridMap();
        // 这里假设GridMap有GetMapOrigin和GetMapSize方法
        FVector Origin = GridMap->GetMapOrigin();
        FVector Size = GridMap->GetMapSize();
        FVector MinBound = Origin;
        FVector MaxBound = Origin + Size;
        if (NewGoal.X < MinBound.X || NewGoal.X > MaxBound.X ||
            NewGoal.Y < MinBound.Y || NewGoal.Y > MaxBound.Y ||
            NewGoal.Z < MinBound.Z || NewGoal.Z > MaxBound.Z)
        {
            bGoalInsideGrid = false;
        }
    }
    if (!bGoalInsideGrid)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Drone %d] 目标点超出地图范围，忽略本次设置: %s"), DroneID, *NewGoal.ToString());
        return;
    }

    GoalLocation = NewGoal;
    UE_LOG(LogTemp, Log, TEXT("[Drone %d] 设置目标位置: %s"), DroneID, *NewGoal.ToString());
    if (PathFinder->FindPath(GetActorLocation(), NewGoal, CurrentPath, DroneID))
    {
        SetPath(CurrentPath);
        // UE_LOG(LogTemp, Log, TEXT("[Drone %d] 路径重规划成功，新路径点数: %d"), DroneID, CurrentPath.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Drone %d] 路径规划失败，目标点: %s"), DroneID, *NewGoal.ToString());
        // 不清空CurrentPath，保持无人机停在原地
        StopMovement();
    }
}

void ADroneActor::StartMovement()
{
    if (CurrentPath.Num() > 0)
    {
        CurrentPathIndex = 0;
        bIsMoving = true;
        UE_LOG(LogTemp, Log, TEXT("[Drone %d] 开始移动，路径点数量: %d"), 
            DroneID, CurrentPath.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Drone %d] 无法开始移动：路径为空"), DroneID);
    }
}

void ADroneActor::StopMovement()
{
    bIsMoving = false;
    UE_LOG(LogTemp, Log, TEXT("[Drone %d] 停止移动，当前位置: %s，当前索引: %d，总路径点数: %d"), 
        DroneID, *GetActorLocation().ToString(), CurrentPathIndex, CurrentPath.Num());
    // TODO: 可在此处添加无人机停止时的动画、特效或其他反馈
}

void ADroneActor::SetPath(const TArray<FVector>& NewPath)
{
    CurrentPath = NewPath;
    // 重新计算最近的路径点索引
    FVector CurrentLocation = GetActorLocation();
    float MinDistance = MAX_FLT;
    int32 ClosestIndex = 0;
    for (int32 i = 0; i < CurrentPath.Num(); ++i)
    {
        float Dist = FVector::Dist(CurrentLocation, CurrentPath[i]);
        if (Dist < MinDistance)
        {
            MinDistance = Dist;
            ClosestIndex = i;
        }
    }
    CurrentPathIndex = ClosestIndex;
    // 输出路径信息
    // UE_LOG(LogTemp, Log, TEXT("[Drone %d] 设置新路径，路径点数量: %d"), DroneID, NewPath.Num());

    // 同时更新路径修改组件的路径
    if (PathModifier)
    {
        PathModifier->SetPath(NewPath);
        // UE_LOG(LogTemp, Log, TEXT("[Drone %d] 路径已更新到PathModifier"), DroneID);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Drone %d] PathModifier组件未找到，无法更新路径"), DroneID);
    }
}

void ADroneActor::UpdateDronePosition(float DeltaTime)
{
    if (CurrentPathIndex >= CurrentPath.Num())
    {
        // 到达终点但保持活跃状态，不停止移动标志
        FVector StopLocation = GetActorLocation();
        // 只保存一次路径
        if (!bHasSavedDonePath)
        {
            SaveDonePathToFile();
            bHasSavedDonePath = true;
        }
        return;
    }

    // 获取当前位置和目标点
    FVector CurrentLocation = GetActorLocation();
    FVector TargetPoint = CurrentPath[CurrentPathIndex];

    // 计算方向和距离，但只考虑水平面的方向
    FVector Direction = (TargetPoint - CurrentLocation);
    Direction.Z = 0; // 忽略Z轴的方向，保持水平
    Direction = Direction.GetSafeNormal(); // 重新归一化
    float DistanceToTarget = FVector::Distance(CurrentLocation, TargetPoint);

    // 计算这一帧要移动的距离
    float MoveDistance = DroneSpeed * DeltaTime;

    if (MoveDistance >= DistanceToTarget)
    {
        // 直接到达目标点
        SetActorLocation(TargetPoint);
        
        // 记录路径点（每到达一个目标点就记录）
        DonePath.Add(TargetPoint);
        
        // 移动到下一个路径点
        CurrentPathIndex++;
        // 检查是否到达终点
        if (CurrentPathIndex >= CurrentPath.Num())
        {
            // 到达终点但保持活跃状态，不停止移动标志
            UE_LOG(LogTemp, Log, TEXT("[Drone %d] 完成整条路径，保持活跃状态进行目标检测"), DroneID);
        }
    }
    else
    {
        // 计算实际移动方向（包含Z轴）
        FVector MoveDirection = (TargetPoint - CurrentLocation).GetSafeNormal();
        
        // 朝目标点移动
        FVector NewLocation = CurrentLocation + MoveDirection * MoveDistance;
        DonePath.Add(NewLocation);
        SetActorLocation(NewLocation);

        // 只更新偏航角（Yaw），保持水平
        if (!Direction.IsNearlyZero())
        {
            FRotator CurrentRotation = GetActorRotation();
            FRotator TargetRotation = Direction.Rotation();
            
            // 只使用新的偏航角，保持原有的俯仰和滚转为0
            FRotator NewRotation(0.0f, TargetRotation.Yaw, 0.0f);
            
            // 可以添加平滑旋转
            const float RotationSpeed = 180.0f; // 每秒旋转180度
            float DeltaRotation = RotationSpeed * DeltaTime;
            
            // 计算当前偏航角到目标偏航角的最短路径
            float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, NewRotation.Yaw);
            float NewYaw = CurrentRotation.Yaw;
            
            if (FMath::Abs(DeltaYaw) > DeltaRotation)
            {
                // 如果需要转动的角度大于这一帧允许的最大转动角度，则只转动最大允许角度
                NewYaw += (DeltaYaw > 0 ? DeltaRotation : -DeltaRotation);
            }
            else
            {
                // 如果需要转动的角度小于这一帧允许的最大转动角度，则直接转到目标角度
                NewYaw = NewRotation.Yaw;
            }
            
            SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
        }
    }
}

void ADroneActor::OnPathModified(const TArray<FVector>& NewPath)
{
    if (NewPath.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Drone %d] 新路径为空，忽略路径更新"), DroneID);
        return;
    }

    // 获取当前位置
    FVector CurrentLocation = GetActorLocation();

    // 创建新的完整路径
    TArray<FVector> CompletePath;

    // 1. 保留原始路径的起点部分（包括已经完成的路径点）
    if (CurrentPath.Num() > 0)
    {
        // 添加原始路径的起点
        CompletePath.Add(CurrentPath[0]);

        // 添加已经完成的路径点（不包括当前点）
        for (int32 i = 1; i < CurrentPathIndex && i < CurrentPath.Num(); ++i)
        {
            CompletePath.Add(CurrentPath[i]);
        }
    }

    // 2. 添加当前位置作为连接点（如果不是路径的起点）
    if (CompletePath.Num() == 0 || !CurrentLocation.Equals(CompletePath.Last(), 1.0f))
    {
        CompletePath.Add(CurrentLocation);
    }

    // 3. 添加新路径点（跳过第一个点如果它太接近当前位置）
    for (int32 i = 0; i < NewPath.Num(); ++i)
    {
        const FVector& NewPoint = NewPath[i];
        // 如果是第一个点且非常接近当前位置，则跳过
        if (i == 0 && CurrentLocation.Equals(NewPoint, 100.0f))
        {
            continue;
        }
        // 如果这个点不是重复的，则添加
        if (CompletePath.Num() == 0 || !NewPoint.Equals(CompletePath.Last(), 1.0f))
        {
            CompletePath.Add(NewPoint);
        }
    }

    // 更新路径
    CurrentPath = CompletePath;
    
    // 重新计算最近的路径点索引
    FVector LocalCurrentLocation = GetActorLocation();
    float MinDistance = MAX_FLT;
    int32 ClosestIndex = 0;
    for (int32 i = 0; i < CurrentPath.Num(); ++i)
    {
        float Dist = FVector::Dist(LocalCurrentLocation, CurrentPath[i]);
        if (Dist < MinDistance)
        {
            MinDistance = Dist;
            ClosestIndex = i;
        }
    }
    CurrentPathIndex = ClosestIndex;
}

void ADroneActor::ResumeMovementFromCurrentPosition()
{
    if (CurrentPath.Num() == 0) return;

    // 找到最近的路径点作为新的起点
    FVector CurrentLocation = GetActorLocation();
    float MinDistance = MAX_FLT;
    int32 ClosestPointIndex = CurrentPathIndex;

    // 从当前索引开始向后查找最近的路径点
    for (int32 i = CurrentPathIndex; i < CurrentPath.Num(); ++i)
    {
        float Distance = FVector::Dist(CurrentLocation, CurrentPath[i]);
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            ClosestPointIndex = i;
        }
    }

    // 设置新的路径索引并开始移动
    CurrentPathIndex = ClosestPointIndex;
    bIsMoving = true;
    UE_LOG(LogTemp, Log, TEXT("[Drone %d] 从位置 %s 继续移动，路径索引: %d"), 
        DroneID, *CurrentLocation.ToString(), CurrentPathIndex);
}

void ADroneActor::SaveDonePathToFile()
{
    FString SaveDir = FPaths::ProjectSavedDir() / TEXT("DronePaths");
    IFileManager::Get().MakeDirectory(*SaveDir, true);
    FString FileName = FString::Printf(TEXT("%s/Drone_%d_DonePath.txt"), *SaveDir, DroneID);

    FString FileContent;
    for (const FVector& Point : DonePath)
    {
        FileContent += FString::Printf(TEXT("%.6f,%.6f,%.6f\n"), Point.X, Point.Y, Point.Z);
    }
    FFileHelper::SaveStringToFile(FileContent, *FileName);
    UE_LOG(LogTemp, Log, TEXT("[Drone %d] DonePath saved to %s"), DroneID, *FileName);
}

bool ADroneActor::LoadDonePathFromFile()
{
    FString SaveDir = FPaths::ProjectSavedDir() / TEXT("DronePaths");
    FString FileName = FString::Printf(TEXT("%s/Drone_%d_DonePath.txt"), *SaveDir, DroneID);

    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *FileName))
        return false;

    DonePath.Empty();
    for (const FString& Line : Lines)
    {
        TArray<FString> Parts;
        Line.ParseIntoArray(Parts, TEXT(","), true);
        if (Parts.Num() == 3)
        {
            DonePath.Add(FVector(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2])));
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[Drone %d] DonePath loaded from %s, count=%d"), DroneID, *FileName, DonePath.Num());
    return true;
}

