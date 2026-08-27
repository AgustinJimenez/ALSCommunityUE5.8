#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ALSStaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FALSOnStaminaChanged, float, NewStamina, float, MaxStamina);

// Drains while the owning ALS character is actually sprinting (EALSGait::Sprinting,
// the *actual* computed gait - already gated by ALS's own CanSprint() rules,
// e.g. aiming), regenerates otherwise. Does not touch ALSBaseCharacter's own
// code: when stamina hits zero it just calls SetDesiredGait(Running) on the
// owner every tick to force the character back out of a sprint, which is
// the same external-gating pattern used elsewhere in this project (see
// AGENTS.md on why UALSWeaponFireComponent binds its own input rather than
// editing the vendored plugin).
UCLASS(ClassGroup = (ALS), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSStaminaComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Stamina")
	float MaxStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Stamina")
	float DrainPerSecondSprinting = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Stamina")
	float RegenPerSecond = 15.f;

	// Regen only kicks in this long after sprinting last stopped, so tapping
	// sprint repeatedly doesn't let stamina regen mid-sprint.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Stamina")
	float RegenDelaySeconds = 1.f;

	UPROPERTY(BlueprintAssignable, Category = "ALS|Stamina")
	FALSOnStaminaChanged OnStaminaChanged;

	UFUNCTION(BlueprintPure, Category = "ALS|Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "ALS|Stamina")
	float GetStaminaPercent() const { return MaxStamina > 0.f ? CurrentStamina / MaxStamina : 0.f; }

	UFUNCTION(BlueprintPure, Category = "ALS|Stamina")
	bool IsExhausted() const { return CurrentStamina <= 0.f; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float CurrentStamina = 0.f;
	float TimeSinceLastSprint = 0.f;

	void SetStamina(float NewValue);
};
