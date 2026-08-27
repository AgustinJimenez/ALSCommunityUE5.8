#include "Inventory/ALSInventoryComponent.h"

UALSInventoryComponent::UALSInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UALSInventoryComponent::AddItem(FName ItemID, FText DisplayName, int32 Quantity, int32 MaxStack)
{
	if (Quantity <= 0 || ItemID.IsNone())
	{
		return 0;
	}

	FALSInventoryItem* Existing = Items.FindByPredicate([ItemID](const FALSInventoryItem& Item) { return Item.ItemID == ItemID; });

	if (!Existing)
	{
		FALSInventoryItem NewItem;
		NewItem.ItemID = ItemID;
		NewItem.DisplayName = DisplayName;
		NewItem.MaxStack = MaxStack;
		NewItem.Quantity = 0;
		Existing = &Items.Add_GetRef(NewItem);
	}

	const int32 SpaceLeft = FMath::Max(Existing->MaxStack - Existing->Quantity, 0);
	const int32 AmountAdded = FMath::Min(Quantity, SpaceLeft);
	Existing->Quantity += AmountAdded;

	if (AmountAdded > 0)
	{
		OnInventoryChanged.Broadcast();
	}

	return AmountAdded;
}

int32 UALSInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return 0;
	}

	const int32 Index = Items.IndexOfByPredicate([ItemID](const FALSInventoryItem& Item) { return Item.ItemID == ItemID; });
	if (Index == INDEX_NONE)
	{
		return 0;
	}

	FALSInventoryItem& Existing = Items[Index];
	const int32 AmountRemoved = FMath::Min(Quantity, Existing.Quantity);
	Existing.Quantity -= AmountRemoved;

	if (Existing.Quantity <= 0)
	{
		Items.RemoveAt(Index);
	}

	if (AmountRemoved > 0)
	{
		OnInventoryChanged.Broadcast();
	}

	return AmountRemoved;
}

int32 UALSInventoryComponent::GetItemQuantity(FName ItemID) const
{
	const FALSInventoryItem* Existing = Items.FindByPredicate([ItemID](const FALSInventoryItem& Item) { return Item.ItemID == ItemID; });
	return Existing ? Existing->Quantity : 0;
}

bool UALSInventoryComponent::HasItem(FName ItemID, int32 Quantity) const
{
	return GetItemQuantity(ItemID) >= Quantity;
}
