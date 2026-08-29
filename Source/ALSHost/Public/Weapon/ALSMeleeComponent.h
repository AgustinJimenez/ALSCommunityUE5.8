#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "ALSMeleeComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class USoundBase;
class UAnimSequenceBase;

// Bare-fist melee is always available regardless of what's equipped
// (unlike UALSWeaponFireComponent, which requires a skeletal-mesh weapon)
// - a short-range sphere sweep from the camera on a cooldown, dealing flat
// damage via ApplyPointDamage same as the ranged weapons. Carrying a knife
// (a plain UALSInventoryComponent item, checked by ItemID - no dedicated
// "equip a melee weapon" visual/overlay-state system exists yet, see
// AGENTS.md) adds a damage bonus on top of the fist value rather than
// replacing the whole mechanic - "stick" was requested too but no stick
// mesh exists anywhere reachable, so it's not implemented.
UCLASS(ClassGroup = (ALSHost), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSMeleeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSMeleeComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee")
	float FistDamage = 15.0f;

	// Added on top of FistDamage while HasKnifeEquipped() is true - not a
	// separate damage value, so tuning FistDamage keeps both proportional.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee")
	float KnifeDamageBonus = 20.0f;

	// Same idea as KnifeDamageBonus but for the axe - a heavier weapon, so a
	// bigger bonus. If both are carried, the axe's bonus wins (see
	// TryMeleeAttack) rather than stacking - it's a strictly better weapon,
	// not a separate damage source.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee")
	float AxeDamageBonus = 35.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee")
	float MeleeRange = 180.0f;

	// Sweep radius, not a thin line trace - a melee swing should be
	// forgiving to aim, not require a pixel-perfect crosshair the way a
	// rifle shot does.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee")
	float SweepRadius = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee")
	float AttackCooldownSeconds = 0.7f;

	// UALSInventoryComponent item ID checked to decide whether the damage
	// bonus applies - matches AALSItemPickup.ItemID on whichever pickup
	// grants the knife (a plain generic item pickup, no dedicated knife
	// pickup class needed).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee")
	FName KnifeItemID = TEXT("Weapon_Knife");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee")
	FName AxeItemID = TEXT("Weapon_Axe");

	// Played via PlaySlotAnimationAsDynamicMontage (same mechanism
	// UALSWeaponFireComponent::Reload() already uses) whenever a swing
	// actually happens - a bare-fist attack previously played nothing at
	// all. WeaponSwingAnimation is used instead of FistSwingAnimation
	// whenever HasAxeEquipped()/HasKnifeEquipped() is true (a weapon swing
	// reads differently from a punch); either can be left unset to just not
	// play anything for that case.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee|Animation")
	TObjectPtr<UAnimSequenceBase> FistSwingAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee|Animation")
	TObjectPtr<UAnimSequenceBase> WeaponSwingAnimation;

	// Same full-body slot Reload() already plays on - a melee swing needs
	// both arms plus the spine just like a two-handed reload does (see
	// AGENTS.md on why "Grounded Slot" was picked for that).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee|Animation")
	FName MeleeMontageSlotName = TEXT("Grounded Slot");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee|Effects")
	TObjectPtr<USoundBase> SwingSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee|Effects")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee|Damage")
	TSubclassOf<class UDamageType> DamageTypeClass;

	// Attempts a melee attack right now - what pressing the input action
	// does, exposed directly so it's testable/callable without needing
	// real input injection. No-op (returns false) if still on cooldown.
	UFUNCTION(BlueprintCallable, Category = "ALS|Melee")
	bool TryMeleeAttack();

	UFUNCTION(BlueprintPure, Category = "ALS|Melee")
	bool HasKnifeEquipped() const;

	UFUNCTION(BlueprintPure, Category = "ALS|Melee")
	bool HasAxeEquipped() const;

	UFUNCTION(BlueprintPure, Category = "ALS|Melee")
	bool IsOnCooldown() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void TrySetupInput();
	void HandleMeleeInput(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee|Input")
	TObjectPtr<UInputAction> MeleeInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Melee|Input")
	TObjectPtr<UInputMappingContext> MeleeInputMappingContext;

private:
	bool bInputBound = false;
	float LastAttackWorldTime = -1000.0f;
};
