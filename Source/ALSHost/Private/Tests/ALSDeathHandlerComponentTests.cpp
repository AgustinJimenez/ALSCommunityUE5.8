#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Combat/ALSDeathHandlerComponent.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Inventory/ALSItemPickup.h"
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

	void SpawnEnemy(float RespawnDelaySeconds, FName LootItemID = NAME_None, int32 LootQuantity = 1, float LootDropChance = 1.f)
	{
		UClass* EnemyClass = LoadClass<AALSCharacter>(nullptr, TEXT("/Game/ALSHost/Characters/BP_EnemyBasic.BP_EnemyBasic_C"));
		if (!EnemyClass)
		{
			return;
		}

		Enemy = &Spawner->SpawnActor<AALSCharacter>(FActorSpawnParameters(), EnemyClass);
		Health = Enemy->FindComponentByClass<UALSHealthComponent>();

		// BP_EnemyBasic now has its own real UALSDeathHandlerComponent baked
		// in (see the "enemies drop loot" AGENTS.md entry) - reuse THAT one
		// rather than adding a second, independent instance. Adding a second
		// one used to be correct (see git history) back when the Blueprint
		// had none of its own, but doing that now means two components both
		// bind Health->OnDeath and both react - e.g. both spawning loot, or
		// the Blueprint's own baked defaults (RespawnDelaySeconds=0,
		// LootDropChance=0.75) firing independently of whatever this test
		// configured on a *second* component, silently breaking assertions
		// that assumed only one component's config was in play.
		DeathHandler = Enemy->FindComponentByClass<UALSDeathHandlerComponent>();
		if (DeathHandler)
		{
			DeathHandler->RespawnDelaySeconds = RespawnDelaySeconds;
			DeathHandler->LootItemID = LootItemID;
			DeathHandler->LootQuantity = LootQuantity;
			DeathHandler->LootDropChance = LootDropChance;
		}
	}

	int32 CountItemPickupsWithID(FName ItemID) const
	{
		TArray<AActor*> Pickups;
		UGameplayStatics::GetAllActorsOfClass(&Spawner->GetWorld(), AALSItemPickup::StaticClass(), Pickups);

		int32 Count = 0;
		for (AActor* PickupActor : Pickups)
		{
			if (const AALSItemPickup* Pickup = Cast<AALSItemPickup>(PickupActor); Pickup && Pickup->ItemID == ItemID)
			{
				++Count;
			}
		}
		return Count;
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

	// All three loot tests compare against a baseline count taken right
	// before triggering death, rather than asserting an absolute 0/1 - a
	// stray AALSItemPickup possibly left behind by another TEST_METHOD's
	// FMapTestSpawner temp world (unconfirmed whether that can happen, but
	// this makes the assertion robust either way) would otherwise produce a
	// false failure unrelated to this test's own logic.

	TEST_METHOD(PermanentDeath_WithLootConfigured_SpawnsItemPickupAtDeathLocation)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnEnemy(0.f, TEXT("Ammo_Rifle"), 15);
				const int32 BaselineCount = CountItemPickupsWithID(TEXT("Ammo_Rifle"));

				UGameplayStatics::ApplyDamage(Enemy, 99999.f, nullptr, nullptr, UDamageType::StaticClass());

				ASSERT_THAT(AreEqual(CountItemPickupsWithID(TEXT("Ammo_Rifle")), BaselineCount + 1));
			});
	}

	TEST_METHOD(RespawningDeath_NeverSpawnsLoot_EvenWithLootConfigured)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnEnemy(0.2f, TEXT("Ammo_Rifle"), 15);
				const int32 BaselineCount = CountItemPickupsWithID(TEXT("Ammo_Rifle"));

				UGameplayStatics::ApplyDamage(Enemy, 99999.f, nullptr, nullptr, UDamageType::StaticClass());

				ASSERT_THAT(AreEqual(CountItemPickupsWithID(TEXT("Ammo_Rifle")), BaselineCount));
			});
	}

	TEST_METHOD(ZeroDropChance_NeverSpawnsLoot)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnEnemy(0.f, TEXT("Ammo_Rifle"), 15, 0.f);
				const int32 BaselineCount = CountItemPickupsWithID(TEXT("Ammo_Rifle"));

				UGameplayStatics::ApplyDamage(Enemy, 99999.f, nullptr, nullptr, UDamageType::StaticClass());

				ASSERT_THAT(AreEqual(CountItemPickupsWithID(TEXT("Ammo_Rifle")), BaselineCount));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
