// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LyraDynamicItemSpawner.h"

#include "LyraPadBasedItemSpawner.generated.h"

class UStaticMeshComponent;

/**
* The LyraPadBasedItemSpawner class is responsible for spawning items based on a specific platform (PadMesh).This class handles the visual representation of the platform and provides the logic for item spawning on it.
 */
UCLASS()
class LYRAGAME_API ALyraPadBasedItemSpawner : public ALyraDynamicItemSpawner
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALyraPadBasedItemSpawner();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> PadMesh;
};
