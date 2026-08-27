#include "Inventory/ALSHealthPickup.h"

#include "Combat/ALSHealthComponent.h"
#include "GameFramework/Pawn.h"

bool AALSHealthPickup::OnPickedUp(APawn* Pawn)
{
	UALSHealthComponent* Health = Pawn->FindComponentByClass<UALSHealthComponent>();
	if (!Health || Health->IsDead() || Health->GetCurrentHealth() >= Health->MaxHealth)
	{
		return false;
	}

	Health->Heal(HealAmount);
	return true;
}
