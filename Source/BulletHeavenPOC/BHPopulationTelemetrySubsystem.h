// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BHPopulationTelemetrySubsystem.generated.h"

class AActor;

/**
 * Writes lightweight population profiling samples for BH-009.
 */
UCLASS(Config = Game, DefaultConfig)
class BULLETHEAVENPOC_API UBHPopulationTelemetrySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UBHPopulationTelemetrySubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Population Telemetry")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Population Telemetry")
	FName TargetMapName = TEXT("Map_BulletHeavenPOC");

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Population Telemetry", meta = (ClampMin = "0.1", Units = "s"))
	float SampleIntervalSeconds = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Population Telemetry")
	TSoftClassPtr<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, Config, Category = "Bullet Heaven|Population Telemetry")
	TSoftClassPtr<AActor> SpawnerClass;

	FString OutputFilePath;
	float RunStartTime = 0.0f;
	float TimeSinceLastSample = 0.0f;
	float AccumulatedFrameTime = 0.0f;
	float WorstFrameTime = 0.0f;
	int32 FramesInSample = 0;
	bool bIsActiveTargetMap = false;

	void StartTelemetry(UWorld& World);
	void WriteSample(UWorld& World);
	FString BuildOutputFilePath(const UWorld& World) const;
	int32 CountActorsOfClass(UWorld& World, UClass* ActorClass) const;
	int32 CountActorsWithTag(UWorld& World, FName Tag) const;
	double ReadNumericProperty(const UObject* Object, FName PropertyName, double DefaultValue) const;
	AActor* FindFirstSpawner(UWorld& World) const;
	bool IsTargetMap(const UWorld& World) const;
};
