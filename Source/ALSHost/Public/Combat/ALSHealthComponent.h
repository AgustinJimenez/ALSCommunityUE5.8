#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ALSHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FALSOnHealthChanged, float, NewHealth, float, MaxHealth, float, Delta, AActor*, DamageInstigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FALSOnDeath, AActor*, Killer);

// Generic damage/health tracking component, attachable to the player or any
// enemy character. Hooks the owning actor's own AActor::OnTakeAnyDamage
// delegate rather than requiring callers to call a bespoke function, so it
// works with anything already routed through UGameplayStatics::ApplyDamage /
// ApplyPointDamage / ApplyRadialDamage (including engine and third-party
// systems), not just this project's own weapon code.
UCLASS(ClassGroup = (ALS), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSHealthComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Health")
	float MaxHealth = 100.f;

	// Regenerates slowly when above 0 and not in combat (no damage taken for
	// RegenDelaySeconds). Set RegenPerSecond to 0 to disable entirely.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Health")
	float RegenPerSecond = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Health", meta = (EditCondition = "RegenPerSecond > 0"))
	float RegenDelaySeconds = 5.f;

	UPROPERTY(BlueprintAssignable, Category = "ALS|Health")
	FALSOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "ALS|Health")
	FALSOnDeath OnDeath;

	UFUNCTION(BlueprintPure, Category = "ALS|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "ALS|Health")
	float GetHealthPercent() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }

	UFUNCTION(BlueprintPure, Category = "ALS|Health")
	bool IsDead() const { return bIsDead; }

	// Applies healing (positive) or, if callers prefer this over the engine
	// damage system, direct damage (negative). Clamped to [0, MaxHealth].
	UFUNCTION(BlueprintCallable, Category = "ALS|Health")
	void Heal(float Amount);

	// Resets to full health and clears the dead flag - for a respawn/reset
	// flow. Does not undo any gameplay side effects death already caused.
	UFUNCTION(BlueprintCallable, Category = "ALS|Health")
	void ResetHealth();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

private:
	UPROPERTY(VisibleInstanceOnly, Category = "ALS|Health")
	float CurrentHealth = 0.f;

	bool bIsDead = false;
	float TimeSinceLastDamage = 0.f;

	void ApplyHealthDelta(float Delta, AActor* Instigator);
};
