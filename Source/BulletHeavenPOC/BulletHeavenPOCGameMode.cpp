// Copyright Epic Games, Inc. All Rights Reserved.

#include "BulletHeavenPOCGameMode.h"

#include "BHGameplayHUD.h"

ABulletHeavenPOCGameMode::ABulletHeavenPOCGameMode()
{
	HUDClass = ABHGameplayHUD::StaticClass();
}
