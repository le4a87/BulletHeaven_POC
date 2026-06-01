// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHSpawnPressureSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"

namespace BHSpawnPressure
{
	const FName MaxEnemiesAlivePropertyName = TEXT("MaxEnemiesAlive");
	const FName SpawnRatePropertyName = TEXT("SpawnRate");
	const FName TrySpawnEnemyFunctionName = TEXT("TrySpawnEnemy");

	TAutoConsoleVariable<int32> CVarSpawnPressureEnabled(
		TEXT("bh.SpawnPressure.Enabled"),
		1,
		TEXT("Enables time-based Bullet Heaven spawn pressure. Set to 0 before PIE for fixed-cap population profiling."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarPopulationProfileFixedCap(
		TEXT("bh.PopulationProfile.FixedCap"),
		0,
		TEXT("Overrides BP_EnemySpawner.MaxEnemiesAlive for fixed-cap profiling when greater than 0. Set before PIE."),
		ECVF_Default);
}

UBHSpawnPressureSubsystem::UBHSpawnPressureSubsystem()
{
	SpawnerClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Blueprints/Spawners/BP_EnemySpawner.BP_EnemySpawner_C")));

	PressureTiers = {
		{0.0f, 75, 1.00f, false},
		{60.0f, 90, 0.85f, true},
		{120.0f, 110, 0.70f, true},
		{180.0f, 130, 0.60f, true},
		{240.0f, 150, 0.50f, true},
	};
}

bool UBHSpawnPressureSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer)
		&& World
		&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UBHSpawnPressureSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const int32 FixedCapOverride = GetFixedCapOverride();
	bIsActiveTargetMap = bEnabled && IsTargetMap(InWorld) && (IsRuntimePressureEnabled() || FixedCapOverride > 0);
	if (!bIsActiveTargetMap)
	{
		return;
	}

	RunStartTime = InWorld.GetTimeSeconds();
	TimeUntilNextSpawnerDiscovery = 0.0f;
	TimeUntilNextSupplementalSpawn = 0.0f;
	ActiveTierIndex = INDEX_NONE;
	DiscoverSpawners();
	if (FixedCapOverride > 0)
	{
		ApplyFixedCapOverride(FixedCapOverride);
		return;
	}

	ApplyActiveTier(0.0f);
}

void UBHSpawnPressureSubsystem::Tick(float DeltaTime)
{
	if (!bIsActiveTargetMap)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TimeUntilNextSpawnerDiscovery -= DeltaTime;
	if (TimeUntilNextSpawnerDiscovery <= 0.0f)
	{
		TimeUntilNextSpawnerDiscovery = SpawnerDiscoveryInterval;
		DiscoverSpawners();
	}

	const int32 FixedCapOverride = GetFixedCapOverride();
	if (FixedCapOverride > 0)
	{
		ApplyFixedCapOverride(FixedCapOverride);
		return;
	}

	if (!IsRuntimePressureEnabled())
	{
		return;
	}

	const float ElapsedSeconds = FMath::Max(0.0f, World->GetTimeSeconds() - RunStartTime);
	ApplyActiveTier(ElapsedSeconds);

	int32 TierIndex = INDEX_NONE;
	const FBHSpawnPressureTier* Tier = GetTierForElapsedTime(ElapsedSeconds, TierIndex);
	if (!Tier || !Tier->bUseSupplementalSpawns)
	{
		return;
	}

	TimeUntilNextSupplementalSpawn -= DeltaTime;
	if (TimeUntilNextSupplementalSpawn <= 0.0f)
	{
		TimeUntilNextSupplementalSpawn = Tier->SpawnIntervalSeconds;
		TrySupplementalSpawn(*Tier);
	}
}

TStatId UBHSpawnPressureSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBHSpawnPressureSubsystem, STATGROUP_Tickables);
}

void UBHSpawnPressureSubsystem::DiscoverSpawners()
{
	UWorld* World = GetWorld();
	UClass* LoadedSpawnerClass = SpawnerClass.LoadSynchronous();
	if (!World || !LoadedSpawnerClass)
	{
		CachedSpawners.Empty();
		return;
	}

	CachedSpawners.Reset();
	for (TActorIterator<AActor> It(World, LoadedSpawnerClass); It; ++It)
	{
		if (IsUsableSpawner(*It))
		{
			CachedSpawners.Add(*It);
		}
	}
}

