// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraInventoryManagerComponent.h"

#include "Engine/ActorChannel.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "LyraInventoryFilter.h"
#include "LyraInventoryItemDefinition.h"
#include "LyraInventoryItemInstance.h"
#include "LyraInventorySubsystem.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraInventoryManagerComponent)

class FLifetimeProperty;
struct FReplicationFlags;

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Lyra_Inventory_Message_StackChanged, "Lyra.Inventory.Message.StackChanged");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Lyra_Inventory_Item_Count, "Lyra.Inventory.Item.Count");

//////////////////////////////////////////////////////////////////////
// FLyraInventoryEntry

FString FLyraInventoryEntry::GetDebugString() const
{
	TSubclassOf<ULyraInventoryItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

//////////////////////////////////////////////////////////////////////
// FLyraInventoryList

void FLyraInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FLyraInventoryEntry& Stack = Entries[Index];
		if (Stack.Instance && Stack.Instance->GetItemDef())
		{
			// Removing a link from a complex structure
			TArray<FLyraInventoryEntry*>& EntriesForDef = ItemDefToEntries.FindOrAdd(Stack.Instance->GetItemDef());
			EntriesForDef.Remove(&Entries[Index]);
			if (EntriesForDef.Num() == 0)
			{
				ItemDefToEntries.Remove(Stack.Instance->GetItemDef());
			}
		}
		BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.StackCount, /*NewCount=*/ 0);
		Stack.LastObservedCount = 0;
	}
}

void FLyraInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FLyraInventoryEntry& Stack = Entries[Index];
        if (Stack.Instance && Stack.Instance->GetItemDef())
		{
			// Adding a link to the accelerating structure
			TArray<FLyraInventoryEntry*>& EntriesForDef = ItemDefToEntries.FindOrAdd(Stack.Instance->GetItemDef());
			EntriesForDef.Add(&Stack);
		}
		BroadcastChangeMessage(Stack, /*OldCount=*/ 0, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

void FLyraInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FLyraInventoryEntry& Stack = Entries[Index];
		check(Stack.LastObservedCount != INDEX_NONE);
        BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.LastObservedCount, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

void FLyraInventoryList::BroadcastChangeMessage(FLyraInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{
	FLyraInventoryChangeMessage Message;
	Message.InventoryOwner = OwnerComponent;
	Message.Instance = Entry.Instance;
	Message.NewCount = NewCount;
	Message.Delta = NewCount - OldCount;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
	MessageSystem.BroadcastMessage(TAG_Lyra_Inventory_Message_StackChanged, Message);
}

ULyraInventoryItemInstance* FLyraInventoryList::AddEntry(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount)
{
	ULyraInventoryItemInstance* Result = nullptr;

	check(ItemDef != nullptr);
 	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	// Create the item instance through the subsystem
    if (UGameInstance* GameInstance = OwnerComponent->GetWorld()->GetGameInstance())
    {
        if (ULyraInventorySubsystem* InventorySubsystem = GameInstance->GetSubsystem<ULyraInventorySubsystem>())
        {
            FLyraInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
            NewEntry.Instance = InventorySubsystem->CreateInventoryItemInstance(OwnerComponent, ItemDef);
            if (NewEntry.Instance)
            {
                NewEntry.StackCount = StackCount;
				// Add item count as a GameplayTag so it can be retrieved from the ULyraInventoryItemInstance
                NewEntry.Instance->AddStatTagStack(TAG_Lyra_Inventory_Item_Count, StackCount);
                NewEntry.LastObservedCount = StackCount;
                Result = NewEntry.Instance;
                //const ULyraInventoryItemDefinition* ItemCDO = GetDefault<ULyraInventoryItemDefinition>(ItemDef);
				//MarkItemDirty(NewEntry);
                BroadcastChangeMessage(NewEntry, /*OldCount=*/ 0, /*NewCount=*/ StackCount);
            }
        }
    }

	return Result;
}

void FLyraInventoryList::AddEntry(ULyraInventoryItemInstance* Instance)
{
	check(Instance);
 	check(OwnerComponent);
	
    AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());
	
    // Check if the instance is already managed by the inventory subsystem
    if (UGameInstance* GameInstance = OwnerComponent->GetWorld()->GetGameInstance())
    {
        if (ULyraInventorySubsystem* InventorySubsystem = GameInstance->GetSubsystem<ULyraInventorySubsystem>())
        {
            if (!InventorySubsystem->IsValidItemInstance(Instance))
            {
                // If not managed, initialize it through the subsystem
                InventorySubsystem->InitializeItemInstance(Instance, Instance->GetItemDef());
            }
        }
    }
    // Add to inventory
    FLyraInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.Instance = Instance;
    NewEntry.StackCount = 1;
	// Add item count as a GameplayTag so it can be retrieved from the ULyraInventoryItemInstance
    NewEntry.Instance->AddStatTagStack(TAG_Lyra_Inventory_Item_Count, 1);
    NewEntry.LastObservedCount = 1;
    BroadcastChangeMessage(NewEntry, /*OldCount=*/ 0, /*NewCount=*/ 1);
}

void FLyraInventoryList::RemoveEntry(ULyraInventoryItemInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FLyraInventoryEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

TArray<ULyraInventoryItemInstance*> FLyraInventoryList::GetAllItems() const
{
	if (UGameInstance* GameInstance = OwnerComponent->GetWorld()->GetGameInstance())
    {
        if (ULyraInventorySubsystem* InventorySubsystem = GameInstance->GetSubsystem<ULyraInventorySubsystem>())
        {
            return InventorySubsystem->GetValidItemsFromList(Entries);
        }
    }
    return TArray<ULyraInventoryItemInstance*>();
}

ULyraInventoryItemInstance* FLyraInventoryList::FindFirstItemStackByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef) const
{
	// Using an accelerating structure for fast search
	if (const TArray<FLyraInventoryEntry*>* FoundEntries = ItemDefToEntries.Find(ItemDef))
	{
		if (FoundEntries->Num() > 0)
		{
			return (*FoundEntries)[0]->Instance;
		}
	}
	return nullptr;
}

int32 FLyraInventoryList::GetTotalItemCountByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef) const
{
	int32 TotalCount = 0;
	// Using an accelerating structure for fast search
	if (const TArray<FLyraInventoryEntry*>* FoundEntries = ItemDefToEntries.Find(ItemDef))
	{
		for (const FLyraInventoryEntry* Entry : *FoundEntries)
		{
			TotalCount += Entry->StackCount;
		}
	}
	return TotalCount;
}

