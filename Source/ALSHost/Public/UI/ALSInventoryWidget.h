#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSInventoryWidget.generated.h"

class UVerticalBox;
class UALSDebugMenuRowWidget;

// Inventory list, toggled by UALSInventoryComponent's own input binding.
// Finds the UALSInventoryComponent on the owning pawn itself (same pattern
// as UALSStatusBarsWidget/UALSDebugModesMenuWidget) and rebuilds its row
// list on construct and whenever OnInventoryChanged fires. Each row is a
// UALSDebugMenuRowWidget - the same hover-highlight/click-handler row
// already built for the Q debug menu (WBP_DebugMenuRow), reused here rather
// than duplicated; clicking a row equips it if FALSInventoryItem::bEquippable
// is set (e.g. a weapon picked up via AALSWeaponPickup).
UCLASS()
class ALSHOST_API UALSInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Row widget class to spawn per item - must be (or derive from)
	// UALSDebugMenuRowWidget, e.g. WBP_DebugMenuRow.
	UPROPERTY(EditDefaultsOnly, Category = "ALS|Inventory")
	TSubclassOf<UALSDebugMenuRowWidget> RowWidgetClass;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ItemsList;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshItemsList();
	void HandleRowClicked(FName ItemID);
};
