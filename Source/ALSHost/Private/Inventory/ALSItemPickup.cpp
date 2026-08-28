#include "Inventory/ALSItemPickup.h"

#include "Inventory/ALSInventoryComponent.h"
#include "GameFramework/Pawn.h"

bool AALSItemPickup::OnPickedUp(APawn* Pawn)
{
	UALSInventoryComponent* Inventory = Pawn->FindComponentByClass<UALSInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	const int32 AmountAdded = Inventory->AddItem(ItemID, DisplayName, Quantity, MaxStack, bEquippable, EquipOverlayState);
	return AmountAdded > 0;
}
