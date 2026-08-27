#include "UI/ALSInventoryWidget.h"

#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Inventory/ALSInventoryComponent.h"
#include "UI/ALSDebugMenuRowWidget.h"
#include "UI/ALSInventoryContextMenuWidget.h"
#include "Character/ALSCharacter.h"
#include "Blueprint/SlateBlueprintLibrary.h"

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

	// Rows are about to be torn down and rebuilt - the context menu was
	// popped up relative to a row that may no longer exist afterward, so
	// collapse it rather than leave it floating over a stale position.
	if (ContextMenu)
	{
		ContextMenu->SetVisibility(ESlateVisibility::Collapsed);
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
			const FString Suffix = Item.bEquippable ? TEXT("  [right-click]") : FString();
			Row->SetRowLabel(FText::FromString(FString::Printf(TEXT("%s  x%d%s"), *Item.DisplayName.ToString(), Item.Quantity, *Suffix)));

			// Left-click intentionally does nothing beyond the row's own
			// hover highlight now - equipping moved to the right-click
			// context menu's "Equip" option (see ShowContextMenu).
			if (Item.bEquippable)
			{
				const FName ItemID = Item.ItemID;
				Row->SetOnRightClicked([this, ItemID](const FVector2D& ScreenPosition) { ShowContextMenu(ItemID, ScreenPosition); });
			}

			ItemsList->AddChildToVerticalBox(Row);
		}
	}
}

void UALSInventoryWidget::ShowContextMenu(FName ItemID, const FVector2D& ScreenPosition)
{
	if (!ContextMenuWidgetClass || !RootCanvas)
	{
		return;
	}

	if (!ContextMenu)
	{
		ContextMenu = CreateWidget<UALSInventoryContextMenuWidget>(GetOwningPlayer(), ContextMenuWidgetClass);
		if (ContextMenu)
		{
			if (UCanvasPanelSlot* MenuSlot = RootCanvas->AddChildToCanvas(ContextMenu))
			{
				MenuSlot->SetAnchors(FAnchors(0.f, 0.f));
				MenuSlot->SetAlignment(FVector2D(0.f, 0.f));
				MenuSlot->SetAutoSize(true);
			}
		}
	}

	if (!ContextMenu)
	{
		return;
	}

	FVector2D ViewportPosition;
	USlateBlueprintLibrary::ScreenToViewport(this, ScreenPosition, ViewportPosition);

	if (UCanvasPanelSlot* MenuSlot = Cast<UCanvasPanelSlot>(ContextMenu->Slot))
	{
		MenuSlot->SetPosition(ViewportPosition);
	}

	ContextMenu->Setup([this, ItemID]() {
		EquipItem(ItemID);
		if (ContextMenu)
		{
			ContextMenu->SetVisibility(ESlateVisibility::Collapsed);
		}
	});
	ContextMenu->SetVisibility(ESlateVisibility::Visible);
}

bool UALSInventoryWidget::EquipItem(FName ItemID)
{
	APawn* Pawn = GetOwningPlayerPawn();
	AALSCharacter* ALSChar = Pawn ? Cast<AALSCharacter>(Pawn) : nullptr;
	UALSInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UALSInventoryComponent>() : nullptr;
	if (!ALSChar || !Inventory)
	{
		return false;
	}

	for (const FALSInventoryItem& Item : Inventory->GetItems())
	{
		if (Item.ItemID == ItemID && Item.bEquippable)
		{
			ALSChar->SetOverlayState(Item.EquipOverlayState);
			return true;
		}
	}

	return false;
}
