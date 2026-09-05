#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSDebugModesMenuWidget.generated.h"

class UVerticalBox;
class UCanvasPanel;
class UBorder;
class UALSDebugMenuRowWidget;
class UALSHeldObjectGripTuningWidget;

// Submenu opened from the "Debug Modes" row in UALSDebugPropMenuWidget.
// Lists the existing reload-tuning debug actions on UALSWeaponFireComponent
// (previously only reachable via the tap/hold "T" gesture) as clickable
// rows, so they don't need a separate keybind to remember. Extend this list
// as more debug modes show up - see AGENTS.md. Also hosts a further nested
// "Held Object Grip Tuning" submenu, same dynamic-CreateWidget-and-anchor
// pattern UALSDebugPropMenuWidget already uses for this widget itself.
UCLASS()
class ALSHOST_API UALSDebugModesMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> OptionsList;

	// The Border wrapping OptionsList - hiding just OptionsList (a VerticalBox)
	// leaves this Border's own dark background visible as an empty rectangle
	// once Held Object Grip Tuning opens (its rows collapse, but the panel
	// behind them doesn't), which read as "two panels" to the user. Toggle
	// this instead of/alongside OptionsList so the whole empty panel goes
	// away, not just its contents.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> PanelBackground;

	// Root canvas - hosts the Held Object Grip Tuning submenu as a
	// dynamically added, absolutely-positioned child, same reasoning as
	// UALSDebugPropMenuWidget's own RootCanvas.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Debug")
	TSubclassOf<UALSDebugMenuRowWidget> RowWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Debug")
	TSubclassOf<UALSHeldObjectGripTuningWidget> GripTuningWidgetClass;

public:
	// Lets the owning UALSDebugPropMenuWidget hide its own row list and
	// reposition this whole submenu to a screen corner while grip tuning is
	// open, and restore both when it closes - see
	// UALSDebugPropMenuWidget::HandleGripTuningVisibilityChanged.
	void SetOnGripTuningVisibilityChanged(TFunction<void(bool)> InCallback);

private:
	void AddRow(const FText& Label, TFunction<void()> OnClicked);
	void ToggleGripTuningSubmenu();

	UPROPERTY()
	TObjectPtr<UALSHeldObjectGripTuningWidget> GripTuningSubmenu;

	TFunction<void(bool)> OnGripTuningVisibilityChanged;

	// See UALSDebugPropMenuWidget::bRowsPopulated - same reasoning: guards
	// against rebuilding this menu's row list from scratch on every Q
	// toggle (NativeConstruct re-fires each time due to the outer widget's
	// RemoveFromParent/AddToViewport cycle).
	bool bRowsPopulated = false;
};
