#include "Weapon/ALSWeaponFireComponent.h"

#include "Character/ALSCharacter.h"
#include "Character/ALSBaseCharacter.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "HAL/IConsoleManager.h"

// Defaults to true while this feature is under active development, since
// the editor gets restarted frequently for rebuilds and this CVar (like all
// console variables) does not persist across restarts. Flip back to false
// once the weapon system is done.
static TAutoConsoleVariable<bool> CVarALSWeaponShowDebugTrace(
	TEXT("ALS.Weapon.ShowDebugTrace"),
	true,
	TEXT("Draw the hitscan weapon trace (muzzle to hit point) and hit marker in the world. Purely visual - does not affect hit detection."),
	ECVF_Default);

UALSWeaponFireComponent::UALSWeaponFireComponent()
{
	// Needed to smoothly bleed off the recoil pitch kick over several
	// frames (AddPitchInput is a per-frame accumulator, so a one-shot call
	// only nudges the camera for a single frame - applying a decaying
	// remainder every tick is what makes it feel like a kick-and-settle
	// rather than a single frame flicker).
	PrimaryComponentTick.bCanEverTick = true;

	// Default so the Rifle (the only weapon with a working Muzzle socket
	// right now) has a real magazine limit out of the box, without needing
	// manual per-instance Blueprint configuration.
	FALSWeaponAmmoStats RifleStats;
	RifleStats.MagazineSize = 30;
	RifleStats.ReloadSeconds = 2.2f;
	AmmoStatsByOverlayState.Add(EALSOverlayState::Rifle, RifleStats);
}

void UALSWeaponFireComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UALSWeaponFireComponent::HandleControllerChanged);
	}

	// Covers the (uncommon) case where the pawn is already possessed by the
	// time this component's BeginPlay runs.
	TrySetupInput();
}

void UALSWeaponFireComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopFiring();
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UALSWeaponFireComponent::HandleControllerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UALSWeaponFireComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	TrySetupInput();
}

void UALSWeaponFireComponent::TrySetupInput()
{
	// A pawn's BeginPlay typically runs before its PlayerController actually
	// possesses it (GameMode::RestartPlayer possesses after the pawn is
	// already spawned and ticking), so GetController() is frequently still
	// null the first time this runs - that is not an error, just try again
	// when ReceiveControllerChangedDelegate fires. Only bind once actual
	// possession has happened, and only once overall (bInputBound) so a
	// later re-possession does not stack duplicate bindings and double the
	// effective fire rate.
	if (bInputBound || !FireInputAction)
	{
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		return;
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!EIC)
	{
		// InputComponent itself may not exist yet this early either; the
		// next ReceiveControllerChangedDelegate firing (or a later retry)
		// will catch it. Not logged as a warning since this is a normal,
		// expected transient state, not a misconfiguration.
		return;
	}

	// Started fires once on press; Completed/Canceled cover both a
	// normal release and losing the input (e.g. alt-tab, controller
	// disconnect) so full-auto never gets stuck firing.
	EIC->BindAction(FireInputAction, ETriggerEvent::Started, this, &UALSWeaponFireComponent::HandleFireStarted);
	EIC->BindAction(FireInputAction, ETriggerEvent::Completed, this, &UALSWeaponFireComponent::HandleFireStopped);
	EIC->BindAction(FireInputAction, ETriggerEvent::Canceled, this, &UALSWeaponFireComponent::HandleFireStopped);

	if (ReloadInputAction)
	{
		EIC->BindAction(ReloadInputAction, ETriggerEvent::Started, this, &UALSWeaponFireComponent::HandleReloadInput);
	}

	bInputBound = true;

	if (FireInputMappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			FModifyContextOptions Options;
			Options.bForceImmediately = true;
			Subsystem->AddMappingContext(FireInputMappingContext, 10, Options);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ALSWeaponFireComponent: input bound on %s"), *GetOwner()->GetName());
}

void UALSWeaponFireComponent::HandleFireStarted(const FInputActionValue& Value)
{
	StartFiring();
}

void UALSWeaponFireComponent::HandleFireStopped(const FInputActionValue& Value)
{
	StopFiring();
}

void UALSWeaponFireComponent::HandleReloadInput(const FInputActionValue& Value)
{
	Reload();
}

void UALSWeaponFireComponent::StartFiring()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Fire immediately on press, then repeat at the RPM-derived interval
	// for as long as the trigger is held.
	Fire();

	const float IntervalSeconds = 60.0f / FMath::Max(RoundsPerMinute, 1.0f);
	World->GetTimerManager().SetTimer(FireTimerHandle, this, &UALSWeaponFireComponent::Fire, IntervalSeconds, /*bLoop=*/true);
}

