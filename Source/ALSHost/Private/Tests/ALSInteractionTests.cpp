#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Components/ActorTestSpawner.h"
#include "Interaction/ALSDoor.h"
#include "Interaction/ALSLootContainer.h"
#include "Interaction/ALSInteractionComponent.h"
#include "Weapon/ALSMeleeComponent.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Inventory/ALSItemPickup.h"
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

		APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
		ASSERT_THAT(IsNotNull(PC));
		PC->Possess(Character);

		// Possessing resets the movement mode (confirmed - calling
		// DisableMovement() before Possess() here left the character
		// falling indefinitely for the rest of the test despite it, since
		// the temp level has no floor), so disable movement AFTER
		// possessing, not before.
		Character->GetCharacterMovement()->DisableMovement();
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

	// Regression guard: the facing check must use where the CAMERA/controller
	// is looking, not the pawn's own body-forward vector - ALS's rotation
	// modes let a character's body lag behind or fully diverge from camera
	// look direction (e.g. standing still after strafing in Velocity
	// Direction mode), which silently broke detection of anything the
	// player was visually looking at but not physically facing with their
	// character. Body faces one way (+X, via SpawnActorAt's rotation),
	// camera looks a completely different way (+Y, via SetControlRotation)
	// straight at the door - only passes if the facing check uses the
	// controller's rotation.
	TEST_METHOD(InteractionComponent_TryInteract_UsesCameraFacing_NotBodyFacing)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAndPossess(FVector::ZeroVector);
				Character->SetActorRotation(FRotator(0.f, 0.f, 0.f)); // Body faces +X.

				APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
				ASSERT_THAT(IsNotNull(PC));
				PC->SetControlRotation(FRotator(0.f, 90.f, 0.f)); // Camera looks +Y.

				Interaction = NewObject<UALSInteractionComponent>(Character);
				Interaction->RegisterComponent();

				// Placed along +Y (where the camera looks), not +X (where the
				// body faces) - a body-forward-based facing check would miss
				// this entirely.
				Door = &Spawner->SpawnActorAt<AALSDoor>(FVector(0.f, 150.f, 0.f), FRotator::ZeroRotator);
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

	// Same direct-TryInteract() pattern as InteractionComponent_TryInteract_HitsDoorInFront_OpensIt
	// above (not real Enhanced Input injection - FInputTestActions' async,
	// tick-delayed injection combined with this floorless temp map's
	// continuous-fall-despite-DisableMovement quirk made a real-input
	// version of this test unreliably flaky here, for reasons specific to
	// the test harness, not the production Interact system itself -
	// confirmed separately via a fixed-point LineTraceSingleByChannel call
	// that this pickup's collision correctly blocks the Visibility channel
	// the interact trace uses. Real Enhanced Input injection through this
	// exact input-binding path is still covered by
	// PressingToggleAction_ThroughRealInput_TogglesInventoryWidget
	// (ALSInventoryUITests.cpp); what this test adds is the pickup-specific
	// interact wiring.
	TEST_METHOD(InteractionComponent_TryInteract_HitsItemPickupInFront_PicksItUp)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAndPossess(FVector::ZeroVector);

				Interaction = NewObject<UALSInteractionComponent>(Character);
				Interaction->RegisterComponent();

				AALSItemPickup& ItemPickup = Spawner->SpawnActorAt<AALSItemPickup>(FVector(150.f, 0.f, 0.f), FRotator::ZeroRotator);
				ItemPickup.ItemID = TEXT("TestItem");
				ItemPickup.DisplayName = FText::FromString(TEXT("Test Item"));
				ItemPickup.Quantity = 1;
				// Pickups simulate physics now (see AALSPickupBase), and this
				// temp level has no floor - freeze it in place so it doesn't
				// fall out of the trace's path before the interact check runs.
				ItemPickup.GetMesh()->SetSimulatePhysics(false);
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				// A ground-resting item sits below a dead-level (Pitch=0)
				// trace from a raised third-person camera - a real player
				// has to actually look down at it, same as any raycast-based
				// interact system. Aim the camera at the item's real location
				// to simulate that, rather than relying on default control
				// rotation the way the (much taller) Door test can.
				APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
				FVector CamLoc;
				FRotator CamRot;
				PC->GetPlayerViewPoint(CamLoc, CamRot);
				const AALSItemPickup* ItemPickup = Cast<AALSItemPickup>(UGameplayStatics::GetActorOfClass(&Spawner->GetWorld(), AALSItemPickup::StaticClass()));
				ASSERT_THAT(IsNotNull(ItemPickup));
				PC->SetControlRotation((ItemPickup->GetActorLocation() - CamLoc).Rotation());
			})
			// ALS's third-person camera interpolates toward the control
			// rotation rather than snapping instantly - GetPlayerViewPoint
			// called in the same tick as SetControlRotation still reports
			// the old, pre-turn camera angle. Give it a moment to catch up.
			.WaitDelay(FTimespan::FromSeconds(0.3))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Interaction->TryInteract()));

				UALSInventoryComponent* Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
				ASSERT_THAT(IsNotNull(Inventory));
				ASSERT_THAT(IsTrue(Inventory->HasItem(TEXT("TestItem"), 1)));
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
