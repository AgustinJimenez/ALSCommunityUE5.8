#include "UI/ALSStatusBarsWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Combat/ALSHealthComponent.h"
#include "Stats/ALSStaminaComponent.h"

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
