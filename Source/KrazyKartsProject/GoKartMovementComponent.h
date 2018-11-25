// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float Time;

	bool IsValid() const
	{
		return FMath::Abs(Throttle) <= 1 && FMath::Abs(SteeringThrow) <= 1;
	}
};




UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTSPROJECT_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FVector GetKartVelocity() const;

	void SetKartVelocity(FVector Val);

	void SetThrottle(float Val);

	void SetSteeringThrow(float Val);


	void SimulateMove(const FGoKartMove& Move);

	FGoKartMove GetLastMove() const;

private:
	FVector KartVelocity;

	// Force that is in proportion to the kart's acceleration
	float Throttle;

	float SteeringThrow;

	// Mass of the car
	UPROPERTY(EditAnywhere, Category = "GoKart")
	float Mass = 1000;

	UPROPERTY(EditAnywhere, Category = "GoKart")
	float MaxDrivingForce = 10000;

	// The number of degrees rotated per second at full control throw
	UPROPERTY(EditAnywhere, Category = "GoKart")
	float MaxDegreesPerSecond = 90;

	UPROPERTY(EditAnywhere, Category = "GoKart")
	float DragCoefficient = 16.f;

	UPROPERTY(EditAnywhere, Category = "GoKart")
	float RollingResistanceCoefficient = 0.015f;

	// Minimum radius of car turning circle at full lock (m).
	UPROPERTY(EditAnywhere, Category = "GoKart")
	float MinTurnRadius = 10.f;

	FGoKartMove LastMove;

	
private:
	void MoveKart(FGoKartMove Move);

	void UpdateRotation(FGoKartMove Move);

	void UpdateLocationFromVelocity(float DeltaTime);

	FVector GetAirResistance();

	FVector GetRollingResistance();

	FGoKartMove CreateMove(float DeltaTime);

};
