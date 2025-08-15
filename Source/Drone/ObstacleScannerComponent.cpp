// ObstacleScannerComponent.cpp
#include "ObstacleScannerComponent.h"
#include "DroneActor.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Async/ParallelFor.h"
#include "Misc/FileHelper.h"

UObstacleScannerComponent::UObstacleScannerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    GridMap = nullptr;
}

void UObstacleScannerComponent::BeginPlay()
{
    Super::BeginPlay();

    // 启动定时器，每0.1秒执行一次扫描
    if (bAutoScan)
    {
        GetWorld()->GetTimerManager().SetTimer(
            ScanTimerHandle,
            this,
            &UObstacleScannerComponent::OnScanTimer,
            ScanInterval,
            true  // 循环执行
        );

        // UE_LOG(LogTemp, Log, TEXT("[ObstacleScanner] ScanTimerHandle set!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
    }
}

void UObstacleScannerComponent::OnScanTimer()
{
    AActor* Owner = GetOwner();
    if (Owner)
    {
        ScanArea(Owner->GetActorLocation(), ScanRadius);
    }
}

void UObstacleScannerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UObstacleScannerComponent::SetGridMap(UGridMapComponent* InGridMap)
{
    if (!InGridMap)
    {
        UE_LOG(LogTemp, Error, TEXT("ObstacleScanner: Trying to set null GridMap"));
        return;
    }
    GridMap = InGridMap;
    UE_LOG(LogTemp, Log, TEXT("ObstacleScanner: GridMap set successfully"));
}

