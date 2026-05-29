// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UObject/ObjectKey.h"
#include "BHGameplayHUD.generated.h"

class AActor;

/**
 * Minimal runtime HUD for the survivor-loop proof of concept.
 */
UCLASS()
class ABHGameplayHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABHGameplayHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Bullet Heaven|HUD")
	double PlayerMaxHealthFallback = 100.0;

	UPROPERTY(EditDefaultsOnly, Category = "Bullet Heaven|HUD")
	float EnemyMetricRefreshInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Bullet Heaven|HUD")
	TSoftClassPtr<AActor> EnemyClass;

	double RunStartTime = 0.0;
	double GameOverTime = 0.0;
	double LastEnemyMetricRefreshTime = -1.0;

	int32 CachedLiveEnemyCount = 0;
	int32 KillCount = 0;

	TMap<FObjectKey, TWeakObjectPtr<AActor>> KnownEnemies;

	void RefreshEnemyMetrics();
	double GetElapsedTimeSeconds() const;
	void DrawGameplayLine(const FString& Text, float X, float& Y, const FLinearColor& Color, float Scale = 1.0f);

	static double GetNumericProperty(const UObject* Object, FName PropertyName, double DefaultValue);
	static bool GetBoolProperty(const UObject* Object, FName PropertyName, bool DefaultValue);
};
