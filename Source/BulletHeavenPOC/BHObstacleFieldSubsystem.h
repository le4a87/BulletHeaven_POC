// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BHObstacleFieldSubsystem.generated.h"

class AActor;
class UStaticMesh;

/**
 * Spawns a small runtime obstacle field for the Bullet Heaven combat map.
 */
UCLASS(Config = Game, DefaultConfig)
class BULLETHEAVENPOC_API UBHObstacleFieldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UBHObstacleFieldSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles", meta = (ClampMin = "0"))
	int32 ObstacleCount = 18;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles", meta = (ClampMin = "0.0", Units = "cm"))
	float PlacementRadius = 2300.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles", meta = (ClampMin = "0.0", Units = "cm"))
	float PlayerSafeRadius = 750.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles", meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnerSafeRadius = 500.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles", meta = (ClampMin = "0.0", Units = "cm"))
	float MinimumObstacleSpacing = 520.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles", meta = (ClampMin = "1.0", Units = "cm"))
	float ObstacleFootprint = 280.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles", meta = (ClampMin = "1.0", Units = "cm"))
	float ObstacleHeight = 240.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles")
	bool bUseDeterministicSeed = false;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles", meta = (EditCondition = "bUseDeterministicSeed"))
	int32 RandomSeed = 19019;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles", meta = (ClampMin = "1"))
	int32 MaxPlacementAttempts = 500;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles")
	FName TargetMapName = TEXT("Map_BulletHeavenPOC");

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles|Projectiles")
	bool bDestroyProjectilesOnObstacleHit = true;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Obstacles|Projectiles")
	TSoftClassPtr<AActor> ProjectileClass;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedObstacles;

	TMap<TWeakObjectPtr<AActor>, FVector> LastProjectileLocations;
	bool bIsActiveTargetMap = false;

	void SpawnObstacleField(UWorld& World);
	void ClearObstacleField();
	void CollectProtectedLocations(const UWorld& World, TArray<FVector>& OutPlayerSafeLocations, TArray<FVector>& OutSpawnerSafeLocations) const;
	bool IsValidObstacleLocation(const FVector& Location, const TArray<FVector>& ExistingLocations, const TArray<FVector>& PlayerSafeLocations, const TArray<FVector>& SpawnerSafeLocations) const;
	void DestroyProjectilesBlockedByObstacles();
	bool DidProjectileHitObstacle(const AActor* Projectile, const FVector& PreviousLocation, const FVector& CurrentLocation) const;
	bool IsTargetMap(const UWorld& World) const;
};
