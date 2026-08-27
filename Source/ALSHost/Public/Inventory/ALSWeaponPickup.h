#pragma once

#include "CoreMinimal.h"
#include "Inventory/ALSPickupBase.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "ALSWeaponPickup.generated.h"

// Picking this up equips the weapon directly (SetOverlayState) rather than
// adding a slot to a multi-weapon inventory - UALSWeaponFireComponent only
// has one active weapon (whatever the character's current overlay state is)
// and only Rifle has a working muzzle socket right now (see AGENTS.md), so
// there's no multi-weapon-carry system to slot into yet. Also grants a
// starting reserve of that weapon's ammo (via UALSInventoryComponent) if
// BonusAmmoQuantity is set, so picking up a gun doesn't leave you with only
// what's already chambered.
UCLASS()
class ALSHOST_API AALSWeaponPickup : public AALSPickupBase
{
	GENERATED_BODY()

public:
	AALSWeaponPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	EALSOverlayState OverlayStateToEquip = EALSOverlayState::Rifle;

	// Must match the AmmoItemID configured on UALSWeaponFireComponent's
	// AmmoStatsByOverlayState entry for OverlayStateToEquip, or the bonus
	// ammo granted here won't be usable to reload this weapon.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FName AmmoItemID = TEXT("Ammo_Rifle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FText AmmoDisplayName;

	// 0 = grant no bonus reserve ammo, just equip the weapon.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	int32 BonusAmmoQuantity = 90;

protected:
	virtual bool OnPickedUp(APawn* Pawn) override;
};
