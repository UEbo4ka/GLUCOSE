// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LyraStaticItemSpawner.h"

#include "LyraDynamicItemSpawner.generated.h"

namespace EEndPlayReason { enum Type : int; }

/** The Dynamic Spawner manages the periodic spawning of items or weapons at specific locations.
 * After an item is picked up, it automatically respawns at the same location after a set amount of time. */
UCLASS(Blueprintable, BlueprintType)
class LYRAGAME_API ALyraDynamicItemSpawner : public ALyraStaticItemSpawner
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALyraDynamicItemSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	
	//The amount of time between weapon pickup and weapon spawning in seconds
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	float CoolDownTime;

	//Delay between when the weapon is made available and when we check for a pawn standing in the spawner. Used to give the bIsItemAvailable OnRep time to fire and play FX. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	float CheckExistingOverlapDelay;

	//Used to drive weapon respawn time indicators 0-1
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Pickup")
	float CoolDownPercentage;

public:
	
	FTimerHandle CoolDownTimerHandle;

	FTimerHandle CheckOverlapsDelayTimerHandle;

	void StartCoolDown();

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void ResetCoolDown();

	UFUNCTION()
	void OnCoolDownTimerComplete();

	virtual void SetItemPickupVisibility(bool bShouldBeVisible) override;
	
	UFUNCTION(BlueprintNativeEvent, Category = "Pickup")
	void PlayRespawnEffects();
	
	virtual void OnRep_ItemAvailability() override;
};
