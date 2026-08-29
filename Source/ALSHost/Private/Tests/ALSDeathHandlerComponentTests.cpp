#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Combat/ALSDeathHandlerComponent.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

// Covers the gap where a character's health hitting 0 previously did
// nothing at all - no ragdoll, no respawn, the AI controller just stopped
// issuing move commands to a corpse that kept standing. See AGENTS.md.
TEST_CLASS(ALSDeathHandlerComponentTests, "ALSHost.Combat")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Enemy = nullptr;
	UALSHealthComponent* Health = nullptr;
	UALSDeathHandlerComponent* DeathHandler = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnEnemy(float RespawnDelaySeconds)
	{
		UClass* EnemyClass = LoadClass<AALSCharacter>(nullptr, TEXT("/Game/ALSHost/Characters/BP_EnemyBasic.BP_EnemyBasic_C"));
		if (!EnemyClass)
		{
			return;
		}

		Enemy = &Spawner->SpawnActor<AALSCharacter>(FActorSpawnParameters(), EnemyClass);
		Health = Enemy->FindComponentByClass<UALSHealthComponent>();

		// BP_EnemyBasic has no UALSDeathHandlerComponent placed in its own
		// tree (see AGENTS.md - only the player Blueprint has one), so add
		// one dynamically for the test rather than requiring content changes
		// just to exercise this component in isolation.
		DeathHandler = NewObject<UALSDeathHandlerComponent>(Enemy);
		DeathHandler->RespawnDelaySeconds = RespawnDelaySeconds;
		DeathHandler->RegisterComponent();
	}

	TEST_METHOD(LethalDamage_RagdollsMesh_AndStopsMovement)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() { SpawnEnemy(0.f); })
			.Then([this]() {
				ASSERT_THAT(IsNotNull(Health));
				ASSERT_THAT(IsNotNull(DeathHandler));

				UGameplayStatics::ApplyDamage(Enemy, 99999.f, nullptr, nullptr, UDamageType::StaticClass());

				ASSERT_THAT(IsTrue(Health->IsDead()));
				ASSERT_THAT(IsTrue(DeathHandler->IsRagdolling()));
				ASSERT_THAT(IsTrue(Enemy->GetMesh()->IsSimulatingPhysics()));
			});
	}

	TEST_METHOD(ZeroRespawnDelay_StaysRagdolled_NeverRespawns)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() { SpawnEnemy(0.f); })
			.Then([this]() { UGameplayStatics::ApplyDamage(Enemy, 99999.f, nullptr, nullptr, UDamageType::StaticClass()); })
			.WaitDelay(FTimespan::FromSeconds(0.5))
			.Then([this]() {
				ASSERT_THAT(IsTrue(DeathHandler->IsRagdolling()));
				ASSERT_THAT(IsTrue(Health->IsDead()));
			});
	}

	TEST_METHOD(PositiveRespawnDelay_RestoresFullHealth_AndClearsRagdoll)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() { SpawnEnemy(0.2f); })
			.Then([this]() { UGameplayStatics::ApplyDamage(Enemy, 99999.f, nullptr, nullptr, UDamageType::StaticClass()); })
			.WaitDelay(FTimespan::FromSeconds(0.5))
			.Then([this]() {
				ASSERT_THAT(IsFalse(DeathHandler->IsRagdolling()));
				ASSERT_THAT(IsFalse(Health->IsDead()));
				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), Health->MaxHealth, 0.01f));
				ASSERT_THAT(IsFalse(Enemy->GetMesh()->IsSimulatingPhysics()));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
