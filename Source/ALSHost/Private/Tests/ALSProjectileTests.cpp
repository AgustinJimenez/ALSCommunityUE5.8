#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Weapon/ALSProjectile.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Needs real physics/collision (the projectile has to actually travel and
// hit something), so FMapTestSpawner rather than FActorTestSpawner - see
// docs/testing.md.
TEST_CLASS(ALSProjectileTests, "ALSHost.Weapon")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Shooter = nullptr;
	UALSWeaponFireComponent* Weapon = nullptr;
	AALSCharacter* Target = nullptr;
	UALSHealthComponent* TargetHealth = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnShooterAndTarget()
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		if (!CharClass)
		{
			return;
		}

		Shooter = &Spawner->SpawnActorAt<AALSCharacter>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
		Weapon = Shooter->FindComponentByClass<UALSWeaponFireComponent>();

		Target = &Spawner->SpawnActorAt<AALSCharacter>(FVector(500.f, 0.f, 100.f), FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
		TargetHealth = Target->FindComponentByClass<UALSHealthComponent>();
	}

	// Fires a projectile the same way UALSWeaponFireComponent::Fire() does
	// (same InitializeProjectile call, same manual velocity set), without
	// going through the full input/ammo/spread pipeline - isolates the
	// projectile's own travel+hit+damage behavior.
	AALSProjectile* FireProjectile(const FVector& Start, const FVector& Direction, float Speed)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Shooter;
		SpawnParams.Instigator = Shooter;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AALSProjectile* Projectile = Spawner->GetWorld().SpawnActor<AALSProjectile>(AALSProjectile::StaticClass(), Start, Direction.Rotation(), SpawnParams);
		if (!Projectile)
		{
			return nullptr;
		}

		Projectile->InitializeProjectile(Weapon, nullptr, Shooter, nullptr, Start);
		if (UProjectileMovementComponent* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>())
		{
			Movement->InitialSpeed = Speed;
			Movement->MaxSpeed = Speed;
			Movement->Velocity = Direction * Speed;
		}
		return Projectile;
	}

	TEST_METHOD(Projectile_HittingTarget_AppliesDamageMatchingComputeDamageForHit)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnShooterAndTarget();
				ASSERT_THAT(IsNotNull(Weapon));
				ASSERT_THAT(IsNotNull(TargetHealth));

				FireProjectile(FVector(0.f, 0.f, 100.f), FVector::ForwardVector, 8000.f);
			})
			// 500cm at 8000cm/s is ~0.06s of flight - give it generous
			// margin for the first physics tick(s) to actually resolve.
			.Until([this]() { return TargetHealth->GetCurrentHealth() < TargetHealth->MaxHealth; }, FTimespan::FromSeconds(2.0))
			.Then([this]() {
				const float ExpectedDamage = Weapon->ComputeDamageForHit(NAME_None, 500.f);
				const float ActualDamageTaken = TargetHealth->MaxHealth - TargetHealth->GetCurrentHealth();
				// Generous tolerance - the projectile travels a hair more
				// than exactly 500cm to reach the target's capsule surface
				// rather than its center, and gravity drop adds a little more.
				ASSERT_THAT(IsNear(ActualDamageTaken, ExpectedDamage, ExpectedDamage * 0.5f));
			});
	}

	TEST_METHOD(Projectile_IgnoresShooter_DoesNotSelfDamage)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnShooterAndTarget();
				ASSERT_THAT(IsNotNull(Weapon));

				// Fire straight back through the shooter's own location -
				// should pass through without applying damage to self.
				FireProjectile(FVector(0.f, 0.f, 100.f), -FVector::ForwardVector, 8000.f);
			})
			.WaitDelay(FTimespan::FromSeconds(0.3))
			.Then([this]() {
				UALSHealthComponent* ShooterHealth = Shooter->FindComponentByClass<UALSHealthComponent>();
				ASSERT_THAT(IsNotNull(ShooterHealth));
				ASSERT_THAT(IsNear(ShooterHealth->GetCurrentHealth(), ShooterHealth->MaxHealth, 0.01f));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
