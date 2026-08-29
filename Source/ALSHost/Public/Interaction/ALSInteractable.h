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

	// Short prompt text (e.g. "Open Door") - shown by whatever this
	// implementer sets up in SetInteractPromptVisible below.
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Interact")
	FText GetInteractionPrompt() const;

	// Called by UALSInteractionComponent every tick this actor is (or just
	// stopped being) the player's current interact target - true means
	// "show your own floating prompt label now, using GetInteractionPrompt()
	// for the text" (called every tick while active, not just once, so a
	// prompt text change on the same target - e.g. a loot container going
	// from "Open" to "(Empty)" - stays live); false means "hide it, the
	// player looked/walked away." Default does nothing; implementers that
	// want a prompt (see AALSPickupBase/AALSDoor/AALSLootContainer for the
	// pattern - a UTextRenderComponent toggled and billboarded toward the
	// camera, see ALSInteractable.h's FaceCameraBillboard helper) override
	// it, but the interface itself doesn't require every implementer to.
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Interact")
	void SetInteractPromptVisible(bool bVisible);
};

class UTextRenderComponent;

// Shared by every IALSInteractable implementer's own SetInteractPromptVisible
// override that uses a UTextRenderComponent for its floating label -
// UTextRenderComponent has no built-in camera-facing behavior, unlike a
// screen-space UWidgetComponent, so this rotates it manually to face
// whichever direction the player is currently looking from. Free function,
// not a method, since Door/LootContainer/PickupBase share no common actor
// base to hang a protected helper off of.
ALSHOST_API void ALSFaceTextTowardCamera(UTextRenderComponent* Text, const UObject* WorldContextObject);
