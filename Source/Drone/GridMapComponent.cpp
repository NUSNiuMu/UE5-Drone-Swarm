// GridMapComponent.cpp
#include "GridMapComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"

UGridMapComponent::UGridMapComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    
    // Default values
    MapOrigin = FVector::ZeroVector;
    MapSize = FVector(50.0f, 50.0f, 10.0f);
    CellSize = 10.0f;
    InflationRadius = 0.3f;
    
    GridDimX = 0;
    GridDimY = 0;
    GridDimZ = 0;
    
    bAutomaticObstacleDetection = true;
}

void UGridMapComponent::BeginPlay()
{
    Super::BeginPlay();
    
    if (GetOwner())
    {
        // 鍒濆鍖栧湴鍥撅紝浠ユ墍鏈夎?呬负涓績
        FVector NewMapSize = FVector(1000.0f, 1000.0f, 200.0f);  // 鏇村ぇ鐨勫湴鍥惧昂瀵?
        float NewCellSize = 5.0f;  // 鏇村ぇ鐨勭綉鏍煎昂瀵?
        InitializeMap(GetOwner()->GetActorLocation(), NewMapSize, NewCellSize);
        
        UE_LOG(LogTemp, Warning, TEXT("GridMapComponent鍒濆鍖栧畬鎴? - 鍦板浘澶у皬: (%.1f, %.1f, %.1f), 缃戞牸灏哄: %.1f"), 
            NewMapSize.X, NewMapSize.Y, NewMapSize.Z, NewCellSize);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GridMapComponent鍒濆鍖栧け璐? - 鏃犳硶鑾峰彇Owner"));
    }
}

void UGridMapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UGridMapComponent::InitializeMap(FVector Origin, FVector Size, float Resolution)
{
    MapOrigin = Origin - Size / 2.0f; // Center the map at Origin
    MapSize = Size;
    CellSize = Resolution;
    
    // Calculate grid dimensions
    GridDimX = FMath::CeilToInt(Size.X / CellSize);
    GridDimY = FMath::CeilToInt(Size.Y / CellSize);
    GridDimZ = FMath::CeilToInt(Size.Z / CellSize);
    
    // Initialize 3D grid
    OccupancyGrid.SetNum(GridDimX);
    for (int x = 0; x < GridDimX; x++)
    {
        OccupancyGrid[x].SetNum(GridDimY);
        for (int y = 0; y < GridDimY; y++)
        {
            OccupancyGrid[x][y].SetNum(GridDimZ);
            for (int z = 0; z < GridDimZ; z++)
            {
                OccupancyGrid[x][y][z] = false; // Not occupied by default
            }
        }
    }
    if (GetOwner())
        UE_LOG(LogTemp, Warning, TEXT("GridMapComponent Owner: %s"), *GetOwner()->GetName());
    UE_LOG(LogTemp, Log, TEXT("Grid map initialized: %d x %d x %d cells"), GridDimX, GridDimY, GridDimZ);
    UE_LOG(LogTemp, Warning, TEXT("MapOrigin: (%.2f, %.2f, %.2f), MapSize: (%.2f, %.2f, %.2f)"),
    MapOrigin.X, MapOrigin.Y, MapOrigin.Z, MapSize.X, MapSize.Y, MapSize.Z);
}


