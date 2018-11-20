// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include <Components/InputComponent.h>
#include <Engine/World.h>
#include "DrawDebugHelpers.h"
#include <UnrealNetwork.h>
#include "GoKartMovementComponent.h"


// Sets default values
AGoKart::AGoKart()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MovementComp = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComp"));

	bReplicates = true;
}


void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!MovementComp) return;
	MovementComp->SimulateMove(Move);
	
	// Update movement value
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = MovementComp->GetKartVelocity();
	ServerState.LastMove = Move;
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	// TODO
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

	if(!MovementComp) return;

	if (Role == ROLE_AutonomousProxy)
	{
		FGoKartMove SendingMove = MovementComp->CreateMove(DeltaTime);
		UnacknowledgedMoves.Add(SendingMove);

		MovementComp->SimulateMove(SendingMove);
		Server_SendMove(SendingMove);
	}

	// We are server and in control of the pawn
	if (Role == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy)
	{
		FGoKartMove SendingMove = MovementComp->CreateMove(DeltaTime);
		Server_SendMove(SendingMove);
	}

	if (Role == ROLE_SimulatedProxy)
	{
		MovementComp->SimulateMove(ServerState.LastMove);
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



void AGoKart::OnRep_ServerState()
{	
	if (!MovementComp) return;
	if (!HasAuthority())
	{
		// Replicate movement from server to all clients
		SetActorTransform(ServerState.Transform);
		MovementComp->SetKartVelocity(ServerState.Velocity);

		ClearAcknowledgedMoves(ServerState.LastMove);

		for (const FGoKartMove& Move : UnacknowledgedMoves)
		{
			MovementComp->SimulateMove(Move);
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

void AGoKart::MoveForward(float Val)
{
	if (!MovementComp) return;
	MovementComp->SetThrottle(Val);
}

void AGoKart::MoveRight(float Val)
{
	if (!MovementComp) return;
	MovementComp->SetSteeringThrow(Val);
}


void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AGoKart, ServerState);
}