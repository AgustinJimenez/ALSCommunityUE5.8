#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSRifleReloadTuningWidget.generated.h"

class USlider;
class UButton;
class UTextBlock;
class UALSWeaponFireComponent;

// C++-driven logic for WBP_RifleReloadTuning. The MCP tooling used to build
// this project's UI can create/lay out widgets but has no generic
// Blueprint-graph node-authoring tool (no add-arbitrary-CallFunction-node,
// no bind-widget-delegate), so rather than build that out, the slider/button
// wiring lives here in native code and the WBP (reparented to this class)
// just needs its widget names to match the BindWidget properties below.
UCLASS()
class ALSHOST_API UALSRifleReloadTuningWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Call right after CreateWidget, before or after AddToViewport - reads
	// the component's current Rifle reload offsets into the sliders.
	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Debug")
	void SetTargetComponent(UALSWeaponFireComponent* InComponent);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_LocX;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_LocY;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_LocZ;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_RotPitch;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_RotYaw;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_RotRoll;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_LocX;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_LocY;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_LocZ;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_RotPitch;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_RotYaw;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_RotRoll;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CopyButton;

	// Optional: the WBP content-side add_widget step for this button runs
	// separately from this C++ change, so mark it optional to avoid a
	// hard Blueprint-compile error (a plain BindWidget on a not-yet-added
	// widget name fails compilation) in the window between rebuilding this
	// class and actually adding the widget in the editor.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> FreezeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FreezeButtonLabel;

private:
	UFUNCTION()
	void OnLocXChanged(float NewValue);

	UFUNCTION()
	void OnLocYChanged(float NewValue);

	UFUNCTION()
	void OnLocZChanged(float NewValue);

	UFUNCTION()
	void OnRotPitchChanged(float NewValue);

	UFUNCTION()
	void OnRotYawChanged(float NewValue);

	UFUNCTION()
	void OnRotRollChanged(float NewValue);

	UFUNCTION()
	void OnCopyClicked();

	UFUNCTION()
	void OnFreezeClicked();

	void RefreshValueLabels();
	void RefreshFreezeButtonLabel();

	UPROPERTY()
	TObjectPtr<UALSWeaponFireComponent> TargetComponent;
};
