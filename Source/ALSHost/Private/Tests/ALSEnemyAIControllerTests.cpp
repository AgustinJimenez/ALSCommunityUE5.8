#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "AI/ALSEnemyAIController.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

// Covers the distance-gated parts of AALSEnemyAIController's chase/attack
// FSM that don't depend on navigation data (MoveToActor needs a built
// NavMesh, which a bare FMapTestSpawner temp level doesn't have - so the
// "enemy actually walks toward the player" case isn't covered here; that
// would need either a real test map with baked navigation or programmatic
// nav-mesh generation in test setup, neither attempted yet).
//
// BP_EnemyBasic's AIControllerClass only auto-possesses for actors placed
// in a level before play (AutoPossessAI=PlacedInWorld, inherited from ALS's
// own ALS_AIBP) - actors spawned at runtime, like in this test, need
// explicit Controller->Possess(Pawn).
TEST_CLASS(ALSEnemyAIControllerTests, "ALSHost.AI")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Player = nullptr;
	AALSCharacter* Enemy = nullptr;
	AALSEnemyAIController* EnemyController = nullptr;
	UALSHealthComponent* PlayerHealth = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnPlayerAndEnemyAt(float EnemyDistanceFromPlayer)
	{
		UClass* PlayerClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		UClass* EnemyClass = LoadClass<AALSCharacter>(nullptr, TEXT("/Game/ALSHost/Characters/BP_EnemyBasic.BP_EnemyBasic_C"));
		if (!PlayerClass || !EnemyClass)
		{
			return;
		}

		Player = &Spawner->SpawnActorAt<AALSCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters(), PlayerClass);
		PlayerHealth = Player->FindComponentByClass<UALSHealthComponent>();

		// AALSEnemyAIController finds its target via
		// UGameplayStatics::GetPlayerPawn(this, 0), which resolves through
		// the world's first local player's PlayerController - a temp PIE
		// world from FMapTestSpawner already has one (possessing whatever
		// default pawn it auto-spawned), so re-possess our own character
		// with that SAME controller rather than spawning a disconnected one.
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0))
		{
			PC->Possess(Player);
		}

		Enemy = &Spawner->SpawnActorAt<AALSCharacter>(FVector(EnemyDistanceFromPlayer, 0.f, 0.f), FRotator::ZeroRotator, FActorSpawnParameters(), EnemyClass);

		EnemyController = &Spawner->SpawnActor<AALSEnemyAIController>();
		EnemyController->Possess(Enemy);
	}

	TEST_METHOD(Enemy_WithinAttackRange_DamagesPlayerOverTime)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				// AttackRange default is 150 - well inside it.
				SpawnPlayerAndEnemyAt(100.f);
				ASSERT_THAT(IsNotNull(PlayerHealth));
			})
			// AttackIntervalSeconds default is 1.5 - long enough for one attack tick.
			.WaitDelay(FTimespan::FromSeconds(2.0))
			.Then([this]() {
				ASSERT_THAT(IsTrue(PlayerHealth->GetCurrentHealth() < PlayerHealth->MaxHealth));
			});
	}

	TEST_METHOD(Enemy_BeyondSightRange_DoesNotDamagePlayer)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				// SightRange default is 1500 - well outside it.
				SpawnPlayerAndEnemyAt(5000.f);
				ASSERT_THAT(IsNotNull(PlayerHealth));
			})
			.WaitDelay(FTimespan::FromSeconds(1.0))
			.Then([this]() {
				ASSERT_THAT(IsNear(PlayerHealth->GetCurrentHealth(), PlayerHealth->MaxHealth, 0.01f));
			});
	}

	TEST_METHOD(Enemy_WithSelfDead_StopsAttacking)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnPlayerAndEnemyAt(100.f);
				ASSERT_THAT(IsNotNull(Enemy));

				UALSHealthComponent* EnemyHealth = Enemy->FindComponentByClass<UALSHealthComponent>();
				ASSERT_THAT(IsNotNull(EnemyHealth));
				UGameplayStatics::ApplyDamage(Enemy, 99999.f, nullptr, nullptr, nullptr);
				ASSERT_THAT(IsTrue(EnemyHealth->IsDead()));
			})
			.WaitDelay(FTimespan::FromSeconds(2.0))
			.Then([this]() {
				ASSERT_THAT(IsNear(PlayerHealth->GetCurrentHealth(), PlayerHealth->MaxHealth, 0.01f));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
