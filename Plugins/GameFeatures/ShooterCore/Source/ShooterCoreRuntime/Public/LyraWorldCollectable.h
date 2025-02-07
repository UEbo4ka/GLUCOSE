// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Interaction/Actors/LyraWorldInteractable.h"
#include "Inventory/IPickupable.h"

#include "LyraWorldCollectable.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class ALyraWorldCollectable : public ALyraWorldInteractable, public IPickupable
{
	GENERATED_BODY()

public:

	ALyraWorldCollectable();

	virtual FInventoryPickup GetPickupInventory() const override;

protected:

	UPROPERTY(EditAnywhere)
	FInventoryPickup StaticInventory;
};
