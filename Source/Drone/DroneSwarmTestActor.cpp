#include "DroneSwarmTestActor.h"
#include "Kismet/GameplayStatics.h"

ADroneSwarmTestActor::ADroneSwarmTestActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建群组管理器组件
    SwarmManager = CreateDefaultSubobject<UDroneSwarmManagerComponent>(TEXT("SwarmManager"));

    // 创建网格地图组件
    GridMap = CreateDefaultSubobject<UGridMapComponent>(TEXT("GridMap"));
}

void ADroneSwarmTestActor::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Starting drone swarm test initialization. UseBeaconSystem: %d"), bUseBeaconSystem);

    // 插入3秒延迟
    // FPlatformProcess::Sleep(3.0f);

    // 如果使用信标系统，先获取所有信标
    TArray<AActor*> AllActors;
    TMap<int32, AActor*> StartBeaconMap;
    TMap<int32, AActor*> EndBeaconMap;
    
    if (bUseBeaconSystem)
    {
        // 获取所有Actor
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
        
        UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Found %d total actors in the world"), AllActors.Num());
        
        // 遍历所有Actor查找带编号的信标
        for (AActor* Actor : AllActors)
        {
            if (!Actor) continue;
            
            bool bFoundValidTag = false;
            for (const FName& Tag : Actor->Tags)
            {
                FString TagString = Tag.ToString();
                
                // 检查是否是起点信标
                if (TagString.StartsWith(StartBeaconTagPrefix))
                {
                    FString NumberStr = TagString.RightChop(StartBeaconTagPrefix.Len());
                    if (NumberStr.IsNumeric())
                    {
                        int32 Number = FCString::Atoi(*NumberStr);
                        if (Number >= 0)
                        {
                            StartBeaconMap.Add(Number, Actor);
                            UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Found valid start beacon %d at %s with tag %s"), 
                                Number, *Actor->GetActorLocation().ToString(), *TagString);
                            bFoundValidTag = true;
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("[SwarmTest] Ignoring start beacon with invalid number %d (tag: %s)"), 
                                Number, *TagString);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[SwarmTest] Start beacon tag has non-numeric suffix: %s"), *TagString);
                    }
                }
                // 检查是否是终点信标
                else if (TagString.StartsWith(EndBeaconTagPrefix))
                {
                    FString NumberStr = TagString.RightChop(EndBeaconTagPrefix.Len());
                    if (NumberStr.IsNumeric())
                    {
                        int32 Number = FCString::Atoi(*NumberStr);
                        if (Number >= 0)
                        {
                            EndBeaconMap.Add(Number, Actor);
                            UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Found valid end beacon %d at %s with tag %s"), 
                                Number, *Actor->GetActorLocation().ToString(), *TagString);
                            bFoundValidTag = true;
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("[SwarmTest] Ignoring end beacon with invalid number %d (tag: %s)"), 
                                Number, *TagString);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[SwarmTest] End beacon tag has non-numeric suffix: %s"), *TagString);
                    }
                }
            }
            
            if (Actor->Tags.Num() > 0 && !bFoundValidTag)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SwarmTest] Actor %s has tags but none match beacon format. Tags:"), *Actor->GetName());
                for (const FName& Tag : Actor->Tags)
                {
                    UE_LOG(LogTemp, Warning, TEXT("    - %s"), *Tag.ToString());
                }
            }
        }

        // 检查信标配对
        TArray<int32> StartNumbers;
        StartBeaconMap.GetKeys(StartNumbers);
        bool bHasValidPairs = true;
        
        UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Found %d start beacons and %d end beacons"), 
            StartBeaconMap.Num(), EndBeaconMap.Num());
            
        for (int32 Number : StartNumbers)
        {
            if (!EndBeaconMap.Contains(Number))
            {
                UE_LOG(LogTemp, Error, TEXT("[SwarmTest] Missing end beacon for number %d"), Number);
                bHasValidPairs = false;
                break;
            }
        }

        if (!bHasValidPairs || StartBeaconMap.Num() != EndBeaconMap.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("[SwarmTest] Invalid beacon pairs. Start beacons: %d, End beacons: %d"), 
                StartBeaconMap.Num(), EndBeaconMap.Num());
            return;
        }

        // 更新无人机数量为信标对数量
        NumDrones = StartBeaconMap.Num();
        UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Using beacon system with %d beacon pairs"), NumDrones);

        // 计算所有信标的边界框
        FVector MinBound(MAX_FLT);
        FVector MaxBound(-MAX_FLT);
        
        for (const auto& Pair : StartBeaconMap)
        {
            FVector Location = Pair.Value->GetActorLocation();
            MinBound = FVector(
                FMath::Min(MinBound.X, Location.X),
                FMath::Min(MinBound.Y, Location.Y),
                FMath::Min(MinBound.Z, Location.Z)
            );
            MaxBound = FVector(
                FMath::Max(MaxBound.X, Location.X),
                FMath::Max(MaxBound.Y, Location.Y),
                FMath::Max(MaxBound.Z, Location.Z)
            );
        }
        
        for (const auto& Pair : EndBeaconMap)
        {
            FVector Location = Pair.Value->GetActorLocation();
            MinBound = FVector(
                FMath::Min(MinBound.X, Location.X),
                FMath::Min(MinBound.Y, Location.Y),
                FMath::Min(MinBound.Z, Location.Z)
            );
            MaxBound = FVector(
                FMath::Max(MaxBound.X, Location.X),
                FMath::Max(MaxBound.Y, Location.Y),
                FMath::Max(MaxBound.Z, Location.Z)
            );
        }

        // 添加外边距
        MinBound -= FVector(MapMargin);
        MaxBound += FVector(MapMargin);

        // 更新网格地图配置
        FVector MapSize = MaxBound - MinBound;
        FVector MapCenter = (MaxBound + MinBound) * 0.5f;
        
        // 由于 InitializeMap 会将原点偏移 Size/2，这里我们需要补偿这个偏移
        GridMapOrigin = MapCenter;
        GridMapSize = MapSize;

        // --------- 自适应分辨率 ---------
        const int32 MaxGridCount = 500; // 最大格子数
        float MaxMapEdge = FMath::Max(GridMapSize.X, GridMapSize.Y);
        LocalGridResolution = MaxMapEdge / MaxGridCount;
        if (LocalGridResolution < 20.0f)
        {
            GridResolution = 20.0f;
        }
        else if (LocalGridResolution > 200.0f)
        {
            GridResolution = 200.0f;
        }
        // 如果在[20,200]之间则不更改
        UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Adaptive GridResolution: %.2f (MapSize: %.2f x %.2f, MaxGridCount: %d)"),
            GridResolution, GridMapSize.X, GridMapSize.Y, MaxGridCount);
        // --------- END ---------

        UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Grid map configuration - Center: %s, Size: %s"), 
            *GridMapOrigin.ToString(), *GridMapSize.ToString());
    }

    // 初始化网格地图
    InitializeGridMap();

    // 确保设置了无人机类
    if (!DroneClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[SwarmTest] DroneClass not set in DroneSwarmTestActor!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Spawning %d drones"), NumDrones);

    // 生成无人机
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParams.bNoFail = true;
    SpawnParams.ObjectFlags |= RF_Transient;  // 防止自动生成AIController

    // 根据是否使用信标系统决定生成位置
    if (bUseBeaconSystem)
    {
        TArray<int32> Numbers;
        StartBeaconMap.GetKeys(Numbers);
        Numbers.Sort();  // 按编号顺序生成无人机
        
        for (int32 Number : Numbers)
        {
            AActor* StartBeacon = StartBeaconMap[Number];
            FVector SpawnLocation = StartBeacon->GetActorLocation();
            FRotator SpawnRotation = StartBeacon->GetActorRotation();
            
            UE_LOG(LogTemp, Verbose, TEXT("[SwarmTest] Attempting to spawn drone %d at location: %s"), 
                Number, *SpawnLocation.ToString());

            // 生成无人机
            ADroneActor* NewDrone = GetWorld()->SpawnActor<ADroneActor>(
                DroneClass,
                SpawnLocation,
                SpawnRotation,
                SpawnParams
            );

            if (NewDrone)
            {
                // 设置无人机ID
                NewDrone->SetDroneID(Number);
                
                // 设置无人机速度
                NewDrone->SetDroneSpeed(DroneSpeed);
                
                // 设置所有组件的GridMap引用
                NewDrone->SetupGridMapReferences(GridMap);
                
                // 启用障碍扫描
                if (UObstacleScannerComponent* Scanner = NewDrone->GetObstacleScanner())
                {
                    Scanner->bAutoScan = true;  // 启用自动扫描
                    Scanner->ScanInterval = 0.2f;  // 每0.1秒扫描一次
                    Scanner->ScanRadius = 800.0f;  // 扫描半径500厘米
                    Scanner->VerticalScanAngle = 60.0f;  // 垂直扫描角度
                    Scanner->HorizontalScanAngle = 120.0f;  // 水平扫描角度
                    Scanner->NumScanLines = 8;  // 扫描线数量
                    Scanner->PointsPerLine = 100;  // 每线点数
                    Scanner->ScanStartHeight = 0.0f;  // 扫描起始高度
                    Scanner->bShowDebugVisualization = false;  // 禁用调试可视化
                    
                    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Enabled obstacle scanning for drone %d"), Number);
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] something wrong with obstacle scanning "));
                }
                
                // 将无人机添加到数组
                SpawnedDrones.Add(NewDrone);

                UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Successfully spawned drone %d at %s"), 
                    Number, *SpawnLocation.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[SwarmTest] Failed to spawn drone %d"), Number);
            }
        }
    }
    else
    {
        // 使用原有的队形配置
        for (int32 i = 0; i < NumDrones; ++i)
        {
            float Offset = -FormationConfig.FormationWidth * 0.5f + i * (FormationConfig.FormationWidth / (float)(NumDrones - 1));
            FVector ForwardVector = FormationConfig.FormationRotation.Vector();
            FVector RightVector = FRotator(0, 90, 0).RotateVector(ForwardVector);
            FVector SpawnLocation = FormationConfig.StartLocation + (RightVector * Offset);
            SpawnLocation.Z = DroneHeight;
            FRotator SpawnRotation = FormationConfig.FormationRotation;
            
            UE_LOG(LogTemp, Verbose, TEXT("[SwarmTest] Attempting to spawn drone %d at location: %s"), 
                i, *SpawnLocation.ToString());

            // 生成无人机
            ADroneActor* NewDrone = GetWorld()->SpawnActor<ADroneActor>(
                DroneClass,
                SpawnLocation,
                SpawnRotation,
                SpawnParams
            );

            if (NewDrone)
            {
                NewDrone->SetDroneID(i);
                NewDrone->SetupGridMapReferences(GridMap);
                
                if (UObstacleScannerComponent* Scanner = NewDrone->GetObstacleScanner())
                {
                    Scanner->bAutoScan = true;
                    Scanner->ScanInterval = 0.1f;
                    Scanner->ScanRadius = 800.0f;
                    Scanner->VerticalScanAngle = 120.0f;
                    Scanner->HorizontalScanAngle = 180.0f;
                    Scanner->NumScanLines = 16;
                    Scanner->PointsPerLine = 100;
                    Scanner->ScanStartHeight = 0.0f;
                    Scanner->bShowDebugVisualization = false;
                    
                    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Enabled obstacle scanning for drone %d"), i);
                }
                
                SpawnedDrones.Add(NewDrone);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Successfully spawned %d/%d drones"), 
        SpawnedDrones.Num(), NumDrones);

    // 修改StartSwarmTest调用，传递终点信标信息
    if (bUseBeaconSystem)
    {
        TArray<FVector> TargetLocations;
        TArray<int32> Numbers;
        StartBeaconMap.GetKeys(Numbers);
        Numbers.Sort();  // 确保目标位置顺序与无人机ID匹配
        
        for (int32 Number : Numbers)
        {
            if (AActor* EndBeacon = EndBeaconMap[Number])
            {
                TargetLocations.Add(EndBeacon->GetActorLocation());
            }
        }
        
        TargetConfig.bUseRandomTargets = false;
        TargetConfig.FixedTargetLocations = TargetLocations;
    }

    StartSwarmTest();
}

void ADroneSwarmTestActor::InitializeGridMap()
{
    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Initializing grid map"));

    if (GridMap)
    {
        // 初始化网格地图
        GridMap->InitializeMap(GridMapOrigin, GridMapSize, GridResolution);
        
        UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Grid map initialized - Origin: %s, Size: %s, Resolution: %.2f"), 
            *GridMapOrigin.ToString(), *GridMapSize.ToString(), GridResolution);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SwarmTest] Failed to initialize grid map: component not found"));
    }
}

