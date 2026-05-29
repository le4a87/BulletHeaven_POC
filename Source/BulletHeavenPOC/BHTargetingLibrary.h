// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BHTargetingLibrary.generated.h"

class AActor;

UCLASS()
class BULLETHEAVENPOC_API UBHTargetingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Bullet Heaven|Targeting", meta = (DefaultToSelf = "SourceActor"))
	static AActor* FindNearestRegisteredEnemy(AActor* SourceActor);

	UFUNCTION(BlueprintCallable, Category = "Bullet Heaven|Targeting", meta = (WorldContext = "WorldContextObject"))
	static int32 GetRegisteredLiveEnemyCount(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Bullet Heaven|Targeting", meta = (WorldContext = "WorldContextObject"))
	static int32 GetRegisteredDefeatedEnemyCount(const UObject* WorldContextObject);
};
