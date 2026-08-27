#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Character/ALSCharacter.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

// Reload() used to always refill the magazine to full for free - infinite
// ammo. It now only pulls from whatever "Ammo_Rifle" reserve the
// character's UALSInventoryComponent actually holds (see
// AGENT_TASKS/0003_core_gameplay_systems.md). FMapTestSpawner (not
// FActorTestSpawner) is needed since this exercises real component
// interaction through BeginPlay, same reasoning as ALSPickupTests.
TEST_CLASS(ALSWeaponAmmoTests, "ALSHost.Weapon")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;
	UALSWeaponFireComponent* Weapon = nullptr;
	UALSInventoryComponent* Inventory = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnRiflemanWithEmptyMagazine()
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		ASSERT_THAT(IsNotNull(CharClass));

		Character = &Spawner->SpawnActorAt<AALSCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
		Weapon = Character->FindComponentByClass<UALSWeaponFireComponent>();
		Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
		ASSERT_THAT(IsNotNull(Weapon));
		ASSERT_THAT(IsNotNull(Inventory));

		Character->SetOverlayState(EALSOverlayState::Rifle);

		// Fire() requires a valid controller (checked before it ever reaches
		// SyncAmmoForCurrentWeapon), so possess with the temp level's real
		// PlayerController first - same pattern as ALSWeaponFireInputTests.
		APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
		ASSERT_THAT(IsNotNull(PC));
		PC->Possess(Character);

		// Drain the magazine down from its initial full 30 by firing
		// directly (BlueprintCallable, no input injection needed).
		for (int32 i = 0; i < 30; ++i)
		{
			Weapon->Fire();
		}
		ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() == 0));
	}

	TEST_METHOD(Reload_WithNoReserveAmmo_DoesNothing)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnRiflemanWithEmptyMagazine();
				ASSERT_THAT(IsFalse(Inventory->HasItem(TEXT("Ammo_Rifle"), 1)));

				Weapon->Reload();
			})
			.Then([this]() {
				ASSERT_THAT(IsFalse(Weapon->IsReloading()));
				ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() == 0));
			});
	}

	TEST_METHOD(Reload_WithLimitedReserve_OnlyLoadsWhatsAvailable_AndConsumesInventory)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnRiflemanWithEmptyMagazine();
				Inventory->AddItem(TEXT("Ammo_Rifle"), FText::FromString(TEXT("Rifle Ammo")), 10, 999);

				Weapon->Reload();
				ASSERT_THAT(IsTrue(Weapon->IsReloading()));
			})
			.WaitDelay(FTimespan::FromSeconds(2.5))
			.Then([this]() {
				ASSERT_THAT(IsFalse(Weapon->IsReloading()));
				// Only had 10 reserve rounds, magazine holds 30 - should top
				// up to exactly 10, not the full 30 (that would still be the
				// old infinite-ammo behavior), and the reserve is now spent.
				ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() == 10));
				ASSERT_THAT(IsFalse(Inventory->HasItem(TEXT("Ammo_Rifle"), 1)));
			});
	}

	// Pistol (M9) and Bow both gained a real "Muzzle" socket and
	// AmmoStatsByOverlayState entry alongside the Rifle - these lock in that
	// firing/reloading actually works for them too, not just visually
	// equipping. Same direct-Fire()-call pattern as SpawnRiflemanWithEmptyMagazine,
	// parameterized by overlay state instead of duplicating the whole fixture.
	void SpawnCharacterEquippedAndDrained(EALSOverlayState OverlayState, int32 MagazineSize)
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		ASSERT_THAT(IsNotNull(CharClass));

		Character = &Spawner->SpawnActorAt<AALSCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
		Weapon = Character->FindComponentByClass<UALSWeaponFireComponent>();
		Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
		ASSERT_THAT(IsNotNull(Weapon));
		ASSERT_THAT(IsNotNull(Inventory));

		Character->SetOverlayState(OverlayState);

		APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
		ASSERT_THAT(IsNotNull(PC));
		PC->Possess(Character);

		for (int32 i = 0; i < MagazineSize; ++i)
		{
			Weapon->Fire();
		}
		ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() == 0));
	}

	TEST_METHOD(PistolFire_DrainsMagazine_AndReloadsFromReserve)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterEquippedAndDrained(EALSOverlayState::PistolOneHanded, 12);
				Inventory->AddItem(TEXT("Ammo_Pistol"), FText::FromString(TEXT("Pistol Ammo")), 12, 999);

				Weapon->Reload();
				ASSERT_THAT(IsTrue(Weapon->IsReloading()));
			})
			.WaitDelay(FTimespan::FromSeconds(2.0))
			.Then([this]() {
				ASSERT_THAT(IsFalse(Weapon->IsReloading()));
				ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() == 12));
			});
	}

	TEST_METHOD(BowFire_DrainsSingleArrow_AndReloadsFromReserve)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacterEquippedAndDrained(EALSOverlayState::Bow, 1);
				Inventory->AddItem(TEXT("Ammo_Bow"), FText::FromString(TEXT("Arrows")), 5, 999);

				Weapon->Reload();
				ASSERT_THAT(IsTrue(Weapon->IsReloading()));
			})
			.WaitDelay(FTimespan::FromSeconds(1.5))
			.Then([this]() {
				ASSERT_THAT(IsFalse(Weapon->IsReloading()));
				ASSERT_THAT(IsTrue(Weapon->GetCurrentAmmoInMagazine() == 1));
				ASSERT_THAT(IsTrue(Inventory->HasItem(TEXT("Ammo_Bow"), 4)));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
