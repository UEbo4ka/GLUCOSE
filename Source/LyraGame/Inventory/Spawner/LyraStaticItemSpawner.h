// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LyraItemSpawner.h"

#include "LyraStaticItemSpawner.generated.h"

class UStaticMeshComponent;

/** The Static Spawner is responsible for the one-time spawning of items or weapons at predetermined locations.
 * After spawning, the item remains in place until picked up and does not automatically respawn. */
UCLASS(Blueprintable, BlueprintType)
class LYRAGAME_API ALyraStaticItemSpawner : public ALyraItemSpawner {
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALyraStaticItemSpawner();
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void OnConstruction(const FTransform& Transform) override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup")
	float ItemMeshRotationSpeed;
	
	virtual void OnPickedUp() override;
	
	virtual void SetItemPickupVisibility(bool bShouldBeVisible);
	
	virtual void OnRep_ItemAvailability() override;
};
