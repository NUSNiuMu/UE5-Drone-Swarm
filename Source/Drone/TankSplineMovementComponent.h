#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "TankSplineMovementComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DRONE_API UTankSplineMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTankSplineMovementComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 设置要跟随的Spline
	UFUNCTION(BlueprintCallable, Category = "Tank Spline Movement")
	void SetSplineToFollow(USplineComponent* SplineComponent);

	// 设置移动速度 (单位/秒)
	UFUNCTION(BlueprintCallable, Category = "Tank Spline Movement")
	void SetMovementSpeed(float Speed);

	// 开始移动
	UFUNCTION(BlueprintCallable, Category = "Tank Spline Movement")
	void StartMovement();

	// 停止移动
	UFUNCTION(BlueprintCallable, Category = "Tank Spline Movement")
	void StopMovement();

	// 重置到起始位置
	UFUNCTION(BlueprintCallable, Category = "Tank Spline Movement")
	void ResetToStart();

	// 是否循环移动
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Movement")
	bool bLoopMovement = true;

	// 移动速度 (单位/秒)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Movement", meta = (ClampMin = "0.1"))
	float MovementSpeed = 200.0f;

	// 是否在移动
	UPROPERTY(BlueprintReadOnly, Category = "Tank Spline Movement")
	bool bIsMoving = false;

	// 当前在spline上的距离
	UPROPERTY(BlueprintReadOnly, Category = "Tank Spline Movement")
	float CurrentDistance = 0.0f;

	// 坦克朝向是否跟随spline方向
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Movement")
	bool bFollowSplineDirection = true;

	// 平滑旋转速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Movement", meta = (ClampMin = "0.1"))
	float RotationSpeed = 5.0f;

	// 指定要移动的坦克Actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Movement")
	AActor* TankActor = nullptr;

private:
	// 要跟随的Spline组件
	UPROPERTY()
	USplineComponent* TargetSpline = nullptr;

	// Spline的总长度
	float SplineLength = 0.0f;

	// 目标旋转
	FRotator TargetRotation;

	// 当前旋转
	FRotator CurrentRotation;
}; 