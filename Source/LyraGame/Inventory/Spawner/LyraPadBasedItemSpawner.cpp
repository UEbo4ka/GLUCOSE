// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraPadBasedItemSpawner.h"

#include "Components/StaticMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraPadBasedItemSpawner)

// Sets default values
ALyraPadBasedItemSpawner::ALyraPadBasedItemSpawner()
{
	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(RootComponent);
}