void UObstacleScannerComponent::ScanArea(const FVector& Center, float Radius)
{
    if (!GridMap)
    {
        // 尝试在场景中查找GridMap组件
        TArray<AActor*> GridMapActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), GridMapActors);
        
        // 遍历所有Actor查找GridMap组件
        for (AActor* Actor : GridMapActors)
        {
            if (UGridMapComponent* FoundGridMap = Actor->FindComponentByClass<UGridMapComponent>())
            {
                GridMap = FoundGridMap;
                UE_LOG(LogTemp, Warning, TEXT("ObstacleScanner: Found GridMap in actor: %s"), *Actor->GetName());
                break;
            }
        }
        
        if (!GridMap)
        {
            UE_LOG(LogTemp, Error, TEXT("ObstacleScanner: GridMap is not valid and not found in scene"));
            return;
        }
    }

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("ObstacleScanner: World is not valid"));
        return;
    }

    // 获取无人机前向方向
    AActor* Owner = GetOwner();
    if (!Owner)
        return;
    
    // 获取所有无人机的引用，用于忽略扫描
    TArray<AActor*> DroneActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADroneActor::StaticClass(), DroneActors);
    
    // UE_LOG(LogTemp, Warning, TEXT("Found %d drones to ignore"), DroneActors.Num());
    
    FRotator OwnerRotation = Owner->GetActorRotation();
    FVector ForwardVector = OwnerRotation.Vector();
    FVector RightVector = FVector::CrossProduct(FRotator(0, OwnerRotation.Yaw, 0).Vector(), FVector::UpVector);
    FVector UpVector = FVector::CrossProduct(ForwardVector, RightVector);

    // 使用可配置的高度偏移
    FVector ScanStartPoint = Center + FVector(0, 0, ScanStartHeight);

    // 计算扫描范围
    float StartDistance = 80.0f;  // 从扫描起点前方80厘米开始扫描
    float EndDistance = Radius;   // 扫描到最大半径
    
    // 计算垂直角度步长
    float VerticalStep = FMath::DegreesToRadians(VerticalScanAngle) / (NumScanLines - 1);
    float VerticalStart = -FMath::DegreesToRadians(VerticalScanAngle) / 2.0f;
    
    // 计算水平角度步长
    float HorizontalStep = FMath::DegreesToRadians(HorizontalScanAngle) / (PointsPerLine - 1);
    float HorizontalStart = -FMath::DegreesToRadians(HorizontalScanAngle) / 2.0f;

    // 创建扫描数据数组
    TArray<FScanData> ScanDataArray;
    ScanDataArray.SetNum(NumScanLines * PointsPerLine);

    // 设置碰撞查询参数
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = true;  // 启用复杂碰撞以检测所有组件
    QueryParams.bReturnPhysicalMaterial = false;
    QueryParams.bIgnoreTouches = true;  // 忽略Touch事件，只关注实际的碰撞
    
    // 忽略所有无人机及其组件
    QueryParams.AddIgnoredActor(Owner);  // 首先忽略自身
    
    // 存储所有需要忽略的组件
    TArray<UPrimitiveComponent*> AllDroneComponents;
    
    for (AActor* DroneActor : DroneActors)
    {
        if (DroneActor != Owner)  // 避免重复添加自身
        {
            QueryParams.AddIgnoredActor(DroneActor);
            
            // 获取并忽略所有组件
            TArray<USceneComponent*> Components;
            DroneActor->GetComponents<USceneComponent*>(Components);
            for (USceneComponent* Component : Components)
            {
                if (UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(Component))
                {
                    QueryParams.AddIgnoredComponent(PrimComponent);
                    AllDroneComponents.Add(PrimComponent);
                    
                    // 获取所有附加的子组件
                    TArray<USceneComponent*> ChildComponents;
                    PrimComponent->GetChildrenComponents(true, ChildComponents);
                    for (USceneComponent* ChildComponent : ChildComponents)
                    {
                        if (UPrimitiveComponent* ChildPrimComponent = Cast<UPrimitiveComponent>(ChildComponent))
                        {
                            QueryParams.AddIgnoredComponent(ChildPrimComponent);
                            AllDroneComponents.Add(ChildPrimComponent);
                        }
                    }
                }
            }
        }
    }

    // 修改碰撞检测通道
    ECollisionChannel CollisionChannel = ECC_GameTraceChannel1;  // 使用自定义通道

    // 使用ParallelFor并行处理扫描线
    ParallelFor(NumScanLines, [&](int32 LineIndex)
    {
        // 计算当前线的垂直角度
        float VerticalAngle = VerticalStart + LineIndex * VerticalStep;
        
        // 计算当前线的垂直方向（相对于水平面）
        FVector VerticalDirection = FVector::UpVector * FMath::Sin(VerticalAngle);
        
        // 对当前线进行水平扫描
        for (int32 PointIndex = 0; PointIndex < PointsPerLine; PointIndex++)
        {
            // 计算当前点的水平角度
            float HorizontalAngle = HorizontalStart + PointIndex * HorizontalStep;
            
            // 计算水平方向（相对于前向方向）
            FVector HorizontalDirection = ForwardVector * FMath::Cos(HorizontalAngle) + RightVector * FMath::Sin(HorizontalAngle);
            
            // 组合垂直和水平方向
            FVector Direction = (HorizontalDirection + VerticalDirection).GetSafeNormal();
            
            // 计算扫描起点和终点
            FVector Start = ScanStartPoint + Direction * StartDistance;
            FVector End = ScanStartPoint + Direction * EndDistance;

            // 执行射线检测
            FHitResult HitResult;
            bool bHit = GetWorld()->LineTraceSingleByChannel(
                HitResult,
                Start,
                End,
                CollisionChannel,  // 使用自定义通道
                QueryParams
            );

            // 存储扫描结果
            int32 ArrayIndex = LineIndex * PointsPerLine + PointIndex;
            ScanDataArray[ArrayIndex].bHit = bHit;
            if (bHit)
            {
                bool bIsValidHit = true;
                // 检查是否击中无人机或其组件
                if (HitResult.GetActor() || HitResult.GetComponent())
                {
                    AActor* HitActor = HitResult.GetActor();
                    UPrimitiveComponent* HitComponent = HitResult.GetComponent();
                    
                    // 检查是否击中无人机
                    bool bIsDroneHit = false;
                    
                    // 检查1：直接检查Actor
                    if (Cast<ADroneActor>(HitActor))
                    {
                        bIsDroneHit = true;
                    }
                    // 检查2：检查组件是否属于无人机
                    else if (HitComponent)
                    {
                        // 检查组件是否在我们的忽略列表中
                        if (AllDroneComponents.Contains(HitComponent))
                        {
                            bIsDroneHit = true;
                        }
                        // 检查组件的所有父级是否是无人机组件
                        else
                        {
                            USceneComponent* CurrentComponent = HitComponent->GetAttachParent();
                            while (CurrentComponent)
                            {
                                if (UPrimitiveComponent* ParentPrim = Cast<UPrimitiveComponent>(CurrentComponent))
                                {
                                    if (AllDroneComponents.Contains(ParentPrim))
                                    {
                                        bIsDroneHit = true;
                                        break;
                                    }
                                }
                                CurrentComponent = CurrentComponent->GetAttachParent();
                            }
                        }
                    }
                    
                    if (bIsDroneHit)
                    {
                        bIsValidHit = false;
                    }
                }

                if (bIsValidHit)
                {
                    ScanDataArray[ArrayIndex].HitLocation = HitResult.Location;
                }
                else
                {
                    ScanDataArray[ArrayIndex].bHit = false;
                }
            }
        }
    });

    // 处理扫描结果
    for (const FScanData& ScanData : ScanDataArray)
    {
        if (ScanData.bHit)
        {
            // 对障碍物进行膨胀处理
            const float InflationRadius = 80.0f; // 膨胀半径80厘米
           
            // 计算膨胀后的网格范围
            int32 GridX, GridY, GridZ;
            if (GridMap->WorldToGrid(ScanData.HitLocation, GridX, GridY, GridZ))
            {
                // 计算膨胀范围（以网格单位）
                int32 InflationCells = FMath::CeilToInt(InflationRadius / GridMap->GetResolution());
                    
                // 在膨胀范围内标记障碍物
                for (int32 X = -InflationCells; X <= InflationCells; X++)
                {
                    for (int32 Y = -InflationCells; Y <= InflationCells; Y++)
                    {
                        for (int32 Z = -InflationCells/2; Z <= InflationCells/2; Z++)
                        {
                            FVector InflatedLocation = GridMap->GridToWorld(GridX + X, GridY + Y, GridZ + Z);
                            float Distance = FVector::Distance(InflatedLocation, ScanData.HitLocation);
                           
                            if (Distance <= InflationRadius)
                            {
                                // 检查该位置是否已经被标记为障碍物
                                if (!GridMap->IsOccupied(InflatedLocation))
                                {
                                    int32 GridX2, GridY2, GridZ2;
                                    if (GridMap->WorldToGrid(InflatedLocation,GridX2, GridY2, GridZ2))
                                    {
                                        UpdateGridMap(InflatedLocation, true);
                                        // UE_LOG(LogTemp, Warning, TEXT("update obs"))
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 调试可视化
            if (bShowDebugVisualization)
            {
                DrawDebugSphere(
                    GetWorld(),
                    ScanData.HitLocation,
                    5.0f,  // 显示膨胀半径
                    16,     // 增加分段数以显示更平滑的球体
                    FColor::Red,
                    false,
                    ScanInterval,
                    0,
                    1.0f
                );
            }
        }
    }
    ScanDataArray.Empty();
}


void UObstacleScannerComponent::UpdateGridMap(const FVector& HitLocation, bool bIsObstacle)
{
    // UE_LOG(LogTemp,Log,TEXT("[obstaclescan]:we are going to update"));
    if (!GridMap)
    {
        UE_LOG(LogTemp,Log,TEXT("[obstaclescan]:Fuck , we fail because can not find gridmap"));
        return;
    }
    
    // 更新网格状态
    if (bIsObstacle)
    {
        GridMap->MarkAsOccupied(HitLocation);
    }
    
}




