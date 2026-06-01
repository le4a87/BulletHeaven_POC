// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BHSpawnPressureSubsystem.generated.h"

class AActor;

USTRUCT()
struct FBHSpawnPressureTier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Spawn Pressure", meta = (ClampMin = "0.0", Units = "s"))
	float StartTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Spawn Pressure", meta = (ClampMin = "1"))
	int32 MaxEnemiesAlive = 75;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Spawn Pressure", meta = (ClampMin = "0.05", Units = "s"))
	float SpawnIntervalSeconds = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Spawn Pressure")
	bool bUseSupplementalSpawns = false;
};

/**
 * Applies time-based survivor-run spawn pressure to the existing Blueprint spawner.
 */
UCLASS(Config = Game, DefaultConfig)
class BULLETHEAVENPOC_API UBHSpawnPressureSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UBHSpawnPressureSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Spawn Pressure")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Spawn Pressure")
	FName TargetMapName = TEXT("Map_BulletHeavenPOC");

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Spawn Pressure")
	TSoftClassPtr<AActor> SpawnerClass;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Spawn Pressure")
	TArray<FBHSpawnPressureTier> PressureTiers;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Spawn Pressure", meta = (ClampMin = "0.05", Units = "s"))
	float SpawnerDiscoveryInterval = 1.0f;

	TArray<TWeakObjectPtr<AActor>> CachedSpawners;
	float RunStartTime = 0.0f;
	float TimeUntilNextSpawnerDiscovery = 0.0f;
	float TimeUntilNextSupplementalSpawn = 0.0f;
	int32 ActiveTierIndex = INDEX_NONE;
	bool bIsActiveTargetMap = false;

	void DiscoverSpawners();
	void ApplyActiveTier(float ElapsedSeconds);
	void ApplyTierToSpawner(AActor* Spawner, const FBHSpawnPressureTier& Tier) const;
	void ApplyFixedCapOverride(int32 FixedCap);
	void TrySupplementalSpawn(const FBHSpawnPressureTier& Tier);
	const FBHSpawnPressureTier* GetTierForElapsedTime(float ElapsedSeconds, int32& OutTierIndex) const;
	static void SetNumericProperty(UObject* Object, FName PropertyName, double Value);
	static bool IsUsableSpawner(const AActor* Spawner);
	static bool IsRuntimePressureEnabled();
	static int32 GetFixedCapOverride();
	bool IsTargetMap(const UWorld& World) const;
};
