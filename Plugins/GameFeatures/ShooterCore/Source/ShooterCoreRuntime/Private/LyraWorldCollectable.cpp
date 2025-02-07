// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraWorldCollectable.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraWorldCollectable)

ALyraWorldCollectable::ALyraWorldCollectable()
{
}

FInventoryPickup ALyraWorldCollectable::GetPickupInventory() const
{
	return StaticInventory;
}
