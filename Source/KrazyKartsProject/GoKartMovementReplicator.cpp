// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicator.h"
#include <UnrealNetwork.h>
#include <GameFramework/Actor.h>


// Sets default values for this component's properties
UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


// Called when the game starts
void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	// Assign movement component
	MovementComp = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
	
}


// Called every frame
void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MovementComp) return;

	FGoKartMove LastMove = MovementComp->GetLastMove();

	if (GetOwner()->Role == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(LastMove);

		Server_SendMove(LastMove);
	}
	
	// We are server and in control of the pawn
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(LastMove);
	}

	if (GetOwner()->Role == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}

}

void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!MovementComp) return;
	MovementComp->SimulateMove(Move);

	UpdateServerState(Move);
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)
{
	// TODO
	return true;
}

void UGoKartMovementReplicator::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
		case ROLE_AutonomousProxy:
		{
			AutonomousProxy_OnRep_ServerState();
			break;
		}
		case ROLE_SimulatedProxy:
		{
			SimulatedProxy_OnRep_ServerState();
			break;
		}
		default: break;
	}
	
}


void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
{
	if(!MovementComp) return;

	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	if (MeshOffsetRoot)
	{
		ClientTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		ClientTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
	}
	ClientStartVelocity = MovementComp->GetKartVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);
}

void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
{
	if (!MovementComp) return;
	if (!GetOwner()->HasAuthority())
	{
		// Replicate movement from server to all clients
		GetOwner()->SetActorTransform(ServerState.Transform);
		MovementComp->SetKartVelocity(ServerState.Velocity);

		ClearAcknowledgedMoves(ServerState.LastMove);

		for (const FGoKartMove& Move : UnacknowledgedMoves)
		{
			MovementComp->SimulateMove(Move);
		}
	}
}

void UGoKartMovementReplicator::ClearAcknowledgedMoves(FGoKartMove LastMove)
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

void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move)
{
	// Update movement value
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComp->GetKartVelocity();
	ServerState.LastMove = Move;
}

void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;
	if(!MovementComp) return;

	double LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
 	FHermiteCubicSpline Spline = CreateSpline();

	InterpolateLocation(LerpRatio, Spline);

	InterpolateVelocity(LerpRatio, Spline);

	InterpolateRotation(LerpRatio);
}

FHermiteCubicSpline UGoKartMovementReplicator::CreateSpline()
{
	FHermiteCubicSpline Spline;

	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientTransform.GetLocation();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();

	return Spline;
}

void UGoKartMovementReplicator::InterpolateLocation(float LerpRatio, const FHermiteCubicSpline& Spline)
{
	FVector NextLocation = Spline.InterpolateLocation(LerpRatio);
	if (MeshOffsetRoot)
	{
		MeshOffsetRoot->SetWorldLocation(NextLocation);
	}
}

void UGoKartMovementReplicator::InterpolateVelocity(float LerpRatio, const FHermiteCubicSpline& Spline)
{
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative();
	MovementComp->SetKartVelocity(NewVelocity);
}

void UGoKartMovementReplicator::InterpolateRotation(float LerpRatio)
{
	FQuat StartRotation = ClientTransform.GetRotation();
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);
	if (MeshOffsetRoot)
	{
		MeshOffsetRoot->SetWorldRotation(NewRotation);
	}
}

float UGoKartMovementReplicator::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdates * 100;
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}

