#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Stats/ALSStaminaComponent.h"
#include "Character/ALSCharacter.h"
#include "Library/ALSCharacterEnumLibrary.h"

TEST_CLASS(ALSStaminaComponentTests, "ALSHost.Stats")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;
	UALSStaminaComponent* Stamina = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	// The player character (ALS_CharacterBP) is the only one with a
	// StaminaComponent this pass - enemies don't sprint (see AGENT_TASKS/0003).
	void SpawnPlayerCharacter()
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		if (!CharClass)
		{
			return;
		}

		Character = &Spawner->SpawnActor<AALSCharacter>(FActorSpawnParameters(), CharClass);
		Stamina = Character->FindComponentByClass<UALSStaminaComponent>();
	}

	TEST_METHOD(ALSCharacterBP_Spawned_HasStaminaComponent_AtMax)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() { SpawnPlayerCharacter(); })
			.Then([this]() {
				ASSERT_THAT(IsNotNull(Stamina));
				ASSERT_THAT(IsNear(Stamina->GetCurrentStamina(), Stamina->MaxStamina, 0.01f));
				ASSERT_THAT(IsFalse(Stamina->IsExhausted()));
			});
	}

	TEST_METHOD(Sprinting_DrainsStamina_OverTime)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnPlayerCharacter();
				ASSERT_THAT(IsNotNull(Character));
				Character->SetGait(EALSGait::Sprinting, /*bForce=*/true);
			})
			// Tick the world forward so UALSStaminaComponent::TickComponent
			// actually runs a few times against the sprinting gait.
			.WaitDelay(FTimespan::FromSeconds(0.6))
			.Then([this]() {
				ASSERT_THAT(IsNotNull(Stamina));
				ASSERT_THAT(IsTrue(Stamina->GetCurrentStamina() < Stamina->MaxStamina));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
