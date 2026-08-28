#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "ALSInteractionComponent.generated.h"

class UInputAction;
class UInputMappingContext;

// Binds its own Enhanced Input action directly, same self-contained pattern
// as UALSWeaponFireComponent/UALSInventoryComponent - no edits needed to
// the vendored ALS-Community-UE5 plugin. On press, line-traces forward from
// the camera; if the first blocking hit implements IALSInteractable, calls
// its Interact(). Deliberately a single short-range trace on a discrete
// keypress, not a continuous overlap-based prompt system - simplest thing
// that actually works, can grow a "looking at something interactable" UI
// prompt later without changing this core mechanic.
UCLASS(ClassGroup = (ALSHost), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSInteractionComponent();

	// Max distance the interact trace reaches. 400 rather than a tighter
	// number because ALS's third-person camera pulls back further from the
	// player when looking down at steep pitch (to avoid clipping into the
	// ground) - exactly the look angle needed to aim at a ground-level
	// item - so the camera-to-target distance for a close-by item is
	// larger than it looks from behind the character.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Interact")
	float InteractRange = 400.0f;

	// The trace is swept as a small sphere, not a bare line - aiming a
	// thin raycast at a small ground item is unreasonably precise for a
	// player to do reliably with a third-person camera.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Interact")
	float InteractSphereRadius = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Interact")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// Attempts an interact trace right now - same thing pressing the input
	// action does, exposed directly so it's testable/callable without
	// needing real input injection. Returns true if something interactable
	// was actually hit and its Interact() was called.
	UFUNCTION(BlueprintCallable, Category = "ALS|Interact")
	bool TryInteract();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void TrySetupInput();
	void HandleInteractInput(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Interact|Input")
	TObjectPtr<UInputAction> InteractInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Interact|Input")
	TObjectPtr<UInputMappingContext> InteractInputMappingContext;

private:
	bool bInputBound = false;
};
