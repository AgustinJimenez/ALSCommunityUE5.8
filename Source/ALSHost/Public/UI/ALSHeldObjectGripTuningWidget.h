#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSHeldObjectGripTuningWidget.generated.h"

class USlider;
class UButton;
class UTextBlock;
class AALSCharacter;

// C++-driven logic for WBP_HeldObjectGripTuning. Reads/writes the same
// ALS.HeldObject.Tune* console variables AALSCharacter::Tick() already
// applies on top of whatever's currently held (see ALSCharacter.cpp) - no
// separate Debug Get/Set plumbing needed on the character, this is purely a
// slider front-end over cvars that already work from the console. Same
// no-Blueprint-graph-authoring-tool reasoning as UALSRifleReloadTuningWidget.
//
// The sliders/labels show and edit the ABSOLUTE grip value (the character's
// baked HeldObjectBase* plus the live tune delta), not the raw tune delta by
// itself - showing a bare delta from an invisible baseline (the original
// design) was confusing: after baking tuned values into the Blueprint pin
// and resetting the tune cvars to 0 (the normal post-tuning workflow), the
// sliders would show 0 even though the actual grip was the tuned pose, with
// no way to see the real current rotation from the UI. Moving a slider now
// computes the tune delta needed to reach the requested absolute value
// (NewValue - CurrentBase) and writes only that delta into the cvar - the
// underlying cvar-based live-tuning mechanism in AALSCharacter is unchanged.
UCLASS()
class ALSHOST_API UALSHeldObjectGripTuningWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_OffsetX;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_OffsetY;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_OffsetZ;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Pitch;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Yaw;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Roll;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_OffsetX;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_OffsetY;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_OffsetZ;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_Pitch;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_Yaw;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_Roll;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CopyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResetButton;

private:
	UFUNCTION()
	void OnOffsetXChanged(float NewValue);

	UFUNCTION()
	void OnOffsetYChanged(float NewValue);

	UFUNCTION()
	void OnOffsetZChanged(float NewValue);

	UFUNCTION()
	void OnPitchChanged(float NewValue);

	UFUNCTION()
	void OnYawChanged(float NewValue);

	UFUNCTION()
	void OnRollChanged(float NewValue);

	UFUNCTION()
	void OnCopyClicked();

	UFUNCTION()
	void OnResetClicked();

	void RefreshValueLabels();
	void RefreshSlidersFromCVars();

	// Resolves the local player's currently-possessed AALSCharacter, or
	// nullptr if none - the character owns the base grip values this widget
	// needs to compute absolute-value sliders.
	AALSCharacter* GetTargetCharacter() const;

	static void SetCVarFloat(const TCHAR* Name, float Value);
	static float GetCVarFloat(const TCHAR* Name);

	// NativeConstruct fires again every time this widget's underlying Slate
	// representation is rebuilt - which happens on every RemoveFromParent +
	// AddToViewport cycle of the *outer* debug menu this is nested in, not
	// just when this UObject itself is first created (the widget instance is
	// cached and persists across Q toggles, but UMG still re-runs
	// NativeConstruct on it each time the outer menu is reopened). Without
	// this guard, OnValueChanged.AddDynamic accumulates a duplicate binding
	// on every reopen, which trips a "delegate already bound" ensure in
	// TMulticastScriptDelegate. Bind delegates exactly once per instance;
	// still refresh the displayed values every NativeConstruct.
	bool bDelegatesBound = false;
};
