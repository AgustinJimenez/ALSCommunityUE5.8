#include "UI/ALSInventoryWidget.h"

#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Inventory/ALSInventoryComponent.h"

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
		if (UTextBlock* Row = NewObject<UTextBlock>(this))
		{
			Row->SetText(FText::FromString(FString::Printf(TEXT("%s  x%d"), *Item.DisplayName.ToString(), Item.Quantity)));
			ItemsList->AddChildToVerticalBox(Row);
		}
	}
}
