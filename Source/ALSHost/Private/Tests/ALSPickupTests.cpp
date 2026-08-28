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
#include "Library/ALSCharacterEnumLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "UI/ALSInventoryWidget.h"
#include "Blueprint/UserWidget.h"

// Overlap-triggered pickups need real physics/collision, which (like
// BeginPlay-dependent component logic - see docs/testing.md) needs
// FMapTestSpawner, not FActorTestSpawner.
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
	}

	// Pickups are spawned well away from the character, not on top of it -
	// a pickup's overlap (and therefore OnPickedUp) fires *synchronously*
	// during SpawnActor if it's already overlapping at spawn, which is
	// before any properties set on the returned reference (ItemID,
	// HealAmount, ...) take effect. Configure first, then SetActorLocation
	// with sweep to move it into the character and trigger the overlap for
	// real, matching how a dropped-then-walked-into pickup actually behaves.
	static constexpr float FarAwayOffset = 2000.f;

	TEST_METHOD(ItemPickup_OverlappedByCharacter_AddsToInventory_AndDestroysSelf)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAt(FVector::ZeroVector);
				ASSERT_THAT(IsNotNull(Character));

				AALSItemPickup& Pickup = Spawner->SpawnActorAt<AALSItemPickup>(FVector(FarAwayOffset, 0.f, 0.f), FRotator::ZeroRotator);
				Pickup.ItemID = TEXT("TestItem");
				Pickup.DisplayName = FText::FromString(TEXT("Test Item"));
				Pickup.Quantity = 3;
				Pickup.SetActorLocation(FVector::ZeroVector, /*bSweep=*/true);
			})
			.WaitDelay(FTimespan::FromSeconds(0.3))
			.Then([this]() {
				UALSInventoryComponent* Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
				ASSERT_THAT(IsNotNull(Inventory));
				ASSERT_THAT(IsTrue(Inventory->HasItem(TEXT("TestItem"), 3)));
			});
	}

	TEST_METHOD(HealthPickup_OverlappedByDamagedCharacter_Heals)
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
				HealthPickup.SetActorLocation(FVector::ZeroVector, /*bSweep=*/true);
			})
			.WaitDelay(FTimespan::FromSeconds(0.3))
			.Then([this]() {
				UALSHealthComponent* Health = Character->FindComponentByClass<UALSHealthComponent>();
				ASSERT_THAT(IsNotNull(Health));
				// Started at MaxHealth(100) - 40 damage = 60, then +25 heal = 85.
				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), Health->MaxHealth - 40.f + 25.f, 0.5f));
			});
	}

	TEST_METHOD(HealthPickup_OverlappedByFullHealthCharacter_IsNotConsumed)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterAt(FVector::ZeroVector);
				ASSERT_THAT(IsNotNull(Character));
				Pickup = &Spawner->SpawnActorAt<AALSHealthPickup>(FVector(FarAwayOffset, 0.f, 0.f), FRotator::ZeroRotator);
				Pickup->SetActorLocation(FVector::ZeroVector, /*bSweep=*/true);
			})
			.WaitDelay(FTimespan::FromSeconds(0.3))
			.Then([this]() {
				ASSERT_THAT(IsNotNull(Pickup));
				ASSERT_THAT(IsFalse(Pickup->IsActorBeingDestroyed()));
			});
	}

	TEST_METHOD(WeaponPickup_OverlappedByCharacter_EquipsOverlayAndGrantsAmmo)
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
				WeaponPickup.SetActorLocation(FVector::ZeroVector, /*bSweep=*/true);
			})
			.WaitDelay(FTimespan::FromSeconds(0.3))
			.Then([this]() {
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
				PistolPickup.SetActorLocation(FVector::ZeroVector, /*bSweep=*/true);
			})
			.WaitDelay(FTimespan::FromSeconds(0.3))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Character->GetOverlayState() == EALSOverlayState::PistolOneHanded));

				AALSWeaponPickup& RiflePickup = Spawner->SpawnActorAt<AALSWeaponPickup>(FVector(FarAwayOffset, 0.f, 0.f), FRotator::ZeroRotator);
				RiflePickup.OverlayStateToEquip = EALSOverlayState::Rifle;
				RiflePickup.WeaponItemID = TEXT("Weapon_Rifle");
				RiflePickup.BonusAmmoQuantity = 0;
				RiflePickup.SetActorLocation(FVector::ZeroVector, /*bSweep=*/true);
			})
			.WaitDelay(FTimespan::FromSeconds(0.3))
			.Then([this]() {
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