void UBHSpawnPressureSubsystem::ApplyActiveTier(float ElapsedSeconds)
{
	int32 TierIndex = INDEX_NONE;
	const FBHSpawnPressureTier* Tier = GetTierForElapsedTime(ElapsedSeconds, TierIndex);
	if (!Tier)
	{
		return;
	}

	if (TierIndex == ActiveTierIndex)
	{
		return;
	}

	ActiveTierIndex = TierIndex;
	TimeUntilNextSupplementalSpawn = 0.0f;
	for (TWeakObjectPtr<AActor>& SpawnerPtr : CachedSpawners)
	{
		if (AActor* Spawner = SpawnerPtr.Get(); IsUsableSpawner(Spawner))
		{
			ApplyTierToSpawner(Spawner, *Tier);
		}
	}
}

void UBHSpawnPressureSubsystem::ApplyTierToSpawner(AActor* Spawner, const FBHSpawnPressureTier& Tier) const
{
	SetNumericProperty(Spawner, BHSpawnPressure::MaxEnemiesAlivePropertyName, Tier.MaxEnemiesAlive);
	SetNumericProperty(Spawner, BHSpawnPressure::SpawnRatePropertyName, Tier.SpawnIntervalSeconds);
}

void UBHSpawnPressureSubsystem::ApplyFixedCapOverride(int32 FixedCap)
{
	if (FixedCap <= 0)
	{
		return;
	}

	for (TWeakObjectPtr<AActor>& SpawnerPtr : CachedSpawners)
	{
		if (AActor* Spawner = SpawnerPtr.Get(); IsUsableSpawner(Spawner))
		{
			SetNumericProperty(Spawner, BHSpawnPressure::MaxEnemiesAlivePropertyName, FixedCap);
		}
	}
}

void UBHSpawnPressureSubsystem::TrySupplementalSpawn(const FBHSpawnPressureTier& Tier)
{
	for (TWeakObjectPtr<AActor>& SpawnerPtr : CachedSpawners)
	{
		AActor* Spawner = SpawnerPtr.Get();
		if (!IsUsableSpawner(Spawner))
		{
			continue;
		}

		ApplyTierToSpawner(Spawner, Tier);
		if (UFunction* TrySpawnFunction = Spawner->FindFunction(BHSpawnPressure::TrySpawnEnemyFunctionName))
		{
			Spawner->ProcessEvent(TrySpawnFunction, nullptr);
		}
	}
}

const FBHSpawnPressureTier* UBHSpawnPressureSubsystem::GetTierForElapsedTime(float ElapsedSeconds, int32& OutTierIndex) const
{
	OutTierIndex = INDEX_NONE;
	const FBHSpawnPressureTier* ActiveTier = nullptr;
	for (int32 TierIndex = 0; TierIndex < PressureTiers.Num(); ++TierIndex)
	{
		const FBHSpawnPressureTier& Tier = PressureTiers[TierIndex];
		if (ElapsedSeconds >= Tier.StartTimeSeconds)
		{
			ActiveTier = &Tier;
			OutTierIndex = TierIndex;
		}
	}
	return ActiveTier;
}

void UBHSpawnPressureSubsystem::SetNumericProperty(UObject* Object, FName PropertyName, double Value)
{
	if (!Object)
	{
		return;
	}

	FNumericProperty* Property = CastField<FNumericProperty>(Object->GetClass()->FindPropertyByName(PropertyName));
	if (!Property)
	{
		return;
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
	if (Property->IsFloatingPoint())
	{
		Property->SetFloatingPointPropertyValue(ValuePtr, Value);
	}
	else if (Property->IsInteger())
	{
		Property->SetIntPropertyValue(ValuePtr, FMath::RoundToInt64(Value));
	}
}

bool UBHSpawnPressureSubsystem::IsUsableSpawner(const AActor* Spawner)
{
	return IsValid(Spawner) && !Spawner->IsActorBeingDestroyed();
}

bool UBHSpawnPressureSubsystem::IsRuntimePressureEnabled()
{
	return BHSpawnPressure::CVarSpawnPressureEnabled.GetValueOnGameThread() != 0;
}

int32 UBHSpawnPressureSubsystem::GetFixedCapOverride()
{
	return FMath::Max(0, BHSpawnPressure::CVarPopulationProfileFixedCap.GetValueOnGameThread());
}

bool UBHSpawnPressureSubsystem::IsTargetMap(const UWorld& World) const
{
	const FString MapName = World.GetMapName();
	return TargetMapName.IsNone() || MapName.Contains(TargetMapName.ToString());
}
