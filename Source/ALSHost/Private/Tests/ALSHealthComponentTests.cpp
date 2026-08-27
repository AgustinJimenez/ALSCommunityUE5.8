#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

// First real automated-gameplay-test pass for this project - see
// docs/testing.md for why this is necessary (no MCP tool can reach live PIE
// state) and AGENTS.md for the write-up of getting CQTest wired up. Spawns
// the actual BP_EnemyBasic Blueprint (not a synthetic stand-in) via
// FMapTestSpawner::CreateFromTempLevel, so this exercises the real Blueprint
// construction + UALSHealthComponent BeginPlay wiring end to end.
//
// FActorTestSpawner (CQTest's other spawn helper) was tried first and does
// NOT work for this - its own header says "no PIE loaded", confirmed
// directly: World::HasBegunPlay()/Actor::HasActorBegunPlay() were both false
// after spawning through it, so UALSHealthComponent::BeginPlay() (which sets
// CurrentHealth = MaxHealth) never ran. FMapTestSpawner actually brings a
// world up for play, so BeginPlay fires correctly.
TEST_CLASS(ALSHealthComponentTests, "ALSHost.Combat")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Enemy = nullptr;
	UALSHealthComponent* Health = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnEnemy()
	{
		UClass* EnemyClass = LoadClass<AALSCharacter>(nullptr, TEXT("/Game/ALSHost/Characters/BP_EnemyBasic.BP_EnemyBasic_C"));
		if (!EnemyClass)
		{
			return;
		}

		Enemy = &Spawner->SpawnActor<AALSCharacter>(FActorSpawnParameters(), EnemyClass);
		Health = Enemy->FindComponentByClass<UALSHealthComponent>();
	}

	TEST_METHOD(BP_EnemyBasic_Spawned_HasHealthComponent)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() { SpawnEnemy(); })
			.Then([this]() { ASSERT_THAT(IsNotNull(Health)); });
	}

	TEST_METHOD(BP_EnemyBasic_Spawned_StartsAtMaxHealth)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() { SpawnEnemy(); })
			.Then([this]() {
				ASSERT_THAT(IsNotNull(Health));
				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), Health->MaxHealth, 0.01f));
				ASSERT_THAT(IsFalse(Health->IsDead()));
			});
	}

	TEST_METHOD(ApplyDamage_ReducesHealth_ByDamageAmount)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() { SpawnEnemy(); })
			.Then([this]() {
				ASSERT_THAT(IsNotNull(Health));
				const float StartHealth = Health->GetCurrentHealth();

				UGameplayStatics::ApplyDamage(Enemy, 25.f, nullptr, nullptr, UDamageType::StaticClass());

				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), StartHealth - 25.f, 0.01f));
				ASSERT_THAT(IsFalse(Health->IsDead()));
			});
	}

	TEST_METHOD(ApplyLethalDamage_MarksDead_AndClampsAtZero)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() { SpawnEnemy(); })
			.Then([this]() {
				ASSERT_THAT(IsNotNull(Health));

				UGameplayStatics::ApplyDamage(Enemy, 99999.f, nullptr, nullptr, UDamageType::StaticClass());

				ASSERT_THAT(IsTrue(Health->IsDead()));
				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), 0.f, 0.01f));
			});
	}

	TEST_METHOD(ApplyDamage_AfterDeath_DoesNotGoNegative)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() { SpawnEnemy(); })
			.Then([this]() {
				ASSERT_THAT(IsNotNull(Health));

				UGameplayStatics::ApplyDamage(Enemy, 99999.f, nullptr, nullptr, UDamageType::StaticClass());
				UGameplayStatics::ApplyDamage(Enemy, 50.f, nullptr, nullptr, UDamageType::StaticClass());

				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), 0.f, 0.01f));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
