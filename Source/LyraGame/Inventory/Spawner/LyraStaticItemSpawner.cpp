// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraStaticItemSpawner.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/LyraPickupDefinition.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraStaticItemSpawner)

// Sets default values
ALyraStaticItemSpawner::ALyraStaticItemSpawner()
{
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(CollisionVolume);
	ItemMesh->SetCollisionProfileName(FName("NoCollision"));

	ItemMeshRotationSpeed = 0.0f;
}

// Called every frame
void ALyraStaticItemSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Update the CoolDownPercentage property to drive respawn time indicators
	UWorld* World = GetWorld();
	if (ItemMeshRotationSpeed > 0.0f)
	{
		ItemMesh->AddRelativeRotation(FRotator(0.0f, World->GetDeltaSeconds() * ItemMeshRotationSpeed, 0.0f));
	}
}

void ALyraStaticItemSpawner::OnConstruction(const FTransform& Transform)
{
	if (ItemDefinition != nullptr && ItemDefinition->DisplayMesh != nullptr)
	{
		ItemMesh->SetStaticMesh(ItemDefinition->DisplayMesh);
		ItemMesh->SetRelativeTransform(ItemDefinition->ItemMeshTransform);
	}	
}

void ALyraStaticItemSpawner::OnPickedUp()
{
	bIsItemAvailable = false;
	SetItemPickupVisibility(false);
	PlayPickupEffects();
}

void ALyraStaticItemSpawner::SetItemPickupVisibility(bool bShouldBeVisible)
{
	ItemMesh->SetVisibility(bShouldBeVisible, true);
}

void ALyraStaticItemSpawner::OnRep_ItemAvailability()
{
	if (!bIsItemAvailable)
	{
		SetItemPickupVisibility(false);
		PlayPickupEffects();
		Destroy();
	}	
}
