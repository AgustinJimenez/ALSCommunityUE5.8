#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "ALSWeaponFireComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UAnimSequenceBase;
class UAnimMontage;
class UUserWidget;

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

	// Played via PlaySlotAnimationAsDynamicMontage on the component's
	// ReloadMontageSlotName - no hand-authored AnimMontage asset needed for
	// a simple "play this once" reload. If unset, Reload() still runs the
	// timer/ammo logic, just with nothing to play.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimSequenceBase> ReloadAnimation;

	// Static correction added to HeldObjectRoot's relative transform for the
	// duration of Reload() only, then reverted once FinishReload() runs. A
	// reload animation retargeted from a different project's rig (see
	// AGENTS.md / docs/mcp-notes.md - cross-project migration+retargeting)
	// doesn't necessarily line up with this weapon's hand-attach point the
	// same way our own idle/fire poses do, since it was authored against a
	// different weapon's grip geometry. These live on the Blueprint CDO
	// like MagazineSize/ReloadSeconds, so they can be tuned by eye in PIE
	// without a C++ rebuild.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ReloadHeldObjectLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator ReloadHeldObjectRotationOffset = FRotator::ZeroRotator;
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
	// configured. Plays the weapon type's ReloadAnimation (if set) via
	// PlaySlotAnimationAsDynamicMontage on ReloadMontageSlotName; firing is
	// blocked for ReloadSeconds regardless of whether an animation is
	// playing, then the magazine refills.
	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Ammo")
	void Reload();

	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Ammo")
	int32 GetCurrentAmmoInMagazine() const { return CurrentAmmoInMagazine; }

	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Ammo")
	bool IsReloading() const { return bIsReloading; }

	// --- Reload hand/gun offset live-tuning (debug tool) ---
	// Equips the Rifle, disables movement (camera look still works so you
	// can orbit and inspect), shows the mouse cursor, spawns
	// DebugReloadTuningWidgetClass, and repeatedly replays ReloadAnimation
	// in a loop. While active, every tick re-applies the Rifle entry's
	// current ReloadHeldObjectLocationOffset/RotationOffset to
	// HeldObjectRoot, so a slider bound to
	// DebugSetReloadLocationOffset/RotationOffset updates the pose live.
	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Debug")
	void DebugStartReloadOffsetTuning();

	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Debug")
	void DebugStopReloadOffsetTuning();

	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Debug")
	void ToggleDebugReloadOffsetTuning();

	// Starts/stops the reload animation auto-repeating while tuning is
	// active (on by default whenever DebugStartReloadOffsetTuning() runs).
	// Turning it off lets the current play finish and then just stop
	// repeating, instead of forcing a frozen frame.
	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Debug")
	void ToggleDebugReloadAnimLoop();

	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Debug")
	bool IsDebugTuningReloadOffset() const { return bDebugTuningReloadOffset; }

	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Debug")
	bool IsDebugReloadAnimLoopEnabled() const { return bDebugReloadLoopEnabled; }

	// Pauses/resumes the currently-playing reload montage in place (via
	// Montage_Pause/Montage_Resume), for holding a fixed pose to adjust
	// against rather than a moving target. Also pauses/resumes the replay
	// timer so a paused montage doesn't get silently restarted underneath
	// the frozen pose.
	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Debug")
	void ToggleDebugReloadFreeze();

	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Debug")
	bool IsDebugReloadFrozen() const { return bDebugReloadFrozen; }

	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Debug")
	FVector DebugGetReloadLocationOffset() const;

	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Debug")
	FRotator DebugGetReloadRotationOffset() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Debug")
	void DebugSetReloadLocationOffset(FVector NewOffset);

	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Debug")
	void DebugSetReloadRotationOffset(FRotator NewOffset);

	// Copies the Rifle entry's current offsets as plain text (e.g.
	// "Location=(X=1.00,Y=0.00,Z=-2.50) Rotation=(Pitch=0.00,Yaw=5.00,Roll=0.00)")
	// to the OS clipboard, so they can be pasted straight back into chat.
	UFUNCTION(BlueprintCallable, Category = "ALS|Weapon|Debug")
	void DebugCopyReloadOffsetsToClipboard();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleFireStarted(const FInputActionValue& Value);
	void HandleFireStopped(const FInputActionValue& Value);
	void HandleReloadInput(const FInputActionValue& Value);
	// Tap-vs-hold on the same key, distinguished by a plain timer rather
	// than Enhanced Input Tap/Hold trigger assets: Pressed starts a
	// DebugReloadHoldThresholdSeconds timer; if Released fires first, it's
	// a tap (toggles the tuning panel); if the timer fires first, it's a
	// hold (toggles the anim loop), and the subsequent Released is then
	// just the end of that hold, not a second tap.
	void HandleDebugReloadTuningPressed(const FInputActionValue& Value);
	void HandleDebugReloadTuningReleased(const FInputActionValue& Value);
	void HandleDebugReloadTuningHoldThresholdReached();
	void HandleCameraZoomInput(const FInputActionValue& Value);

	// ALS's own Q-held debug overlay menu has a scroll-to-cycle interaction
	// that doesn't visually respond (traced deep into ALS's own Blueprints
	// with no fix found within scope - see
	// AGENT_TASKS/0002_q_menu_scroll_not_working.md). Rather than fix that,
	// make the menu mouse-clickable instead: while Q is held, show the
	// cursor and suspend ALS's DefaultInputMappingContext entirely (camera
	// look + movement), so nothing fights the user's mouse clicks; restore
	// both on release.
	void HandleDebugOverlayMenuOpened(const FInputActionValue& Value);
	void HandleDebugOverlayMenuClosed(const FInputActionValue& Value);

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

	// Tap toggles DebugStartReloadOffsetTuning()/DebugStopReloadOffsetTuning();
	// holding for DebugReloadHoldThresholdSeconds toggles
	// ToggleDebugReloadAnimLoop() instead. Can point at the same mapping
	// context as FireInputAction, same as ReloadInputAction above.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Debug")
	TObjectPtr<UInputAction> DebugReloadTuningInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Debug")
	float DebugReloadHoldThresholdSeconds = 2.0f;

	// Third-person mouse wheel zoom. Not conceptually a weapon feature -
	// bound here purely to reuse this component's existing
	// TrySetupInput()/EnhancedInputComponent plumbing rather than building
	// a second component just for one axis binding. Forwards to
	// AALSHostPlayerCameraManager::AddZoomInput().
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Input")
	TObjectPtr<UInputAction> CameraZoomInputAction;

	// Its own mapping context, separate from FireInputMappingContext. This
	// was originally added on a hunch that zoom was stealing MouseWheelAxis
	// from ALS's own Q-held debug overlay menu (DebugOverlayMenuCycleAction) -
	// confirmed WRONG by an isolation test (disabling zoom entirely didn't
	// fix the menu; see AGENT_TASKS/0002_q_menu_scroll_not_working.md).
	// Left as its own context anyway since it's a reasonable separation
	// regardless, just not load-bearing for that bug.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Input")
	TObjectPtr<UInputMappingContext> CameraZoomInputMappingContext;

	// Same InputAction asset ALS's own AALSPlayerController binds
	// DebugOpenOverlayMenuAction to (Q) - reused directly rather than
	// duplicated, bound here purely so this component gets its own
	// Started/Completed events for showing the mouse cursor and suspending
	// gameplay input while the menu is open. See
	// HandleDebugOverlayMenuOpened/Closed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Debug")
	TObjectPtr<UInputAction> DebugOverlayMenuInputAction;

	// Mapping context for DebugOverlayMenuInputAction. Just needs a Q
	// mapping - priority doesn't matter much since this doesn't need to
	// win/block anything, it only observes press/release.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Debug")
	TObjectPtr<UInputMappingContext> DebugOverlayMenuInputMappingContext;

	// Replacement prop-picker menu shown while Q is held (see
	// UALSDebugPropMenuWidget) - ALS's own OverlayStateSwitcher menu never
	// actually populated its button list or had click/hover implemented.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Debug")
	TSubclassOf<class UALSDebugPropMenuWidget> DebugPropMenuWidgetClass;

	// Widget class shown while reload-offset tuning is active. Expected to
	// expose sliders/buttons that call DebugGetReloadLocationOffset,
	// DebugSetReloadLocationOffset, DebugGetReloadRotationOffset,
	// DebugSetReloadRotationOffset, and DebugCopyReloadOffsetsToClipboard
	// on this component.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Debug")
	TSubclassOf<UUserWidget> DebugReloadTuningWidgetClass;

	// Ammo capacity/reload time per overlay state (weapon type). An overlay
	// state with no entry here is treated as having unlimited ammo and
	// instant reload - i.e. ammo limits are opt-in per weapon, not a
	// default restriction.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Ammo")
	TMap<EALSOverlayState, FALSWeaponAmmoStats> AmmoStatsByOverlayState;

	// Name of the socket on the currently held weapon mesh to trace from.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// AnimGraph slot to play reload animations on. "Grounded Slot" is
	// ALS_AnimBP's dedicated full-body one-off action slot (used for things
	// like rolls/mantles) - a two-handed reload needs both arms plus the
	// spine, which the single-limb slots (Arm L/Arm R individually) don't
	// cover. Confirmed via reading every AnimGraphNode_Slot in ALS_AnimBP:
	// the full slot list is Legs, Pelvis, Curves, Arm R, Head, Arm L,
	// Spine, Grounded Slot, BaseLayer.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Ammo")
	FName ReloadMontageSlotName = TEXT("Grounded Slot");

