// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "LyraItemSpawner.generated.h"

class APawn;
class UCapsuleComponent;
class ULyraInventoryItemDefinition;
class ULyraPickupDefinition;
class UObject;
class UPrimitiveComponent;
struct FFrame;
struct FGameplayTag;
struct FHitResult;

UCLASS(Abstract, Blueprintable, BlueprintType)
class LYRAGAME_API ALyraItemSpawner : public AActor {
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
	ALyraItemSpawner();
	
	// Collision component for detecting overlap with the player
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UCapsuleComponent> CollisionVolume;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	//Check for pawns standing on pad when the weapon is spawned. 
	void CheckForExistingOverlaps();

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	virtual void AttemptPickUpItem(APawn* Pawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	bool GiveItem(TSubclassOf<ULyraInventoryItemDefinition> ItemClass, APawn* ReceivingPawn);

	// Function to handle the pickup event
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	virtual void OnPickedUp();

	UFUNCTION(BlueprintNativeEvent, Category = "Pickup")
	void PlayPickupEffects();

	UFUNCTION()
	virtual void OnRep_ItemAvailability();

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	//Data asset used to configure a Item Spawner
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<ULyraPickupDefinition> ItemDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, ReplicatedUsing = OnRep_ItemAvailability, Category = "Pickup")
	bool bIsItemAvailable;
	
	/** Searches an item definition type for a matching stat and returns the value, or 0 if not found */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pickup")
	static int32 GetDefaultStatFromItemDef(const TSubclassOf<ULyraInventoryItemDefinition> ItemClass, FGameplayTag StatTag);
};
