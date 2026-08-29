#include "Inventory/ALSItemPickup.h"

#include "Inventory/ALSInventoryComponent.h"
#include "Character/ALSBaseCharacter.h"
#include "GameFramework/Pawn.h"

bool AALSItemPickup::OnPickedUp(APawn* Pawn)
{
	UALSInventoryComponent* Inventory = Pawn->FindComponentByClass<UALSInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	const int32 AmountAdded = Inventory->AddItem(ItemID, DisplayName, Quantity, MaxStack, bEquippable, EquipOverlayState);
	if (AmountAdded <= 0)
	{
		return false;
	}

	// Auto-equip into an empty hand - only when nothing is currently
	// equipped, so grabbing a second equippable item never silently swaps
	// out whatever's already held. Picking up while already holding
	// something still requires the usual manual equip (inventory panel row
	// click) - see UALSInventoryComponent::EquipItem.
	if (bEquippable)
	{
		if (const AALSBaseCharacter* Character = Cast<AALSBaseCharacter>(Pawn);
			Character && Character->GetOverlayState() == EALSOverlayState::Default)
		{
			Inventory->EquipItem(ItemID);
		}
	}

	return true;
}
