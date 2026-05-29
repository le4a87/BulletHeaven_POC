// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHHealthFeedbackSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UnrealType.h"

namespace BHHealthFeedback
{
	constexpr float ActorScanInterval = 0.25f;
	constexpr float DamageNumberHeight = 150.0f;
	constexpr float DamageNumberLifetime = 0.65f;
	constexpr float DamageNumberFadeDuration = 0.35f;
	constexpr float DamageNumberRiseSpeed = 55.0f;
	constexpr float DamageNumberTextSize = 42.0f;
}

bool UBHHealthFeedbackSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer)
		&& World
		&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UBHHealthFeedbackSubsystem::Deinitialize()
{
	for (const TWeakObjectPtr<AActor>& ActorPtr : ObservedActors)
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			Actor->OnTakeAnyDamage.RemoveDynamic(this, &UBHHealthFeedbackSubsystem::HandleActorDamaged);
		}
	}

	ObservedActors.Empty();
	InitialHealthByActor.Empty();
	for (FFloatingDamageNumber& Number : FloatingDamageNumbers)
	{
		if (ATextRenderActor* TextActor = Number.TextActor.Get())
		{
			TextActor->Destroy();
		}
	}
	FloatingDamageNumbers.Empty();

	Super::Deinitialize();
}

void UBHHealthFeedbackSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TimeUntilNextActorScan -= DeltaTime;
	if (TimeUntilNextActorScan <= 0.0f)
	{
		TimeUntilNextActorScan = BHHealthFeedback::ActorScanInterval;
		RegisterHealthActors();
	}

	DrawDamageNumbers(DeltaTime);
}

void UBHHealthFeedbackSubsystem::RegisterHealthActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		double CurrentHealth = 0.0;
		double MaxHealth = 0.0;
		if (!TryGetHealth(Actor, CurrentHealth, MaxHealth))
		{
			continue;
		}

		const TWeakObjectPtr<AActor> ActorPtr(Actor);
		if (!ObservedActors.Contains(ActorPtr))
		{
			ObservedActors.Add(ActorPtr);
			Actor->OnTakeAnyDamage.AddDynamic(this, &UBHHealthFeedbackSubsystem::HandleActorDamaged);
		}
	}
}

TStatId UBHHealthFeedbackSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBHHealthFeedbackSubsystem, STATGROUP_Tickables);
}

void UBHHealthFeedbackSubsystem::HandleActorDamaged(
	AActor* DamagedActor,
	float Damage,
	const UDamageType* DamageType,
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	if (!DamagedActor || Damage <= 0.0f)
	{
		return;
	}

	FFloatingDamageNumber& Number = FloatingDamageNumbers.AddDefaulted_GetRef();
	Number.Location = DamagedActor->GetActorLocation() + FVector(0.0f, 0.0f, BHHealthFeedback::DamageNumberHeight);
	Number.Damage = Damage;
	Number.RemainingTime = BHHealthFeedback::DamageNumberLifetime;

	UWorld* World = GetWorld();
	if (!World)
	{
		FloatingDamageNumbers.Pop();
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	ATextRenderActor* TextActor = World->SpawnActor<ATextRenderActor>(
		ATextRenderActor::StaticClass(),
		Number.Location,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TextActor)
	{
		FloatingDamageNumbers.Pop();
		return;
	}

	UTextRenderComponent* TextComponent = TextActor->GetTextRender();
	TextComponent->SetText(FText::FromString(FString::Printf(TEXT("-%.0f"), Damage)));
	TextComponent->SetWorldSize(BHHealthFeedback::DamageNumberTextSize);
	TextComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	TextComponent->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	TextComponent->SetTextRenderColor(FColor(255, 70, 35));

	static UMaterialInterface* TranslucentTextMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/EngineMaterials/DefaultTextMaterialTranslucent.DefaultTextMaterialTranslucent"));
	if (TranslucentTextMaterial)
	{
		TextComponent->SetTextMaterial(TranslucentTextMaterial);
	}

	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(World, 0))
	{
		const FVector ToCamera = CameraManager->GetCameraLocation() - Number.Location;
		if (!ToCamera.IsNearlyZero())
		{
			TextActor->SetActorRotation(ToCamera.Rotation());
		}
	}

	Number.TextActor = TextActor;
}

bool UBHHealthFeedbackSubsystem::TryGetHealth(AActor* Actor, double& OutCurrentHealth, double& OutMaxHealth)
{
	if (TryReadNumericProperty(Actor, TEXT("CurrentHealth"), OutCurrentHealth)
		&& TryReadNumericProperty(Actor, TEXT("MaxHealth"), OutMaxHealth))
	{
		return OutMaxHealth > 0.0;
	}

	if (!TryReadNumericProperty(Actor, TEXT("Health"), OutCurrentHealth))
	{
		return false;
	}

	double& InitialHealth = InitialHealthByActor.FindOrAdd(Actor);
	if (InitialHealth <= 0.0)
	{
		InitialHealth = FMath::Max(OutCurrentHealth, 1.0);
	}
	OutMaxHealth = InitialHealth;
	return true;
}

bool UBHHealthFeedbackSubsystem::TryReadNumericProperty(const AActor* Actor, FName PropertyName, double& OutValue)
{
	if (!Actor)
	{
		return false;
	}

	const FNumericProperty* Property = CastField<FNumericProperty>(Actor->GetClass()->FindPropertyByName(PropertyName));
	if (!Property)
	{
		return false;
	}

	const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);
	if (Property->IsFloatingPoint())
	{
		OutValue = Property->GetFloatingPointPropertyValue(ValuePtr);
		return true;
	}
	if (Property->IsInteger())
	{
		OutValue = static_cast<double>(Property->GetSignedIntPropertyValue(ValuePtr));
		return true;
	}
	return false;
}

void UBHHealthFeedbackSubsystem::DrawDamageNumbers(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(World, 0);
	for (int32 Index = FloatingDamageNumbers.Num() - 1; Index >= 0; --Index)
	{
		FFloatingDamageNumber& Number = FloatingDamageNumbers[Index];
		Number.RemainingTime -= DeltaTime;
		if (Number.RemainingTime <= 0.0f)
		{
			if (ATextRenderActor* TextActor = Number.TextActor.Get())
			{
				TextActor->Destroy();
			}
			FloatingDamageNumbers.RemoveAtSwap(Index);
			continue;
		}

		Number.Location.Z += BHHealthFeedback::DamageNumberRiseSpeed * DeltaTime;
		ATextRenderActor* TextActor = Number.TextActor.Get();
		if (!TextActor)
		{
			FloatingDamageNumbers.RemoveAtSwap(Index);
			continue;
		}

		TextActor->SetActorLocation(Number.Location);
		if (CameraManager)
		{
			const FVector ToCamera = CameraManager->GetCameraLocation() - Number.Location;
			if (!ToCamera.IsNearlyZero())
			{
				TextActor->SetActorRotation(ToCamera.Rotation());
			}
		}

		const float FadeAlpha = FMath::Clamp(
			Number.RemainingTime / BHHealthFeedback::DamageNumberFadeDuration,
			0.0f,
			1.0f);
		const uint8 Alpha = static_cast<uint8>(FMath::RoundToInt(FadeAlpha * 255.0f));
		TextActor->GetTextRender()->SetTextRenderColor(FColor(255, 70, 35, Alpha));
	}
}
