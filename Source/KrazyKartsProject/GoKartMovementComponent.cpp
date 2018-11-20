// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Engine/World.h"
#include <GameFramework/GameStateBase.h>


// Sets default values for this component's properties
UGoKartMovementComponent::UGoKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FVector UGoKartMovementComponent::GetKartVelocity() const
{
	return KartVelocity;
}

void UGoKartMovementComponent::SetKartVelocity(FVector Val)
{
	KartVelocity = Val;
}

void UGoKartMovementComponent::SetThrottle(float Val)
{
	Throttle = Val;
}

void UGoKartMovementComponent::SetSteeringThrow(float Val)
{
	SteeringThrow= Val;
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move)
{
	Throttle = Move.Throttle;
	SteeringThrow = Move.SteeringThrow;
	MoveKart(Move.DeltaTime);
}


FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime)
{
	FGoKartMove SendingMove;
	SendingMove.DeltaTime = DeltaTime;
	SendingMove.SteeringThrow = SteeringThrow;
	SendingMove.Throttle = Throttle;
	SendingMove.Time = UGameplayStatics::GetGameState(this)->GetServerWorldTimeSeconds();

	return SendingMove;
}

void UGoKartMovementComponent::MoveKart(float DeltaTime)
{
	FVector Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;
	KartVelocity += Acceleration * DeltaTime;


	UpdateRotation(DeltaTime);

	UpdateLocationFromVelocity(DeltaTime);

}

void UGoKartMovementComponent::UpdateRotation(float DeltaTime)
{
	float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), KartVelocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurnRadius * SteeringThrow;
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);
	GetOwner()->AddActorWorldRotation(RotationDelta);

	KartVelocity = RotationDelta.RotateVector(KartVelocity);
}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime)
{
	// Velocity in cm/s so transform in m/s
	FVector Translation = KartVelocity * 100 * DeltaTime;
	FHitResult HitResult;
	GetOwner()->AddActorWorldOffset(Translation, true, &HitResult);

	//Reset velocity if hit
	if (HitResult.IsValidBlockingHit())
	{
		KartVelocity = FVector::ZeroVector;
	}
}

FVector UGoKartMovementComponent::GetAirResistance()
{
	// Calculate air resistance
	FVector AirResistance = -KartVelocity.GetSafeNormal() * KartVelocity.SizeSquared() * DragCoefficient;
	return AirResistance;
}

FVector UGoKartMovementComponent::GetRollingResistance()
{
	// Calculate roll resistance
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	FVector RollResistance = -KartVelocity.GetSafeNormal() * AccelerationDueToGravity * Mass * RollingResistanceCoefficient;

	return RollResistance;
}



