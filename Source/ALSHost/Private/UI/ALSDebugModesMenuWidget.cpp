#include "UI/ALSDebugModesMenuWidget.h"

#include "UI/ALSDebugMenuRowWidget.h"
#include "Components/VerticalBox.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Character/ALSCharacter.h"

void UALSDebugModesMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!OptionsList || !RowWidgetClass)
	{
		return;
	}

	OptionsList->ClearChildren();

	UALSWeaponFireComponent* WeaponFireComponent = nullptr;
	if (AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwningPlayerPawn()))
	{
		WeaponFireComponent = ALSChar->FindComponentByClass<UALSWeaponFireComponent>();
	}

	if (!WeaponFireComponent)
	{
		return;
	}

	AddRow(FText::FromString(TEXT("Toggle Reload Offset Tuning")), [WeaponFireComponent]()
	{
		WeaponFireComponent->ToggleDebugReloadOffsetTuning();
	});

	AddRow(FText::FromString(TEXT("Toggle Reload Anim Loop")), [WeaponFireComponent]()
	{
		WeaponFireComponent->ToggleDebugReloadAnimLoop();
	});

	AddRow(FText::FromString(TEXT("Toggle Reload Freeze")), [WeaponFireComponent]()
	{
		WeaponFireComponent->ToggleDebugReloadFreeze();
	});

	AddRow(FText::FromString(TEXT("Copy Reload Offsets To Clipboard")), [WeaponFireComponent]()
	{
		WeaponFireComponent->DebugCopyReloadOffsetsToClipboard();
	});
}

void UALSDebugModesMenuWidget::AddRow(const FText& Label, TFunction<void()> OnClicked)
{
	if (UALSDebugMenuRowWidget* Row = CreateWidget<UALSDebugMenuRowWidget>(GetOwningPlayer(), RowWidgetClass))
	{
		Row->SetRowLabel(Label);
		Row->SetOnClicked(MoveTemp(OnClicked));
		OptionsList->AddChildToVerticalBox(Row);
	}
}