//////////////////////////////////////////////////////////////////////
// ULyraInventoryManagerComponent

ULyraInventoryManagerComponent::ULyraInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	SetIsReplicatedByDefault(true);
}

void ULyraInventoryManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

bool ULyraInventoryManagerComponent::CanAddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount)
{
	return GetMaxItemsCanAdd(ItemDef, StackCount) > 0;
}

ULyraInventoryItemInstance* ULyraInventoryManagerComponent::AddItemDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount)
{
	const int32 ActualStackCount = GetMaxItemsCanAdd(ItemDef, StackCount);
    if (ActualStackCount <= 0)
    {
        return nullptr;
    }
	
    const ULyraInventoryItemDefinition* ItemCDO = ItemDef->GetDefaultObject<ULyraInventoryItemDefinition>();
    const int32 MaxStackSize = ItemCDO->MaxStackSize;
	
    // If MaxStackSize is 0 or 1, use original behavior
    if (MaxStackSize <= 1)
    {
        ULyraInventoryItemInstance* Result = InventoryList.AddEntry(ItemDef, ActualStackCount);
        if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
        {
            AddReplicatedSubObject(Result);
        }
        return Result;
    }
	
    // Try to add to existing stacks first
    int32 RemainingCount = ActualStackCount;
    ULyraInventoryItemInstance* LastInstance = nullptr;
	
	// First pass: fill existing stacks
    for (FLyraInventoryEntry& Entry : InventoryList.Entries)
    {
        if (Entry.Instance && Entry.Instance->GetItemDef() == ItemDef && Entry.StackCount < MaxStackSize)
        {
            const int32 FreeSpace = MaxStackSize - Entry.StackCount;
            const int32 AmountToAdd = FMath::Min(RemainingCount, FreeSpace);
			
            Entry.StackCount += AmountToAdd;
            // Add item count as a GameplayTag so it can be retrieved from the ULyraInventoryItemInstance
            Entry.Instance->AddStatTagStack(TAG_Lyra_Inventory_Item_Count, AmountToAdd);
            RemainingCount -= AmountToAdd;
            LastInstance = Entry.Instance;
			
            if (RemainingCount <= 0)
            {
                break;
            }
        }
    }
	
	// Second pass: create new stacks if needed
    while (RemainingCount > 0)
    {
        const int32 NewStackCount = FMath::Min(RemainingCount, MaxStackSize);
        ULyraInventoryItemInstance* Result = InventoryList.AddEntry(ItemDef, NewStackCount);
        if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
        {
            AddReplicatedSubObject(Result);
        }
        LastInstance = Result;
        RemainingCount -= NewStackCount;
    }
	
    return LastInstance;
}

void ULyraInventoryManagerComponent::AddItemInstance(ULyraInventoryItemInstance* ItemInstance)
{
	InventoryList.AddEntry(ItemInstance);
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
	{
		AddReplicatedSubObject(ItemInstance);
	}
}

void ULyraInventoryManagerComponent::RemoveItemInstance(ULyraInventoryItemInstance* ItemInstance)
{
	InventoryList.RemoveEntry(ItemInstance);

	if (ItemInstance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(ItemInstance);
	}
}

TArray<ULyraInventoryItemInstance*> ULyraInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllItems();
}