void UALSWeaponFireComponent::StopFiring()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
	}
}

void UALSWeaponFireComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (RemainingRecoilPitch <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwner());
	APlayerController* PC = ALSChar ? Cast<APlayerController>(ALSChar->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}

	const float ThisFrameKick = RemainingRecoilPitch * FMath::Clamp(RecoilRecoverySpeed * DeltaTime, 0.0f, 1.0f);
	// Negated: AddPitchInput's positive convention is "look down", so a
	// negative value kicks the camera up.
	PC->AddPitchInput(-ThisFrameKick);
	RemainingRecoilPitch -= ThisFrameKick;
}

void UALSWeaponFireComponent::UpdateBloomForShot()
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float Elapsed = Now - LastFireWorldTime;
	if (Elapsed > BloomDecayDelaySeconds)
	{
		const float DecayTime = Elapsed - BloomDecayDelaySeconds;
		CurrentBloomDegrees = FMath::Max(0.0f, CurrentBloomDegrees - BloomDecayDegreesPerSecond * DecayTime);
	}
	LastFireWorldTime = Now;

	CurrentBloomDegrees = FMath::Min(CurrentBloomDegrees + BloomPerShotDegrees, MaxBloomDegrees);
	RemainingRecoilPitch = FMath::Min(RemainingRecoilPitch + RecoilKickPerShotDegrees, MaxRecoilPitchDegrees);
}

void UALSWeaponFireComponent::SyncAmmoForCurrentWeapon(const AALSBaseCharacter* Character)
{
	if (!Character)
	{
		return;
	}

	const EALSOverlayState CurrentState = Character->GetOverlayState();
	if (bAmmoSynced && CurrentState == LastSyncedOverlayState)
	{
		return;
	}

	LastSyncedOverlayState = CurrentState;
	bAmmoSynced = true;

	if (const FALSWeaponAmmoStats* Stats = AmmoStatsByOverlayState.Find(CurrentState))
	{
		CurrentAmmoInMagazine = Stats->MagazineSize;
	}
	else
	{
		// No ammo stats configured for this overlay state: treated as
		// unlimited ammo, matching props like the Torch/Binoculars that
		// were never meant to have a magazine at all.
		CurrentAmmoInMagazine = -1;
	}

	// Switching weapons cancels an in-progress reload of the previous one.
	if (bIsReloading)
	{
		bIsReloading = false;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ReloadTimerHandle);
		}
	}
}

void UALSWeaponFireComponent::Reload()
{
	AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwner());
	SyncAmmoForCurrentWeapon(ALSChar);

	if (bIsReloading)
	{
		return;
	}

	const FALSWeaponAmmoStats* Stats = ALSChar ? AmmoStatsByOverlayState.Find(ALSChar->GetOverlayState()) : nullptr;
	if (!Stats)
	{
		// Unlimited ammo for this weapon (or no weapon/character) - nothing
		// to reload.
		return;
	}

	if (CurrentAmmoInMagazine >= Stats->MagazineSize)
	{
		return;
	}

	// Full-auto should not keep firing through a reload.
	StopFiring();

	bIsReloading = true;
	UE_LOG(LogTemp, Log, TEXT("ALSWeaponFireComponent: reloading (%.1fs, no reload animation available yet - see AGENTS.md)"), Stats->ReloadSeconds);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UALSWeaponFireComponent::FinishReload, Stats->ReloadSeconds, /*bLoop=*/false);
	}
}

void UALSWeaponFireComponent::FinishReload()
{
	AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwner());
	const FALSWeaponAmmoStats* Stats = ALSChar ? AmmoStatsByOverlayState.Find(ALSChar->GetOverlayState()) : nullptr;

	bIsReloading = false;
	CurrentAmmoInMagazine = Stats ? Stats->MagazineSize : CurrentAmmoInMagazine;

	UE_LOG(LogTemp, Log, TEXT("ALSWeaponFireComponent: reload complete, %d rounds"), CurrentAmmoInMagazine);
}

