#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Inventory/ALSItemPickup.h"
#include "Inventory/ALSHealthPickup.h"
#include "Inventory/ALSWeaponPickup.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

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
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
