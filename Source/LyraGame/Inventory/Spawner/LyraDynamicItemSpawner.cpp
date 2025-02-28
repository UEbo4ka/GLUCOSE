// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraDynamicItemSpawner.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/LyraPickupDefinition.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraDynamicItemSpawner)

class UNiagaraSystem;
class USoundBase;

// Sets default values
ALyraDynamicItemSpawner::ALyraDynamicItemSpawner()
{
	ItemMeshRotationSpeed = 40.0f;
	CoolDownTime = 30.0f;
}

// Called when the game starts or when spawned
void ALyraDynamicItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (ItemDefinition && ItemDefinition->InventoryItemDefinition)
	{
		CoolDownTime = ItemDefinition->SpawnCoolDownSeconds;	
	}
}

void ALyraDynamicItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CoolDownTimerHandle);
		World->GetTimerManager().ClearTimer(CheckOverlapsDelayTimerHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ALyraDynamicItemSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Update the CoolDownPercentage property to drive respawn time indicators
	UWorld* World = GetWorld();
	if (World->GetTimerManager().IsTimerActive(CoolDownTimerHandle))
	{
		CoolDownPercentage = 1.0f - World->GetTimerManager().GetTimerRemaining(CoolDownTimerHandle)/CoolDownTime;
	}
}

void ALyraDynamicItemSpawner::StartCoolDown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CoolDownTimerHandle, this, &ALyraDynamicItemSpawner::OnCoolDownTimerComplete, CoolDownTime);
	}
}

void ALyraDynamicItemSpawner::ResetCoolDown()
{
	UWorld* World = GetWorld();

	if (World)
	{
		World->GetTimerManager().ClearTimer(CoolDownTimerHandle);
	}

	if (GetLocalRole() == ROLE_Authority)
	{
		bIsItemAvailable = true;
		PlayRespawnEffects();
		SetItemPickupVisibility(true);

		if (World)
		{
			World->GetTimerManager().SetTimer(CheckOverlapsDelayTimerHandle, this, &ALyraDynamicItemSpawner::CheckForExistingOverlaps, CheckExistingOverlapDelay);
		}
	}

	CoolDownPercentage = 0.0f;
}

void ALyraDynamicItemSpawner::OnCoolDownTimerComplete()
{
	ResetCoolDown();
}

void ALyraDynamicItemSpawner::SetItemPickupVisibility(bool bShouldBeVisible)
{
	Super::SetItemPickupVisibility(bShouldBeVisible);
	
	if (!bShouldBeVisible)
	{
		StartCoolDown();
	}
}

void ALyraDynamicItemSpawner::OnRep_ItemAvailability()
{
	Super::OnRep_ItemAvailability();
	
	if (bIsItemAvailable)
	{
		PlayRespawnEffects();
		SetItemPickupVisibility(true);
	}
}

void ALyraDynamicItemSpawner::PlayRespawnEffects_Implementation()
{
	if (ItemDefinition != nullptr)
	{
		USoundBase* RespawnSound = ItemDefinition->RespawnedSound;
		if (RespawnSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}

		UNiagaraSystem* RespawnEffect = ItemDefinition->RespawnedEffect;
		if (RespawnEffect != nullptr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, RespawnEffect, ItemMesh->GetComponentLocation());
		}
	}
}
