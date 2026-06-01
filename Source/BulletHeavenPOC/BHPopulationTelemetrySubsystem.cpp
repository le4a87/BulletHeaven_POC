// Copyright Epic Games, Inc. All Rights Reserved.

#include "BHPopulationTelemetrySubsystem.h"

#include "BHEnemyRegistrySubsystem.h"
#include "BHEnemyUpdateBudgetSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

namespace BHPopulationTelemetry
{
	const FName ObstacleTag = TEXT("BHObstacle");
	const FName MaxEnemiesAlivePropertyName = TEXT("MaxEnemiesAlive");
	const FName SpawnRatePropertyName = TEXT("SpawnRate");
	const TCHAR* Header = TEXT("ElapsedSeconds,AvgFrameMs,WorstFrameMs,AvgFPS,LiveEnemies,DefeatedEnemies,Projectiles,Obstacles,SpawnerMaxEnemiesAlive,SpawnerSpawnRate,UsedPhysicalMB,PeakUsedPhysicalMB,UsedVirtualMB,PeakUsedVirtualMB,EnemyBudgetEnabled,EnemyBudgetNear,EnemyBudgetMid,EnemyBudgetFar,EnemyAnimRateVaried\n");

	double BytesToMiB(uint64 Bytes)
	{
		return static_cast<double>(Bytes) / (1024.0 * 1024.0);
	}
}

UBHPopulationTelemetrySubsystem::UBHPopulationTelemetrySubsystem()
{
	ProjectileClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Blueprints/Projectiles/BP_Projectile.BP_Projectile_C")));
	SpawnerClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Blueprints/Spawners/BP_EnemySpawner.BP_EnemySpawner_C")));
}

bool UBHPopulationTelemetrySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer)
		&& World
		&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UBHPopulationTelemetrySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	bIsActiveTargetMap = bEnabled && IsTargetMap(InWorld);
	if (bIsActiveTargetMap)
	{
		StartTelemetry(InWorld);
	}
}

void UBHPopulationTelemetrySubsystem::Deinitialize()
{
	if (bIsActiveTargetMap && !OutputFilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("BH population telemetry written to %s"), *OutputFilePath);
	}

	bIsActiveTargetMap = false;
	OutputFilePath.Reset();
	Super::Deinitialize();
}

void UBHPopulationTelemetrySubsystem::Tick(float DeltaTime)
{
	if (!bIsActiveTargetMap)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TimeSinceLastSample += DeltaTime;
	AccumulatedFrameTime += DeltaTime;
	WorstFrameTime = FMath::Max(WorstFrameTime, DeltaTime);
	++FramesInSample;

	if (TimeSinceLastSample >= SampleIntervalSeconds)
	{
		WriteSample(*World);
		TimeSinceLastSample = 0.0f;
		AccumulatedFrameTime = 0.0f;
		WorstFrameTime = 0.0f;
		FramesInSample = 0;
	}
}

TStatId UBHPopulationTelemetrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBHPopulationTelemetrySubsystem, STATGROUP_Tickables);
}

void UBHPopulationTelemetrySubsystem::StartTelemetry(UWorld& World)
{
	OutputFilePath = BuildOutputFilePath(World);
	RunStartTime = World.GetTimeSeconds();
	TimeSinceLastSample = 0.0f;
	AccumulatedFrameTime = 0.0f;
	WorstFrameTime = 0.0f;
	FramesInSample = 0;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*FPaths::GetPath(OutputFilePath));
	FFileHelper::SaveStringToFile(BHPopulationTelemetry::Header, *OutputFilePath);
	UE_LOG(LogTemp, Log, TEXT("BH population telemetry started: %s"), *OutputFilePath);
}

