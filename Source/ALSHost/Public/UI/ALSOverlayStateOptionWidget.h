#pragma once

#include "CoreMinimal.h"
#include "UI/ALSDebugMenuRowWidget.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "ALSOverlayStateOptionWidget.generated.h"

// A single row in the replacement debug prop-picker menu (see
// UALSDebugPropMenuWidget) that selects one EALSOverlayState on click.
// Hover/click chrome comes from UALSDebugMenuRowWidget - this class only
// adds the enum binding on top.
UCLASS()
class ALSHOST_API UALSOverlayStateOptionWidget : public UALSDebugMenuRowWidget
{
	GENERATED_BODY()

public:
	// Sets which overlay state this row selects, and updates the label
	// text to the enum's display name. Call right after CreateWidget.
	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void SetOverlayStateOption(EALSOverlayState InState);

private:
	EALSOverlayState State = EALSOverlayState::Default;
};
