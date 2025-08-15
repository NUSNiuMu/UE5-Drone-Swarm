#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TankSplineMovementComponent.h"
#include "TankSplineController.generated.h"

UCLASS(BlueprintType, Blueprintable)
class DRONE_API ATankSplineController : public AActor
{
	GENERATED_BODY()
	
public:	
	ATankSplineController();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// 坦克引用
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Controller")
	AActor* TankActor = nullptr;

	// Spline引用
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Controller")
	class USplineComponent* SplineComponent = nullptr;

	// 坦克移动组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank Spline Controller")
	UTankSplineMovementComponent* TankMovementComponent = nullptr;

	// 自动开始移动
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Controller")
	bool bAutoStartMovement = true;

	// 移动速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Controller", meta = (ClampMin = "0.1"))
	float TankMovementSpeed = 200.0f;

	// 是否循环移动
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Controller")
	bool bLoopMovement = true;

	// 是否跟随spline方向
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Controller")
	bool bFollowSplineDirection = true;

	// 旋转速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Spline Controller", meta = (ClampMin = "0.1"))
	float RotationSpeed = 5.0f;

	// 蓝图可调用的函数
	UFUNCTION(BlueprintCallable, Category = "Tank Spline Controller")
	void SetupTankMovement();

	UFUNCTION(BlueprintCallable, Category = "Tank Spline Controller")
	void StartTankMovement();

	UFUNCTION(BlueprintCallable, Category = "Tank Spline Controller")
	void StopTankMovement();

	UFUNCTION(BlueprintCallable, Category = "Tank Spline Controller")
	void ResetTankToStart();

	UFUNCTION(BlueprintCallable, Category = "Tank Spline Controller")
	void SetTankMovementSpeed(float Speed);

private:
	// 查找坦克和spline组件
	void FindTankAndSpline();
}; 