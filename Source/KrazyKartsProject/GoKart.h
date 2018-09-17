// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"


UCLASS()
class KRAZYKARTSPROJECT_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

	/** Handle pressing forwards */
	void MoveForward(float Val);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	FVector KartVelocity;

	// Force that is in proportion to the kart's acceleration
	UPROPERTY(EditAnywhere, Category = "GoKart")
	float Throttle;

	// Mass of the car
	UPROPERTY(EditAnywhere, Category = "GoKart")
	float Mass = 1000;

	UPROPERTY(EditAnywhere, Category = "GoKart")
	float MaxDrivingForce = 10000;

	
};
