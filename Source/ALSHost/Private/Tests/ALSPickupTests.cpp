#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Components/ActorTestSpawner.h"
#include "Inventory/ALSItemPickup.h"
#include "Inventory/ALSHealthPickup.h"
#include "Inventory/ALSWeaponPickup.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/ALSInteractable.h"
#include "UI/ALSInventoryWidget.h"
#include "Blueprint/UserWidget.h"

// Interact-triggered pickups (press E, see AALSPickupBase) still need real
// physics/collision and BeginPlay-dependent component logic (see
// docs/testing.md), so FMapTestSpawner, not FActorTestSpawner.
TEST_CLASS(ALSPickupTests, "ALSHost.Inventory")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;
	AALSHealthPickup* Pickup = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnCharacterAt(const FVector& Location)
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		if (!CharClass)
		{
			return;
		}
		Character = &Spawner->SpawnActorAt<AALSCharacter>(Location, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
		// The temp level has no floor (see docs/testing.md) - keep the
		// character from free-falling for the test's duration.
		Character->GetCharacterMovement()->DisableMovement();
	}

	// Pickups no longer need to be swept into the character - Interact()
	// (IALSInteractable, what pressing E actually calls) runs synchronously
	// regardless of proximity, so spawn location doesn't matter for these
	// tests. Still spawned away from the character (rather than at the same
	// location) purely so a pickup's PhysicsActor collision doesn't
	// interpenetrate the character capsule at spawn.
	static constexpr float FarAwayOffset = 2000.f;

	TEST_METHOD(ItemPickup_Interacted_AddsToInventory_AndDestroysSelf)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAt(FVector::ZeroVector);
				ASSERT_THAT(IsNotNull(Character));

				AALSItemPickup& ItemPickup = Spawner->SpawnActorAt<AALSItemPickup>(FVector(FarAwayOffset, 0.f, 0.f), FRotator::ZeroRotator);
				ItemPickup.ItemID = TEXT("TestItem");
				ItemPickup.DisplayName = FText::FromString(TEXT("Test Item"));
				ItemPickup.Quantity = 3;

				IALSInteractable::Execute_Interact(&ItemPickup, Character);

				UALSInventoryComponent* Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
				ASSERT_THAT(IsNotNull(Inventory));
				ASSERT_THAT(IsTrue(Inventory->HasItem(TEXT("TestItem"), 3)));
			});
	}

	// Guards the pickup-physics change: Mesh is now the root and simulates
	// physics (PhysicsActor profile) so a pickup falls under gravity and
	// rests on the ground instead of floating at its placed height. Spawn
	// high up, far from the character (so no overlap fires), and confirm Z
	// actually decreases after a short real tick - this is the only
	// reliable way to observe physics motion at all, since every MCP
	// actor-query tool resolves to the editor world, not a live PIE world
	// (see docs/mcp-notes.md), so a floating-item report can't be diagnosed
	// by just reading actor transforms through the MCP bridge.
	TEST_METHOD(HealthPickup_SpawnedInAir_FallsUnderGravity)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				Pickup = &Spawner->SpawnActorAt<AALSHealthPickup>(FVector(FarAwayOffset, 0.f, 500.f), FRotator::ZeroRotator);
			})
			.WaitDelay(FTimespan::FromSeconds(0.5))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Pickup->GetActorLocation().Z < 490.f));
			});
	}

	TEST_METHOD(HealthPickup_InteractedByDamagedCharacter_Heals)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAt(FVector::ZeroVector);
				ASSERT_THAT(IsNotNull(Character));

				UALSHealthComponent* CharHealth = Character->FindComponentByClass<UALSHealthComponent>();
				ASSERT_THAT(IsNotNull(CharHealth));
				CharHealth->Heal(-40.f);

				AALSHealthPickup& HealthPickup = Spawner->SpawnActorAt<AALSHealthPickup>(FVector(FarAwayOffset, 0.f, 0.f), FRotator::ZeroRotator);
				HealthPickup.HealAmount = 25.f;
				IALSInteractable::Execute_Interact(&HealthPickup, Character);

				UALSHealthComponent* Health = Character->FindComponentByClass<UALSHealthComponent>();
				ASSERT_THAT(IsNotNull(Health));
				// Started at MaxHealth(100) - 40 damage = 60, then +25 heal = 85.
				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), Health->MaxHealth - 40.f + 25.f, 0.5f));
			});
	}

	TEST_METHOD(HealthPickup_InteractedByFullHealthCharacter_IsNotConsumed)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAt(FVector::ZeroVector);
				ASSERT_THAT(IsNotNull(Character));
				Pickup = &Spawner->SpawnActorAt<AALSHealthPickup>(FVector(FarAwayOffset, 0.f, 0.f), FRotator::ZeroRotator);
				IALSInteractable::Execute_Interact(Pickup, Character);

				ASSERT_THAT(IsFalse(Pickup->IsActorBeingDestroyed()));
			});
	}

	TEST_METHOD(WeaponPickup_Interacted_EquipsOverlayAndGrantsAmmo)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAt(FVector::ZeroVector);
				ASSERT_THAT(IsNotNull(Character));
				ASSERT_THAT(IsTrue(Character->GetOverlayState() != EALSOverlayState::Rifle));

				AALSWeaponPickup& WeaponPickup = Spawner->SpawnActorAt<AALSWeaponPickup>(FVector(FarAwayOffset, 0.f, 0.f), FRotator::ZeroRotator);
				WeaponPickup.OverlayStateToEquip = EALSOverlayState::Rifle;
				WeaponPickup.WeaponItemID = TEXT("Weapon_Rifle");
				WeaponPickup.AmmoItemID = TEXT("Ammo_Rifle");
				WeaponPickup.BonusAmmoQuantity = 60;
				IALSInteractable::Execute_Interact(&WeaponPickup, Character);

				ASSERT_THAT(IsTrue(Character->GetOverlayState() == EALSOverlayState::Rifle));

				UALSInventoryComponent* Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
				ASSERT_THAT(IsNotNull(Inventory));
				ASSERT_THAT(IsTrue(Inventory->HasItem(TEXT("Ammo_Rifle"), 60)));

				// Picking the weapon back up should also have recorded it as
				// an equippable inventory entry - lets the player re-equip it
				// from the inventory panel later (see UALSInventoryWidget)
				// even after switching to a different overlay state.
				ASSERT_THAT(IsTrue(Inventory->HasItem(TEXT("Weapon_Rifle"), 1)));
				const FALSInventoryItem* WeaponItem = Inventory->GetItems().FindByPredicate(
					[](const FALSInventoryItem& Item) { return Item.ItemID == TEXT("Weapon_Rifle"); });
				ASSERT_THAT(IsNotNull(WeaponItem));
				ASSERT_THAT(IsTrue(WeaponItem->bEquippable));
				ASSERT_THAT(IsTrue(WeaponItem->EquipOverlayState == EALSOverlayState::Rifle));
			});
	}

	// User report: pick up a Pistol then a Rifle (ends up equipped with the
	// Rifle, since the most recently picked-up weapon wins), then click the
	// Pistol's row in the inventory panel to switch back to it - doesn't
	// work. First locks in that picking up a SECOND weapon doesn't clobber
	// the first one's equippable inventory entry.
	TEST_METHOD(PickingUpPistolThenRifle_KeepsBothEquippableInInventory)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAt(FVector::ZeroVector);
				ASSERT_THAT(IsNotNull(Character));

				AALSWeaponPickup& PistolPickup = Spawner->SpawnActorAt<AALSWeaponPickup>(FVector(FarAwayOffset, 0.f, 0.f), FRotator::ZeroRotator);
				PistolPickup.OverlayStateToEquip = EALSOverlayState::PistolOneHanded;
				PistolPickup.WeaponItemID = TEXT("Weapon_Pistol");
				PistolPickup.BonusAmmoQuantity = 0;
				IALSInteractable::Execute_Interact(&PistolPickup, Character);

				ASSERT_THAT(IsTrue(Character->GetOverlayState() == EALSOverlayState::PistolOneHanded));

				AALSWeaponPickup& RiflePickup = Spawner->SpawnActorAt<AALSWeaponPickup>(FVector(FarAwayOffset, 0.f, 0.f), FRotator::ZeroRotator);
				RiflePickup.OverlayStateToEquip = EALSOverlayState::Rifle;
				RiflePickup.WeaponItemID = TEXT("Weapon_Rifle");
				RiflePickup.BonusAmmoQuantity = 0;
				IALSInteractable::Execute_Interact(&RiflePickup, Character);

				ASSERT_THAT(IsTrue(Character->GetOverlayState() == EALSOverlayState::Rifle));

				UALSInventoryComponent* Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
				ASSERT_THAT(IsNotNull(Inventory));
				ASSERT_THAT(IsTrue(Inventory->HasItem(TEXT("Weapon_Pistol"), 1)));
				ASSERT_THAT(IsTrue(Inventory->HasItem(TEXT("Weapon_Rifle"), 1)));
			});
	}

	// Second half of the same repro: with the Rifle currently equipped,
	// clicking the Pistol's inventory row (UALSInventoryWidget::EquipItem,
	// what HandleRowClicked actually calls) should switch back to it.
	TEST_METHOD(EquipItem_WhileDifferentWeaponEquipped_SwitchesToClickedWeapon)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAt(FVector::ZeroVector);
				ASSERT_THAT(IsNotNull(Character));

				APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
				ASSERT_THAT(IsNotNull(PC));
				PC->Possess(Character);

				UALSInventoryComponent* Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
				ASSERT_THAT(IsNotNull(Inventory));
				Inventory->AddItem(TEXT("Weapon_Pistol"), FText::FromString(TEXT("Pistol")), 1, 1, /*bEquippable=*/true, EALSOverlayState::PistolOneHanded);
				Inventory->AddItem(TEXT("Weapon_Rifle"), FText::FromString(TEXT("Rifle")), 1, 1, /*bEquippable=*/true, EALSOverlayState::Rifle);
				Character->SetOverlayState(EALSOverlayState::Rifle);
				ASSERT_THAT(IsTrue(Character->GetOverlayState() == EALSOverlayState::Rifle));

				UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/ALSHost/UI/WBP_InventoryPanel.WBP_InventoryPanel_C"));
				ASSERT_THAT(IsNotNull(WidgetClass));
				UALSInventoryWidget* Widget = CreateWidget<UALSInventoryWidget>(PC, WidgetClass);
				ASSERT_THAT(IsNotNull(Widget));

				ASSERT_THAT(IsTrue(Widget->EquipItem(TEXT("Weapon_Pistol"))));
				ASSERT_THAT(IsTrue(Character->GetOverlayState() == EALSOverlayState::PistolOneHanded));
			});
	}
};

