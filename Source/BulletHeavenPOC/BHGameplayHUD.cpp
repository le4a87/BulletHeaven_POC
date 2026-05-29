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

	constexpr float PanelX = 24.0f;
	constexpr float PanelY = 24.0f;
	constexpr float PanelWidth = 420.0f;
	constexpr float PanelHeight = 142.0f;
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f), PanelX, PanelY, PanelWidth, PanelHeight);
	DrawRect(FLinearColor(0.0f, 0.72f, 0.95f, 0.85f), PanelX, PanelY, 5.0f, PanelHeight);

	constexpr float PrimaryX = PanelX + 20.0f;
	constexpr float PrimaryY = PanelY + 12.0f;
	DrawPrimaryStat(TEXT("TIME"), FormatElapsedTime(GetElapsedTimeSeconds()), PrimaryX, PrimaryY, 1.9f);
	DrawPrimaryStat(TEXT("KILLS"), FString::FromInt(KillCount), PrimaryX + 210.0f, PrimaryY, 1.9f);

	float SecondaryY = PanelY + 92.0f;
	float LeftY = SecondaryY;
	float RightY = SecondaryY;
	DrawGameplayLine(FString::Printf(TEXT("Health: %.0f / %.0f"), CurrentHealth, MaxHealth), PrimaryX, LeftY, FLinearColor::White, 1.0f);
	DrawGameplayLine(FString::Printf(TEXT("Live Enemies: %d"), LiveEnemyCount), PrimaryX + 210.0f, RightY, FLinearColor::White, 1.0f);

	if (bIsGameOver)
	{
		const float CenterX = (Canvas->SizeX * 0.5f) - 125.0f;
		const float CenterY = Canvas->SizeY * 0.42f;
		DrawReadableText(TEXT("GAME OVER"), FLinearColor::Red, CenterX, CenterY, 2.0f);
		DrawReadableText(TEXT("Stop PIE or restart the level to try again."), FLinearColor::White, CenterX - 70.0f, CenterY + 42.0f, 1.0f);
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
