#include "TankSplineMovementComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"

UTankSplineMovementComponent::UTankSplineMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTankSplineMovementComponent::BeginPlay()
{
	// Super::BeginPlay();
	//
	// // 初始化旋转
	// if (AActor* Owner = GetOwner())
	// {
	// 	CurrentRotation = Owner->GetActorRotation();
	// 	TargetRotation = CurrentRotation;
	// }
}

void UTankSplineMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 获取目标Actor（优先TankActor）
	AActor* TargetActor = TankActor ? TankActor : GetOwner();
	if (!bIsMoving || !TargetSpline || !TargetActor)
	{
		return;
	}


	// 计算新的距离位置
	float DistanceDelta = MovementSpeed * DeltaTime;
	CurrentDistance += DistanceDelta;

	// 检查是否到达spline末端
	if (CurrentDistance >= SplineLength)
	{
		if (bLoopMovement)
		{
			// 循环：重置到起始位置
			CurrentDistance = 0.0f;
		}
		else
		{
			// 不循环：停止移动
			StopMovement();
			return;
		}
	}

	// 获取spline上的位置和方向
	FVector SplineLocation = TargetSpline->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
	FVector SplineDirection = TargetSpline->GetDirectionAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);

	// 设置坦克位置
	TargetActor->SetActorLocation(SplineLocation);

	// 设置坦克朝向
	if (bFollowSplineDirection)
	{
		// 计算目标旋转
		TargetRotation = SplineDirection.Rotation();
		
		// 平滑插值旋转
		CurrentRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed);
		TargetActor->SetActorRotation(CurrentRotation);
	}
}

void UTankSplineMovementComponent::SetSplineToFollow(USplineComponent* SplineComponent)
{
	TargetSpline = SplineComponent;
	
	if (TargetSpline)
	{
		SplineLength = TargetSpline->GetSplineLength();
		UE_LOG(LogTemp, Log, TEXT("TankSplineMovementComponent: Set spline to follow, length: %f"), SplineLength);
	}
	else
	{
		SplineLength = 0.0f;
		UE_LOG(LogTemp, Warning, TEXT("TankSplineMovementComponent: Invalid spline component provided"));
	}
}

void UTankSplineMovementComponent::SetMovementSpeed(float Speed)
{
	MovementSpeed = FMath::Max(0.1f, Speed);
	UE_LOG(LogTemp, Log, TEXT("TankSplineMovementComponent: Movement speed set to %f"), MovementSpeed);
}

void UTankSplineMovementComponent::StartMovement()
{
	if (!TargetSpline)
	{
		UE_LOG(LogTemp, Warning, TEXT("TankSplineMovementComponent: Cannot start movement - no spline set"));
		return;
	}

	bIsMoving = true;
	UE_LOG(LogTemp, Log, TEXT("TankSplineMovementComponent: Movement started"));
}

void UTankSplineMovementComponent::StopMovement()
{
	bIsMoving = false;
	UE_LOG(LogTemp, Log, TEXT("TankSplineMovementComponent: Movement stopped"));
}

void UTankSplineMovementComponent::ResetToStart()
{
	CurrentDistance = 0.0f;

	AActor* TargetActor = TankActor ? TankActor : GetOwner();
	if (TargetSpline && TargetActor)
	{
		FVector StartLocation = TargetSpline->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
		TargetActor->SetActorLocation(StartLocation);
		
		if (bFollowSplineDirection)
		{
			FVector StartDirection = TargetSpline->GetDirectionAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
			FRotator StartRotation = StartDirection.Rotation();
			TargetActor->SetActorRotation(StartRotation);
			CurrentRotation = StartRotation;
			TargetRotation = StartRotation;
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("TankSplineMovementComponent: Reset to start position"));
} 