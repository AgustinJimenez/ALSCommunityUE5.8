#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSInventoryWidget.generated.h"

class UVerticalBox;
class UCanvasPanel;
class UALSDebugMenuRowWidget;
class UALSInventoryContextMenuWidget;

// Inventory list, toggled by UALSInventoryComponent's own input binding.
// Finds the UALSInventoryComponent on the owning pawn itself (same pattern
// as UALSStatusBarsWidget/UALSDebugModesMenuWidget) and rebuilds its row
// list on construct and whenever OnInventoryChanged fires. Each row is a
// UALSDebugMenuRowWidget - the same hover-highlight/click-handler row
// already built for the Q debug menu (WBP_DebugMenuRow), reused here rather
// than duplicated. Left-click only highlights (UALSDebugMenuRowWidget's own
// hover chrome) - right-clicking an equippable row pops up a small
// Equip/Inspect context menu (UALSInventoryContextMenuWidget) positioned at
// the cursor; Inspect is intentionally not implemented yet.
UCLASS()
class ALSHOST_API UALSInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Row widget class to spawn per item - must be (or derive from)
	// UALSDebugMenuRowWidget, e.g. WBP_DebugMenuRow.
	UPROPERTY(EditDefaultsOnly, Category = "ALS|Inventory")
	TSubclassOf<UALSDebugMenuRowWidget> RowWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "ALS|Inventory")
	TSubclassOf<UALSInventoryContextMenuWidget> ContextMenuWidgetClass;

	// Equips ItemID if it's a known, bEquippable item in the owning pawn's
	// inventory - what the context menu's "Equip" row actually calls.
	// Public and BlueprintCallable so it's directly testable without needing
	// to simulate a real mouse click through Slate hit-testing. Returns
	// false if ItemID isn't found or isn't equippable.
	UFUNCTION(BlueprintCallable, Category = "ALS|Inventory")
	bool EquipItem(FName ItemID);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ItemsList;

	// Root canvas - hosts the context menu as a dynamically added,
	// absolutely-positioned child, same pattern as
	// UALSDebugPropMenuWidget's Debug Modes submenu (see AGENTS.md - no MCP
	// tool to nest one Widget Blueprint's instance inside another's static
	// tree at design time).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvas;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshItemsList();
	void ShowContextMenu(FName ItemID, const FVector2D& ScreenPosition);

	UPROPERTY()
	TObjectPtr<UALSInventoryContextMenuWidget> ContextMenu;
};
