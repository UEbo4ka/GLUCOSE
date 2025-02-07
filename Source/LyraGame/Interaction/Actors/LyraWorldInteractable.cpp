// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraWorldInteractable.h"

#include "Async/TaskGraphInterfaces.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraWorldInteractable)

struct FInteractionQuery;

// Sets default values
ALyraWorldInteractable::ALyraWorldInteractable()
{
}

void ALyraWorldInteractable::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder)
{
	// InteractionBuilder.AddInteractionOption(Option);
	// Options = GetInteractionOptions(InteractQuery);
	for (FInteractionOption InteractionOption : Options)
	{
		InteractionBuilder.AddInteractionOption(InteractionOption);
	}
}
