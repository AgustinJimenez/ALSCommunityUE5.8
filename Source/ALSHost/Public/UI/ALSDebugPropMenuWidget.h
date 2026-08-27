#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSDebugPropMenuWidget.generated.h"

class UVerticalBox;
class UCanvasPanel;
class UALSOverlayStateOptionWidget;
class UALSDebugMenuRowWidget;
class UALSDebugModesMenuWidget;

// Replacement for ALS-Community's own OverlayStateSwitcher debug menu
// (never actually functional - its EventGraph never populates the
// OverlayStateButtons array it depends on, and there was no click/hover
// handling implemented anywhere in it - see AGENTS.md). Spawns one
// UALSOverlayStateOptionWidget row per EALSOverlayState enum value on
// construct, each independently clickable/hoverable, plus a trailing
// "Debug Modes" row that toggles a UALSDebugModesMenuWidget submenu
// positioned to the right.
UCLASS()
class ALSHOST_API UALSDebugPropMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> OptionsList;

	// Root canvas - used to host the Debug Modes submenu as a dynamically
	// added, absolutely-positioned child so it can sit to the right of the
	// options list without needing a Designer-authored child (ClaudeUnrealMCP
	// has no tool to nest one Widget Blueprint's instance inside another's
	// static tree - see AGENTS.md).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Debug")
	TSubclassOf<UALSOverlayStateOptionWidget> OptionWidgetClass;

	// Generic clickable row (label + click handler only) used for the
	// trailing "Debug Modes" entry.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Debug")
	TSubclassOf<UALSDebugMenuRowWidget> GenericRowWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Debug")
	TSubclassOf<UALSDebugModesMenuWidget> DebugModesMenuWidgetClass;

private:
	void ToggleDebugModesSubmenu();

	UPROPERTY()
	TObjectPtr<UALSDebugModesMenuWidget> DebugModesSubmenu;
};
