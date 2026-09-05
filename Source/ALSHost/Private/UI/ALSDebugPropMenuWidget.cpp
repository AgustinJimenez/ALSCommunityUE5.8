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

	if (!bRowsPopulated)
	{
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

		bRowsPopulated = true;
	}

	if (DebugModesMenuWidgetClass && RootCanvas && !DebugModesSubmenu)
	{
		DebugModesSubmenu = CreateWidget<UALSDebugModesMenuWidget>(GetOwningPlayer(), DebugModesMenuWidgetClass);
		if (DebugModesSubmenu)
		{
			DebugModesSubmenuSlot = RootCanvas->AddChildToCanvas(DebugModesSubmenu);
			if (DebugModesSubmenuSlot)
			{
				DebugModesSubmenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				DebugModesSubmenuSlot->SetAlignment(FVector2D(0.f, 0.5f));
				DebugModesSubmenuSlot->SetOffsets(FMargin(160.f, 0.f, 340.f, 260.f));
			}
			DebugModesSubmenu->SetVisibility(ESlateVisibility::Collapsed);
			DebugModesSubmenu->SetOnGripTuningVisibilityChanged([this](bool bOpen) { HandleGripTuningVisibilityChanged(bOpen); });
		}
	}
}

void UALSDebugPropMenuWidget::HandleGripTuningVisibilityChanged(bool bGripTuningOpen)
{
	if (OptionsList)
	{
		OptionsList->SetVisibility(bGripTuningOpen ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (!DebugModesSubmenuSlot)
	{
		return;
	}

	if (bGripTuningOpen)
	{
		// Top-left screen corner - the Grip Tuning panel is the only thing
		// left visible in this whole nested menu at this point (both this
		// menu's own row list and DebugModesSubmenu's row list are hidden),
		// so it gets the corner to itself rather than fighting for the
		// center-right spot the collapsed menus normally share.
		DebugModesSubmenuSlot->SetAnchors(FAnchors(0.f, 0.f));
		DebugModesSubmenuSlot->SetAlignment(FVector2D(0.f, 0.f));
		DebugModesSubmenuSlot->SetOffsets(FMargin(20.f, 20.f, 700.f, 540.f));
	}
	else
	{
		DebugModesSubmenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		DebugModesSubmenuSlot->SetAlignment(FVector2D(0.f, 0.5f));
		DebugModesSubmenuSlot->SetOffsets(FMargin(160.f, 0.f, 340.f, 260.f));
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
