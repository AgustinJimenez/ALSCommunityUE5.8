#include "UI/ALSHeldObjectGripTuningWidget.h"

#include "Components/Slider.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Character/ALSCharacter.h"
#include "Kismet/GameplayStatics.h"

void UALSHeldObjectGripTuningWidget::SetCVarFloat(const TCHAR* Name, float Value)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		CVar->Set(Value, ECVF_SetByConsole);
	}
}

float UALSHeldObjectGripTuningWidget::GetCVarFloat(const TCHAR* Name)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		return CVar->GetFloat();
	}
	return 0.0f;
}

AALSCharacter* UALSHeldObjectGripTuningWidget::GetTargetCharacter() const
{
	return Cast<AALSCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void UALSHeldObjectGripTuningWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!bDelegatesBound)
	{
		if (Slider_OffsetX) { Slider_OffsetX->OnValueChanged.AddDynamic(this, &UALSHeldObjectGripTuningWidget::OnOffsetXChanged); }
		if (Slider_OffsetY) { Slider_OffsetY->OnValueChanged.AddDynamic(this, &UALSHeldObjectGripTuningWidget::OnOffsetYChanged); }
		if (Slider_OffsetZ) { Slider_OffsetZ->OnValueChanged.AddDynamic(this, &UALSHeldObjectGripTuningWidget::OnOffsetZChanged); }
		if (Slider_Pitch) { Slider_Pitch->OnValueChanged.AddDynamic(this, &UALSHeldObjectGripTuningWidget::OnPitchChanged); }
		if (Slider_Yaw) { Slider_Yaw->OnValueChanged.AddDynamic(this, &UALSHeldObjectGripTuningWidget::OnYawChanged); }
		if (Slider_Roll) { Slider_Roll->OnValueChanged.AddDynamic(this, &UALSHeldObjectGripTuningWidget::OnRollChanged); }
		if (CopyButton) { CopyButton->OnClicked.AddDynamic(this, &UALSHeldObjectGripTuningWidget::OnCopyClicked); }
		if (ResetButton) { ResetButton->OnClicked.AddDynamic(this, &UALSHeldObjectGripTuningWidget::OnResetClicked); }
		bDelegatesBound = true;
	}

	RefreshSlidersFromCVars();
	RefreshValueLabels();
}

void UALSHeldObjectGripTuningWidget::OnOffsetXChanged(float NewValue)
{
	const float Base = GetTargetCharacter() ? GetTargetCharacter()->GetHeldObjectBaseOffset().X : 0.0f;
	SetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetX"), NewValue - Base);
	RefreshValueLabels();
}

void UALSHeldObjectGripTuningWidget::OnOffsetYChanged(float NewValue)
{
	const float Base = GetTargetCharacter() ? GetTargetCharacter()->GetHeldObjectBaseOffset().Y : 0.0f;
	SetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetY"), NewValue - Base);
	RefreshValueLabels();
}

void UALSHeldObjectGripTuningWidget::OnOffsetZChanged(float NewValue)
{
	const float Base = GetTargetCharacter() ? GetTargetCharacter()->GetHeldObjectBaseOffset().Z : 0.0f;
	SetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetZ"), NewValue - Base);
	RefreshValueLabels();
}

void UALSHeldObjectGripTuningWidget::OnPitchChanged(float NewValue)
{
	const float Base = GetTargetCharacter() ? GetTargetCharacter()->GetHeldObjectBaseRotationOffset().Pitch : 0.0f;
	SetCVarFloat(TEXT("ALS.HeldObject.TunePitchOffset"), NewValue - Base);
	RefreshValueLabels();
}

void UALSHeldObjectGripTuningWidget::OnYawChanged(float NewValue)
{
	const float Base = GetTargetCharacter() ? GetTargetCharacter()->GetHeldObjectBaseRotationOffset().Yaw : 0.0f;
	SetCVarFloat(TEXT("ALS.HeldObject.TuneYawOffset"), NewValue - Base);
	RefreshValueLabels();
}

void UALSHeldObjectGripTuningWidget::OnRollChanged(float NewValue)
{
	const float Base = GetTargetCharacter() ? GetTargetCharacter()->GetHeldObjectBaseRotationOffset().Roll : 0.0f;
	SetCVarFloat(TEXT("ALS.HeldObject.TuneRollOffset"), NewValue - Base);
	RefreshValueLabels();
}

