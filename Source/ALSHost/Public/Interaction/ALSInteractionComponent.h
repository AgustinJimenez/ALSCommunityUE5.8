#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "ALSInteractionComponent.generated.h"

class UInputAction;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FALSOnInteractableChanged, AActor*, NewInteractable, FText, Prompt);

// Binds its own Enhanced Input action directly, same self-contained pattern
// as UALSWeaponFireComponent/UALSInventoryComponent - no edits needed to
// the vendored ALS-Community-UE5 plugin.
//
// Detection is proximity-based, not a camera aim trace: every tick, finds
// the nearest IALSInteractable actor within InteractRange of the owning
// pawn that's also within InteractFacingCosineThreshold of "in front of the
// character" - not the camera. A precise camera-trace requires pixel-
// perfect aim, which is unreasonable in third person (the camera sits
// offset from the player and pulls back further at steep look-down
// angles), so "near and roughly facing it" is the standard third-person
// convention instead. The result is cached as CurrentInteractable and
// broadcast via OnInteractableChanged only when it actually changes, so a
// HUD widget can show a "[E] <prompt>" label near the object without
// polling every frame. Pressing Interact calls Execute_Interact on
// whatever CurrentInteractable currently is - same target the UI is
// showing, by construction.
UCLASS(ClassGroup = (ALSHost), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSInteractionComponent();

	// How far from the owning pawn an interactable can be and still count.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Interact")
	float InteractRange = 250.0f;

	// cos(half-angle) of the forward-facing cone an interactable must fall
	// within to count - 0.5 is a 60-degree half-angle (120-degree total
	// cone), generous enough that the player doesn't have to aim precisely,
	// just be facing roughly toward the object.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Interact")
	float InteractFacingCosineThreshold = 0.5f;

	UPROPERTY(BlueprintAssignable, Category = "ALS|Interact")
	FALSOnInteractableChanged OnInteractableChanged;

	UFUNCTION(BlueprintPure, Category = "ALS|Interact")
	AActor* GetCurrentInteractable() const { return CurrentInteractable; }

	// Attempts to interact with CurrentInteractable right now - same thing
	// pressing the input action does, exposed directly so it's testable/
	// callable without needing real input injection. Returns true if there
	// was a valid interactable and its Interact() was called.
	UFUNCTION(BlueprintCallable, Category = "ALS|Interact")
	bool TryInteract();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void TrySetupInput();
	void HandleInteractInput(const FInputActionValue& Value);
	void RefreshCurrentInteractable();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Interact|Input")
	TObjectPtr<UInputAction> InteractInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Interact|Input")
	TObjectPtr<UInputMappingContext> InteractInputMappingContext;

private:
	bool bInputBound = false;

	UPROPERTY()
	TObjectPtr<AActor> CurrentInteractable;

	// Compared alongside CurrentInteractable so a prompt text change on the
	// SAME target (e.g. a loot container's prompt going from "Open" to
	// "(Empty)" the instant it's looted) still re-broadcasts, not just a
	// change of target.
	FText LastBroadcastPrompt;
};
