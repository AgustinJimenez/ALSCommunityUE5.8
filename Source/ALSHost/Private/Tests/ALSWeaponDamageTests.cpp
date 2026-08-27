#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Character/ALSCharacter.h"

// Pure math, no BeginPlay dependency (ComputeDamageForHit only reads
// UPROPERTY defaults) - FActorTestSpawner (no PIE, no world BeginPlay) is
// the right, cheap tool here, unlike the BeginPlay-dependent Health/Stamina/
// EnemyAI tests which need FMapTestSpawner instead (see docs/testing.md).
TEST_CLASS(ALSWeaponDamageTests, "ALSHost.Weapon")
{
	FActorTestSpawner Spawner;
	UALSWeaponFireComponent* Weapon = nullptr;

	BEFORE_EACH()
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		ASSERT_THAT(IsNotNull(CharClass));

		AALSCharacter& Character = Spawner.SpawnActor<AALSCharacter>(FActorSpawnParameters(), CharClass);
		Weapon = Character.FindComponentByClass<UALSWeaponFireComponent>();
		ASSERT_THAT(IsNotNull(Weapon));
	}

	TEST_METHOD(PointBlankBodyShot_DealsBaseDamage)
	{
		const float Damage = Weapon->ComputeDamageForHit(NAME_None, 0.f);
		ASSERT_THAT(IsNear(Damage, Weapon->DamagePerShot, 0.01f));
	}

	TEST_METHOD(PointBlankHeadshot_AppliesHeadshotMultiplier)
	{
		const float Damage = Weapon->ComputeDamageForHit(Weapon->HeadBoneName, 0.f);
		ASSERT_THAT(IsNear(Damage, Weapon->DamagePerShot * Weapon->HeadshotMultiplier, 0.01f));
	}

	TEST_METHOD(ShotAtMaxRange_AppliesMinDamageMultiplier)
	{
		const float Damage = Weapon->ComputeDamageForHit(NAME_None, Weapon->MaxRange);
		ASSERT_THAT(IsNear(Damage, Weapon->DamagePerShot * Weapon->MinDamageMultiplier, 0.01f));
	}

	TEST_METHOD(ShotBeyondFalloffStart_IsStrictlyLessThanBaseDamage)
	{
		const float MidRangeDistance = (Weapon->DamageFalloffStartRange + Weapon->MaxRange) * 0.5f;
		const float Damage = Weapon->ComputeDamageForHit(NAME_None, MidRangeDistance);
		ASSERT_THAT(IsTrue(Damage < Weapon->DamagePerShot));
		ASSERT_THAT(IsTrue(Damage > Weapon->DamagePerShot * Weapon->MinDamageMultiplier));
	}

	TEST_METHOD(ShotWithinFalloffStart_DealsFullDamage_RegardlessOfDistance)
	{
		const float Damage = Weapon->ComputeDamageForHit(NAME_None, Weapon->DamageFalloffStartRange * 0.5f);
		ASSERT_THAT(IsNear(Damage, Weapon->DamagePerShot, 0.01f));
	}

	TEST_METHOD(DistantHeadshot_CombinesFalloffAndHeadshotMultipliers)
	{
		const float Damage = Weapon->ComputeDamageForHit(Weapon->HeadBoneName, Weapon->MaxRange);
		const float Expected = Weapon->DamagePerShot * Weapon->MinDamageMultiplier * Weapon->HeadshotMultiplier;
		ASSERT_THAT(IsNear(Damage, Expected, 0.01f));
	}
};

#endif // WITH_AUTOMATION_TESTS
