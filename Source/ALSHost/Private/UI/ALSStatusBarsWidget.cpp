#include "UI/ALSStatusBarsWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Combat/ALSHealthComponent.h"
#include "Stats/ALSStaminaComponent.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Combat/ALSMedkitComponent.h"
#include "Interaction/ALSInteractionComponent.h"
#include "GameFramework/PlayerController.h"

void UALSStatusBarsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn)
	{
		return;
	}

	if (MedkitApplyBar)
	{
		MedkitApplyBar->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (InteractPromptText)
	{
		InteractPromptText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UALSMedkitComponent* Medkit = Pawn->FindComponentByClass<UALSMedkitComponent>())
	{
		Medkit->OnMedkitApplyStarted.AddDynamic(this, &UALSStatusBarsWidget::HandleMedkitApplyStarted);
		Medkit->OnMedkitApplyProgress.AddDynamic(this, &UALSStatusBarsWidget::HandleMedkitApplyProgress);
		Medkit->OnMedkitApplyEnded.AddDynamic(this, &UALSStatusBarsWidget::HandleMedkitApplyEnded);
	}

	if (UALSHealthComponent* Health = Pawn->FindComponentByClass<UALSHealthComponent>())
	{
		Health->OnHealthChanged.AddDynamic(this, &UALSStatusBarsWidget::HandleHealthChanged);
		HandleHealthChanged(Health->GetCurrentHealth(), Health->MaxHealth, 0.f, nullptr);
	}

	if (UALSStaminaComponent* Stamina = Pawn->FindComponentByClass<UALSStaminaComponent>())
	{
		Stamina->OnStaminaChanged.AddDynamic(this, &UALSStatusBarsWidget::HandleStaminaChanged);
		HandleStaminaChanged(Stamina->GetCurrentStamina(), Stamina->MaxStamina);
	}

	if (UALSWeaponFireComponent* Weapon = Pawn->FindComponentByClass<UALSWeaponFireComponent>())
	{
		Weapon->OnAmmoChanged.AddDynamic(this, &UALSStatusBarsWidget::RefreshAmmoText);
	}

	if (UALSInventoryComponent* Inventory = Pawn->FindComponentByClass<UALSInventoryComponent>())
	{
		Inventory->OnInventoryChanged.AddDynamic(this, &UALSStatusBarsWidget::RefreshAmmoText);
	}

	if (UALSInteractionComponent* Interaction = Pawn->FindComponentByClass<UALSInteractionComponent>())
	{
		Interaction->OnInteractableChanged.AddDynamic(this, &UALSStatusBarsWidget::HandleInteractableChanged);
	}

	RefreshAmmoText();
}

void UALSStatusBarsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!InteractPromptText || !InteractTarget)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	// Float the label a little above the object's own origin, not right on
	// top of it - most interactable pivots sit at or near the ground.
	FVector2D ScreenPosition;
	const bool bOnScreen = PC->ProjectWorldLocationToScreen(InteractTarget->GetActorLocation() + FVector(0.f, 0.f, 80.f), ScreenPosition);
	if (!bOnScreen)
	{
		InteractPromptText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	InteractPromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
	InteractPromptText->SetRenderTranslation(ScreenPosition);
}

void UALSStatusBarsWidget::HandleHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageInstigator)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(MaxHealth > 0.f ? NewHealth / MaxHealth : 0.f);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(NewHealth), FMath::RoundToInt(MaxHealth))));
	}
}

void UALSStatusBarsWidget::HandleStaminaChanged(float NewStamina, float MaxStamina)
{
	if (StaminaBar)
	{
		StaminaBar->SetPercent(MaxStamina > 0.f ? NewStamina / MaxStamina : 0.f);
	}

	if (StaminaText)
	{
		StaminaText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(NewStamina), FMath::RoundToInt(MaxStamina))));
	}
}

void UALSStatusBarsWidget::RefreshAmmoText()
{
	if (!AmmoText)
	{
		return;
	}

	APawn* Pawn = GetOwningPlayerPawn();
	UALSWeaponFireComponent* Weapon = Pawn ? Pawn->FindComponentByClass<UALSWeaponFireComponent>() : nullptr;
	if (!Weapon || !Weapon->HasWeaponEquipped())
	{
		AmmoText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	AmmoText->SetVisibility(ESlateVisibility::Visible);

	if (Weapon->IsReloading())
	{
		AmmoText->SetText(FText::FromString(TEXT("Reloading...")));
		return;
	}

	AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Weapon->GetCurrentAmmoInMagazine(), Weapon->GetCurrentReserveAmmo())));
}

void UALSStatusBarsWidget::HandleMedkitApplyStarted()
{
	if (MedkitApplyBar)
	{
		MedkitApplyBar->SetVisibility(ESlateVisibility::Visible);
		MedkitApplyBar->SetPercent(0.0f);
	}
}

void UALSStatusBarsWidget::HandleMedkitApplyProgress(float Progress01)
{
	if (MedkitApplyBar)
	{
		MedkitApplyBar->SetPercent(Progress01);
	}
}

void UALSStatusBarsWidget::HandleMedkitApplyEnded(bool bCompleted)
{
	if (MedkitApplyBar)
	{
		MedkitApplyBar->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UALSStatusBarsWidget::HandleInteractableChanged(AActor* NewInteractable, FText Prompt)
{
	InteractTarget = NewInteractable;

	if (!InteractPromptText)
	{
		return;
	}

	if (!NewInteractable)
	{
		InteractPromptText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	InteractPromptText->SetText(FText::Format(NSLOCTEXT("ALSHost", "InteractPrompt", "[E] {0}"), Prompt));
}
