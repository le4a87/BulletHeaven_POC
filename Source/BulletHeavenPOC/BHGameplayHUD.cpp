// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHGameplayHUD.h"

#include "BHTargetingLibrary.h"
#include "Engine/Canvas.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

ABHGameplayHUD::ABHGameplayHUD() = default;

void ABHGameplayHUD::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		RunStartTime = World->GetTimeSeconds();
		GameOverTime = RunStartTime;
	}
}

void ABHGameplayHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const double CurrentHealth = GetNumericProperty(PlayerPawn, TEXT("Health"), 0.0);
	const double MaxHealth = GetNumericProperty(PlayerPawn, TEXT("MaxHealth"), PlayerMaxHealthFallback);
	const bool bIsGameOver = GetBoolProperty(PlayerPawn, TEXT("IsGameOver"), false);
	const int32 KillCount = UBHTargetingLibrary::GetRegisteredDefeatedEnemyCount(this);
	const int32 LiveEnemyCount = UBHTargetingLibrary::GetRegisteredLiveEnemyCount(this);

	if (bIsGameOver && GameOverTime <= RunStartTime)
	{
		if (const UWorld* World = GetWorld())
		{
			GameOverTime = World->GetTimeSeconds();
		}
	}

	const float ViewWidth = FMath::Max(1.0f, static_cast<float>(Canvas->SizeX));
	const float Margin = FMath::Clamp(ViewWidth * 0.025f, 12.0f, 24.0f);
	const float PanelX = Margin;
	const float PanelY = Margin;
	const float PanelWidth = FMath::Min(FMath::Max(1.0f, ViewWidth - (Margin * 2.0f)), 460.0f);
	constexpr float PanelHeight = 176.0f;
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f), PanelX, PanelY, PanelWidth, PanelHeight);
	DrawRect(FLinearColor(0.0f, 0.72f, 0.95f, 0.85f), PanelX, PanelY, 5.0f, PanelHeight);

	const float PrimaryX = PanelX + 20.0f;
	const float PrimaryY = PanelY + 12.0f;
	const float StatColumnWidth = (PanelWidth - 64.0f) * 0.5f;
	DrawPrimaryStat(TEXT("TIME"), FormatElapsedTime(GetElapsedTimeSeconds()), PrimaryX, PrimaryY, 1.9f);
	DrawPrimaryStat(TEXT("KILLS"), FString::FromInt(KillCount), PrimaryX + StatColumnWidth + 24.0f, PrimaryY, 1.9f);

	float SecondaryY = PanelY + 92.0f;
	DrawGameplayLine(FString::Printf(TEXT("Health: %.0f / %.0f"), CurrentHealth, MaxHealth), PrimaryX, SecondaryY, FLinearColor::White, 1.0f);
	DrawGameplayLine(FString::Printf(TEXT("Live Enemies: %d"), LiveEnemyCount), PrimaryX, SecondaryY, FLinearColor::White, 1.0f);

	constexpr float HealthBarHeight = 10.0f;
	const float HealthBarX = PrimaryX;
	const float HealthBarY = PanelY + PanelHeight - 24.0f;
	const float HealthBarWidth = PanelWidth - 40.0f;
	const float HealthPercent = MaxHealth > 0.0
		? FMath::Clamp(static_cast<float>(CurrentHealth / MaxHealth), 0.0f, 1.0f)
		: 0.0f;
	DrawRect(FLinearColor(0.16f, 0.02f, 0.02f, 0.9f), HealthBarX, HealthBarY, HealthBarWidth, HealthBarHeight);
	DrawRect(FLinearColor(0.0f, 0.78f, 0.36f, 0.95f), HealthBarX, HealthBarY, HealthBarWidth * HealthPercent, HealthBarHeight);

	if (bIsGameOver)
	{
		const float CenterX = ViewWidth * 0.5f;
		const float CenterY = Canvas->SizeY * 0.42f;
		DrawCenteredReadableText(TEXT("GAME OVER"), FLinearColor::Red, CenterX, CenterY, 2.0f);
		DrawCenteredReadableText(TEXT("Stop PIE or restart the level to try again."), FLinearColor::White, CenterX, CenterY + 42.0f, 1.0f);
	}
}

double ABHGameplayHUD::GetElapsedTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bIsGameOver = GetBoolProperty(PlayerPawn, TEXT("IsGameOver"), false);
	const double EndTime = bIsGameOver ? GameOverTime : World->GetTimeSeconds();
	return FMath::Max(0.0, EndTime - RunStartTime);
}

FString ABHGameplayHUD::FormatElapsedTime(double ElapsedSeconds) const
{
	const int32 TotalSeconds = FMath::FloorToInt(FMath::Max(0.0, ElapsedSeconds));
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

void ABHGameplayHUD::DrawPrimaryStat(const FString& Label, const FString& Value, float X, float Y, float ValueScale)
{
	DrawReadableText(Label, FLinearColor(0.78f, 0.92f, 1.0f, 1.0f), X, Y, 0.82f);
	DrawReadableText(Value, FLinearColor::White, X, Y + 24.0f, ValueScale);
}

void ABHGameplayHUD::DrawGameplayLine(const FString& Text, float X, float& Y, const FLinearColor& Color, float Scale)
{
	constexpr float LineHeight = 24.0f;
	DrawReadableText(Text, Color, X, Y, Scale);
	Y += LineHeight * Scale;
}

void ABHGameplayHUD::DrawReadableText(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale)
{
	DrawText(Text, FLinearColor(0.0f, 0.0f, 0.0f, 0.9f), X + 2.0f, Y + 2.0f, nullptr, Scale, false);
	DrawText(Text, Color, X, Y, nullptr, Scale, false);
}

void ABHGameplayHUD::DrawCenteredReadableText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, float Scale)
{
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(Text, TextWidth, TextHeight, nullptr, Scale);
	DrawReadableText(Text, Color, CenterX - (TextWidth * 0.5f), Y, Scale);
}

double ABHGameplayHUD::GetNumericProperty(const UObject* Object, FName PropertyName, double DefaultValue)
{
	if (!Object)
	{
		return DefaultValue;
	}

	const FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		const void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Object);
		return NumericProperty->IsFloatingPoint()
			? NumericProperty->GetFloatingPointPropertyValue(ValuePtr)
			: static_cast<double>(NumericProperty->GetSignedIntPropertyValue(ValuePtr));
	}

	return DefaultValue;
}

bool ABHGameplayHUD::GetBoolProperty(const UObject* Object, FName PropertyName, bool DefaultValue)
{
	if (!Object)
	{
		return DefaultValue;
	}

	const FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName);
	if (!BoolProperty)
	{
		return DefaultValue;
	}

	return BoolProperty->GetPropertyValue_InContainer(Object);
}
