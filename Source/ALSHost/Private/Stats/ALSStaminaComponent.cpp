#include "Stats/ALSStaminaComponent.h"

#include "Character/ALSBaseCharacter.h"
#include "Library/ALSCharacterEnumLibrary.h"

UALSStaminaComponent::UALSStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UALSStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentStamina = MaxStamina;
}

void UALSStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AALSBaseCharacter* ALSChar = Cast<AALSBaseCharacter>(GetOwner());
	if (!ALSChar)
	{
		return;
	}

	const bool bIsSprinting = ALSChar->GetGait() == EALSGait::Sprinting;

	if (bIsSprinting)
	{
		TimeSinceLastSprint = 0.f;
		SetStamina(CurrentStamina - DrainPerSecondSprinting * DeltaTime);

		if (CurrentStamina <= 0.f)
		{
			ALSChar->SetDesiredGait(EALSGait::Running);
		}
	}
	else
	{
		TimeSinceLastSprint += DeltaTime;
		if (TimeSinceLastSprint >= RegenDelaySeconds && CurrentStamina < MaxStamina)
		{
			SetStamina(CurrentStamina + RegenPerSecond * DeltaTime);
		}
	}
}

void UALSStaminaComponent::SetStamina(float NewValue)
{
	const float Clamped = FMath::Clamp(NewValue, 0.f, MaxStamina);
	if (FMath::IsNearlyEqual(Clamped, CurrentStamina))
	{
		return;
	}

	CurrentStamina = Clamped;
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}
