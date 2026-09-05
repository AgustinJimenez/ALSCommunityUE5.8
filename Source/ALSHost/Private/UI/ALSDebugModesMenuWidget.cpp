#include "UI/ALSDebugModesMenuWidget.h"

#include "UI/ALSDebugMenuRowWidget.h"
#include "UI/ALSHeldObjectGripTuningWidget.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Character/ALSCharacter.h"

void UALSDebugModesMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!OptionsList || !RowWidgetClass)
	{
		return;
	}

	if (!bRowsPopulated)
	{
		UALSWeaponFireComponent* WeaponFireComponent = nullptr;
		if (AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwningPlayerPawn()))
		{
			WeaponFireComponent = ALSChar->FindComponentByClass<UALSWeaponFireComponent>();
		}

		if (!WeaponFireComponent)
		{
			return;
		}

		AddRow(FText::FromString(TEXT("Toggle Reload Offset Tuning")), [WeaponFireComponent]()
		{
			WeaponFireComponent->ToggleDebugReloadOffsetTuning();
		});

		AddRow(FText::FromString(TEXT("Toggle Reload Anim Loop")), [WeaponFireComponent]()
		{
			WeaponFireComponent->ToggleDebugReloadAnimLoop();
		});

		AddRow(FText::FromString(TEXT("Toggle Reload Freeze")), [WeaponFireComponent]()
		{
			WeaponFireComponent->ToggleDebugReloadFreeze();
		});

		AddRow(FText::FromString(TEXT("Copy Reload Offsets To Clipboard")), [WeaponFireComponent]()
		{
			WeaponFireComponent->DebugCopyReloadOffsetsToClipboard();
		});

		AddRow(FText::FromString(TEXT("Held Object Grip Tuning >")), [this]()
		{
			ToggleGripTuningSubmenu();
		});

		bRowsPopulated = true;
	}

	if (GripTuningWidgetClass && RootCanvas && !GripTuningSubmenu)
	{
		GripTuningSubmenu = CreateWidget<UALSHeldObjectGripTuningWidget>(GetOwningPlayer(), GripTuningWidgetClass);
		if (GripTuningSubmenu)
		{
			// This slot is what actually constrains GripTuningSubmenu's own
			// rendered geometry - a CanvasPanel slot's Offsets are a hard
			// clip on the child widget, regardless of how large that widget's
			// own internal content (PanelBackground, sliders) declares itself
			// to be. Must be at least as big as WBP_HeldObjectGripTuning's
			// own PanelBackground (currently 2900x500) or its content gets
			// squeezed/clipped down to whatever this slot allows - this was
			// the actual cause of "sliders barely moving" even after the
			// widget's own internal sizes were fixed and verified correct.
			if (UCanvasPanelSlot* SubmenuSlot = RootCanvas->AddChildToCanvas(GripTuningSubmenu))
			{
				SubmenuSlot->SetAnchors(FAnchors(0.f, 0.f));
				SubmenuSlot->SetAlignment(FVector2D(0.f, 0.f));
				SubmenuSlot->SetOffsets(FMargin(0.f, 0.f, 680.f, 520.f));
			}
			GripTuningSubmenu->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UALSDebugModesMenuWidget::AddRow(const FText& Label, TFunction<void()> OnClicked)
{
	if (UALSDebugMenuRowWidget* Row = CreateWidget<UALSDebugMenuRowWidget>(GetOwningPlayer(), RowWidgetClass))
	{
		Row->SetRowLabel(Label);
		Row->SetOnClicked(MoveTemp(OnClicked));
		OptionsList->AddChildToVerticalBox(Row);
	}
}

void UALSDebugModesMenuWidget::ToggleGripTuningSubmenu()
{
	if (!GripTuningSubmenu)
	{
		return;
	}

	const bool bCurrentlyVisible = GripTuningSubmenu->GetVisibility() != ESlateVisibility::Collapsed;
	const bool bNowVisible = !bCurrentlyVisible;
	GripTuningSubmenu->SetVisibility(bNowVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// Hide this menu's own row panel (Border + its rows, not just the rows)
	// while grip tuning is open - it's the same slot, no room for both, and
	// the parent (UALSDebugPropMenuWidget) is about to reposition/enlarge
	// this whole submenu into a screen corner. Hiding the Border rather than
	// just OptionsList avoids leaving its dark background visible as an
	// empty panel alongside the grip tuning one.
	if (PanelBackground)
	{
		PanelBackground->SetVisibility(bNowVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (OnGripTuningVisibilityChanged)
	{
		OnGripTuningVisibilityChanged(bNowVisible);
	}
}

void UALSDebugModesMenuWidget::SetOnGripTuningVisibilityChanged(TFunction<void(bool)> InCallback)
{
	OnGripTuningVisibilityChanged = MoveTemp(InCallback);
}
