// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include <Components/InputComponent.h>
#include <Engine/World.h>


// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AGoKart::MoveForward(float Val)
{
	Throttle = Val;
}

void AGoKart::MoveRight(float Val)
{
	SteeringThrow = Val;
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MoveKart(DeltaTime);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::MoveKart(float DeltaTime)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;
	KartVelocity += Acceleration * DeltaTime;

	

	UpdateRotation(DeltaTime);

	UpdateLocationFromVelocity(DeltaTime);

}

void AGoKart::UpdateRotation(float DeltaTime)
{
	float RotationAngle = MaxDegreesPerSecond * DeltaTime * SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), FMath::DegreesToRadians(RotationAngle));
	AddActorWorldRotation(RotationDelta);

	 KartVelocity = RotationDelta.RotateVector(KartVelocity);
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	// Velocity in cm/s so transform in m/s
	FVector Translation = KartVelocity * 100 * DeltaTime;
	FHitResult HitResult;
	AddActorWorldOffset(Translation, true, &HitResult);

	//Reset velocity if hit
	if (HitResult.IsValidBlockingHit())
	{
		KartVelocity = FVector::ZeroVector;
	}
}

FVector AGoKart::GetAirResistance()
{
	// Caculate air resistance
	FVector AirResistance = -KartVelocity.GetSafeNormal() * KartVelocity.SizeSquared() * DragCoefficient;
	return AirResistance;
}

FVector AGoKart::GetRollingResistance()
{
	// Calculate roll resistance
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	FVector RollResistance = -KartVelocity.GetSafeNormal() * AccelerationDueToGravity * Mass * RollingResistanceCoefficient;

	return RollResistance;
}

