#include "UI/ALSDebugMenuRowWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor NormalColor(0.05f, 0.05f, 0.05f, 0.85f);
	const FLinearColor HoverColor(0.2f, 0.45f, 0.75f, 0.9f);
}

void UALSDebugMenuRowWidget::SetRowLabel(const FText& InText)
{
	if (Label)
	{
		Label->SetText(InText);
	}
}

void UALSDebugMenuRowWidget::SetOnClicked(TFunction<void()> InHandler)
{
	ClickHandler = MoveTemp(InHandler);
}

void UALSDebugMenuRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Background)
	{
		Background->SetBrushColor(NormalColor);
	}
}

FReply UALSDebugMenuRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (ClickHandler)
		{
			ClickHandler();
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UALSDebugMenuRowWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (Background)
	{
		Background->SetBrushColor(HoverColor);
	}
}

void UALSDebugMenuRowWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (Background)
	{
		Background->SetBrushColor(NormalColor);
	}
}
