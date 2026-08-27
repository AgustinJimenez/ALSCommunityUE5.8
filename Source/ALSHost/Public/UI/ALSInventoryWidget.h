#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSInventoryWidget.generated.h"

class UVerticalBox;

// Read-only inventory list, toggled by UALSInventoryComponent's own input
// binding. Finds the UALSInventoryComponent on the owning pawn itself (same
// pattern as UALSStatusBarsWidget/UALSDebugModesMenuWidget) and rebuilds its
// row list on construct and whenever OnInventoryChanged fires.
UCLASS()
class ALSHOST_API UALSInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ItemsList;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshItemsList();
};
