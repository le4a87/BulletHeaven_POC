// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "BHEnemyRegistrySubsystem.generated.h"

class AActor;

/**
 * Tracks live Blueprint enemies for systems that need target candidates without
 * repeatedly querying the whole world from gameplay paths.
 */
UCLASS()
class BULLETHEAVENPOC_API UBHEnemyRegistrySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UBHEnemyRegistrySubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintCallable, Category = "Bullet Heaven|Enemies")
	void RegisterEnemy(AActor* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Bullet Heaven|Enemies")
	void UnregisterEnemy(AActor* Enemy);

	UFUNCTION(BlueprintPure, Category = "Bullet Heaven|Enemies")
	int32 GetLiveEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Bullet Heaven|Enemies")
	int32 GetDefeatedEnemyCount() const;

	UFUNCTION(BlueprintCallable, Category = "Bullet Heaven|Enemies")
	AActor* FindNearestEnemy(const AActor* SourceActor);

private:
	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Enemies")
	TSoftClassPtr<AActor> EnemyClass;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Enemies", meta = (ClampMin = "0.05"))
	float DiscoveryInterval = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Separation", meta = (ClampMin = "0.0", Units = "cm"))
	float EnemySeparationRadius = 140.0f;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Separation", meta = (ClampMin = "0.0"))
	float EnemySeparationStrength = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Bullet Heaven|Separation", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxSeparationStep = 60.0f;

	TMap<FObjectKey, TWeakObjectPtr<AActor>> LiveEnemies;
	float TimeUntilNextDiscovery = 0.0f;
	int32 DefeatedEnemyCount = 0;

	void DiscoverEnemies();
	void PruneInvalidEnemies();
	void ApplyEnemySeparation(float DeltaTime);
	bool HasLineOfSightToEnemy(const AActor* SourceActor, const AActor* Enemy) const;
	static bool IsUsableEnemy(const AActor* Enemy);
};
