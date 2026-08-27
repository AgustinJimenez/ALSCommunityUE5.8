#include "UI/ALSDebugPropMenuWidget.h"

#include "UI/ALSOverlayStateOptionWidget.h"
#include "UI/ALSDebugMenuRowWidget.h"
#include "UI/ALSDebugModesMenuWidget.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Library/ALSCharacterEnumLibrary.h"

void UALSDebugPropMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!OptionsList || !OptionWidgetClass)
	{
		return;
	}

	OptionsList->ClearChildren();

	const UEnum* EnumPtr = StaticEnum<EALSOverlayState>();
	if (!EnumPtr)
	{
		return;
	}

	// UENUM(BlueprintType) always reserves a trailing hidden _MAX entry -
	// skip it explicitly rather than assume its position.
	const int64 MaxValue = EnumPtr->GetMaxEnumValue();
	const int32 NumEntries = EnumPtr->NumEnums();
	for (int32 Index = 0; Index < NumEntries; ++Index)
	{
		const int64 Value = EnumPtr->GetValueByIndex(Index);
		if (Value == MaxValue)
		{
			continue;
		}

		if (UALSOverlayStateOptionWidget* Option = CreateWidget<UALSOverlayStateOptionWidget>(GetOwningPlayer(), OptionWidgetClass))
		{
			Option->SetOverlayStateOption(static_cast<EALSOverlayState>(Value));
			OptionsList->AddChildToVerticalBox(Option);
		}
	}

	if (GenericRowWidgetClass)
	{
		if (UALSDebugMenuRowWidget* DebugModesRow = CreateWidget<UALSDebugMenuRowWidget>(GetOwningPlayer(), GenericRowWidgetClass))
		{
			DebugModesRow->SetRowLabel(FText::FromString(TEXT("Debug Modes >")));
			DebugModesRow->SetOnClicked([this]() { ToggleDebugModesSubmenu(); });
			OptionsList->AddChildToVerticalBox(DebugModesRow);
		}
	}

	if (DebugModesMenuWidgetClass && RootCanvas && !DebugModesSubmenu)
	{
		DebugModesSubmenu = CreateWidget<UALSDebugModesMenuWidget>(GetOwningPlayer(), DebugModesMenuWidgetClass);
		if (DebugModesSubmenu)
		{
			if (UCanvasPanelSlot* SubmenuSlot = RootCanvas->AddChildToCanvas(DebugModesSubmenu))
			{
				SubmenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				SubmenuSlot->SetAlignment(FVector2D(0.f, 0.5f));
				SubmenuSlot->SetOffsets(FMargin(160.f, 0.f, 340.f, 260.f));
			}
			DebugModesSubmenu->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UALSDebugPropMenuWidget::ToggleDebugModesSubmenu()
{
	if (!DebugModesSubmenu)
	{
		return;
	}

	const bool bCurrentlyVisible = DebugModesSubmenu->GetVisibility() != ESlateVisibility::Collapsed;
	DebugModesSubmenu->SetVisibility(bCurrentlyVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}
