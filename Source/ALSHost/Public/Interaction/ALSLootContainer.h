#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/ALSInteractable.h"
#include "ALSLootContainer.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

// A single-line loot entry - deliberately the same shape as
// UALSInventoryComponent::AddItem's own parameters, since that's exactly
// what Interact() calls per entry.
USTRUCT(BlueprintType)
struct FALSLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStack = 99;
};

// Interact() grants every configured LootItems entry to the interactor's
// UALSInventoryComponent, once - a second interact on an already-opened
// container does nothing (no re-looting). Uses ALS-Community's own Box
// prop mesh (Props/Meshes/Box, currently only used as a *carried* item via
// EALSOverlayState::Box) as a placed container instead - already in the
// project, no import needed, and a plain box reads fine as a loot crate.
UCLASS()
class ALSHOST_API AALSLootContainer : public AActor, public IALSInteractable
{
	GENERATED_BODY()

public:
	AALSLootContainer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Loot")
	TArray<FALSLootEntry> LootItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Loot")
	TObjectPtr<UStaticMeshComponent> ContainerMesh;

	// Floating prompt label - hidden by default, only shown while this
	// container is the player's current interact target. See
	// ALSInteractable.h.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Loot")
	TObjectPtr<UTextRenderComponent> PromptText;

	UFUNCTION(BlueprintPure, Category = "ALS|Loot")
	bool IsOpened() const { return bOpened; }

	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual void SetInteractPromptVisible_Implementation(bool bVisible) override;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	bool bOpened = false;
};
