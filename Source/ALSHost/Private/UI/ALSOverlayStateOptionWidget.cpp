#include "UI/ALSOverlayStateOptionWidget.h"

#include "Character/ALSBaseCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogALSDebugMenu, Log, All);

void UALSOverlayStateOptionWidget::SetOverlayStateOption(EALSOverlayState InState)
{
	State = InState;

	const UEnum* EnumPtr = StaticEnum<EALSOverlayState>();
	SetRowLabel(EnumPtr ? EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(InState)) : FText::FromString(TEXT("Unknown")));

	SetOnClicked([this]()
	{
		if (APawn* Pawn = GetOwningPlayerPawn())
		{
			if (AALSBaseCharacter* ALSChar = Cast<AALSBaseCharacter>(Pawn))
			{
				ALSChar->SetOverlayState(State);
			}
		}
	});
}