float UALSWeaponFireComponent::ComputeCurrentSpreadDegrees(const AALSBaseCharacter* Character) const
{
	if (!Character)
	{
		return SpreadDegreesWalking;
	}

	float SpreadDegrees = SpreadDegreesWalking;
	switch (Character->GetGait())
	{
	case EALSGait::Walking:
		SpreadDegrees = SpreadDegreesWalking;
		break;
	case EALSGait::Running:
		SpreadDegrees = SpreadDegreesRunning;
		break;
	case EALSGait::Sprinting:
		SpreadDegrees = SpreadDegreesSprinting;
		break;
	}

	if (Character->GetRotationMode() == EALSRotationMode::Aiming)
	{
		SpreadDegrees *= AimingSpreadMultiplier;
	}

	return SpreadDegrees;
}

void UALSWeaponFireComponent::Fire()
{
	AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwner());
	if (!ALSChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALSWeaponFireComponent::Fire - owner is not an AALSCharacter"));
		return;
	}

	APlayerController* PC = Cast<APlayerController>(ALSChar->GetController());
	if (!PC)
	{
		return;
	}

	USkeletalMeshComponent* WeaponMesh = ALSChar->SkeletalMesh;
	if (!WeaponMesh || !WeaponMesh->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("ALSWeaponFireComponent::Fire - no skeletal-mesh weapon currently held"));
		return;
	}

	SyncAmmoForCurrentWeapon(ALSChar);

	if (bIsReloading)
	{
		return;
	}

	if (CurrentAmmoInMagazine == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("ALSWeaponFireComponent::Fire - empty, click"));
		StopFiring();
		return;
	}

	FVector MuzzleLocation;
	if (WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ALSWeaponFireComponent::Fire - held weapon has no '%s' socket, falling back to mesh origin"), *MuzzleSocketName.ToString());
		MuzzleLocation = WeaponMesh->GetComponentLocation();
	}

	// Aim toward where the camera is actually looking, not the muzzle
	// bone's own rotation - otherwise shots go wherever the gun mesh
	// happens to be posed, which rarely matches a center-screen crosshair.
	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector AimTarget = CameraLocation + CameraRotation.Vector() * MaxRange;
	const FVector AimDirection = (AimTarget - MuzzleLocation).GetSafeNormal();

	UpdateBloomForShot();
	const float SpreadDegrees = ComputeCurrentSpreadDegrees(ALSChar) + CurrentBloomDegrees;
	const FVector FireDirection = SpreadDegrees > 0.0f
		? FMath::VRandCone(AimDirection, FMath::DegreesToRadians(SpreadDegrees))
		: AimDirection;

	const FVector TraceEnd = MuzzleLocation + FireDirection * MaxRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ALSWeaponFire), /*bTraceComplex=*/true);
	QueryParams.AddIgnoredActor(ALSChar);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, TraceEnd, TraceChannel, QueryParams);

	if (CVarALSWeaponShowDebugTrace.GetValueOnGameThread())
	{
		const FVector DebugEnd = bHit ? Hit.ImpactPoint : TraceEnd;
		DrawDebugLine(GetWorld(), MuzzleLocation, DebugEnd, bHit ? FColor::Red : FColor::Yellow, false, 1.0f, 0, 0.5f);
		if (bHit)
		{
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 8.0f, FColor::Red, false, 1.0f);
		}
	}

	if (bHit)
	{
		UE_LOG(LogTemp, Log, TEXT("ALSWeaponFireComponent::Fire - hit %s at %s"),
			Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("<no actor>"),
			*Hit.ImpactPoint.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("ALSWeaponFireComponent::Fire - no hit"));
	}

	if (CurrentAmmoInMagazine > 0)
	{
		--CurrentAmmoInMagazine;
		if (CurrentAmmoInMagazine == 0)
		{
			// Ran dry mid-burst: stop full-auto cleanly instead of spinning
			// the timer doing nothing but log "empty, click" every interval.
			StopFiring();
		}
	}
}
