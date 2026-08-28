#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Components/InputTestActions.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Character/ALSCharacter.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

// The one piece of the original docs/testing.md plan not yet covered:
// firing through the *real* Enhanced Input action (CQTestEnhancedInput's
// FInputTestActions programmatically injects it via the engine's real
// input-injection API), not a direct C++ call to Fire(). Everything else
// in this file follows the documented boilerplate in InputTestActions.h.
struct FALSFireTestAction : public FTestAction
{
	FALSFireTestAction()
	{
		InputActionName = TEXT("IA_Fire");
		InputActionValue = FInputActionValue(true);
	}
};

struct FALSFireTestToggleInventoryAction : public FTestAction
{
	FALSFireTestToggleInventoryAction()
	{
		InputActionName = TEXT("IA_ToggleInventory");
		InputActionValue = FInputActionValue(true);
	}
};

class FALSShooterTestActions : public FInputTestActions
{
public:
	explicit FALSShooterTestActions(APawn* InPawn) : FInputTestActions(InPawn)
	{
	}

	void PressFire()
	{
		PerformAction(FALSFireTestAction{});
	}

	void PressToggleInventory()
	{
		PerformAction(FALSFireTestToggleInventoryAction{});
	}
};

TEST_CLASS(ALSWeaponFireInputTests, "ALSHost.Weapon")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Shooter = nullptr;
	UALSWeaponFireComponent* Weapon = nullptr;
	TUniquePtr<FALSShooterTestActions> ShooterActions;
	bool bObservedAiming = false;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	TEST_METHOD(PressingFireAction_ThroughRealInput_ConsumesAmmo)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));

				Shooter = &Spawner->SpawnActorAt<AALSCharacter>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Weapon = Shooter->FindComponentByClass<UALSWeaponFireComponent>();
				ASSERT_THAT(IsNotNull(Weapon));

				// Equip the Rifle so Fire() has a valid weapon mesh/muzzle
				// and ammo to work with (OnUpdateHeldObject, a Blueprint
				// event on ALS_CharacterBP, does the actual AttachToHand).
				Shooter->SetOverlayState(EALSOverlayState::Rifle);

				// Same pattern as ALSEnemyAIControllerTests: a temp
				// FMapTestSpawner world already has one real local player's
				// PlayerController (with a fully initialized Enhanced Input
				// subsystem) - re-possess our shooter with that one rather
				// than spawning a disconnected PlayerController, which
				// FInputTestActions' input injection needs.
				APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
				ASSERT_THAT(IsNotNull(PC));
				PC->Possess(Shooter);

				ShooterActions = MakeUnique<FALSShooterTestActions>(Shooter);
			})
			// UALSWeaponFireComponent binds its input on
			// ReceiveControllerChangedDelegate, which Possess() above
			// triggers synchronously - but give it a beat before injecting
			// input in case the EnhancedInputComponent isn't fully ready
			// the same frame.
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				ShooterActions->PressFire();
			})
			.Until([this]() { return Weapon->GetCurrentAmmoInMagazine() < 30; }, FTimespan::FromSeconds(2.0))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() < 30));
			});
	}

	TEST_METHOD(PressingFireAction_WhileSprinting_FiresImmediately_AndCancelsSprint)
	{
		// Pressing fire while sprinting should transition straight into
		// walk+aim+shoot in that same press - not cancel sprint on the first
		// press and require a second press to actually fire (which is what
		// an earlier version of this behavior did, and is exactly what this
		// test would have caught: ammo staying at 0/unfired).
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));

				Shooter = &Spawner->SpawnActorAt<AALSCharacter>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Weapon = Shooter->FindComponentByClass<UALSWeaponFireComponent>();
				ASSERT_THAT(IsNotNull(Weapon));
				Shooter->SetOverlayState(EALSOverlayState::Rifle);
				Shooter->SetGait(EALSGait::Sprinting, /*bForce=*/true);
				ASSERT_THAT(IsTrue(Shooter->GetGait() == EALSGait::Sprinting));

				APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
				ASSERT_THAT(IsNotNull(PC));
				PC->Possess(Shooter);

				ShooterActions = MakeUnique<FALSShooterTestActions>(Shooter);
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() { ShooterActions->PressFire(); })
			.Until([this]() { return Weapon->GetCurrentAmmoInMagazine() != 0; }, FTimespan::FromSeconds(2.0))
			.Then([this]() {
				// Ammo is lazily synced to the magazine size inside Fire()
				// itself, so "not 0 anymore" already proves a shot actually
				// went off (29, one shot into a fresh 30-round magazine).
				ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() == 29));
			});
	}

	TEST_METHOD(PressingFireAction_WhileNotSprinting_EntersAimingRotationMode)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));

				Shooter = &Spawner->SpawnActorAt<AALSCharacter>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Weapon = Shooter->FindComponentByClass<UALSWeaponFireComponent>();
				ASSERT_THAT(IsNotNull(Weapon));
				Shooter->SetOverlayState(EALSOverlayState::Rifle);
				Shooter->SetGait(EALSGait::Walking, /*bForce=*/true);
				ASSERT_THAT(IsTrue(Shooter->GetRotationMode() != EALSRotationMode::Aiming));

				APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
				ASSERT_THAT(IsNotNull(PC));
				PC->Possess(Shooter);

				ShooterActions = MakeUnique<FALSShooterTestActions>(Shooter);
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() { ShooterActions->PressFire(); })
			// PerformAction()'s injection lands on a later tick, not
			// synchronously within this call - and FInputTestActions'
			// single injection behaves like a tap rather than a sustained
			// hold (StopFiring appears to run a tick or two later with no
			// explicit release injected, reverting RotationMode again), so
			// Aiming is only true for a single-tick window. Latch it into a
			// member bool the instant the predicate sees it, rather than
			// re-checking RotationMode again afterward - by the time any
			// later .Then() runs, on a later tick, it may have already
			// reverted even though it was genuinely true a moment earlier.
			.Until([this]() {
				if (Shooter->GetRotationMode() == EALSRotationMode::Aiming)
				{
					bObservedAiming = true;
				}
				return bObservedAiming;
			}, FTimespan::FromSeconds(1.0))
			.Then([this]() {
				// This is the actual bug report this covers: the upper-body
				// aim pose (driven by RotationMode == Aiming) previously
				// never activated just from firing while walking, only from
				// a separate manual Aim hold.
				ASSERT_THAT(IsTrue(bObservedAiming));
			});
	}

	TEST_METHOD(PressingFireAction_WhileCrouching_StillFires)
	{
		// User report: "i can't shoot while crouching" - turned out to be a
		// perception issue (user confirmed it does work), not a real block,
		// but nothing in ALSWeaponFireComponent or ALS's own
		// StanceAction/OnStanceChanged gates firing on Stance, so this locks
		// that in against a regression.
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));

				Shooter = &Spawner->SpawnActorAt<AALSCharacter>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Weapon = Shooter->FindComponentByClass<UALSWeaponFireComponent>();
				ASSERT_THAT(IsNotNull(Weapon));
				Shooter->SetOverlayState(EALSOverlayState::Rifle);
				Shooter->SetStance(EALSStance::Crouching, /*bForce=*/true);
				ASSERT_THAT(IsTrue(Shooter->GetStance() == EALSStance::Crouching));

				APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
				ASSERT_THAT(IsNotNull(PC));
				PC->Possess(Shooter);

				ShooterActions = MakeUnique<FALSShooterTestActions>(Shooter);
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Shooter->GetStance() == EALSStance::Crouching));
				ShooterActions->PressFire();
			})
			.Until([this]() { return Weapon->GetCurrentAmmoInMagazine() != 0; }, FTimespan::FromSeconds(2.0))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() == 29));
				ASSERT_THAT(IsTrue(Shooter->GetStance() == EALSStance::Crouching));
			});
	}

	TEST_METHOD(PressingFireAction_WhileUnarmed_DoesNotEnterAimingMode)
	{
		// User report: "when unarmed, and i do left click, some zoom
		// appears" - StartFiring() used to call AimAction(true)
		// unconditionally, entering the Aiming rotation mode (which drives
		// ALS_PlayerCameraBehavior's zoom) even with no weapon equipped.
		// Calling StartFiring() directly (BlueprintCallable) rather than
		// through real input injection - no tap-vs-hold timing to fight,
		// since nothing here is meant to become true even transiently.
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));

				Shooter = &Spawner->SpawnActorAt<AALSCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Weapon = Shooter->FindComponentByClass<UALSWeaponFireComponent>();
				ASSERT_THAT(IsNotNull(Weapon));
				ASSERT_THAT(IsTrue(Shooter->GetOverlayState() != EALSOverlayState::Rifle));
				ASSERT_THAT(IsTrue(Shooter->GetRotationMode() != EALSRotationMode::Aiming));

				Weapon->StartFiring();
			})
			.Then([this]() {
				ASSERT_THAT(IsTrue(Shooter->GetRotationMode() != EALSRotationMode::Aiming));
			});
	}

	int32 AmmoBeforeInventoryOpened = -1;

	TEST_METHOD(PressingFireAction_WhileInventoryOpen_DoesNotFire)
	{
		// User report: "when on the inventory open, the gun shoot with
		// click feature must not work" - the Q debug menu suspends
		// FireInputMappingContext entirely while open so a click on a menu
		// option doesn't also fire; the inventory panel (a separate
		// component) didn't do the equivalent, so left-click both clicked
		// the UI and fired the weapon underneath it. StartFiring() now
		// checks UALSInventoryComponent::IsInventoryUIOpen() directly.
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));

				Shooter = &Spawner->SpawnActorAt<AALSCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Weapon = Shooter->FindComponentByClass<UALSWeaponFireComponent>();
				ASSERT_THAT(IsNotNull(Weapon));
				Shooter->SetOverlayState(EALSOverlayState::Rifle);

				APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
				ASSERT_THAT(IsNotNull(PC));
				PC->Possess(Shooter);

				// One direct Fire() call to force SyncAmmoForCurrentWeapon to
				// run (lazily synced on first Fire()/Reload() - see
				// ALSWeaponFireComponent.h) - without this,
				// GetCurrentAmmoInMagazine() would still read its unsynced
				// default of 0, indistinguishable from "emptied by firing"
				// below (the exact wrong-baseline mistake documented for the
				// sprint test earlier in this project's history).
				Weapon->Fire();
				AmmoBeforeInventoryOpened = Weapon->GetCurrentAmmoInMagazine();
				ASSERT_THAT(IsTrue(AmmoBeforeInventoryOpened == 29));

				ShooterActions = MakeUnique<FALSShooterTestActions>(Shooter);
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() { ShooterActions->PressToggleInventory(); })
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				UALSInventoryComponent* Inventory = Shooter->FindComponentByClass<UALSInventoryComponent>();
				ASSERT_THAT(IsNotNull(Inventory));
				ASSERT_THAT(IsTrue(Inventory->IsInventoryUIOpen()));

				ShooterActions->PressFire();
			})
			.WaitDelay(FTimespan::FromSeconds(0.5))
			.Then([this]() {
				// Ammo unchanged from the pre-inventory baseline -
				// StartFiring() bailed out before ever calling Fire() again.
				ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() == AmmoBeforeInventoryOpened));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
