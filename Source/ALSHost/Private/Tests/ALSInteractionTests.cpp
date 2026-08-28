#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Components/ActorTestSpawner.h"
#include "Interaction/ALSDoor.h"
#include "Interaction/ALSLootContainer.h"
#include "Interaction/ALSInteractionComponent.h"
#include "Weapon/ALSMeleeComponent.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Door/loot toggling itself is pure state, no BeginPlay dependency -
// FActorTestSpawner is enough (matches ALSWeaponDamageTests' reasoning).
TEST_CLASS(ALSDoorAndLootContainerLogicTests, "ALSHost.Interaction")
{
	FActorTestSpawner Spawner;

	TEST_METHOD(Door_Interact_TogglesOpenState)
	{
		AALSDoor& Door = Spawner.SpawnActor<AALSDoor>();
		ASSERT_THAT(IsFalse(Door.IsOpen()));

		Door.Interact_Implementation(nullptr);
		ASSERT_THAT(IsTrue(Door.IsOpen()));

		Door.Interact_Implementation(nullptr);
		ASSERT_THAT(IsFalse(Door.IsOpen()));
	}

	TEST_METHOD(LootContainer_Interact_GrantsConfiguredItems)
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		ASSERT_THAT(IsNotNull(CharClass));
		AALSCharacter& Character = Spawner.SpawnActor<AALSCharacter>(FActorSpawnParameters(), CharClass);

		AALSLootContainer& Loot = Spawner.SpawnActor<AALSLootContainer>();
		FALSLootEntry Entry;
		Entry.ItemID = TEXT("Ammo_Rifle");
		Entry.DisplayName = FText::FromString(TEXT("Rifle Ammo"));
		Entry.Quantity = 15;
		Entry.MaxStack = 90;
		Loot.LootItems.Add(Entry);

		ASSERT_THAT(IsFalse(Loot.IsOpened()));
		Loot.Interact_Implementation(&Character);
		ASSERT_THAT(IsTrue(Loot.IsOpened()));

		UALSInventoryComponent* Inventory = Character.FindComponentByClass<UALSInventoryComponent>();
		ASSERT_THAT(IsNotNull(Inventory));
		ASSERT_THAT(IsTrue(Inventory->HasItem(TEXT("Ammo_Rifle"), 15)));
	}

	TEST_METHOD(LootContainer_Interact_Twice_DoesNotDuplicateItems)
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		ASSERT_THAT(IsNotNull(CharClass));
		AALSCharacter& Character = Spawner.SpawnActor<AALSCharacter>(FActorSpawnParameters(), CharClass);

		AALSLootContainer& Loot = Spawner.SpawnActor<AALSLootContainer>();
		FALSLootEntry Entry;
		Entry.ItemID = TEXT("Ammo_Rifle");
		Entry.DisplayName = FText::FromString(TEXT("Rifle Ammo"));
		Entry.Quantity = 15;
		Entry.MaxStack = 90;
		Loot.LootItems.Add(Entry);

		Loot.Interact_Implementation(&Character);
		Loot.Interact_Implementation(&Character);

		UALSInventoryComponent* Inventory = Character.FindComponentByClass<UALSInventoryComponent>();
		ASSERT_THAT(IsNotNull(Inventory));
		ASSERT_THAT(IsTrue(Inventory->GetItemQuantity(TEXT("Ammo_Rifle")) == 15));
	}
};