TArray<ULyraInventoryItemInstance*> ULyraInventoryManagerComponent::GetFilteredItems(TSubclassOf<ULyraInventoryFilter> Filter) const
{
    TArray<ULyraInventoryItemInstance*> Results;

    if (Filter == nullptr)
    {
    // If no filter is specified, return all items
        return InventoryList.GetAllItems();
    }
    // Get all items and filter them
    TArray<ULyraInventoryItemInstance*> AllItems = InventoryList.GetAllItems();
    for (ULyraInventoryItemInstance* Item : AllItems)
    {
        if (const auto* FilterDef = Filter.GetDefaultObject())
        {
            if (FilterDef->PassesFilter(Item))
            {
                Results.Add(Item);
            }
        }
    }
    return Results;
}

ULyraInventoryItemInstance* ULyraInventoryManagerComponent::FindFirstItemStackByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef) const
{
	return InventoryList.FindFirstItemStackByDefinition(ItemDef);
}

int32 ULyraInventoryManagerComponent::GetTotalItemCountByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef) const
{
	return InventoryList.GetTotalItemCountByDefinition(ItemDef);
}

bool ULyraInventoryManagerComponent::ConsumeItemsByDefinition(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 NumToConsume)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}

	//@TODO: N squared right now as there's no acceleration structure
	int32 TotalConsumed = 0;
	while (TotalConsumed < NumToConsume)
	{
		if (ULyraInventoryItemInstance* Instance = FindFirstItemStackByDefinition(ItemDef))
		{
			InventoryList.RemoveEntry(Instance);
			++TotalConsumed;
		}
		else
		{
			return false;
		}
	}

	return TotalConsumed == NumToConsume;
}

void ULyraInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing ULyraInventoryItemInstance
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FLyraInventoryEntry& Entry : InventoryList.Entries)
		{
			ULyraInventoryItemInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

bool ULyraInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FLyraInventoryEntry& Entry : InventoryList.Entries)
	{
		ULyraInventoryItemInstance* Instance = Entry.Instance;

		if (Instance && IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

int32 ULyraInventoryManagerComponent::GetMaxItemsCanAdd(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 RequestedCount) const
{
    if (!ItemDef || RequestedCount <= 0)
    {
        return 0;
    }
    const ULyraInventoryItemDefinition* ItemCDO = ItemDef->GetDefaultObject<ULyraInventoryItemDefinition>();
    const int32 MaxStackSize = ItemCDO->MaxStackSize;
    const float ItemWeight = ItemCDO->Weight;
    // Check weight limit
    if (MaxInventoryWeight > 0.0f)
    {
        const float CurrentWeight = GetCurrentTotalWeight();
        const float RemainingWeight = MaxInventoryWeight - CurrentWeight;
        const int32 MaxByWeight = FMath::FloorToInt(RemainingWeight / ItemWeight);
        RequestedCount = FMath::Min(RequestedCount, MaxByWeight);
    }
    // If we can't add any items due to weight, return early
    if (RequestedCount <= 0)
    {
        return 0;
    }
    // Get current stacks of this item
    TArray<FLyraInventoryEntry*> ExistingStacks;
    int32 CurrentSlotCount = 0;
    for (auto Entry : InventoryList.Entries)
    {
        CurrentSlotCount++;
        if (Entry.Instance && Entry.Instance->GetItemDef() == ItemDef)
        {
            ExistingStacks.Add(&Entry);
        }
    }
    int32 RemainingCount = RequestedCount;
    // Try to fit items into existing stacks first
    if (MaxStackSize != 1)  // Skip if items can't be stacked
    {
        for (FLyraInventoryEntry* Entry : ExistingStacks)
        {
            if (Entry->StackCount < MaxStackSize)
            {
                const int32 FreeSpace = MaxStackSize - Entry->StackCount;
                const int32 AmountToAdd = FMath::Min(RemainingCount, FreeSpace);
                RemainingCount -= AmountToAdd;
                if (RemainingCount <= 0)
                {
                    return RequestedCount;
                }
            }
        }
    }
    // If we still have items to add, we need new slots
    if (RemainingCount > 0)
    {
        // Check slot limit
        if (MaxInventorySlots > 0)
        {
            const int32 RemainingSlots = MaxInventorySlots - CurrentSlotCount;
            if (RemainingSlots <= 0)
            {
                return RequestedCount - RemainingCount;  // Return what we could fit in existing stacks
            }
            const int32 NeededSlots = FMath::DivideAndRoundUp(RemainingCount, (MaxStackSize > 1) ? MaxStackSize : 1);
            if (NeededSlots > RemainingSlots)
            {
                // Calculate how many items we can fit in remaining slots
                const int32 ItemsInNewStacks = RemainingSlots * ((MaxStackSize > 1) ? MaxStackSize : 1);
                return (RequestedCount - RemainingCount) + ItemsInNewStacks;
            }
        }
    }
    return RequestedCount;  // We can add all requested items
}
float ULyraInventoryManagerComponent::GetCurrentTotalWeight() const
{
    float TotalWeight = 0.0f;
    for (const FLyraInventoryEntry& Entry : InventoryList.Entries)
    {
        if (Entry.Instance && Entry.Instance->GetItemDef())
        {
            const float ItemWeight = Entry.Instance->GetItemDef()->GetDefaultObject<ULyraInventoryItemDefinition>()->Weight;
            TotalWeight += ItemWeight * Entry.StackCount;
        }
    }
    return TotalWeight;
}
