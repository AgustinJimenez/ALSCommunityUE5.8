#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSInventoryContextMenuWidget.generated.h"

class UVerticalBox;
class UALSDebugMenuRowWidget;

// Small right-click context menu popped up over an inventory row (see
// UALSInventoryWidget) - "Equip" and "Inspect", the latter intentionally a
// no-op for now (not built yet). Same generic row-list shape as
// UALSDebugModesMenuWidget, just with a fixed two-item content list handed
// in via Setup() at popup time rather than reading it from a component.
UCLASS()
class ALSHOST_API UALSInventoryContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// (Re)populates the two rows and wires OnEquip to the "Equip" row.
	// "Inspect" is intentionally wired to nothing yet.
	void Setup(TFunction<void()> OnEquip);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> OptionsList;

	UPROPERTY(EditDefaultsOnly, Category = "ALS|Inventory")
	TSubclassOf<UALSDebugMenuRowWidget> RowWidgetClass;

private:
	void AddRow(const FText& Label, TFunction<void()> OnClicked);
};
