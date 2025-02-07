// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionOption.h"

#include "LyraWorldInteractable.generated.h"

class UObject;
struct FInteractionQuery;

// An actor that can provide interaction options
UCLASS(Abstract, Blueprintable)
class LYRAGAME_API ALyraWorldInteractable : public AActor, public IInteractableTarget
{
  GENERATED_BODY()

public:
	
	// Sets default values for this actor's properties
	ALyraWorldInteractable();
	
	/** Interaction options map 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Interaction")
	TMap<FName, FAVInteractionOption> Options;*/
	/** Blueprint implementable event to get interaction options */
	UFUNCTION(BlueprintImplementableEvent)
	TArray<FInteractionOption> GetInteractionOptions(const FInteractionQuery InteractQuery);
	
	//~IInteractableTarget interface
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder) override;
	//~End of IInteractableTarget interface
	
protected:
	
	UPROPERTY(EditAnywhere, Category = "Interaction")
	TArray<FInteractionOption> Options;
};
