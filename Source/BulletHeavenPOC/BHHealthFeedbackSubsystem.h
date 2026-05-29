// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BHHealthFeedbackSubsystem.generated.h"

class AActor;
class AController;
class ATextRenderActor;
class UDamageType;

/**
 * Minimal world-space combat feedback for the Blueprint proof-of-concept actors.
 *
 * Gameplay health currently lives in Blueprint variables, so this subsystem observes
 * health-bearing actors without moving ownership of their health state.
 */
UCLASS()
class BULLETHEAVENPOC_API UBHHealthFeedbackSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	struct FFloatingDamageNumber
	{
		FVector Location = FVector::ZeroVector;
		float Damage = 0.0f;
		float RemainingTime = 0.0f;
		TWeakObjectPtr<ATextRenderActor> TextActor;
	};

	UFUNCTION()
	void HandleActorDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	bool TryGetHealth(AActor* Actor, double& OutCurrentHealth, double& OutMaxHealth);
	static bool TryReadNumericProperty(const AActor* Actor, FName PropertyName, double& OutValue);
	void RegisterHealthActors();
	void DrawDamageNumbers(float DeltaTime);

	TSet<TWeakObjectPtr<AActor>> ObservedActors;
	TMap<TWeakObjectPtr<AActor>, double> InitialHealthByActor;
	TArray<FFloatingDamageNumber> FloatingDamageNumbers;
	float TimeUntilNextActorScan = 0.0f;
};
