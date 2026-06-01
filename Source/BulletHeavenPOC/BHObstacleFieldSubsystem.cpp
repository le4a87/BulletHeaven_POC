// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHObstacleFieldSubsystem.h"

#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

namespace BHObstacleField
{
	constexpr float SpawnHeightOffset = 2.0f;
	const FName ObstacleTag = TEXT("BHObstacle");
	const FName ProjectileObstacleTraceTag = TEXT("BH_ProjectileObstacle");

	UStaticMesh* GetCubeMesh()
	{
		static UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		return CubeMesh;
	}
}

UBHObstacleFieldSubsystem::UBHObstacleFieldSubsystem()
{
	ProjectileClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Blueprints/Projectiles/BP_Projectile.BP_Projectile_C")));
}

bool UBHObstacleFieldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer)
		&& World
		&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UBHObstacleFieldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (bEnabled && IsTargetMap(InWorld))
	{
		bIsActiveTargetMap = true;
		SpawnObstacleField(InWorld);
	}
}

void UBHObstacleFieldSubsystem::Deinitialize()
{
	ClearObstacleField();
	LastProjectileLocations.Empty();
	bIsActiveTargetMap = false;
	Super::Deinitialize();
}

void UBHObstacleFieldSubsystem::Tick(float DeltaTime)
{
	if (bIsActiveTargetMap && bDestroyProjectilesOnObstacleHit)
	{
		DestroyProjectilesBlockedByObstacles();
	}
}

TStatId UBHObstacleFieldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBHObstacleFieldSubsystem, STATGROUP_Tickables);
}

void UBHObstacleFieldSubsystem::SpawnObstacleField(UWorld& World)
{
	ClearObstacleField();

	UStaticMesh* CubeMesh = BHObstacleField::GetCubeMesh();
	if (!CubeMesh || ObstacleCount <= 0 || PlacementRadius <= 0.0f)
	{
		return;
	}

	TArray<FVector> PlayerSafeLocations;
	TArray<FVector> SpawnerSafeLocations;
	CollectProtectedLocations(World, PlayerSafeLocations, SpawnerSafeLocations);

	const int32 Seed = bUseDeterministicSeed
		? RandomSeed
		: static_cast<int32>(FPlatformTime::Cycles());
	FRandomStream RandomStream(Seed);

	TArray<FVector> ObstacleLocations;
	ObstacleLocations.Reserve(ObstacleCount);

	const int32 AttemptLimit = FMath::Max(MaxPlacementAttempts, ObstacleCount);
	for (int32 Attempt = 0; Attempt < AttemptLimit && ObstacleLocations.Num() < ObstacleCount; ++Attempt)
	{
		const float Angle = RandomStream.FRandRange(0.0f, UE_TWO_PI);
		const float Distance = PlacementRadius * FMath::Sqrt(RandomStream.FRand());
		FVector Candidate(
			FMath::Cos(Angle) * Distance,
			FMath::Sin(Angle) * Distance,
			(ObstacleHeight * 0.5f) + BHObstacleField::SpawnHeightOffset);

		if (IsValidObstacleLocation(Candidate, ObstacleLocations, PlayerSafeLocations, SpawnerSafeLocations))
		{
			ObstacleLocations.Add(Candidate);
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = NAME_None;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	const FVector Scale(ObstacleFootprint / 100.0f, ObstacleFootprint / 100.0f, ObstacleHeight / 100.0f);
	for (const FVector& Location : ObstacleLocations)
	{
		AStaticMeshActor* Obstacle = World.SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(),
			Location,
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!Obstacle)
		{
			continue;
		}

		Obstacle->SetMobility(EComponentMobility::Static);
		Obstacle->SetActorScale3D(Scale);
		Obstacle->Tags.AddUnique(BHObstacleField::ObstacleTag);

		UStaticMeshComponent* MeshComponent = Obstacle->GetStaticMeshComponent();
		if (MeshComponent)
		{
			MeshComponent->SetStaticMesh(CubeMesh);
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
			MeshComponent->SetGenerateOverlapEvents(false);
			MeshComponent->CanCharacterStepUpOn = ECB_No;
		}

		SpawnedObstacles.Add(Obstacle);
	}
}

void UBHObstacleFieldSubsystem::ClearObstacleField()
{
	for (AActor* Obstacle : SpawnedObstacles)
	{
		if (IsValid(Obstacle))
		{
			Obstacle->Destroy();
		}
	}
	SpawnedObstacles.Empty();
}

void UBHObstacleFieldSubsystem::CollectProtectedLocations(
	const UWorld& World,
	TArray<FVector>& OutPlayerSafeLocations,
	TArray<FVector>& OutSpawnerSafeLocations) const
{
	OutPlayerSafeLocations.Reset();
	OutSpawnerSafeLocations.Reset();

	if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(&World, 0))
	{
		OutPlayerSafeLocations.Add(PlayerPawn->GetActorLocation());
	}

	for (TActorIterator<APlayerStart> It(&World); It; ++It)
	{
		OutPlayerSafeLocations.Add(It->GetActorLocation());
	}

	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		const AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		const FString ActorName = Actor->GetName();
		const FString ActorClassName = Actor->GetClass()->GetName();
		if (ActorName.Contains(TEXT("EnemySpawner")) || ActorClassName.Contains(TEXT("EnemySpawner")))
		{
			OutSpawnerSafeLocations.Add(Actor->GetActorLocation());
		}
	}

	if (OutPlayerSafeLocations.IsEmpty())
	{
		OutPlayerSafeLocations.Add(FVector::ZeroVector);
	}
}

