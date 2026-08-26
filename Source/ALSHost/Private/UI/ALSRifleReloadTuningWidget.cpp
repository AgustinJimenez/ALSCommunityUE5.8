#include "UI/ALSRifleReloadTuningWidget.h"

#include "Weapon/ALSWeaponFireComponent.h"
#include "Components/Slider.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UALSRifleReloadTuningWidget::SetTargetComponent(UALSWeaponFireComponent* InComponent)
{
	TargetComponent = InComponent;
	UE_LOG(LogTemp, Log, TEXT("ALSRifleReloadTuningWidget::SetTargetComponent: InComponent=%s"),
		InComponent ? *InComponent->GetName() : TEXT("null"));
	if (!TargetComponent)
	{
		return;
	}

	const FVector Loc = TargetComponent->DebugGetReloadLocationOffset();
	const FRotator Rot = TargetComponent->DebugGetReloadRotationOffset();
	UE_LOG(LogTemp, Log, TEXT("ALSRifleReloadTuningWidget::SetTargetComponent: Loc=%s Rot=%s Slider_LocX=%s"),
		*Loc.ToString(), *Rot.ToString(), Slider_LocX ? TEXT("bound") : TEXT("NULL - BindWidget failed"));

	if (Slider_LocX) { Slider_LocX->SetValue(Loc.X); }
	if (Slider_LocY) { Slider_LocY->SetValue(Loc.Y); }
	if (Slider_LocZ) { Slider_LocZ->SetValue(Loc.Z); }
	if (Slider_RotPitch) { Slider_RotPitch->SetValue(Rot.Pitch); }
	if (Slider_RotYaw) { Slider_RotYaw->SetValue(Rot.Yaw); }
	if (Slider_RotRoll) { Slider_RotRoll->SetValue(Rot.Roll); }

	RefreshValueLabels();
}

void UALSRifleReloadTuningWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Log, TEXT("ALSRifleReloadTuningWidget::NativeConstruct: Slider_LocX=%s Value_LocX=%s CopyButton=%s TargetComponent=%s"),
		Slider_LocX ? TEXT("bound") : TEXT("NULL"),
		Value_LocX ? TEXT("bound") : TEXT("NULL"),
		CopyButton ? TEXT("bound") : TEXT("NULL"),
		TargetComponent ? *TargetComponent->GetName() : TEXT("null"));

	if (Slider_LocX) { Slider_LocX->OnValueChanged.AddDynamic(this, &UALSRifleReloadTuningWidget::OnLocXChanged); }
	if (Slider_LocY) { Slider_LocY->OnValueChanged.AddDynamic(this, &UALSRifleReloadTuningWidget::OnLocYChanged); }
	if (Slider_LocZ) { Slider_LocZ->OnValueChanged.AddDynamic(this, &UALSRifleReloadTuningWidget::OnLocZChanged); }
	if (Slider_RotPitch) { Slider_RotPitch->OnValueChanged.AddDynamic(this, &UALSRifleReloadTuningWidget::OnRotPitchChanged); }
	if (Slider_RotYaw) { Slider_RotYaw->OnValueChanged.AddDynamic(this, &UALSRifleReloadTuningWidget::OnRotYawChanged); }
	if (Slider_RotRoll) { Slider_RotRoll->OnValueChanged.AddDynamic(this, &UALSRifleReloadTuningWidget::OnRotRollChanged); }
	if (CopyButton) { CopyButton->OnClicked.AddDynamic(this, &UALSRifleReloadTuningWidget::OnCopyClicked); }
	if (FreezeButton) { FreezeButton->OnClicked.AddDynamic(this, &UALSRifleReloadTuningWidget::OnFreezeClicked); }

	RefreshValueLabels();
	RefreshFreezeButtonLabel();
}

void UALSRifleReloadTuningWidget::OnLocXChanged(float NewValue)
{
	UE_LOG(LogTemp, Log, TEXT("ALSRifleReloadTuningWidget::OnLocXChanged: NewValue=%f TargetComponent=%s"),
		NewValue, TargetComponent ? TEXT("valid") : TEXT("NULL"));
	if (!TargetComponent) { return; }
	FVector Loc = TargetComponent->DebugGetReloadLocationOffset();
	Loc.X = NewValue;
	TargetComponent->DebugSetReloadLocationOffset(Loc);
	RefreshValueLabels();
}

