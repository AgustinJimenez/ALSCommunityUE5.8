#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Components/InputTestActions.h"
#include "Weapon/ALSWeaponFireComponent.h"
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
};

TEST_CLASS(ALSWeaponFireInputTests, "ALSHost.Weapon")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Shooter = nullptr;
	UALSWeaponFireComponent* Weapon = nullptr;
	TUniquePtr<FALSShooterTestActions> ShooterActions;

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
};

#endif // WITH_AUTOMATION_TESTS
