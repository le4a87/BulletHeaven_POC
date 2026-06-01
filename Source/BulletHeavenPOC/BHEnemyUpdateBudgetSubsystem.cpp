// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHEnemyUpdateBudgetSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace BHEnemyUpdateBudget
{
	TAutoConsoleVariable<int32> CVarEnemyBudgetEnabled(
		TEXT("bh.EnemyBudget.Enabled"),
		1,
		TEXT("Enables distance-based enemy tick and animation budgeting."),
		ECVF_Default);

	constexpr int32 OriginalProfileIndex = INDEX_NONE;
	constexpr int32 MidProfileIndex = 0;
	constexpr int32 FarProfileIndex = 1;
}

UBHEnemyUpdateBudgetSubsystem::UBHEnemyUpdateBudgetSubsystem()
{
	EnemyClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Blueprints/Enemies/BP_Enemy.BP_Enemy_C")));

	MidProfile.MaxDistanceFromPlayer = 4200.0f;
	MidProfile.ActorTickInterval = 0.05f;
	MidProfile.MovementTickInterval = 0.033f;
	MidProfile.MeshTickInterval = 0.05f;
	MidProfile.AnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	FarProfile.MaxDistanceFromPlayer = TNumericLimits<float>::Max();
	FarProfile.ActorTickInterval = 0.12f;
	FarProfile.MovementTickInterval = 0.08f;
	FarProfile.MeshTickInterval = 0.15f;
	FarProfile.AnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
}

bool UBHEnemyUpdateBudgetSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer)
		&& World
		&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UBHEnemyUpdateBudgetSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	bIsActiveTargetMap = bEnabled && IsTargetMap(InWorld) && IsRuntimeBudgetEnabled();
	if (!bIsActiveTargetMap)
	{
		return;
	}

	TimeUntilNextDiscovery = 0.0f;
	TimeUntilNextBudgetRefresh = 0.0f;
	DiscoverEnemies();
	ApplyBudgetPolicies();
}

void UBHEnemyUpdateBudgetSubsystem::Deinitialize()
{
	for (TPair<FObjectKey, FCachedEnemySettings>& EnemyPair : CachedEnemies)
	{
		RestoreOriginalSettings(EnemyPair.Value, true);
	}

	CachedEnemies.Empty();
	bIsActiveTargetMap = false;
	Super::Deinitialize();
}

void UBHEnemyUpdateBudgetSubsystem::Tick(float DeltaTime)
{
	if (!bIsActiveTargetMap)
	{
		return;
	}

	if (!IsRuntimeBudgetEnabled())
	{
		for (TPair<FObjectKey, FCachedEnemySettings>& EnemyPair : CachedEnemies)
		{
			RestoreOriginalSettings(EnemyPair.Value, true);
		}
		return;
	}

	TimeUntilNextDiscovery -= DeltaTime;
	if (TimeUntilNextDiscovery <= 0.0f)
	{
		TimeUntilNextDiscovery = DiscoveryInterval;
		DiscoverEnemies();
	}
	else
	{
		PruneInvalidEnemies();
	}

	TimeUntilNextBudgetRefresh -= DeltaTime;
	if (TimeUntilNextBudgetRefresh <= 0.0f)
	{
		TimeUntilNextBudgetRefresh = BudgetRefreshInterval;
		ApplyBudgetPolicies();
	}
}

TStatId UBHEnemyUpdateBudgetSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBHEnemyUpdateBudgetSubsystem, STATGROUP_Tickables);
}

FBHEnemyUpdateBudgetSnapshot UBHEnemyUpdateBudgetSubsystem::GetTelemetrySnapshot() const
{
	FBHEnemyUpdateBudgetSnapshot Snapshot;
	Snapshot.bRuntimeBudgetEnabled = bIsActiveTargetMap && IsRuntimeBudgetEnabled();

	for (const TPair<FObjectKey, FCachedEnemySettings>& EnemyPair : CachedEnemies)
	{
		const FCachedEnemySettings& Settings = EnemyPair.Value;
		if (!IsUsableEnemy(Settings.Enemy.Get()))
		{
			continue;
		}

		switch (Settings.AppliedProfileIndex)
		{
		case BHEnemyUpdateBudget::MidProfileIndex:
			++Snapshot.MidEnemies;
			break;
		case BHEnemyUpdateBudget::FarProfileIndex:
			++Snapshot.FarEnemies;
			break;
		default:
			++Snapshot.NearEnemies;
			break;
		}

		if (Settings.bAppliedAnimRateScale)
		{
			++Snapshot.AnimRateVariedEnemies;
		}
	}

	return Snapshot;
}

