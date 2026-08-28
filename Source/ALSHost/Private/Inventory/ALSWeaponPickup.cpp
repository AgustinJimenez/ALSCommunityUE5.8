#include "Inventory/ALSWeaponPickup.h"

#include "Inventory/ALSInventoryComponent.h"
#include "Character/ALSCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AALSWeaponPickup::AALSWeaponPickup()
{
	// The inherited static Mesh is a placeholder Cube/Sphere and can't
	// display a real (skeletal) weapon mesh - hide it rather than remove it,
	// so AALSPickupBase's shared logic keeps working unmodified.
	Mesh->SetHiddenInGame(true);
	Mesh->SetVisibility(false);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