public:
	// Max hitscan trace distance, in cm.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon")
	float MaxRange = 10000.0f;

	// Applied via UGameplayStatics::ApplyPointDamage on hit - reaches any
	// actor with an AActor::OnTakeAnyDamage listener, notably
	// UALSHealthComponent, without this component needing to know about
	// health at all.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Damage")
	float DamagePerShot = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Damage")
	TSubclassOf<class UDamageType> DamageTypeClass;

	// Damage is full strength up to this distance, then falls off linearly
	// down to MinDamageMultiplier at MaxRange - real ballistics lose
	// stopping power over distance, a flat hitscan number regardless of
	// range never did.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Damage")
	float DamageFalloffStartRange = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Damage")
	float MinDamageMultiplier = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Damage")
	float HeadshotMultiplier = 2.5f;

	// Bone name(s) considered a headshot, matched against FHitResult::BoneName.
	// ALS_Mannequin_Skeleton (and stock Epic Manny/Quinn) name this "head".
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Damage")
	FName HeadBoneName = TEXT("head");

	// Public (rather than an internal Fire() helper) so it's directly unit
	// testable and reusable for UI damage previews later. BoneName drives
	// the headshot check, DistanceFromMuzzle drives falloff.
	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Damage")
	float ComputeDamageForHit(FName BoneName, float DistanceFromMuzzle) const;

	// When true, Fire() spawns a real AALSProjectile with actual flight time
	// and gravity drop instead of resolving the shot with an instant hitscan
	// trace - a shot at range visibly takes time to arrive and arcs instead
	// of hitting the instant the trigger is pulled. Both modes deal identical
	// damage at identical distances/hit zones via the same
	// ComputeDamageForHit - only the timing/trajectory differs. Falls back to
	// the hitscan trace if true but ProjectileClass is unset.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Projectile")
	bool bUseProjectilePhysics = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Projectile")
	TSubclassOf<class AALSProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Projectile")
	float ProjectileSpeed = 8000.0f;

	// 0 = travels in a dead-straight line (no drop), 1 = full engine gravity.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Projectile")
	float ProjectileGravityScale = 0.3f;

