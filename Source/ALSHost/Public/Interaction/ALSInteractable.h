#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ALSInteractable.generated.h"

UINTERFACE(BlueprintType)
class ALSHOST_API UALSInteractable : public UInterface
{
	GENERATED_BODY()
};

// Implemented by anything the player can press the Interact key on while
// looking at it (doors, loot containers, ...) - a deliberate, explicit
// second interaction model alongside AALSPickupBase's overlap-triggers-
// automatically one. Neither replaces the other: a health pickup should
// still just be walked over, but a door swinging open the instant a player
// brushes past it would be wrong.
class ALSHOST_API IALSInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Interact")
	void Interact(APawn* Interactor);

	// Short prompt text (e.g. "Open Door") for a future on-screen interact
	// prompt - not wired to any UI yet, but every implementer should still
	// return something meaningful rather than leave this to guess later.
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Interact")
	FText GetInteractionPrompt() const;
};
