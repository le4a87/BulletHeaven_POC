// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHGameplayHUD.h"

#include "Engine/Canvas.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

ABHGameplayHUD::ABHGameplayHUD()
{
	EnemyClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Blueprints/Enemies/BP_Enemy.BP_Enemy_C")));
}

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

	RefreshEnemyMetrics();

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const double CurrentHealth = GetNumericProperty(PlayerPawn, TEXT("Health"), 0.0);
	const double MaxHealth = GetNumericProperty(PlayerPawn, TEXT("MaxHealth"), PlayerMaxHealthFallback);
	const bool bIsGameOver = GetBoolProperty(PlayerPawn, TEXT("IsGameOver"), false);

	if (bIsGameOver && GameOverTime <= RunStartTime)
	{
		if (const UWorld* World = GetWorld())
		{
			GameOverTime = World->GetTimeSeconds();
		}
	}

	float Y = 28.0f;
	constexpr float X = 32.0f;
	DrawGameplayLine(FString::Printf(TEXT("Health: %.0f / %.0f"), CurrentHealth, MaxHealth), X, Y, FLinearColor::White, 1.25f);
	DrawGameplayLine(FString::Printf(TEXT("Elapsed: %.1fs"), GetElapsedTimeSeconds()), X, Y, FLinearColor::White, 1.0f);
	DrawGameplayLine(FString::Printf(TEXT("Kills: %d"), KillCount), X, Y, FLinearColor::White, 1.0f);
	DrawGameplayLine(FString::Printf(TEXT("Live Enemies: %d"), CachedLiveEnemyCount), X, Y, FLinearColor::White, 1.0f);

	if (bIsGameOver)
	{
		const float CenterX = (Canvas->SizeX * 0.5f) - 125.0f;
		const float CenterY = Canvas->SizeY * 0.42f;
		DrawText(TEXT("GAME OVER"), FLinearColor::Red, CenterX, CenterY, nullptr, 2.0f, false);
		DrawText(TEXT("Stop PIE or restart the level to try again."), FLinearColor::White, CenterX - 70.0f, CenterY + 42.0f, nullptr, 1.0f, false);
	}
}

void ABHGameplayHUD::RefreshEnemyMetrics()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();
	if (LastEnemyMetricRefreshTime >= 0.0 && CurrentTime - LastEnemyMetricRefreshTime < EnemyMetricRefreshInterval)
	{
		return;
	}

	LastEnemyMetricRefreshTime = CurrentTime;

	UClass* LoadedEnemyClass = EnemyClass.LoadSynchronous();
	if (!LoadedEnemyClass)
	{
		CachedLiveEnemyCount = 0;
		KnownEnemies.Empty();
		return;
	}

	TArray<AActor*> CurrentEnemies;
	UGameplayStatics::GetAllActorsOfClass(this, LoadedEnemyClass, CurrentEnemies);

	TSet<FObjectKey> CurrentEnemyKeys;
	for (AActor* Enemy : CurrentEnemies)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		const FObjectKey EnemyKey(Enemy);
		CurrentEnemyKeys.Add(EnemyKey);
		KnownEnemies.FindOrAdd(EnemyKey) = Enemy;
	}

	TArray<FObjectKey> RemovedEnemyKeys;
	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& KnownEnemy : KnownEnemies)
	{
		if (!CurrentEnemyKeys.Contains(KnownEnemy.Key))
		{
			++KillCount;
			RemovedEnemyKeys.Add(KnownEnemy.Key);
		}
	}

	for (const FObjectKey& RemovedKey : RemovedEnemyKeys)
	{
		KnownEnemies.Remove(RemovedKey);
	}

	CachedLiveEnemyCount = CurrentEnemyKeys.Num();
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

void ABHGameplayHUD::DrawGameplayLine(const FString& Text, float X, float& Y, const FLinearColor& Color, float Scale)
{
	constexpr float LineHeight = 24.0f;
	DrawText(Text, Color, X, Y, nullptr, Scale, false);
	Y += LineHeight * Scale;
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
