#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "ALSWeaponFireComponent.generated.h"

class UInputAction;
class UInputMappingContext;

// Per-weapon-type ammo capacity/reload time, keyed by the character's
// EALSOverlayState (the same enum ALS_CharacterBP's OnUpdateHeldObject
// already switches on to pick which mesh to attach). Only Rifle actually
// has a working Muzzle socket right now (see AGENTS.md), but keying this
// by overlay state now means adding a second working weapon later is just
// adding a map entry and a socket, not a rework.
USTRUCT(BlueprintType)
struct FALSWeaponAmmoStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 MagazineSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1"))
	float ReloadSeconds = 2.0f;
};

// Hitscan weapon firing. Binds its own Enhanced Input action directly
// (rather than routing through ALSPlayerController's name-based action
// dispatch) so this stays entirely in ALSHost's own module - no edits
// needed to the vendored ALS-Community-UE5 plugin.
UCLASS(ClassGroup = (ALSHost), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSWeaponFireComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSWeaponFireComponent();

	// Fire once from the currently held weapon's muzzle socket. Safe to call
	// directly (e.g. from Blueprint) even without the input binding.
	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon")
	void Fire();

	// Start/stop full-auto firing directly (e.g. from Blueprint), mirroring
	// what the input bindings do.
	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon")
	void StartFiring();

	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon")
	void StopFiring();

	// Start reloading the currently held weapon. No-op if already full,
	// already reloading, or the current overlay state has no ammo stats
	// configured. There is no reload animation available in this project's
	// content (checked - none exist), so this is a pure timed gameplay
	// mechanic for now: firing is blocked for ReloadSeconds, then the
	// magazine refills. See AGENTS.md for the animation-gap note.
	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Ammo")
	void Reload();

	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Ammo")
	int32 GetCurrentAmmoInMagazine() const { return CurrentAmmoInMagazine; }

	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Ammo")
	bool IsReloading() const { return bIsReloading; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleFireStarted(const FInputActionValue& Value);
	void HandleFireStopped(const FInputActionValue& Value);
	void HandleReloadInput(const FInputActionValue& Value);

	// A pawn's BeginPlay typically runs before its PlayerController actually
	// possesses it, so GetController() is often still null when this
	// component's own BeginPlay runs - binding only there silently does
	// nothing. TrySetupInput() is called both from BeginPlay (covers the
	// case where possession already happened) and from
	// HandleControllerChanged (covers the far more common case where it
	// happens after).
	void TrySetupInput();

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	// Input action to bind for firing. If unset, only Fire() is usable
	// (e.g. called from elsewhere), no key binding is set up.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Input")
	TObjectPtr<UInputAction> FireInputAction;

	// Mapping context containing FireInputAction's key binding. Added to the
	// EnhancedInput subsystem directly by this component - it does not rely
	// on ALS's own DefaultInputMappingContext, so no vendored plugin content
	// needs editing to add a new action.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Input")
	TObjectPtr<UInputMappingContext> FireInputMappingContext;

	// Reload input action. Can point at the same mapping context as
	// FireInputAction (a mapping context just holds a list of key
	// mappings, nothing stops it holding more than one action) - only
	// FireInputMappingContext actually needs adding to the EnhancedInput
	// subsystem, this action just needs to exist in whichever context is.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Input")
	TObjectPtr<UInputAction> ReloadInputAction;

	// Ammo capacity/reload time per overlay state (weapon type). An overlay
	// state with no entry here is treated as having unlimited ammo and
	// instant reload - i.e. ammo limits are opt-in per weapon, not a
	// default restriction.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Ammo")
	TMap<EALSOverlayState, FALSWeaponAmmoStats> AmmoStatsByOverlayState;

	// Name of the socket on the currently held weapon mesh to trace from.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// Max hitscan trace distance, in cm.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon")
	float MaxRange = 10000.0f;

	// Spread half-angle in degrees per gait, applied around the aim
	// direction. Tuned as a starting point, not measured against a real
	// weapon.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Spread")
	float SpreadDegreesWalking = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Spread")
	float SpreadDegreesRunning = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Spread")
	float SpreadDegreesSprinting = 4.0f;

	// Multiplier applied to the above when RotationMode is Aiming.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Spread")
	float AimingSpreadMultiplier = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// Full-auto fire rate. The weapon fires once immediately on press, then
	// repeats at 60/RoundsPerMinute second intervals for as long as the
	// input is held.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon", meta = (ClampMin = "1.0"))
	float RoundsPerMinute = 600.0f;

	// --- Bloom: spread that escalates while firing, on top of the base
	// gait/aim spread above, and recovers after a short pause. ---

	// Added to the current bloom on every shot.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Bloom")
	float BloomPerShotDegrees = 0.35f;

	// Bloom is clamped to this, so sustained full-auto does not spiral to an
	// absurd cone.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Bloom")
	float MaxBloomDegrees = 6.0f;

	// How long after the last shot before bloom starts recovering.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Bloom")
	float BloomDecayDelaySeconds = 0.3f;

	// Recovery rate once the delay above has elapsed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Bloom")
	float BloomDecayDegreesPerSecond = 6.0f;

	// --- Recoil: an actual camera pitch kick per shot, applied the same way
	// normal mouse look is (AddPitchInput), so it composes naturally with
	// player input and can be fought by pulling the mouse down. Recovers
	// over a few frames rather than instantly. ---

	// Pitch kick added per shot, in degrees. Sign convention: UE's
	// AddPitchInput takes positive = look down, so this is applied negated
	// to kick the camera *up*. If it kicks the wrong way on first test,
	// flip the sign where it is applied in Fire(), not this value.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Recoil")
	float RecoilKickPerShotDegrees = 0.6f;

	// Cap on accumulated (not-yet-recovered) kick, so sustained full-auto
	// does not climb forever.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Recoil")
	float MaxRecoilPitchDegrees = 6.0f;

	// How fast accumulated recoil bleeds off, in (fraction of remaining
	// kick) per second. Higher recovers snappier.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Recoil")
	float RecoilRecoverySpeed = 8.0f;

private:
	float ComputeCurrentSpreadDegrees(const class AALSBaseCharacter* Character) const;
	void UpdateBloomForShot();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// If the character's overlay state has changed since the last check (or
	// this is the first check), refills CurrentAmmoInMagazine to the new
	// weapon's full capacity. No delegate exists for "overlay state
	// changed" that this component can hook, so this is checked lazily at
	// the top of Fire() and Reload() instead, the same pattern bloom/recoil
	// already use for their own lazy time-based state.
	void SyncAmmoForCurrentWeapon(const class AALSBaseCharacter* Character);

	void FinishReload();

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	bool bInputBound = false;

	float CurrentBloomDegrees = 0.0f;
	float LastFireWorldTime = -1000.0f;
	float RemainingRecoilPitch = 0.0f;

	EALSOverlayState LastSyncedOverlayState = EALSOverlayState::Default;
	bool bAmmoSynced = false;
	int32 CurrentAmmoInMagazine = 0;
	bool bIsReloading = false;
};
