#pragma once

#include "CoreMinimal.h"
#include "Inventory/ALSPickupBase.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "ALSItemPickup.generated.h"

// Generic inventory-item pickup: adds ItemID/DisplayName/Quantity to
// whatever pawn's UALSInventoryComponent overlaps it, respecting that
// item's MaxStack (leaves the pickup in the world - not consumed - if the
// pawn has no inventory component at all, or the stack is already full).
UCLASS()
class ALSHOST_API AALSItemPickup : public AALSPickupBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	int32 MaxStack = 99;

	// Set true for items that should show up as clickable/equippable in the
	// inventory panel (e.g. a medkit) rather than just being a passive
	// stack (ammo, crafting materials). Mirrors AALSWeaponPickup's own
	// bEquippable/EquipOverlayState pair - see UALSInventoryComponent::AddItem.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	bool bEquippable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup", meta = (EditCondition = "bEquippable"))
	EALSOverlayState EquipOverlayState = EALSOverlayState::Default;

protected:
	virtual bool OnPickedUp(APawn* Pawn) override;
};
