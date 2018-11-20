// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include <Components/InputComponent.h>
#include <Engine/World.h>
#include "DrawDebugHelpers.h"
#include <UnrealNetwork.h>
#include <GameFramework/GameStateBase.h>
#include <Kismet/GameplayStatics.h>


// Sets default values
AGoKart::AGoKart()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void AGoKart::SimulateMove(const FGoKartMove& Move)
{
	Throttle = Move.Throttle;
	SteeringThrow = Move.SteeringThrow;
	MoveKart(Move.DeltaTime);
}

void AGoKart::MoveForward(float Val)
{
	Throttle = Val;
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	SimulateMove(Move);
	
	// Update transformation value
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = KartVelocity;
	ServerState.LastMove = Move;
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	// TODO
	return true;
}

void AGoKart::MoveRight(float Val)
{
	SteeringThrow = Val;
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

	//MoveKart(DeltaTime);

	if (IsLocallyControlled())
	{
		FGoKartMove SendingMove = CreateMove(DeltaTime);

		if (!HasAuthority())
		{
			UnacknowledgedMoves.Add(SendingMove);
			UE_LOG(LogTemp, Warning, TEXT("Queue length: %d"), UnacknowledgedMoves.Num());
		}

		Server_SendMove(SendingMove);

		SimulateMove(SendingMove);
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

FGoKartMove AGoKart::CreateMove(float DeltaTime)
{
	FGoKartMove SendingMove;
	SendingMove.DeltaTime = DeltaTime;
	SendingMove.SteeringThrow = SteeringThrow;
	SendingMove.Throttle = Throttle;
	SendingMove.Time = UGameplayStatics::GetGameState(this)->GetServerWorldTimeSeconds();
	
	return SendingMove;
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

void AGoKart::OnRep_ServerState()
{
	if (!HasAuthority())
	{
		// Replicate movement from server to all clients
		SetActorTransform(ServerState.Transform);
		KartVelocity = ServerState.Velocity;

		ClearAcknowledgedMoves(ServerState.LastMove);

		for (const FGoKartMove& Move : UnacknowledgedMoves)
		{
			SimulateMove(Move);
		}
	}
}


void AGoKart::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}
	UnacknowledgedMoves = NewMoves;
}

void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AGoKart, ServerState);
}