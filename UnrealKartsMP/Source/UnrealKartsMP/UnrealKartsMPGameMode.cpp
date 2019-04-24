// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "UnrealKartsMPGameMode.h"
#include "UnrealKartsMPPawn.h"
#include "UnrealKartsMPHud.h"

AUnrealKartsMPGameMode::AUnrealKartsMPGameMode()
{
	DefaultPawnClass = AUnrealKartsMPPawn::StaticClass();
	HUDClass = AUnrealKartsMPHud::StaticClass();
}
