#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Combat/ALSObjectiveSubsystem.h"
#include "Combat/ALSHealthComponent.h"
#include "AI/ALSEnemyAIController.h"
#include "Character/ALSCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

// Covers the "defeat all enemies" objective tracking added alongside the
// death/respawn work - this project previously had no GameMode, no score,
// no win condition anywhere (see AGENTS.md). UALSObjectiveSubsystem is a
// UWorldSubsystem so it's auto-instantiated in FMapTestSpawner's temp world
// with no level placement needed.
TEST_CLASS(ALSObjectiveSubsystemTests, "ALSHost.Combat")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	UALSObjectiveSubsystem* Objective = nullptr;
	AALSCharacter* EnemyA = nullptr;
	AALSCharacter* EnemyB = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnEnemyWithController(AALSCharacter*& OutEnemy, const FVector& Location)
	{
		UClass* EnemyClass = LoadClass<AALSCharacter>(nullptr, TEXT("/Game/ALSHost/Characters/BP_EnemyBasic.BP_EnemyBasic_C"));
		ASSERT_THAT(IsNotNull(EnemyClass));

		OutEnemy = &Spawner->SpawnActorAt<AALSCharacter>(Location, FRotator::ZeroRotator, FActorSpawnParameters(), EnemyClass);

		AALSEnemyAIController* Controller = &Spawner->SpawnActor<AALSEnemyAIController>();
		Controller->Possess(OutEnemy);
	}

	TEST_METHOD(NoEnemiesSpawned_RemainingCountIsZero_AndNeverBroadcastsVictory)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				Objective = Spawner->GetWorld().GetSubsystem<UALSObjectiveSubsystem>();
				ASSERT_THAT(IsNotNull(Objective));
				ASSERT_THAT(AreEqual(Objective->GetRemainingEnemyCount(), 0));
				ASSERT_THAT(IsFalse(Objective->HasAnyTrackedEnemies()));
			});
	}

	TEST_METHOD(KillingAllTrackedEnemies_DropsRemainingCountToZero)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnEnemyWithController(EnemyA, FVector(0.f, 0.f, 0.f));
				SpawnEnemyWithController(EnemyB, FVector(500.f, 0.f, 0.f));

				Objective = Spawner->GetWorld().GetSubsystem<UALSObjectiveSubsystem>();
				ASSERT_THAT(IsNotNull(Objective));
				Objective->RefreshTrackedEnemies();

				ASSERT_THAT(AreEqual(Objective->GetRemainingEnemyCount(), 2));
				ASSERT_THAT(IsTrue(Objective->HasAnyTrackedEnemies()));

				UGameplayStatics::ApplyDamage(EnemyA, 99999.f, nullptr, nullptr, UDamageType::StaticClass());
			})
			.Then([this]() {
				ASSERT_THAT(AreEqual(Objective->GetRemainingEnemyCount(), 1));

				UGameplayStatics::ApplyDamage(EnemyB, 99999.f, nullptr, nullptr, UDamageType::StaticClass());
			})
			.Then([this]() {
				ASSERT_THAT(AreEqual(Objective->GetRemainingEnemyCount(), 0));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
