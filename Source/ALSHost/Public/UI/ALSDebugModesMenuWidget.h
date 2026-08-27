#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSDebugModesMenuWidget.generated.h"

class UVerticalBox;
class UALSDebugMenuRowWidget;

// Submenu opened from the "Debug Modes" row in UALSDebugPropMenuWidget.
// Lists the existing reload-tuning debug actions on UALSWeaponFireComponent
// (previously only reachable via the tap/hold "T" gesture) as clickable
// rows, so they don't need a separate keybind to remember. Extend this list
// as more debug modes show up - see AGENTS.md.
UCLASS()
class ALSHOST_API UALSDebugModesMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> OptionsList;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Debug")
	TSubclassOf<UALSDebugMenuRowWidget> RowWidgetClass;

private:
	void AddRow(const FText& Label, TFunction<void()> OnClicked);
};
