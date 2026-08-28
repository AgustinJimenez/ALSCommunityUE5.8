#include "UI/ALSStatusBarsWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Combat/ALSHealthComponent.h"
#include "Stats/ALSStaminaComponent.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Inventory/ALSInventoryComponent.h"

void UALSStatusBarsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn)
	{
		return;
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

	RefreshAmmoText();
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
