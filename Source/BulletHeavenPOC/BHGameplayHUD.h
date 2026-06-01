// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
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

	double RunStartTime = 0.0;
	double GameOverTime = 0.0;

	double GetElapsedTimeSeconds() const;
	FString FormatElapsedTime(double ElapsedSeconds) const;
	void DrawPrimaryStat(const FString& Label, const FString& Value, float X, float Y, float ValueScale = 1.8f);
	void DrawGameplayLine(const FString& Text, float X, float& Y, const FLinearColor& Color, float Scale = 1.0f);
	void DrawReadableText(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale = 1.0f);
	void DrawCenteredReadableText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, float Scale = 1.0f);

	static double GetNumericProperty(const UObject* Object, FName PropertyName, double DefaultValue);
	static bool GetBoolProperty(const UObject* Object, FName PropertyName, bool DefaultValue);
};