void UALSRifleReloadTuningWidget::OnLocYChanged(float NewValue)
{
	if (!TargetComponent) { return; }
	FVector Loc = TargetComponent->DebugGetReloadLocationOffset();
	Loc.Y = NewValue;
	TargetComponent->DebugSetReloadLocationOffset(Loc);
	RefreshValueLabels();
}

void UALSRifleReloadTuningWidget::OnLocZChanged(float NewValue)
{
	if (!TargetComponent) { return; }
	FVector Loc = TargetComponent->DebugGetReloadLocationOffset();
	Loc.Z = NewValue;
	TargetComponent->DebugSetReloadLocationOffset(Loc);
	RefreshValueLabels();
}

void UALSRifleReloadTuningWidget::OnRotPitchChanged(float NewValue)
{
	if (!TargetComponent) { return; }
	FRotator Rot = TargetComponent->DebugGetReloadRotationOffset();
	Rot.Pitch = NewValue;
	TargetComponent->DebugSetReloadRotationOffset(Rot);
	RefreshValueLabels();
}

void UALSRifleReloadTuningWidget::OnRotYawChanged(float NewValue)
{
	if (!TargetComponent) { return; }
	FRotator Rot = TargetComponent->DebugGetReloadRotationOffset();
	Rot.Yaw = NewValue;
	TargetComponent->DebugSetReloadRotationOffset(Rot);
	RefreshValueLabels();
}

void UALSRifleReloadTuningWidget::OnRotRollChanged(float NewValue)
{
	if (!TargetComponent) { return; }
	FRotator Rot = TargetComponent->DebugGetReloadRotationOffset();
	Rot.Roll = NewValue;
	TargetComponent->DebugSetReloadRotationOffset(Rot);
	RefreshValueLabels();
}

void UALSRifleReloadTuningWidget::OnCopyClicked()
{
	if (TargetComponent)
	{
		TargetComponent->DebugCopyReloadOffsetsToClipboard();
	}
}

void UALSRifleReloadTuningWidget::OnFreezeClicked()
{
	if (TargetComponent)
	{
		TargetComponent->ToggleDebugReloadFreeze();
	}
	RefreshFreezeButtonLabel();
}

void UALSRifleReloadTuningWidget::RefreshFreezeButtonLabel()
{
	if (!FreezeButtonLabel)
	{
		return;
	}

	const bool bFrozen = TargetComponent && TargetComponent->IsDebugReloadFrozen();
	FreezeButtonLabel->SetText(FText::FromString(bFrozen ? TEXT("Unfreeze Animation") : TEXT("Freeze Animation")));
}

void UALSRifleReloadTuningWidget::RefreshValueLabels()
{
	if (!TargetComponent)
	{
		return;
	}

	const FVector Loc = TargetComponent->DebugGetReloadLocationOffset();
	const FRotator Rot = TargetComponent->DebugGetReloadRotationOffset();

	const FNumberFormattingOptions FormatOptions = FNumberFormattingOptions()
		.SetMinimumFractionalDigits(1)
		.SetMaximumFractionalDigits(1);

	if (Value_LocX) { Value_LocX->SetText(FText::AsNumber(Loc.X, &FormatOptions)); }
	if (Value_LocY) { Value_LocY->SetText(FText::AsNumber(Loc.Y, &FormatOptions)); }
	if (Value_LocZ) { Value_LocZ->SetText(FText::AsNumber(Loc.Z, &FormatOptions)); }
	if (Value_RotPitch) { Value_RotPitch->SetText(FText::AsNumber(Rot.Pitch, &FormatOptions)); }
	if (Value_RotYaw) { Value_RotYaw->SetText(FText::AsNumber(Rot.Yaw, &FormatOptions)); }
	if (Value_RotRoll) { Value_RotRoll->SetText(FText::AsNumber(Rot.Roll, &FormatOptions)); }
}