void UALSHeldObjectGripTuningWidget::OnCopyClicked()
{
	const AALSCharacter* Character = GetTargetCharacter();
	const FVector BaseOffset = Character ? Character->GetHeldObjectBaseOffset() : FVector::ZeroVector;
	const FRotator BaseRotation = Character ? Character->GetHeldObjectBaseRotationOffset() : FRotator::ZeroRotator;

	const FString Text = FString::Printf(TEXT("Offset=(X=%.2f,Y=%.2f,Z=%.2f) RotationOffset=(Pitch=%.2f,Yaw=%.2f,Roll=%.2f)"),
		BaseOffset.X + GetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetX")),
		BaseOffset.Y + GetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetY")),
		BaseOffset.Z + GetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetZ")),
		BaseRotation.Pitch + GetCVarFloat(TEXT("ALS.HeldObject.TunePitchOffset")),
		BaseRotation.Yaw + GetCVarFloat(TEXT("ALS.HeldObject.TuneYawOffset")),
		BaseRotation.Roll + GetCVarFloat(TEXT("ALS.HeldObject.TuneRollOffset")));
	FPlatformApplicationMisc::ClipboardCopy(*Text);
}

void UALSHeldObjectGripTuningWidget::OnResetClicked()
{
	SetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetX"), 0.0f);
	SetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetY"), 0.0f);
	SetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetZ"), 0.0f);
	SetCVarFloat(TEXT("ALS.HeldObject.TunePitchOffset"), 0.0f);
	SetCVarFloat(TEXT("ALS.HeldObject.TuneYawOffset"), 0.0f);
	SetCVarFloat(TEXT("ALS.HeldObject.TuneRollOffset"), 0.0f);
	RefreshSlidersFromCVars();
	RefreshValueLabels();
}

void UALSHeldObjectGripTuningWidget::RefreshSlidersFromCVars()
{
	const AALSCharacter* Character = GetTargetCharacter();
	const FVector BaseOffset = Character ? Character->GetHeldObjectBaseOffset() : FVector::ZeroVector;
	const FRotator BaseRotation = Character ? Character->GetHeldObjectBaseRotationOffset() : FRotator::ZeroRotator;

	if (Slider_OffsetX) { Slider_OffsetX->SetValue(BaseOffset.X + GetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetX"))); }
	if (Slider_OffsetY) { Slider_OffsetY->SetValue(BaseOffset.Y + GetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetY"))); }
	if (Slider_OffsetZ) { Slider_OffsetZ->SetValue(BaseOffset.Z + GetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetZ"))); }
	if (Slider_Pitch) { Slider_Pitch->SetValue(BaseRotation.Pitch + GetCVarFloat(TEXT("ALS.HeldObject.TunePitchOffset"))); }
	if (Slider_Yaw) { Slider_Yaw->SetValue(BaseRotation.Yaw + GetCVarFloat(TEXT("ALS.HeldObject.TuneYawOffset"))); }
	if (Slider_Roll) { Slider_Roll->SetValue(BaseRotation.Roll + GetCVarFloat(TEXT("ALS.HeldObject.TuneRollOffset"))); }
}

void UALSHeldObjectGripTuningWidget::RefreshValueLabels()
{
	const AALSCharacter* Character = GetTargetCharacter();
	const FVector BaseOffset = Character ? Character->GetHeldObjectBaseOffset() : FVector::ZeroVector;
	const FRotator BaseRotation = Character ? Character->GetHeldObjectBaseRotationOffset() : FRotator::ZeroRotator;

	const FNumberFormattingOptions FormatOptions = FNumberFormattingOptions()
		.SetMinimumFractionalDigits(1)
		.SetMaximumFractionalDigits(1);

	if (Value_OffsetX) { Value_OffsetX->SetText(FText::AsNumber(BaseOffset.X + GetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetX")), &FormatOptions)); }
	if (Value_OffsetY) { Value_OffsetY->SetText(FText::AsNumber(BaseOffset.Y + GetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetY")), &FormatOptions)); }
	if (Value_OffsetZ) { Value_OffsetZ->SetText(FText::AsNumber(BaseOffset.Z + GetCVarFloat(TEXT("ALS.HeldObject.TuneOffsetZ")), &FormatOptions)); }
	if (Value_Pitch) { Value_Pitch->SetText(FText::AsNumber(BaseRotation.Pitch + GetCVarFloat(TEXT("ALS.HeldObject.TunePitchOffset")), &FormatOptions)); }
	if (Value_Yaw) { Value_Yaw->SetText(FText::AsNumber(BaseRotation.Yaw + GetCVarFloat(TEXT("ALS.HeldObject.TuneYawOffset")), &FormatOptions)); }
	if (Value_Roll) { Value_Roll->SetText(FText::AsNumber(BaseRotation.Roll + GetCVarFloat(TEXT("ALS.HeldObject.TuneRollOffset")), &FormatOptions)); }
}
