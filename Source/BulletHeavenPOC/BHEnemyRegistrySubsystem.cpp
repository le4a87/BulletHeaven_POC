// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHEnemyRegistrySubsystem.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

namespace BHEnemyRegistry
{
	constexpr float TargetTraceHeight = 90.0f;
	const FName ObstacleTag = TEXT("BHObstacle");
	const FName TargetLineOfSightTraceTag = TEXT("BH_TargetLineOfSight");
}

UBHEnemyRegistrySubsystem::UBHEnemyRegistrySubsystem()
{
	EnemyClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Blueprints/Enemies/BP_Enemy.BP_Enemy_C")));
}

bool UBHEnemyRegistrySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer)
		&& World
		&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UBHEnemyRegistrySubsystem::Deinitialize()
{
	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& EnemyPair : LiveEnemies)
	{
		if (AActor* Enemy = EnemyPair.Value.Get())
		{
			Enemy->OnDestroyed.RemoveDynamic(this, &UBHEnemyRegistrySubsystem::HandleEnemyDestroyed);
		}
	}
	LiveEnemies.Empty();

	Super::Deinitialize();
}

void UBHEnemyRegistrySubsystem::Tick(float DeltaTime)
{
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

	ApplyEnemySeparation(DeltaTime);
}

TStatId UBHEnemyRegistrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBHEnemyRegistrySubsystem, STATGROUP_Tickables);
}

void UBHEnemyRegistrySubsystem::RegisterEnemy(AActor* Enemy)
{
	if (IsUsableEnemy(Enemy))
	{
		const FObjectKey EnemyKey(Enemy);
		if (!LiveEnemies.Contains(EnemyKey))
		{
			Enemy->OnDestroyed.AddUniqueDynamic(this, &UBHEnemyRegistrySubsystem::HandleEnemyDestroyed);
		}
		LiveEnemies.FindOrAdd(EnemyKey) = Enemy;
	}
}

void UBHEnemyRegistrySubsystem::UnregisterEnemy(AActor* Enemy)
{
	if (Enemy)
	{
		Enemy->OnDestroyed.RemoveDynamic(this, &UBHEnemyRegistrySubsystem::HandleEnemyDestroyed);
		LiveEnemies.Remove(FObjectKey(Enemy));
	}
}

int32 UBHEnemyRegistrySubsystem::GetLiveEnemyCount() const
{
	int32 Count = 0;
	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& EnemyPair : LiveEnemies)
	{
		if (IsUsableEnemy(EnemyPair.Value.Get()))
		{
			++Count;
		}
	}
	return Count;
}

int32 UBHEnemyRegistrySubsystem::GetDefeatedEnemyCount() const
{
	return DefeatedEnemyCount;
}

AActor* UBHEnemyRegistrySubsystem::FindNearestEnemy(const AActor* SourceActor)
{
	if (!SourceActor)
	{
		return nullptr;
	}

	PruneInvalidEnemies();

	const FVector SourceLocation = SourceActor->GetActorLocation();
	double BestDistanceSquared = TNumericLimits<double>::Max();
	AActor* BestEnemy = nullptr;

	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& EnemyPair : LiveEnemies)
	{
		AActor* Enemy = EnemyPair.Value.Get();
		if (!IsUsableEnemy(Enemy))
		{
			continue;
		}

		const double DistanceSquared = FVector::DistSquared2D(SourceLocation, Enemy->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared && HasLineOfSightToEnemy(SourceActor, Enemy))
		{
			BestDistanceSquared = DistanceSquared;
			BestEnemy = Enemy;
		}
	}

	return BestEnemy;
}

void UBHEnemyRegistrySubsystem::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	if (!DestroyedActor)
	{
		return;
	}

	const int32 RemovedCount = LiveEnemies.Remove(FObjectKey(DestroyedActor));
	if (RemovedCount > 0)
	{
		++DefeatedEnemyCount;
	}
}

void UBHEnemyRegistrySubsystem::DiscoverEnemies()
{
	UWorld* World = GetWorld();
	UClass* LoadedEnemyClass = EnemyClass.LoadSynchronous();
	if (!World || !LoadedEnemyClass)
	{
		LiveEnemies.Empty();
		return;
	}

	for (TActorIterator<AActor> It(World, LoadedEnemyClass); It; ++It)
	{
		RegisterEnemy(*It);
	}

	PruneInvalidEnemies();
}

