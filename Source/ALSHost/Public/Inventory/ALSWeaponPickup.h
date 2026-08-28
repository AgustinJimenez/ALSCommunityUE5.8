#pragma once

#include "CoreMinimal.h"
#include "Inventory/ALSPickupBase.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "ALSWeaponPickup.generated.h"

class USkeletalMeshComponent;

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

	// Also recorded as an equippable entry in UALSInventoryComponent (not
	// just equipped immediately) - MaxStack 1, since carrying two of the
	// same weapon doesn't mean anything yet (no multi-weapon-carry system,
	// see AALSWeaponPickup.cpp). Lets the player re-equip this weapon later
	// from the inventory panel (click a row to equip - see
	// UALSInventoryWidget) even after switching to something else.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FName WeaponItemID = TEXT("Weapon_Rifle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FText WeaponDisplayName;

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

	// Every real weapon mesh in this project (M4A1, M9, Bow, and the
	// migrated Lyra SK_Rifle/SKM_Shotgun/SK_Pistol) is a SkeletalMesh with
	// its own tiny per-weapon skeleton, not a StaticMesh - see AGENTS.md.
	// AALSPickupBase's inherited Mesh (StaticMeshComponent, a placeholder
	// Cube/Sphere) can't display one, so weapon pickups get their own
	// skeletal slot instead; the inherited Mesh is hidden in the
	// constructor. Set per-instance in the level to the actual weapon mesh
	// matching OverlayStateToEquip.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Pickup")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

protected:
	virtual bool OnPickedUp(APawn* Pawn) override;
};