// Pure construction-time check, no world/BeginPlay needed - guards against
// AALSWeaponPickup silently reverting to the inherited Cube/Sphere
// placeholder (which every real weapon mesh in this project, being a
// SkeletalMesh, can't be assigned to at all).
TEST_CLASS(ALSWeaponPickupMeshTests, "ALSHost.Inventory")
{
	FActorTestSpawner Spawner;

	TEST_METHOD(WeaponPickup_HasSkeletalMeshComponent_AndHidesInheritedStaticMesh)
	{
		AALSWeaponPickup& Pickup = Spawner.SpawnActor<AALSWeaponPickup>();
		ASSERT_THAT(IsNotNull(Pickup.WeaponMesh));
		ASSERT_THAT(IsFalse(Pickup.GetMesh() ? Pickup.GetMesh()->IsVisible() : false));
	}
};

// Guards against AALSHealthPickup silently reverting to the inherited
// placeholder Sphere - both the box body (inherited Mesh, repurposed) and
// the CoverMesh should have a real medkit static mesh assigned by default.
TEST_CLASS(ALSHealthPickupMeshTests, "ALSHost.Inventory")
{
	FActorTestSpawner Spawner;

	TEST_METHOD(HealthPickup_HasBoxAndCoverMeshesAssigned)
	{
		AALSHealthPickup& Pickup = Spawner.SpawnActor<AALSHealthPickup>();
		ASSERT_THAT(IsNotNull(Pickup.GetMesh()));
		ASSERT_THAT(IsNotNull(Pickup.GetMesh()->GetStaticMesh()));
		ASSERT_THAT(IsNotNull(Pickup.CoverMesh));
		ASSERT_THAT(IsNotNull(Pickup.CoverMesh->GetStaticMesh()));
	}
};

#endif // WITH_AUTOMATION_TESTS
