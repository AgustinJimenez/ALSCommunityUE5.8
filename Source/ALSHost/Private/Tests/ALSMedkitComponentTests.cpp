#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Combat/ALSMedkitComponent.h"
#include "Combat/ALSHealthComponent.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Character/ALSCharacter.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Real world/tick needed for the apply timer and movement-based cancel, so
// FMapTestSpawner (not FActorTestSpawner) throughout - see docs/testing.md.
TEST_CLASS(ALSMedkitComponentTests, "ALSHost.Combat")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;
	UALSMedkitComponent* Medkit = nullptr;
	UALSHealthComponent* Health = nullptr;
	UALSInventoryComponent* Inventory = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnCharacterWithMedkitComponent()
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		ASSERT_THAT(IsNotNull(CharClass));

		Character = &Spawner->SpawnActorAt<AALSCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
		Character->GetCharacterMovement()->DisableMovement();

		APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
		ASSERT_THAT(IsNotNull(PC));
		PC->Possess(Character);

		Medkit = NewObject<UALSMedkitComponent>(Character);
		Medkit->ApplyDurationSeconds = 0.2f;
		Medkit->RegisterComponent();
		// RegisterComponent() on an already-playing actor still runs
		// BeginPlay() (which is where OnHealthChanged gets bound), but not
		// synchronously within this same call - it's deferred by at least a
		// tick. Tests that need that binding live (the damage-cancel test)
		// must wait a beat after this before acting, or the binding won't
		// have happened yet.

		Health = Character->FindComponentByClass<UALSHealthComponent>();
		Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
		ASSERT_THAT(IsNotNull(Health));
		ASSERT_THAT(IsNotNull(Inventory));
	}

	TEST_METHOD(EquipMedkit_SetsBoxOverlayState_AndAttachesRealMesh)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterWithMedkitComponent();
				Medkit->EquipMedkit();

				ASSERT_THAT(IsTrue(Character->GetOverlayState() == EALSOverlayState::Box));
				ASSERT_THAT(IsNotNull(Character->StaticMesh));
				ASSERT_THAT(IsTrue(Character->StaticMesh->GetStaticMesh() == Medkit->MedkitHeldMesh));
			});
	}

	TEST_METHOD(TryStartApplyingMedkit_WithoutMedkitEquipped_Fails)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterWithMedkitComponent();
				Inventory->AddItem(Medkit->MedkitItemID, FText::FromString(TEXT("Medkit")), 1, 1);
				// Deliberately not calling EquipMedkit() - overlay state stays Default.
				ASSERT_THAT(IsFalse(Medkit->TryStartApplyingMedkit()));
				ASSERT_THAT(IsFalse(Medkit->IsApplyingMedkit()));
			});
	}

	TEST_METHOD(TryStartApplyingMedkit_EquippedButNotInInventory_Fails)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterWithMedkitComponent();
				Medkit->EquipMedkit();
				// Inventory deliberately left empty.
				ASSERT_THAT(IsFalse(Medkit->TryStartApplyingMedkit()));
			});
	}

	TEST_METHOD(TryStartApplyingMedkit_FullHealth_Fails)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterWithMedkitComponent();
				Medkit->EquipMedkit();
				Inventory->AddItem(Medkit->MedkitItemID, FText::FromString(TEXT("Medkit")), 1, 1);
				ASSERT_THAT(IsTrue(Health->GetCurrentHealth() >= Health->MaxHealth));
				ASSERT_THAT(IsFalse(Medkit->TryStartApplyingMedkit()));
			});
	}

	TEST_METHOD(ApplyingMedkit_CompletesAfterDuration_HealsAndConsumesOne)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterWithMedkitComponent();
				// Damage by more than HealAmount so the expected result
				// stays clear of Heal()'s clamp-to-MaxHealth behavior -
				// keeps this assertion unambiguous either way.
				Health->Heal(-60.0f);
				Medkit->EquipMedkit();
				Inventory->AddItem(Medkit->MedkitItemID, FText::FromString(TEXT("Medkit")), 1, 1);

				ASSERT_THAT(IsTrue(Medkit->TryStartApplyingMedkit()));
				ASSERT_THAT(IsTrue(Medkit->IsApplyingMedkit()));
			})
			.WaitDelay(FTimespan::FromSeconds(0.5))
			.Then([this]() {
				ASSERT_THAT(IsFalse(Medkit->IsApplyingMedkit()));
				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), Health->MaxHealth - 60.0f + Medkit->HealAmount, 0.5f));
				ASSERT_THAT(IsFalse(Inventory->HasItem(Medkit->MedkitItemID, 1)));
			});
	}

	TEST_METHOD(MovingWhileApplying_CancelsWithoutHealingOrConsuming)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterWithMedkitComponent();
				Health->Heal(-40.0f);
				Medkit->EquipMedkit();
				Inventory->AddItem(Medkit->MedkitItemID, FText::FromString(TEXT("Medkit")), 1, 1);
				Medkit->ApplyDurationSeconds = 5.0f;

				ASSERT_THAT(IsTrue(Medkit->TryStartApplyingMedkit()));

				// DisableMovement() (set in SpawnCharacterWithMedkitComponent)
				// stops CharacterMovementComponent's own simulation from
				// producing velocity, so directly launch the character to
				// simulate "the player moved" without needing real input
				// injection - GetVelocity() is what TickComponent checks.
				Character->LaunchCharacter(FVector(500.f, 0.f, 0.f), true, true);
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				ASSERT_THAT(IsFalse(Medkit->IsApplyingMedkit()));
				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), Health->MaxHealth - 40.0f, 0.5f));
				ASSERT_THAT(IsTrue(Inventory->HasItem(Medkit->MedkitItemID, 1)));
			});
	}

	TEST_METHOD(TakingDamageWhileApplying_Cancels)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterWithMedkitComponent();
				Health->Heal(-10.0f);
				Medkit->EquipMedkit();
				Inventory->AddItem(Medkit->MedkitItemID, FText::FromString(TEXT("Medkit")), 1, 1);
				Medkit->ApplyDurationSeconds = 5.0f;
			})
			// RegisterComponent() defers BeginPlay (where OnHealthChanged
			// gets bound) by at least a tick - wait for it before relying
			// on that binding below.
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Medkit->TryStartApplyingMedkit()));
				Health->Heal(-10.0f);
			})
			.WaitDelay(FTimespan::FromSeconds(0.1))
			.Then([this]() {
				ASSERT_THAT(IsFalse(Medkit->IsApplyingMedkit()));
				ASSERT_THAT(IsTrue(Inventory->HasItem(Medkit->MedkitItemID, 1)));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
