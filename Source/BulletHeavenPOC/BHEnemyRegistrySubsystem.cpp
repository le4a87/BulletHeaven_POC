// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHEnemyRegistrySubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

UBHEnemyRegistrySubsystem::UBHEnemyRegistrySubsystem()
{
	EnemyClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Blueprints/Enemies/BP_Enemy.BP_Enemy_C")));
}

bool UBHEnemyRegistrySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer)
		&& World
		&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UBHEnemyRegistrySubsystem::Deinitialize()
{
	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& EnemyPair : LiveEnemies)
	{
		if (AActor* Enemy = EnemyPair.Value.Get())
		{
			Enemy->OnDestroyed.RemoveDynamic(this, &UBHEnemyRegistrySubsystem::HandleEnemyDestroyed);
		}
	}
	LiveEnemies.Empty();

	Super::Deinitialize();
}

void UBHEnemyRegistrySubsystem::Tick(float DeltaTime)
{
	TimeUntilNextDiscovery -= DeltaTime;
	if (TimeUntilNextDiscovery <= 0.0f)
	{
		TimeUntilNextDiscovery = DiscoveryInterval;
		DiscoverEnemies();
	}
	else
	{
		PruneInvalidEnemies();
	}
}

TStatId UBHEnemyRegistrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBHEnemyRegistrySubsystem, STATGROUP_Tickables);
}

void UBHEnemyRegistrySubsystem::RegisterEnemy(AActor* Enemy)
{
	if (IsUsableEnemy(Enemy))
	{
		const FObjectKey EnemyKey(Enemy);
		if (!LiveEnemies.Contains(EnemyKey))
		{
			Enemy->OnDestroyed.AddUniqueDynamic(this, &UBHEnemyRegistrySubsystem::HandleEnemyDestroyed);
		}
		LiveEnemies.FindOrAdd(EnemyKey) = Enemy;
	}
}

void UBHEnemyRegistrySubsystem::UnregisterEnemy(AActor* Enemy)
{
	if (Enemy)
	{
		Enemy->OnDestroyed.RemoveDynamic(this, &UBHEnemyRegistrySubsystem::HandleEnemyDestroyed);
		LiveEnemies.Remove(FObjectKey(Enemy));
	}
}

int32 UBHEnemyRegistrySubsystem::GetLiveEnemyCount() const
{
	int32 Count = 0;
	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& EnemyPair : LiveEnemies)
	{
		if (IsUsableEnemy(EnemyPair.Value.Get()))
		{
			++Count;
		}
	}
	return Count;
}

int32 UBHEnemyRegistrySubsystem::GetDefeatedEnemyCount() const
{
	return DefeatedEnemyCount;
}

AActor* UBHEnemyRegistrySubsystem::FindNearestEnemy(const AActor* SourceActor)
{
	if (!SourceActor)
	{
		return nullptr;
	}

	PruneInvalidEnemies();

	const FVector SourceLocation = SourceActor->GetActorLocation();
	double BestDistanceSquared = TNumericLimits<double>::Max();
	AActor* BestEnemy = nullptr;

	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& EnemyPair : LiveEnemies)
	{
		AActor* Enemy = EnemyPair.Value.Get();
		if (!IsUsableEnemy(Enemy))
		{
			continue;
		}

		const double DistanceSquared = FVector::DistSquared2D(SourceLocation, Enemy->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestEnemy = Enemy;
		}
	}

	return BestEnemy;
}

void UBHEnemyRegistrySubsystem::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	if (!DestroyedActor)
	{
		return;
	}

	const int32 RemovedCount = LiveEnemies.Remove(FObjectKey(DestroyedActor));
	if (RemovedCount > 0)
	{
		++DefeatedEnemyCount;
	}
}

void UBHEnemyRegistrySubsystem::DiscoverEnemies()
{
	UWorld* World = GetWorld();
	UClass* LoadedEnemyClass = EnemyClass.LoadSynchronous();
	if (!World || !LoadedEnemyClass)
	{
		LiveEnemies.Empty();
		return;
	}

	for (TActorIterator<AActor> It(World, LoadedEnemyClass); It; ++It)
	{
		RegisterEnemy(*It);
	}

	PruneInvalidEnemies();
}

void UBHEnemyRegistrySubsystem::PruneInvalidEnemies()
{
	TArray<FObjectKey> RemovedEnemies;
	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& EnemyPair : LiveEnemies)
	{
		if (!IsUsableEnemy(EnemyPair.Value.Get()))
		{
			RemovedEnemies.Add(EnemyPair.Key);
		}
	}

	for (const FObjectKey& RemovedEnemy : RemovedEnemies)
	{
		LiveEnemies.Remove(RemovedEnemy);
	}
}

bool UBHEnemyRegistrySubsystem::IsUsableEnemy(const AActor* Enemy)
{
	return IsValid(Enemy) && !Enemy->IsActorBeingDestroyed();
}
