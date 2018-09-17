// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "KrazyKartsProjectGameMode.h"
#include "KrazyKartsProjectPawn.h"
#include "KrazyKartsProjectHud.h"

AKrazyKartsProjectGameMode::AKrazyKartsProjectGameMode()
{
	DefaultPawnClass = AKrazyKartsProjectPawn::StaticClass();
	HUDClass = AKrazyKartsProjectHud::StaticClass();
}