public:
	// Spread half-angle in degrees, applied around the aim direction. Tuned
	// as a starting point, not measured against a real weapon.
	//
	// Standing (AALSBaseCharacter::IsMoving() false - actually stationary,
	// not just "Gait is Walking") is a genuinely separate, tighter tier from
	// Walking gait: a character can be in Walking gait while still ramping
	// up to speed or slowing to a stop, so gait alone doesn't distinguish
	// "braced and still" from "in motion at walk speed". Real-world accuracy
	// is meaningfully better stationary than even walking.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Spread")
	float SpreadDegreesStanding = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Spread")
	float SpreadDegreesWalking = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Spread")
	float SpreadDegreesRunning = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Spread")
	float SpreadDegreesSprinting = 4.0f;

	// Multiplier applied to the above when RotationMode is Aiming.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Spread")
	float AimingSpreadMultiplier = 0.25f;

	// Public (rather than an internal Fire() helper) so it's directly unit
	// testable - see ALSWeaponDamageTests.cpp-style coverage in
	// ALSWeaponSpreadTests.cpp.
	UFUNCTION(BlueprintPure, Category = "ALS|Weapon|Spread")
	float ComputeCurrentSpreadDegrees(const class AALSBaseCharacter* Character) const;

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

	// Multiplies RecoilKickPerShotDegrees when the character is actually
	// stationary (AALSBaseCharacter::IsMoving() false) - a braced, still
	// shooter controls recoil better than one in motion. 1.0 = no change.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Weapon|Recoil")
	float RecoilStandingMultiplier = 0.6f;