void UBHEnemyUpdateBudgetSubsystem::DiscoverEnemies()
{
	UWorld* World = GetWorld();
	UClass* LoadedEnemyClass = EnemyClass.LoadSynchronous();
	if (!World || !LoadedEnemyClass)
	{
		CachedEnemies.Empty();
		return;
	}

	for (TActorIterator<AActor> It(World, LoadedEnemyClass); It; ++It)
	{
		CacheEnemy(*It);
	}

	PruneInvalidEnemies();
}

void UBHEnemyUpdateBudgetSubsystem::PruneInvalidEnemies()
{
	TArray<FObjectKey> RemovedEnemies;
	for (const TPair<FObjectKey, FCachedEnemySettings>& EnemyPair : CachedEnemies)
	{
		if (!IsUsableEnemy(EnemyPair.Value.Enemy.Get()))
		{
			RemovedEnemies.Add(EnemyPair.Key);
		}
	}

	for (const FObjectKey& RemovedEnemy : RemovedEnemies)
	{
		CachedEnemies.Remove(RemovedEnemy);
	}
}

void UBHEnemyUpdateBudgetSubsystem::ApplyBudgetPolicies()
{
	UWorld* World = GetWorld();
	const APawn* PlayerPawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	for (TPair<FObjectKey, FCachedEnemySettings>& EnemyPair : CachedEnemies)
	{
		FCachedEnemySettings& Settings = EnemyPair.Value;
		AActor* Enemy = Settings.Enemy.Get();
		if (!IsUsableEnemy(Enemy))
		{
			continue;
		}

		ApplyAnimRateVariation(Settings);

		int32 ProfileIndex = BHEnemyUpdateBudget::OriginalProfileIndex;
		const FBHEnemyUpdateBudgetProfile* Profile = SelectProfile(
			FVector::DistSquared2D(PlayerLocation, Enemy->GetActorLocation()),
			ProfileIndex);

		if (!Profile)
		{
			RestoreOriginalSettings(Settings, false);
		}
		else
		{
			ApplyProfile(Settings, *Profile, ProfileIndex);
		}
	}
}

void UBHEnemyUpdateBudgetSubsystem::CacheEnemy(AActor* Enemy)
{
	if (!IsUsableEnemy(Enemy))
	{
		return;
	}

	const FObjectKey EnemyKey(Enemy);
	if (CachedEnemies.Contains(EnemyKey))
	{
		return;
	}

	FCachedEnemySettings Settings;
	Settings.Enemy = Enemy;
	Settings.ActorTickInterval = Enemy->GetActorTickInterval();

	if (ACharacter* Character = Cast<ACharacter>(Enemy))
	{
		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			Settings.MovementComponent = MovementComponent;
			Settings.MovementTickInterval = MovementComponent->GetComponentTickInterval();
		}

		if (USkeletalMeshComponent* MeshComponent = Character->GetMesh())
		{
			Settings.MeshComponent = MeshComponent;
			Settings.MeshTickInterval = MeshComponent->GetComponentTickInterval();
			Settings.GlobalAnimRateScale = MeshComponent->GlobalAnimRateScale;
			Settings.AnimTickOption = MeshComponent->VisibilityBasedAnimTickOption;
		}
	}

	ApplyAnimRateVariation(Settings);
	CachedEnemies.Add(EnemyKey, Settings);
}

void UBHEnemyUpdateBudgetSubsystem::ApplyAnimRateVariation(FCachedEnemySettings& Settings) const
{
	if (!bRandomizeAnimRate || Settings.bAppliedAnimRateScale)
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = Settings.MeshComponent.Get();
	if (!MeshComponent)
	{
		return;
	}

	const float MinRate = FMath::Min(MinAnimRateScale, MaxAnimRateScale);
	const float MaxRate = FMath::Max(MinAnimRateScale, MaxAnimRateScale);
	if (FMath::IsNearlyEqual(MinRate, MaxRate))
	{
		MeshComponent->GlobalAnimRateScale = MinRate;
	}
	else
	{
		const uint32 Seed = GetTypeHash(FObjectKey(Settings.Enemy.Get()));
		FRandomStream RandomStream(static_cast<int32>(Seed));
		MeshComponent->GlobalAnimRateScale = RandomStream.FRandRange(MinRate, MaxRate);
	}

	Settings.bAppliedAnimRateScale = true;
}

