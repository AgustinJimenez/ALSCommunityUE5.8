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

private:
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageInstigator);

	UFUNCTION()
	void HandleStaminaChanged(float NewStamina, float MaxStamina);
};
