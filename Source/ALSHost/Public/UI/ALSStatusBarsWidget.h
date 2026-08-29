#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ALSStatusBarsWidget.generated.h"

class UProgressBar;
class UTextBlock;

// Player HUD: health + stamina bars. Finds UALSHealthComponent/
// UALSStaminaComponent on the owning pawn itself (same
// find-it-from-the-owning-pawn pattern as UALSDebugModesMenuWidget) rather
// than being handed references, so it works from a plain
// CreateWidget+AddToViewport with no extra wiring.
UCLASS()
class ALSHOST_API UALSStatusBarsWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> StaminaBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StaminaText;

	// "12 / 48" (magazine / reserve), "Reloading...", or hidden entirely
	// while unarmed - see RefreshAmmoText.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmmoText;

	// Hidden (Collapsed) except while UALSMedkitComponent is actively
	// applying a medkit - see HandleMedkitApplyStarted/Progress/Ended.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> MedkitApplyBar;

	// Hidden (Collapsed) except from the moment UALSHealthComponent::OnDeath
	// fires until health next changes off of 0 (i.e. UALSDeathHandlerComponent's
	// respawn actually restores it) - previously death had zero UI feedback
	// at all, see AGENTS.md.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeathText;

	// "Enemies remaining: N" - hidden entirely on a level with no
	// UALSObjectiveSubsystem-tracked enemies (nothing to show).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ObjectiveText;

	// Shown once, permanently, the moment UALSObjectiveSubsystem::OnAllEnemiesDefeated fires.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VictoryText;

private:
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageInstigator);

	UFUNCTION()
	void HandleDeath(AActor* Killer);

	UFUNCTION()
	void HandleEnemyCountChanged(int32 RemainingEnemyCount);

	UFUNCTION()
	void HandleAllEnemiesDefeated();

	UFUNCTION()
	void HandleStaminaChanged(float NewStamina, float MaxStamina);

	// Bound to both UALSWeaponFireComponent::OnAmmoChanged (fire/reload/
	// weapon-switch) and UALSInventoryComponent::OnInventoryChanged (picking
	// up ammo updates the reserve count without touching the weapon
	// component at all) - either one means the displayed numbers are stale.
	UFUNCTION()
	void RefreshAmmoText();

	UFUNCTION()
	void HandleMedkitApplyStarted();

	UFUNCTION()
	void HandleMedkitApplyProgress(float Progress01);

	UFUNCTION()
	void HandleMedkitApplyEnded(bool bCompleted);
};