void UBHPopulationTelemetrySubsystem::WriteSample(UWorld& World)
{
	if (OutputFilePath.IsEmpty() || FramesInSample <= 0)
	{
		return;
	}

	const UBHEnemyRegistrySubsystem* Registry = World.GetSubsystem<UBHEnemyRegistrySubsystem>();
	const int32 LiveEnemies = Registry ? Registry->GetLiveEnemyCount() : 0;
	const int32 DefeatedEnemies = Registry ? Registry->GetDefeatedEnemyCount() : 0;
	const UBHEnemyUpdateBudgetSubsystem* EnemyBudget = World.GetSubsystem<UBHEnemyUpdateBudgetSubsystem>();
	const FBHEnemyUpdateBudgetSnapshot BudgetSnapshot = EnemyBudget
		? EnemyBudget->GetTelemetrySnapshot()
		: FBHEnemyUpdateBudgetSnapshot();

	UClass* LoadedProjectileClass = ProjectileClass.LoadSynchronous();
	const int32 Projectiles = LoadedProjectileClass ? CountActorsOfClass(World, LoadedProjectileClass) : 0;
	const int32 Obstacles = CountActorsWithTag(World, BHPopulationTelemetry::ObstacleTag);

	const AActor* Spawner = FindFirstSpawner(World);
	const double MaxEnemiesAlive = ReadNumericProperty(Spawner, BHPopulationTelemetry::MaxEnemiesAlivePropertyName, 0.0);
	const double SpawnRate = ReadNumericProperty(Spawner, BHPopulationTelemetry::SpawnRatePropertyName, 0.0);

	const float ElapsedSeconds = FMath::Max(0.0f, World.GetTimeSeconds() - RunStartTime);
	const float AverageFrameTime = AccumulatedFrameTime / static_cast<float>(FramesInSample);
	const float AverageFrameMs = AverageFrameTime * 1000.0f;
	const float WorstFrameMs = WorstFrameTime * 1000.0f;
	const float AverageFps = AverageFrameTime > UE_SMALL_NUMBER ? 1.0f / AverageFrameTime : 0.0f;
	const FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();

	const FString Line = FString::Printf(
		TEXT("%.2f,%.3f,%.3f,%.2f,%d,%d,%d,%d,%.0f,%.3f,%.2f,%.2f,%.2f,%.2f,%d,%d,%d,%d,%d\n"),
		ElapsedSeconds,
		AverageFrameMs,
		WorstFrameMs,
		AverageFps,
		LiveEnemies,
		DefeatedEnemies,
		Projectiles,
		Obstacles,
		MaxEnemiesAlive,
		SpawnRate,
		BHPopulationTelemetry::BytesToMiB(MemoryStats.UsedPhysical),
		BHPopulationTelemetry::BytesToMiB(MemoryStats.PeakUsedPhysical),
		BHPopulationTelemetry::BytesToMiB(MemoryStats.UsedVirtual),
		BHPopulationTelemetry::BytesToMiB(MemoryStats.PeakUsedVirtual),
		BudgetSnapshot.bRuntimeBudgetEnabled ? 1 : 0,
		BudgetSnapshot.NearEnemies,
		BudgetSnapshot.MidEnemies,
		BudgetSnapshot.FarEnemies,
		BudgetSnapshot.AnimRateVariedEnemies);
	FFileHelper::SaveStringToFile(Line, *OutputFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}

FString UBHPopulationTelemetrySubsystem::BuildOutputFilePath(const UWorld& World) const
{
	const FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString CleanMapName = World.GetMapName().Replace(TEXT("/"), TEXT("_"));
	return FPaths::ProjectSavedDir() / TEXT("Profiling") / FString::Printf(TEXT("BH009_%s_%s.csv"), *CleanMapName, *Timestamp);
}

int32 UBHPopulationTelemetrySubsystem::CountActorsOfClass(UWorld& World, UClass* ActorClass) const
{
	int32 Count = 0;
	for (TActorIterator<AActor> It(&World, ActorClass); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed())
		{
			++Count;
		}
	}
	return Count;
}

int32 UBHPopulationTelemetrySubsystem::CountActorsWithTag(UWorld& World, FName Tag) const
{
	int32 Count = 0;
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->ActorHasTag(Tag))
		{
			++Count;
		}
	}
	return Count;
}

double UBHPopulationTelemetrySubsystem::ReadNumericProperty(const UObject* Object, FName PropertyName, double DefaultValue) const
{
	if (!Object)
	{
		return DefaultValue;
	}

	const FNumericProperty* Property = CastField<FNumericProperty>(Object->GetClass()->FindPropertyByName(PropertyName));
	if (!Property)
	{
		return DefaultValue;
	}

	const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
	return Property->IsFloatingPoint()
		? Property->GetFloatingPointPropertyValue(ValuePtr)
		: static_cast<double>(Property->GetSignedIntPropertyValue(ValuePtr));
}

AActor* UBHPopulationTelemetrySubsystem::FindFirstSpawner(UWorld& World) const
{
	UClass* LoadedSpawnerClass = SpawnerClass.LoadSynchronous();
	if (!LoadedSpawnerClass)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(&World, LoadedSpawnerClass); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed())
		{
			return *It;
		}
	}
	return nullptr;
}

bool UBHPopulationTelemetrySubsystem::IsTargetMap(const UWorld& World) const
{
	const FString MapName = World.GetMapName();
	return TargetMapName.IsNone() || MapName.Contains(TargetMapName.ToString());
}