void UBHEnemyUpdateBudgetSubsystem::RestoreOriginalSettings(FCachedEnemySettings& Settings, bool bRestoreAnimRate) const
{
	if (AActor* Enemy = Settings.Enemy.Get(); IsUsableEnemy(Enemy))
	{
		if (Settings.AppliedProfileIndex != BHEnemyUpdateBudget::OriginalProfileIndex)
		{
			Enemy->SetActorTickInterval(Settings.ActorTickInterval);
		}
	}

	if (UCharacterMovementComponent* MovementComponent = Settings.MovementComponent.Get())
	{
		if (Settings.AppliedProfileIndex != BHEnemyUpdateBudget::OriginalProfileIndex)
		{
			MovementComponent->SetComponentTickInterval(Settings.MovementTickInterval);
		}
	}

	if (USkeletalMeshComponent* MeshComponent = Settings.MeshComponent.Get())
	{
		if (Settings.AppliedProfileIndex != BHEnemyUpdateBudget::OriginalProfileIndex)
		{
			MeshComponent->SetComponentTickInterval(Settings.MeshTickInterval);
			MeshComponent->VisibilityBasedAnimTickOption = Settings.AnimTickOption;
		}

		if (bRestoreAnimRate && Settings.bAppliedAnimRateScale)
		{
			MeshComponent->GlobalAnimRateScale = Settings.GlobalAnimRateScale;
			Settings.bAppliedAnimRateScale = false;
		}
	}

	Settings.AppliedProfileIndex = BHEnemyUpdateBudget::OriginalProfileIndex;
}

void UBHEnemyUpdateBudgetSubsystem::ApplyProfile(
	FCachedEnemySettings& Settings,
	const FBHEnemyUpdateBudgetProfile& Profile,
	int32 ProfileIndex) const
{
	if (Settings.AppliedProfileIndex == ProfileIndex)
	{
		return;
	}

	if (AActor* Enemy = Settings.Enemy.Get(); IsUsableEnemy(Enemy))
	{
		Enemy->SetActorTickInterval(Profile.ActorTickInterval);
	}

	if (UCharacterMovementComponent* MovementComponent = Settings.MovementComponent.Get())
	{
		MovementComponent->SetComponentTickInterval(Profile.MovementTickInterval);
	}

	if (USkeletalMeshComponent* MeshComponent = Settings.MeshComponent.Get())
	{
		MeshComponent->SetComponentTickInterval(Profile.MeshTickInterval);
		MeshComponent->VisibilityBasedAnimTickOption = Profile.AnimTickOption;
	}

	Settings.AppliedProfileIndex = ProfileIndex;
}

const FBHEnemyUpdateBudgetProfile* UBHEnemyUpdateBudgetSubsystem::SelectProfile(
	float DistanceSquared,
	int32& OutProfileIndex) const
{
	OutProfileIndex = BHEnemyUpdateBudget::OriginalProfileIndex;
	if (DistanceSquared <= FMath::Square(NearDistance))
	{
		return nullptr;
	}

	OutProfileIndex = BHEnemyUpdateBudget::MidProfileIndex;
	if (DistanceSquared <= FMath::Square(MidProfile.MaxDistanceFromPlayer))
	{
		return &MidProfile;
	}

	OutProfileIndex = BHEnemyUpdateBudget::FarProfileIndex;
	return &FarProfile;
}

bool UBHEnemyUpdateBudgetSubsystem::IsTargetMap(const UWorld& World) const
{
	const FString MapName = World.GetMapName();
	return TargetMapName.IsNone() || MapName.Contains(TargetMapName.ToString());
}

bool UBHEnemyUpdateBudgetSubsystem::IsRuntimeBudgetEnabled()
{
	const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("bh.EnemyBudget.Enabled"));
	return !CVar || CVar->GetInt() != 0;
}

bool UBHEnemyUpdateBudgetSubsystem::IsUsableEnemy(const AActor* Enemy)
{
	return IsValid(Enemy) && !Enemy->IsActorBeingDestroyed();
}
