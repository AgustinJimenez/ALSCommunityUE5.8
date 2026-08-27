#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Character/ALSCharacter.h"

// Pure logic driven off AALSBaseCharacter::IsMoving()/GetGait()/GetRotationMode(),
// no BeginPlay dependency - FActorTestSpawner is the right, cheap tool here
// (see docs/testing.md). A freshly spawned, never-ticked character reports
// IsMoving() == false by its default member value, which conveniently is
// exactly the "standing still" case we want to check without needing to
// simulate real movement.
TEST_CLASS(ALSWeaponSpreadTests, "ALSHost.Weapon")
{
	FActorTestSpawner Spawner;
	UALSWeaponFireComponent* Weapon = nullptr;
	AALSCharacter* Character = nullptr;

	BEFORE_EACH()
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		ASSERT_THAT(IsNotNull(CharClass));

		Character = &Spawner.SpawnActor<AALSCharacter>(FActorSpawnParameters(), CharClass);
		Weapon = Character->FindComponentByClass<UALSWeaponFireComponent>();
		ASSERT_THAT(IsNotNull(Weapon));
	}

	TEST_METHOD(StandingStill_UsesStandingSpread_NotWalkingSpread)
	{
		ASSERT_THAT(IsFalse(Character->IsMoving()));

		const float Spread = Weapon->ComputeCurrentSpreadDegrees(Character);

		ASSERT_THAT(IsNear(Spread, Weapon->SpreadDegreesStanding, 0.001f));
	}

	TEST_METHOD(StandingSpread_IsTighterThan_WalkingSpread)
	{
		// Sanity-checks the tuning itself, not just the branch taken - if
		// someone edits the defaults so standing is no longer the tightest
		// tier, this should catch it.
		ASSERT_THAT(IsTrue(Weapon->SpreadDegreesStanding < Weapon->SpreadDegreesWalking));
		ASSERT_THAT(IsTrue(Weapon->SpreadDegreesWalking < Weapon->SpreadDegreesRunning));
		ASSERT_THAT(IsTrue(Weapon->SpreadDegreesRunning < Weapon->SpreadDegreesSprinting));
	}

	TEST_METHOD(RecoilStandingMultiplier_ReducesKick_ComparedToMoving)
	{
		// Same "moving reduces control" principle as spread - tuning sanity
		// check, since there's no unit currently spawned/ticking to actually
		// flip IsMoving() true in this lightweight test.
		ASSERT_THAT(IsTrue(Weapon->RecoilStandingMultiplier < 1.0f));
		ASSERT_THAT(IsTrue(Weapon->RecoilStandingMultiplier > 0.0f));
	}
};

#endif // WITH_AUTOMATION_TESTS
