#include "UI/ALSInventoryWidget.h"

#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Inventory/ALSInventoryComponent.h"
#include "UI/ALSDebugMenuRowWidget.h"
#include "Character/ALSCharacter.h"

void UALSInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (UALSInventoryComponent* Inventory = Pawn->FindComponentByClass<UALSInventoryComponent>())
		{
			Inventory->OnInventoryChanged.AddDynamic(this, &UALSInventoryWidget::HandleInventoryChanged);
		}
	}

	RefreshItemsList();
}

void UALSInventoryWidget::HandleInventoryChanged()
{
	RefreshItemsList();
}

void UALSInventoryWidget::RefreshItemsList()
{
	if (!ItemsList)
	{
		return;
	}

	ItemsList->ClearChildren();

	APawn* Pawn = GetOwningPlayerPawn();
	UALSInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UALSInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return;
	}

	const TArray<FALSInventoryItem>& Items = Inventory->GetItems();
	if (Items.IsEmpty())
	{
		if (UTextBlock* EmptyLabel = NewObject<UTextBlock>(this))
		{
			EmptyLabel->SetText(FText::FromString(TEXT("(empty)")));
			ItemsList->AddChildToVerticalBox(EmptyLabel);
		}
		return;
	}

	for (const FALSInventoryItem& Item : Items)
	{
		if (!RowWidgetClass)
		{
			// No row class configured - fall back to a plain, non-interactive
			// label rather than silently showing nothing.
			if (UTextBlock* Row = NewObject<UTextBlock>(this))
			{
				Row->SetText(FText::FromString(FString::Printf(TEXT("%s  x%d"), *Item.DisplayName.ToString(), Item.Quantity)));
				ItemsList->AddChildToVerticalBox(Row);
			}
			continue;
		}

		if (UALSDebugMenuRowWidget* Row = CreateWidget<UALSDebugMenuRowWidget>(GetOwningPlayer(), RowWidgetClass))
		{
			const FString Suffix = Item.bEquippable ? TEXT("  [click to equip]") : FString();
			Row->SetRowLabel(FText::FromString(FString::Printf(TEXT("%s  x%d%s"), *Item.DisplayName.ToString(), Item.Quantity, *Suffix)));

			if (Item.bEquippable)
			{
				const FName ItemID = Item.ItemID;
				Row->SetOnClicked([this, ItemID]() { HandleRowClicked(ItemID); });
			}

			ItemsList->AddChildToVerticalBox(Row);
		}
	}
}

void UALSInventoryWidget::HandleRowClicked(FName ItemID)
{
	APawn* Pawn = GetOwningPlayerPawn();
	AALSCharacter* ALSChar = Pawn ? Cast<AALSCharacter>(Pawn) : nullptr;
	UALSInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UALSInventoryComponent>() : nullptr;
	if (!ALSChar || !Inventory)
	{
		return;
	}

	for (const FALSInventoryItem& Item : Inventory->GetItems())
	{
		if (Item.ItemID == ItemID && Item.bEquippable)
		{
			ALSChar->SetOverlayState(Item.EquipOverlayState);
			return;
		}
	}
}