bool UBHObstacleFieldSubsystem::IsValidObstacleLocation(
	const FVector& Location,
	const TArray<FVector>& ExistingLocations,
	const TArray<FVector>& PlayerSafeLocations,
	const TArray<FVector>& SpawnerSafeLocations) const
{
	for (const FVector& ExistingLocation : ExistingLocations)
	{
		if (FVector::DistSquared2D(Location, ExistingLocation) < FMath::Square(MinimumObstacleSpacing))
		{
			return false;
		}
	}

	for (const FVector& PlayerLocation : PlayerSafeLocations)
	{
		if (FVector::DistSquared2D(Location, PlayerLocation) < FMath::Square(PlayerSafeRadius))
		{
			return false;
		}
	}

	for (const FVector& SpawnerLocation : SpawnerSafeLocations)
	{
		if (FVector::DistSquared2D(Location, SpawnerLocation) < FMath::Square(SpawnerSafeRadius))
		{
			return false;
		}
	}

	return true;
}

void UBHObstacleFieldSubsystem::DestroyProjectilesBlockedByObstacles()
{
	UWorld* World = GetWorld();
	UClass* LoadedProjectileClass = ProjectileClass.LoadSynchronous();
	if (!World || !LoadedProjectileClass)
	{
		LastProjectileLocations.Empty();
		return;
	}

	TSet<TWeakObjectPtr<AActor>> SeenProjectiles;
	for (TActorIterator<AActor> It(World, LoadedProjectileClass); It; ++It)
	{
		AActor* Projectile = *It;
		if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed())
		{
			continue;
		}

		const TWeakObjectPtr<AActor> ProjectilePtr(Projectile);
		SeenProjectiles.Add(ProjectilePtr);

		const FVector CurrentLocation = Projectile->GetActorLocation();
		FVector* PreviousLocation = LastProjectileLocations.Find(ProjectilePtr);
		if (PreviousLocation && DidProjectileHitObstacle(Projectile, *PreviousLocation, CurrentLocation))
		{
			Projectile->Destroy();
			LastProjectileLocations.Remove(ProjectilePtr);
			continue;
		}

		LastProjectileLocations.FindOrAdd(ProjectilePtr) = CurrentLocation;
	}

	for (auto It = LastProjectileLocations.CreateIterator(); It; ++It)
	{
		if (!SeenProjectiles.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
}

bool UBHObstacleFieldSubsystem::DidProjectileHitObstacle(
	const AActor* Projectile,
	const FVector& PreviousLocation,
	const FVector& CurrentLocation) const
{
	const UWorld* World = GetWorld();
	if (!World || !Projectile || PreviousLocation.Equals(CurrentLocation, 1.0f))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(BHObstacleField::ProjectileObstacleTraceTag, false);
	QueryParams.AddIgnoredActor(Projectile);

	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, PreviousLocation, CurrentLocation, ECC_Visibility, QueryParams);
	for (const FHitResult& Hit : Hits)
	{
		const AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->ActorHasTag(BHObstacleField::ObstacleTag))
		{
			return true;
		}

		if (Hit.bBlockingHit)
		{
			break;
		}
	}

	return false;
}

bool UBHObstacleFieldSubsystem::IsTargetMap(const UWorld& World) const
{
	const FString MapName = World.GetMapName();
	return TargetMapName.IsNone() || MapName.Contains(TargetMapName.ToString());
}
