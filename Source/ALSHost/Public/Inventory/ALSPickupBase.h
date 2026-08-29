#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/ALSInteractable.h"
#include "ALSPickupBase.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

// Common base for a world pickup: picked up by pressing the Interact key
// (E) while looking at it - IALSInteractable, the same interface doors and
// loot containers use - rather than by walking into it. Subclasses only
// implement OnPickedUp() - what actually happens (add to inventory, heal,
// etc.) - everything else (mesh, interact wiring, one-pickup-only guard)
// lives here.
UCLASS(Abstract)
class ALSHOST_API AALSPickupBase : public AActor, public IALSInteractable
{
	GENERATED_BODY()

public:
	AALSPickupBase();

	// Used to build the interact prompt ("Pick Up {PickupLabel}") - set
	// per-instance in the level, not tied to any inventory ItemID.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FText PickupLabel;

	// Floating prompt label - hidden by default, only shown (via
	// SetInteractPromptVisible_Implementation) while this pickup is the
	// player's current interact target. See ALSInteractable.h.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Pickup")
	TObjectPtr<UTextRenderComponent> LabelText;

	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual void SetInteractPromptVisible_Implementation(bool bVisible) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Return true if the pawn actually consumed the pickup (destroys the
	// actor and prevents further interacts from doing anything). Return
	// false to leave the pickup in the world (e.g. inventory full).
	virtual bool OnPickedUp(APawn* Pawn) PURE_VIRTUAL(AALSPickupBase::OnPickedUp, return false;);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	bool bConsumed = false;
};