void ADroneSwarmTestActor::AddObstacle(const FVector& Location, float Radius, float Height)
{
    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Adding obstacle at %s (Radius: %.2f, Height: %.2f)"), 
        *Location.ToString(), Radius, Height);

    if (GridMap)
    {
        TArray<FVector> ObstaclePositions;
        ObstaclePositions.Add(Location);
        GridMap->AddCylindricalObstacles(ObstaclePositions, Radius, Height);
        
        UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Successfully added cylindrical obstacle"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SwarmTest] Failed to add obstacle: GridMap not found"));
    }
}

void ADroneSwarmTestActor::StartSwarmTest()
{
    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Starting swarm test with %d drones"), SpawnedDrones.Num());

    if (!SwarmManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[SwarmTest] Cannot start test: SwarmManager not found!"));
        return;
    }

    // 为每个无人机设置目标位置并添加到群组管理器
    for (int32 i = 0; i < SpawnedDrones.Num(); ++i)
    {
        ADroneActor* Drone = SpawnedDrones[i];
        if (Drone)
        {
            FVector GoalLocation;
            
            // 根据配置决定使用随机目标还是固定目标
            if (TargetConfig.bUseRandomTargets)
            {
                GoalLocation = GetRandomPositionInArea(TargetConfig.TargetCenter, TargetConfig.TargetAreaSize);
                UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Generated random target for drone %d at %s"), 
                    i, *GoalLocation.ToString());
            }
            else
            {
                // 如果有固定目标点且索引有效，使用固定目标点
                if (TargetConfig.FixedTargetLocations.IsValidIndex(i))
                {
                    GoalLocation = TargetConfig.FixedTargetLocations[i];
                    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Using fixed target for drone %d at %s"), 
                        i, *GoalLocation.ToString());
                }
                else
                {
                    // 如果没有对应的固定目标点，使用默认目标区域中心
                    GoalLocation = TargetConfig.TargetCenter;
                    UE_LOG(LogTemp, Warning, TEXT("[SwarmTest] No fixed target location for drone %d, using target center: %s"), 
                        i, *GoalLocation.ToString());
                }
            }
            
            // 将无人机添加到群组管理器
            SwarmManager->AddDrone(Drone, GoalLocation);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[SwarmTest] Invalid drone at index %d"), i);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Starting path planning for the swarm"));

    // 开始群体路径规划
    SwarmManager->StartSwarmPathPlanning();
    

    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Path visualization enabled for 10 seconds"));

    // 开始所有无人机的运动
    SwarmManager->StartAllDrones();
    SwarmManager->ToggleAllDronesMovement();

    UE_LOG(LogTemp, Log, TEXT("[SwarmTest] Swarm test started successfully"));
}

FVector ADroneSwarmTestActor::GetRandomPositionInArea(const FVector& Center, const FVector& Size)
{
    FVector RandomOffset;
    RandomOffset.X = FMath::RandRange(-Size.X * 0.5f, Size.X * 0.5f);
    RandomOffset.Y = FMath::RandRange(-Size.Y * 0.5f, Size.Y * 0.5f);
    RandomOffset.Z = FMath::RandRange(-Size.Z * 0.5f, Size.Z * 0.5f);

    FVector Result = Center + RandomOffset;
    
    UE_LOG(LogTemp, Verbose, TEXT("[SwarmTest] Generated random position: %s (Center: %s, Size: %s)"), 
        *Result.ToString(), *Center.ToString(), *Size.ToString());

    return Result;
} 