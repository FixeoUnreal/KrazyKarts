// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include <Components/InputComponent.h>
#include <Engine/World.h>
#include "DrawDebugHelpers.h"
#include <UnrealNetwork.h>


// Sets default values
AGoKart::AGoKart()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void AGoKart::MoveForward(float Val)
{
	Throttle = Val;
	Server_MoveForward(Val);
}

void AGoKart::MoveRight(float Val)
{
	SteeringThrow = Val;
	Server_MoveRight(Val);
}

void AGoKart::Server_MoveForward_Implementation(float Val)
{
	Throttle = Val;
}

bool AGoKart::Server_MoveForward_Validate(float Val)
{
	if (Val > 1.f || Val < -1.f)
	{
		return false;
	}
	return true;
}

void AGoKart::Server_MoveRight_Implementation(float Val)
{
	SteeringThrow = Val;
}

bool AGoKart::Server_MoveRight_Validate(float Val)
{
	if (Val > 1.f || Val < -1.f)
	{
		return false;
	}
	return true;
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	NetUpdateFrequency = 1;
}

FString GetEnumText(ENetRole Role)
{
	switch (Role)
	{
		case ROLE_None: return "None";
		case ROLE_SimulatedProxy: return "SimulatedProxy";
		case ROLE_AutonomousProxy: return "AutonomousProxy";
		case ROLE_Authority: return "Authority";
		default: return "ERROR";
	}
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MoveKart(DeltaTime);

	if (HasAuthority())
	{
		// Update transformation value
		ReplicatedTransform = GetActorTransform();
	}

	DrawDebugString(
		GetWorld(),
		FVector(0,0,100),
		GetEnumText(Role),
		this,
		FColor::White,
		DeltaTime
	);
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
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), KartVelocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurnRadius * SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);
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
	// Calculate air resistance
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

void AGoKart::OnRep_ReplicatedTransform()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Replicate Location on client"));
		// Replicate transform from server to all clients
		SetActorTransform(ReplicatedTransform);
	}
}


void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AGoKart, ReplicatedTransform);
}