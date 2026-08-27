#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Combat/ALSDamageZone.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

// Overlap-triggered, needs real collision - FMapTestSpawner, same reasoning
// as ALSPickupTests.
TEST_CLASS(ALSDamageZoneTests, "ALSHost.Combat")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;
	UALSHealthComponent* Health = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	TEST_METHOD(StandingInZone_TakesDamageOverTime)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));

				Character = &Spawner->SpawnActorAt<AALSCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Health = Character->FindComponentByClass<UALSHealthComponent>();
				ASSERT_THAT(IsNotNull(Health));

				// CQTest's temp level (FMapTestSpawner::CreateFromTempLevel)
				// has no floor - left alone, the character free-falls
				// straight out of the zone's overlap volume in well under a
				// second (confirmed via logging: Z=0 -> Z=-294 in 0.77s,
				// matching unobstructed gravity). Disable movement so it just
				// stays put - this test is about damage application over
				// time, not physics.
				Character->GetCharacterMovement()->DisableMovement();

				// Don't tune DamagePerSecond/DamageIntervalSeconds here: this
				// spawns into an already-playing world, so BeginPlay (and the
				// SetTimer call that captures DamageIntervalSeconds as the
				// timer's fixed rate) runs synchronously inside SpawnActorAt,
				// before any property set on the returned reference afterward
				// takes effect - same class of gotcha as the pickup
				// "far away then sweep in" pattern in ALSPickupTests. Just
				// use the class defaults (10 DPS / 1s interval) and wait past
				// one interval instead.
				Spawner->SpawnActorAt<AALSDamageZone>(FVector::ZeroVector, FRotator::ZeroRotator);
			})
			.WaitDelay(FTimespan::FromSeconds(1.3))
			.Then([this]() {
				// At least one 1s tick (10 DPS * 1s = 10 damage) should have
				// landed by now - well under MaxHealth(100) so this can't be
				// mistaken for "already dead" masking a bug.
				ASSERT_THAT(IsTrue(Health->GetCurrentHealth() < Health->MaxHealth));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