private:
	void UpdateBloomForShot(const class AALSBaseCharacter* Character);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// If the character's overlay state has changed since the last check (or
	// this is the first check), refills CurrentAmmoInMagazine to the new
	// weapon's full capacity. No delegate exists for "overlay state
	// changed" that this component can hook, so this is checked lazily at
	// the top of Fire() and Reload() instead, the same pattern bloom/recoil
	// already use for their own lazy time-based state.
	void SyncAmmoForCurrentWeapon(const class AALSBaseCharacter* Character);

	void FinishReload();
	void DebugReplayReloadAnimLoop();

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	FTimerHandle DebugReloadLoopTimerHandle;
	FTimerHandle DebugReloadHoldTimerHandle;
	bool bInputBound = false;

	bool bDebugTuningReloadOffset = false;
	bool bDebugReloadLoopEnabled = true;
	bool bDebugReloadFrozen = false;
	bool bDebugReloadHoldThresholdFired = false;
	TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;

	UPROPERTY()
	TObjectPtr<UAnimMontage> DebugReloadTuningMontage;

	UPROPERTY()
	TObjectPtr<UUserWidget> DebugReloadTuningWidgetInstance;

	UPROPERTY()
	TObjectPtr<class UALSDebugPropMenuWidget> DebugPropMenuWidgetInstance;

	// HeldObjectRoot's relative transform captured right before Reload()
	// applies ReloadHeldObjectLocationOffset/RotationOffset, so
	// FinishReload() can put it back exactly rather than assuming a base
	// value (the base varies per weapon via AttachToHand's Offset param).
	FVector SavedHeldObjectRelativeLocation = FVector::ZeroVector;
	FRotator SavedHeldObjectRelativeRotation = FRotator::ZeroRotator;
	bool bHeldObjectTransformSaved = false;

	float CurrentBloomDegrees = 0.0f;
	float LastFireWorldTime = -1000.0f;
	float RemainingRecoilPitch = 0.0f;

	EALSOverlayState LastSyncedOverlayState = EALSOverlayState::Default;
	bool bAmmoSynced = false;
	int32 CurrentAmmoInMagazine = 0;
	bool bIsReloading = false;
};
