#include "TankSplineController.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ATankSplineController::ATankSplineController()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建坦克移动组件
	TankMovementComponent = CreateDefaultSubobject<UTankSplineMovementComponent>(TEXT("TankMovementComponent"));
}

void ATankSplineController::BeginPlay()
{
	// Super::BeginPlay();
	//
	// // 查找坦克和spline
	// FindTankAndSpline();
	//
	// // 设置坦克移动
	// SetupTankMovement();
	//
	// // 如果启用自动开始，则开始移动
	// if (bAutoStartMovement)
	// {
	// 	StartTankMovement();
	// }
}

void ATankSplineController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATankSplineController::FindTankAndSpline()
{
	// 如果没有手动设置坦克，尝试在场景中查找
	if (!TankActor)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);
		
		for (AActor* Actor : FoundActors)
		{
			// 查找包含"BP_M1A2Abrams_Showcase1_Explode"的actor
			if (Actor->GetName().Contains(TEXT("BP_M1A2Abrams_Showcase1_Explode")))
			{
				TankActor = Actor;
				UE_LOG(LogTemp, Log, TEXT("TankSplineController: Found tank actor: %s"), *Actor->GetName());
				break;
			}
		}
	}

	// 如果没有手动设置spline，尝试在场景中查找
	if (!SplineComponent)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);
		
		for (AActor* Actor : FoundActors)
		{
			// 查找包含"BP_M1A2_Controller_Chaos1_Trajectory"的actor
			if (Actor->GetName().Contains(TEXT("Task_SplineMy1")))
			{
				// 查找该actor上的SplineComponent
				USplineComponent* FoundSpline = Actor->FindComponentByClass<USplineComponent>();
				if (FoundSpline)
				{
					SplineComponent = FoundSpline;
					UE_LOG(LogTemp, Log, TEXT("TankSplineController: Found spline component from actor: %s"), *Actor->GetName());
					break;
				}
			}
		}
	}
}

void ATankSplineController::SetupTankMovement()
{
	if (!TankMovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("TankSplineController: Tank movement component is null"));
		return;
	}

	// 设置spline
	if (SplineComponent)
	{
		TankMovementComponent->SetSplineToFollow(SplineComponent);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TankSplineController: No spline component found"));
	}

	// 设置要移动的坦克
	TankMovementComponent->TankActor = TankActor;

	// 设置移动参数
	TankMovementComponent->SetMovementSpeed(TankMovementSpeed);
	TankMovementComponent->bLoopMovement = bLoopMovement;
	TankMovementComponent->bFollowSplineDirection = bFollowSplineDirection;
	TankMovementComponent->RotationSpeed = RotationSpeed;

	UE_LOG(LogTemp, Log, TEXT("TankSplineController: Tank movement setup completed"));
}

void ATankSplineController::StartTankMovement()
{
	if (TankMovementComponent)
	{
		TankMovementComponent->StartMovement();
		UE_LOG(LogTemp, Log, TEXT("TankSplineController: Tank movement started"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TankSplineController: Cannot start movement - tank movement component is null"));
	}
}

void ATankSplineController::StopTankMovement()
{
	if (TankMovementComponent)
	{
		TankMovementComponent->StopMovement();
		UE_LOG(LogTemp, Log, TEXT("TankSplineController: Tank movement stopped"));
	}
}

void ATankSplineController::ResetTankToStart()
{
	if (TankMovementComponent)
	{
		TankMovementComponent->ResetToStart();
		UE_LOG(LogTemp, Log, TEXT("TankSplineController: Tank reset to start position"));
	}
}

void ATankSplineController::SetTankMovementSpeed(float Speed)
{
	TankMovementSpeed = Speed;
	if (TankMovementComponent)
	{
		TankMovementComponent->SetMovementSpeed(Speed);
	}
	UE_LOG(LogTemp, Log, TEXT("TankSplineController: Tank movement speed set to %f"), Speed);
} 