#include "Inventory/ALSWeaponPickup.h"

#include "Inventory/ALSInventoryComponent.h"
#include "Character/ALSCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AALSWeaponPickup::AALSWeaponPickup()
{
	// Distinct shape from the Sphere AALSPickupBase defaults to (ammo/health
	// pickups), so the two are visually distinguishable at a glance in PIE.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMeshFinder.Object);
	}
}

bool AALSWeaponPickup::OnPickedUp(APawn* Pawn)
{
	AALSCharacter* ALSChar = Cast<AALSCharacter>(Pawn);
	if (!ALSChar)
	{
		return false;
	}

	ALSChar->SetOverlayState(OverlayStateToEquip);

	if (UALSInventoryComponent* Inventory = ALSChar->FindComponentByClass<UALSInventoryComponent>())
	{
		if (!WeaponItemID.IsNone())
		{
			Inventory->AddItem(WeaponItemID, WeaponDisplayName, /*Quantity=*/1, /*MaxStack=*/1, /*bEquippable=*/true, OverlayStateToEquip);
		}

		if (BonusAmmoQuantity > 0 && !AmmoItemID.IsNone())
		{
			Inventory->AddItem(AmmoItemID, AmmoDisplayName, BonusAmmoQuantity, TNumericLimits<int32>::Max());
		}
	}

	return true;
}
