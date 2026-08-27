#include "UI/ALSInventoryContextMenuWidget.h"

#include "UI/ALSDebugMenuRowWidget.h"
#include "Components/VerticalBox.h"

void UALSInventoryContextMenuWidget::Setup(TFunction<void()> OnEquip)
{
	if (!OptionsList || !RowWidgetClass)
	{
		return;
	}

	OptionsList->ClearChildren();

	AddRow(FText::FromString(TEXT("Equip")), MoveTemp(OnEquip));

	// Deliberately no click handler yet - inspect isn't built.
	AddRow(FText::FromString(TEXT("Inspect")), TFunction<void()>());
}

void UALSInventoryContextMenuWidget::AddRow(const FText& Label, TFunction<void()> OnClicked)
{
	if (UALSDebugMenuRowWidget* Row = CreateWidget<UALSDebugMenuRowWidget>(GetOwningPlayer(), RowWidgetClass))
	{
		Row->SetRowLabel(Label);
		if (OnClicked)
		{
			Row->SetOnClicked(MoveTemp(OnClicked));
		}
		OptionsList->AddChildToVerticalBox(Row);
	}
}
