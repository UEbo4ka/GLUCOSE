// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraItemSpawner.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Equipment/LyraPickupDefinition.h"
#include "GameFramework/Pawn.h"
#include "Inventory/InventoryFragment_SetStats.h"
#include "Kismet/GameplayStatics.h"
#include "LyraLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraItemSpawner)

class FLifetimeProperty;
class UNiagaraSystem;
class USoundBase;
struct FHitResult;

// Sets default values
ALyraItemSpawner::ALyraItemSpawner() {
  // Set this actor to call Tick() every frame.  You can turn this off to
  // improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	RootComponent->SetShouldUpdatePhysicsVolume(true);

	CollisionVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionVolume"));
	CollisionVolume->SetupAttachment(RootComponent);
	CollisionVolume->InitCapsuleSize(80.f, 80.f);
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &ALyraItemSpawner::OnOverlapBegin);
	
	bIsItemAvailable = true;
	bReplicates = true;
}

// Called when the game starts or when spawned
void ALyraItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!ItemDefinition)
	{
		if (const UWorld* World = GetWorld())
		{
			if (!World->IsPlayingReplay())
			{
				UE_LOG(LogLyra, Error, TEXT("'%s' does not have a valid item definition! Make sure to set this data on the instance!"), *GetNameSafe(this));	
			}
		}
	}
}

void ALyraItemSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (GetLocalRole() == ROLE_Authority && bIsItemAvailable && OverlappingPawn != nullptr)
	{
		AttemptPickUpItem(OverlappingPawn);
	}
}

void ALyraItemSpawner::CheckForExistingOverlaps()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		AttemptPickUpItem(Cast<APawn>(OverlappingActor));
	}
}

void ALyraItemSpawner::AttemptPickUpItem(APawn* Pawn)
{
	if (GetLocalRole() == ROLE_Authority && bIsItemAvailable && UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		TSubclassOf<ULyraInventoryItemDefinition> ItemDef = ItemDefinition ? ItemDefinition->InventoryItemDefinition : nullptr;
		if (ItemDef != nullptr)
		{
			//Attempt to grant the item
			if (GiveItem(ItemDef, Pawn))
			{
				//Item picked up by pawn
				OnPickedUp();
			}
		}		
	}
}

void ALyraItemSpawner::OnPickedUp()
{
	bIsItemAvailable = false;
	PlayPickupEffects();
	Destroy();
}

void ALyraItemSpawner::PlayPickupEffects_Implementation()
{
	if (ItemDefinition != nullptr)
	{
		USoundBase* PickupSound = ItemDefinition->PickedUpSound;
		if (PickupSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
		}

		UNiagaraSystem* PickupEffect = ItemDefinition->PickedUpEffect;
		if (PickupEffect != nullptr)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupEffect, RootComponent->GetComponentLocation());
		}
	}
}

int32 ALyraItemSpawner::GetDefaultStatFromItemDef(const TSubclassOf<ULyraInventoryItemDefinition> ItemClass, FGameplayTag StatTag)
{
	if (ItemClass != nullptr)
	{
		if (ULyraInventoryItemDefinition* ItemCDO = ItemClass->GetDefaultObject<ULyraInventoryItemDefinition>())
		{
			if (const UInventoryFragment_SetStats* ItemStatsFragment = Cast<UInventoryFragment_SetStats>( ItemCDO->FindFragmentByClass(UInventoryFragment_SetStats::StaticClass()) ))
			{
				return ItemStatsFragment->GetItemStatByTag(StatTag);
			}
		}
	}

	return 0;
}

void ALyraItemSpawner::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALyraItemSpawner, bIsItemAvailable);
}

void ALyraItemSpawner::OnRep_ItemAvailability()
{
	if (!bIsItemAvailable)
	{
		PlayPickupEffects();
		Destroy();
	}	
}
