// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHTargetingLibrary.h"

#include "BHEnemyRegistrySubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

AActor* UBHTargetingLibrary::FindNearestRegisteredEnemy(AActor* SourceActor)
{
	if (!SourceActor)
	{
		return nullptr;
	}

	UWorld* World = SourceActor->GetWorld();
	UBHEnemyRegistrySubsystem* Registry = World ? World->GetSubsystem<UBHEnemyRegistrySubsystem>() : nullptr;
	return Registry ? Registry->FindNearestEnemy(SourceActor) : nullptr;
}

int32 UBHTargetingLibrary::GetRegisteredLiveEnemyCount(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine && WorldContextObject
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	const UBHEnemyRegistrySubsystem* Registry = World ? World->GetSubsystem<UBHEnemyRegistrySubsystem>() : nullptr;
	return Registry ? Registry->GetLiveEnemyCount() : 0;
}

int32 UBHTargetingLibrary::GetRegisteredDefeatedEnemyCount(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine && WorldContextObject
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	const UBHEnemyRegistrySubsystem* Registry = World ? World->GetSubsystem<UBHEnemyRegistrySubsystem>() : nullptr;
	return Registry ? Registry->GetDefeatedEnemyCount() : 0;
}