void UBHEnemyRegistrySubsystem::PruneInvalidEnemies()
{
	TArray<FObjectKey> RemovedEnemies;
	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& EnemyPair : LiveEnemies)
	{
		if (!IsUsableEnemy(EnemyPair.Value.Get()))
		{
			RemovedEnemies.Add(EnemyPair.Key);
		}
	}

	for (const FObjectKey& RemovedEnemy : RemovedEnemies)
	{
		LiveEnemies.Remove(RemovedEnemy);
	}
}

void UBHEnemyRegistrySubsystem::ApplyEnemySeparation(float DeltaTime)
{
	if (EnemySeparationRadius <= 0.0f || EnemySeparationStrength <= 0.0f || MaxSeparationStep <= 0.0f)
	{
		return;
	}

	TArray<AActor*> Enemies;
	Enemies.Reserve(LiveEnemies.Num());
	for (const TPair<FObjectKey, TWeakObjectPtr<AActor>>& EnemyPair : LiveEnemies)
	{
		if (AActor* Enemy = EnemyPair.Value.Get(); IsUsableEnemy(Enemy))
		{
			Enemies.Add(Enemy);
		}
	}

	const int32 EnemyCount = Enemies.Num();
	if (EnemyCount < 2)
	{
		return;
	}

	TArray<FVector> Offsets;
	Offsets.Init(FVector::ZeroVector, EnemyCount);

	const float SeparationRadiusSquared = FMath::Square(EnemySeparationRadius);
	for (int32 FirstIndex = 0; FirstIndex < EnemyCount - 1; ++FirstIndex)
	{
		AActor* FirstEnemy = Enemies[FirstIndex];
		const FVector FirstLocation = FirstEnemy->GetActorLocation();

		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < EnemyCount; ++SecondIndex)
		{
			AActor* SecondEnemy = Enemies[SecondIndex];
			const FVector SecondLocation = SecondEnemy->GetActorLocation();
			FVector Delta = FirstLocation - SecondLocation;
			Delta.Z = 0.0f;

			const float DistanceSquared = Delta.SizeSquared();
			if (DistanceSquared >= SeparationRadiusSquared)
			{
				continue;
			}

			FVector Direction = FVector::ZeroVector;
			float Distance = 0.0f;
			if (DistanceSquared > UE_SMALL_NUMBER)
			{
				Distance = FMath::Sqrt(DistanceSquared);
				Direction = Delta / Distance;
			}
			else
			{
				const float Angle = static_cast<float>((FirstIndex * 73 + SecondIndex * 37) % 360) * UE_PI / 180.0f;
				Direction = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
			}

			const float Overlap = EnemySeparationRadius - Distance;
			const float Step = FMath::Min(Overlap * EnemySeparationStrength * DeltaTime, MaxSeparationStep);
			const FVector PairOffset = Direction * (Step * 0.5f);
			Offsets[FirstIndex] += PairOffset;
			Offsets[SecondIndex] -= PairOffset;
		}
	}

	for (int32 EnemyIndex = 0; EnemyIndex < EnemyCount; ++EnemyIndex)
	{
		FVector Offset = Offsets[EnemyIndex];
		Offset.Z = 0.0f;
		if (!Offset.IsNearlyZero())
		{
			Enemies[EnemyIndex]->AddActorWorldOffset(Offset.GetClampedToMaxSize(MaxSeparationStep), false);
		}
	}
}

bool UBHEnemyRegistrySubsystem::HasLineOfSightToEnemy(const AActor* SourceActor, const AActor* Enemy) const
{
	const UWorld* World = GetWorld();
	if (!World || !SourceActor || !Enemy)
	{
		return false;
	}

	const FVector TraceStart = SourceActor->GetActorLocation() + FVector(0.0f, 0.0f, BHEnemyRegistry::TargetTraceHeight);
	const FVector TraceEnd = Enemy->GetActorLocation() + FVector(0.0f, 0.0f, BHEnemyRegistry::TargetTraceHeight);

	FCollisionQueryParams QueryParams(BHEnemyRegistry::TargetLineOfSightTraceTag, false);
	QueryParams.AddIgnoredActor(SourceActor);
	QueryParams.AddIgnoredActor(Enemy);

	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	for (const FHitResult& Hit : Hits)
	{
		const AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->ActorHasTag(BHEnemyRegistry::ObstacleTag))
		{
			return false;
		}

		if (Hit.bBlockingHit)
		{
			break;
		}
	}

	return true;
}

bool UBHEnemyRegistrySubsystem::IsUsableEnemy(const AActor* Enemy)
{
	return IsValid(Enemy) && !Enemy->IsActorBeingDestroyed();
}
