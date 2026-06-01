// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkinnedMeshComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "BHEnemyUpdateBudgetSubsystem.generated.h"

class AActor;
class ACharacter;
class UActorComponent;
class UCharacterMovementComponent;
class USkeletalMeshComponent;

USTRUCT()
struct FBHEnemyUpdateBudgetSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	bool bRuntimeBudgetEnabled = false;

	UPROPERTY()
	int32 NearEnemies = 0;

	UPROPERTY()
	int32 MidEnemies = 0;

	UPROPERTY()
	int32 FarEnemies = 0;

	UPROPERTY()
	int32 AnimRateVariedEnemies = 0;
};

USTRUCT()
struct FBHEnemyUpdateBudgetProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Enemy Budget", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxDistanceFromPlayer = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Enemy Budget", meta = (ClampMin = "0.0", Units = "s"))
	float ActorTickInterval = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Enemy Budget", meta = (ClampMin = "0.0", Units = "s"))
	float MovementTickInterval = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Enemy Budget", meta = (ClampMin = "0.0", Units = "s"))
	float MeshTickInterval = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Enemy Budget")
	EVisibilityBasedAnimTickOption AnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
};

/**
 * Applies distance-based tick and animation budgets to Blueprint enemies.
 */
UCLASS(Config = Game, DefaultConfig)
class BULLETHEAVENPOC_API UBHEnemyUpdateBudgetSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UBHEnemyUpdateBudgetSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	FBHEnemyUpdateBudgetSnapshot GetTelemetrySnapshot() const;

private:
	struct FCachedEnemySettings
	{
		TWeakObjectPtr<AActor> Enemy;
		TWeakObjectPtr<UCharacterMovementComponent> MovementComponent;
		TWeakObjectPtr<USkeletalMeshComponent> MeshComponent;
		float ActorTickInterval = 0.0f;
		float MovementTickInterval = 0.0f;
		float MeshTickInterval = 0.0f;
		float GlobalAnimRateScale = 1.0f;
		EVisibilityBasedAnimTickOption AnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		int32 AppliedProfileIndex = INDEX_NONE;
		bool bAppliedAnimRateScale = false;
	};

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget")
	FName TargetMapName = TEXT("Map_BulletHeavenPOC");

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget")
	TSoftClassPtr<AActor> EnemyClass;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget", meta = (ClampMin = "0.05", Units = "s"))
	float DiscoveryInterval = 0.5f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget", meta = (ClampMin = "0.05", Units = "s"))
	float BudgetRefreshInterval = 0.25f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget", meta = (ClampMin = "0.0", Units = "cm"))
	float NearDistance = 2000.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget")
	bool bRandomizeAnimRate = true;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget", meta = (ClampMin = "0.1"))
	float MinAnimRateScale = 0.94f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget", meta = (ClampMin = "0.1"))
	float MaxAnimRateScale = 1.06f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget")
	FBHEnemyUpdateBudgetProfile MidProfile;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Enemy Budget")
	FBHEnemyUpdateBudgetProfile FarProfile;

	TMap<FObjectKey, FCachedEnemySettings> CachedEnemies;
	float TimeUntilNextDiscovery = 0.0f;
	float TimeUntilNextBudgetRefresh = 0.0f;
	bool bIsActiveTargetMap = false;

	void DiscoverEnemies();
	void PruneInvalidEnemies();
	void ApplyBudgetPolicies();
	void CacheEnemy(AActor* Enemy);
	void ApplyAnimRateVariation(FCachedEnemySettings& Settings) const;
	void RestoreOriginalSettings(FCachedEnemySettings& Settings, bool bRestoreAnimRate) const;
	void ApplyProfile(FCachedEnemySettings& Settings, const FBHEnemyUpdateBudgetProfile& Profile, int32 ProfileIndex) const;
	const FBHEnemyUpdateBudgetProfile* SelectProfile(float DistanceSquared, int32& OutProfileIndex) const;
	bool IsTargetMap(const UWorld& World) const;
	static bool IsRuntimeBudgetEnabled();
	static bool IsUsableEnemy(const AActor* Enemy);
};