// Door swing-over-time and the trace-based interaction/melee components need
// a real, ticking world - FMapTestSpawner (see docs/testing.md).
TEST_CLASS(ALSInteractionRuntimeTests, "ALSHost.Interaction")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;
	AALSCharacter* Target = nullptr;
	AALSDoor* Door = nullptr;
	UALSInteractionComponent* Interaction = nullptr;
	UALSMeleeComponent* Melee = nullptr;
	UALSHealthComponent* TargetHealth = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnCharacterAndPossess(const FVector& Location)
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		ASSERT_THAT(IsNotNull(CharClass));
		Character = &Spawner->SpawnActorAt<AALSCharacter>(Location, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
		Character->GetCharacterMovement()->DisableMovement();

		APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
		ASSERT_THAT(IsNotNull(PC));
		PC->Possess(Character);
	}

	TEST_METHOD(Door_Interact_SwingsTowardTargetYaw_OverTime)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				Door = &Spawner->SpawnActorAt<AALSDoor>(FVector::ZeroVector, FRotator::ZeroRotator);
				Door->Interact_Implementation(nullptr);
				ASSERT_THAT(IsTrue(Door->IsOpen()));
			})
			.WaitDelay(FTimespan::FromSeconds(1.0))
			.Then([this]() {
				// SwingSpeedDegreesPerSecond default (180) should have carried
				// the hinge well past halfway to OpenYawDegrees (100) in 1s.
				const float CurrentYaw = Door->HingeRoot->GetRelativeRotation().Yaw;
				ASSERT_THAT(IsTrue(CurrentYaw > 50.f));
			});
	}

	TEST_METHOD(InteractionComponent_TryInteract_HitsDoorInFront_OpensIt)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAndPossess(FVector::ZeroVector);

				Interaction = NewObject<UALSInteractionComponent>(Character);
				Interaction->RegisterComponent();

				Door = &Spawner->SpawnActorAt<AALSDoor>(FVector(150.f, 0.f, 0.f), FRotator::ZeroRotator);
				ASSERT_THAT(IsFalse(Door->IsOpen()));
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Interaction->TryInteract()));
				ASSERT_THAT(IsTrue(Door->IsOpen()));
			});
	}

	TEST_METHOD(MeleeComponent_Fist_DealsDamageToActorInRange)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAndPossess(FVector::ZeroVector);

				Melee = NewObject<UALSMeleeComponent>(Character);
				Melee->MeleeRange = 300.f;
				Melee->SweepRadius = 60.f;
				Melee->RegisterComponent();

				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				Target = &Spawner->SpawnActorAt<AALSCharacter>(FVector(150.f, 0.f, 0.f), FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Target->GetCharacterMovement()->DisableMovement();
				TargetHealth = Target->FindComponentByClass<UALSHealthComponent>();
				ASSERT_THAT(IsNotNull(TargetHealth));
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				const float HealthBefore = TargetHealth->GetCurrentHealth();
				ASSERT_THAT(IsTrue(Melee->TryMeleeAttack()));
				ASSERT_THAT(IsNear(TargetHealth->GetCurrentHealth(), HealthBefore - Melee->FistDamage, 0.5f));
			});
	}

	TEST_METHOD(MeleeComponent_WithKnifeInInventory_DealsBonusDamage)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAndPossess(FVector::ZeroVector);

				UALSInventoryComponent* Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
				ASSERT_THAT(IsNotNull(Inventory));
				Inventory->AddItem(TEXT("Weapon_Knife"), FText::FromString(TEXT("Knife")), 1, 1);

				Melee = NewObject<UALSMeleeComponent>(Character);
				Melee->MeleeRange = 300.f;
				Melee->SweepRadius = 60.f;
				Melee->RegisterComponent();
				ASSERT_THAT(IsTrue(Melee->HasKnifeEquipped()));

				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				Target = &Spawner->SpawnActorAt<AALSCharacter>(FVector(150.f, 0.f, 0.f), FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Target->GetCharacterMovement()->DisableMovement();
				TargetHealth = Target->FindComponentByClass<UALSHealthComponent>();
				ASSERT_THAT(IsNotNull(TargetHealth));
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				const float HealthBefore = TargetHealth->GetCurrentHealth();
				ASSERT_THAT(IsTrue(Melee->TryMeleeAttack()));
				const float ExpectedDamage = Melee->FistDamage + Melee->KnifeDamageBonus;
				ASSERT_THAT(IsNear(TargetHealth->GetCurrentHealth(), HealthBefore - ExpectedDamage, 0.5f));
			});
	}

	TEST_METHOD(MeleeComponent_SecondAttackDuringCooldown_IsRejected)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAndPossess(FVector::ZeroVector);

				Melee = NewObject<UALSMeleeComponent>(Character);
				Melee->AttackCooldownSeconds = 5.0f;
				Melee->RegisterComponent();
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Melee->TryMeleeAttack()));
				ASSERT_THAT(IsTrue(Melee->IsOnCooldown()));
				ASSERT_THAT(IsFalse(Melee->TryMeleeAttack()));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