void UGridMapComponent::AddCylindricalObstacles(const TArray<FVector>& Positions, float Radius, float Height, float InInflationRadius)
{
    if (GridDimX == 0 || GridDimY == 0 || GridDimZ == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid map not initialized"));
        return;
    }
    
    // 璁剧疆鑶ㄨ儉鍗婂緞
    this->InflationRadius = InInflationRadius;
    
    // 閬嶅巻鎵?鏈変綅缃紝涓烘瘡涓綅缃坊鍔犲渾鏌卞舰闅滅鐗?
    for (const FVector& Position : Positions)
    {
        // 淇濆瓨鐪熷疄闅滅鐗╁弬鏁?
        FCylinderObstacle Obstacle;
        Obstacle.Center = Position;
        Obstacle.Radius = Radius;
        Obstacle.Height = Height;
        CylinderObstacles.Add(Obstacle);
        
        // 璁＄畻搴曢儴鍜岄《閮ㄧ殑Z鍧愭爣
        float BottomZ = Position.Z - Height/2;
        float TopZ = Position.Z + Height/2;
        
        // 鑾峰彇XY骞抽潰涓婂渾鐨勮寖鍥?
        int MinGridX, MinGridY, MinGridZ;
        int MaxGridX, MaxGridY, MaxGridZ;
        
        // 鍦嗘煴鐨勬渶灏忓鍖呯洅
        FVector MinPoint(Position.X - Radius, Position.Y - Radius, BottomZ);
        FVector MaxPoint(Position.X + Radius, Position.Y + Radius, TopZ);
        
        // 杞崲涓虹綉鏍煎潗鏍?
        WorldToGrid(MinPoint, MinGridX, MinGridY, MinGridZ);
        WorldToGrid(MaxPoint, MaxGridX, MaxGridY, MaxGridZ);
        
        // 闄愬埗鍦ㄧ綉鏍艰寖鍥村唴
        MinGridX = FMath::Clamp(MinGridX, 0, GridDimX-1);
        MinGridY = FMath::Clamp(MinGridY, 0, GridDimY-1);
        MinGridZ = FMath::Clamp(MinGridZ, 0, GridDimZ-1);
        
        MaxGridX = FMath::Clamp(MaxGridX, 0, GridDimX-1);
        MaxGridY = FMath::Clamp(MaxGridY, 0, GridDimY-1);
        MaxGridZ = FMath::Clamp(MaxGridZ, 0, GridDimZ-1);
        
        // 鍦嗘煴浣撲腑蹇冨湪XY骞抽潰鐨勭綉鏍煎潗鏍?
        int CenterGridX, CenterGridY, TempZ;
        WorldToGrid(Position, CenterGridX, CenterGridY, TempZ);
        
        // 鍗婂緞鐨勫钩鏂?(缃戞牸鍗曚綅)
        float RadiusSquaredInCells = FMath::Square(Radius / CellSize);
        
        // 閬嶅巻骞舵爣璁板渾鏌变綋鍐呯殑鎵?鏈夌綉鏍?
        for (int x = MinGridX; x <= MaxGridX; x++)
        {
            for (int y = MinGridY; y <= MaxGridY; y++)
            {
                // 璁＄畻褰撳墠鐐瑰埌鍦嗗績XY骞抽潰璺濈鐨勫钩鏂?
                float distSquared = FMath::Square(x - CenterGridX) + FMath::Square(y - CenterGridY);
                
                // 濡傛灉鍦ㄥ渾鍐?
                if (distSquared <= RadiusSquaredInCells)
                {
                    // 鏍囪璇ュ瀭鐩寸嚎涓婂湪楂樺害鑼冨洿鍐呯殑鎵?鏈夌綉鏍?
                    for (int z = MinGridZ; z <= MaxGridZ; z++)
                    {
                        OccupancyGrid[x][y][z] = true;
                    }
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Added %d cylindrical obstacles with radius %f and height %f"), 
        Positions.Num(), Radius, Height);
    
    // 搴旂敤鑶ㄨ儉
    InflateObstacles(InInflationRadius);
    
    if (OnGridMapUpdated.IsBound())
    {
        OnGridMapUpdated.Broadcast(FVector::ZeroVector); // 鎴栧悎閫傜殑浣嶇疆鍙傛暟
    }
}

bool UGridMapComponent::IsOccupied(const FVector& Position)
{
    int32 GridX, GridY, GridZ;
    bool real =WorldToGrid(Position, GridX, GridY, GridZ);
    if (real)
    {
        return OccupancyGrid[GridX][GridY][GridZ];
    }
    return false;
}

void UGridMapComponent::MarkAsOccupied(const FVector& Position)
{
    int32 GridX, GridY, GridZ;
    if (WorldToGrid(Position, GridX, GridY, GridZ))
    {
        OccupancyGrid[GridX][GridY][GridZ] = true;
        // UE_LOG(LogTemp, Warning, TEXT("宸茬粡鏇存柊闅滅鐗? (%.2f, %.2f, %.2f)"),Position.X, Position.Y, Position.Z);
        if (OnGridMapUpdated.IsBound())
        {
            OnGridMapUpdated.Broadcast(Position);
            // UE_LOG(LogTemp, Warning, TEXT("GridMap: 骞挎挱鍦板浘鏇存柊浜嬩欢锛屼綅缃?: (%.2f, %.2f, %.2f)"), 
            //     Position.X, Position.Y, Position.Z);
        }
    }
}

void UGridMapComponent::GetMapBounds(FVector& OutOrigin, FVector& OutSize)
{
    OutOrigin = MapOrigin + MapSize / 2.0f; // Return center of map
    OutSize = MapSize;
}

bool UGridMapComponent::WorldToGrid(const FVector& WorldPos, int32& GridX, int32& GridY, int32& GridZ)
{
    // 璁＄畻鐩稿浜庡湴鍥惧師鐐圭殑鍋忕Щ
    FVector RelativePos = WorldPos - MapOrigin;
    
    // 杞崲涓虹綉鏍煎潗鏍?
    GridX = FMath::FloorToInt(RelativePos.X / CellSize);
    GridY = FMath::FloorToInt(RelativePos.Y / CellSize);
    GridZ = FMath::FloorToInt(RelativePos.Z / CellSize);
    
    // 妫?鏌ユ槸鍚﹀湪缃戞牸鑼冨洿鍐?
    if (GridX < 0 || GridX >= GridDimX ||
        GridY < 0 || GridY >= GridDimY ||
        GridZ < 0 || GridZ >= GridDimZ)
    {
        
        return false;
    }
    
    return true;
}

FVector UGridMapComponent::GridToWorld(int32 GridX, int32 GridY, int32 GridZ)
{
    // 璁＄畻缃戞牸涓績鐐圭殑涓栫晫鍧愭爣
    FVector WorldPos;
    WorldPos.X = MapOrigin.X + (GridX + 0.5f) * CellSize;
    WorldPos.Y = MapOrigin.Y + (GridY + 0.5f) * CellSize;
    WorldPos.Z = MapOrigin.Z + (GridZ + 0.5f) * CellSize;
    
    return WorldPos;
}


void UGridMapComponent::InflateObstacles(float Radius)
{
    int InflationCells = FMath::CeilToInt(Radius / CellSize);
    
    // Create a copy of the grid
    TArray<TArray<TArray<bool>>> OriginalGrid = OccupancyGrid;
    
    // Apply inflation
    for (int x = 0; x < GridDimX; x++)
    {
        for (int y = 0; y < GridDimY; y++)
        {
            for (int z = 0; z < GridDimZ; z++)
            {
                if (OriginalGrid[x][y][z])
                {
                    // Inflate in all directions
                    for (int dx = -InflationCells; dx <= InflationCells; dx++)
                    {
                        for (int dy = -InflationCells; dy <= InflationCells; dy++)
                        {
                            for (int dz = -InflationCells; dz <= InflationCells; dz++)
                            {
                                int nx = x + dx;
                                int ny = y + dy;
                                int nz = z + dz;
                                
                                // Check bounds
                                if (nx >= 0 && nx < GridDimX &&
                                    ny >= 0 && ny < GridDimY &&
                                    nz >= 0 && nz < GridDimZ)
                                {
                                    // Check if within inflation radius
                                    float distance = FMath::Sqrt((float)(dx*dx + dy*dy + dz*dz)) * CellSize;
                                    if (distance <= Radius && !OccupancyGrid[nx][ny][nz])
                                    {
                                        OccupancyGrid[nx][ny][nz] = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


void UGridMapComponent::ClearObstacles()
{
    for (int x = 0; x < GridDimX; x++)
        for (int y = 0; y < GridDimY; y++)
            for (int z = 0; z < GridDimZ; z++)
                OccupancyGrid[x][y][z] = false;
    if (OnGridMapUpdated.IsBound())
        OnGridMapUpdated.Broadcast(FVector::ZeroVector);
}

