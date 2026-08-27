#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSDebugMenuRowWidget.generated.h"

class UBorder;
class UTextBlock;

// Common base for a single clickable/hoverable row in the debug menu system
// (see UALSDebugPropMenuWidget, UALSDebugModesMenuWidget). Handles label
// text, hover-highlight, and dispatching a click to whatever handler the
// owner bound via SetOnClicked - subclasses (or the owner, for generic rows)
// decide what a click actually does. Native C++ mouse handling rather than a
// Blueprint Button/OnClicked graph - see AGENTS.md for why (no generic
// Blueprint-graph node-authoring tool in ClaudeUnrealMCP).
UCLASS()
class ALSHOST_API UALSDebugMenuRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void SetRowLabel(const FText& InText);

	// Stores a handler run on left-click. Not a UPROPERTY/delegate exposed to
	// Blueprint - rows are only ever constructed and wired from C++.
	void SetOnClicked(TFunction<void()> InHandler);

	// Stores a handler run on right-click, receiving the click's absolute
	// screen-space position - callers that pop up a context menu need this
	// to position it. Optional; a row with no right-click handler set just
	// ignores right-clicks (falls through to Super, no context menu).
	void SetOnRightClicked(TFunction<void(const FVector2D&)> InHandler);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Background;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Label;

private:
	TFunction<void()> ClickHandler;
	TFunction<void(const FVector2D&)> RightClickHandler;
};
