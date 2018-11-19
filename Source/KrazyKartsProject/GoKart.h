// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

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
};

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FGoKartMove LastMove;
};


UCLASS()
class KRAZYKARTSPROJECT_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(Replicated)
	FVector KartVelocity;

	UPROPERTY(ReplicatedUsing= OnRep_ReplicatedTransform)
	FTransform ReplicatedTransform;

	// Force that is in proportion to the kart's acceleration
	UPROPERTY(Replicated)
	float Throttle;

	// Mass of the car
	UPROPERTY(EditAnywhere, Category = "GoKart")
	float Mass = 1000;

	UPROPERTY(EditAnywhere, Category = "GoKart")
	float MaxDrivingForce = 10000;

	// The number of degrees rotated per second at full control throw
	UPROPERTY(EditAnywhere, Category = "GoKart")
	float MaxDegreesPerSecond = 90;

	UPROPERTY(Replicated)
	float SteeringThrow;

	UPROPERTY(EditAnywhere, Category = "GoKart")
	float DragCoefficient = 16.f;

	UPROPERTY(EditAnywhere, Category = "GoKart")
	float RollingResistanceCoefficient = 0.015f;

	// Minimum radius of car turning circle at full lock (m).
	UPROPERTY(EditAnywhere, Category = "GoKart")
	float MinTurnRadius = 10.f;

private:
	/** Handle pressing forwards */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveForward(float Val);

	void MoveForward(float Val);

	/** Handle pressing right */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveRight(float Val);

	void MoveRight(float Val);

	void MoveKart(float DeltaTime);

	void UpdateRotation(float DeltaTime);

	void UpdateLocationFromVelocity(float DeltaTime);

	FVector GetAirResistance();

	FVector GetRollingResistance();

	UFUNCTION()
	void OnRep_ReplicatedTransform();

};
