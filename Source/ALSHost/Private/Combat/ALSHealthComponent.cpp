#include "Combat/ALSHealthComponent.h"

UALSHealthComponent::UALSHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UALSHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UALSHealthComponent::HandleTakeAnyDamage);
	}
}

void UALSHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsDead || RegenPerSecond <= 0.f || CurrentHealth >= MaxHealth)
	{
		return;
	}

	TimeSinceLastDamage += DeltaTime;
	if (TimeSinceLastDamage >= RegenDelaySeconds)
	{
		ApplyHealthDelta(RegenPerSecond * DeltaTime, nullptr);
	}
}

void UALSHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (bIsDead || Damage <= 0.f)
	{
		return;
	}

	TimeSinceLastDamage = 0.f;
	ApplyHealthDelta(-Damage, DamageCauser);
}

void UALSHealthComponent::Heal(float Amount)
{
	if (bIsDead)
	{
		return;
	}

	ApplyHealthDelta(Amount, nullptr);
}

void UALSHealthComponent::ResetHealth()
{
	bIsDead = false;
	TimeSinceLastDamage = 0.f;
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0.f, nullptr);
}

void UALSHealthComponent::ApplyHealthDelta(float Delta, AActor* Instigator)
{
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Delta, 0.f, MaxHealth);

	if (FMath::IsNearlyEqual(CurrentHealth, PreviousHealth))
	{
		return;
	}

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth - PreviousHealth, Instigator);

	if (CurrentHealth <= 0.f && !bIsDead)
	{
		bIsDead = true;
		OnDeath.Broadcast(Instigator);
	}
}
