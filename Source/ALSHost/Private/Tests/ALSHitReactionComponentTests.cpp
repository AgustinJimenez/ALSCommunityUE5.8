#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Combat/ALSHitReactionComponent.h"
#include "Combat/ALSHealthComponent.h"
#include "Character/ALSCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

// Covers UALSHitReactionComponent - taking damage previously had zero
// animated feedback anywhere in this project. See AGENT_TASKS/0004.
TEST_CLASS(ALSHitReactionComponentTests, "ALSHost.Combat")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;
	UALSHealthComponent* Health = nullptr;
	UALSHitReactionComponent* HitReaction = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	void SpawnCharacter()
	{
		UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
		if (!CharClass)
		{
			return;
		}

		Character = &Spawner->SpawnActor<AALSCharacter>(FActorSpawnParameters(), CharClass);
		Health = Character->FindComponentByClass<UALSHealthComponent>();

		UAnimSequenceBase* ReactAnim = LoadObject<UAnimSequenceBase>(nullptr, TEXT("/Game/ALSHost/Animations/AS_Hit_Chest.AS_Hit_Chest"));

		HitReaction = NewObject<UALSHitReactionComponent>(Character);
		if (ReactAnim)
		{
			HitReaction->HitReactionAnimations.Add(ReactAnim);
		}
		HitReaction->RegisterComponent();
	}

	TEST_METHOD(TakingDamage_PlaysHitReactionMontage)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacter();
				ASSERT_THAT(IsNotNull(Health));
				ASSERT_THAT(IsFalse(HitReaction->HitReactionAnimations.IsEmpty()));

				UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
				ASSERT_THAT(IsNotNull(AnimInstance));
				ASSERT_THAT(IsNull(AnimInstance->GetCurrentActiveMontage()));

				UGameplayStatics::ApplyDamage(Character, 10.f, nullptr, nullptr, UDamageType::StaticClass());

				ASSERT_THAT(IsNotNull(AnimInstance->GetCurrentActiveMontage()));
			});
	}

	// Healing while already at MaxHealth is a genuine no-op in
	// UALSHealthComponent (ApplyHealthDelta returns before broadcasting
	// OnHealthChanged at all when the clamped result doesn't change) - so
	// this also confirms HandleHealthChanged is never even reached for a
	// true no-op, not just that it correctly ignores a positive Delta.
	TEST_METHOD(HealingAtFullHealth_DoesNotPlayHitReactionMontage)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				SpawnCharacter();
				ASSERT_THAT(IsNotNull(Health));
				ASSERT_THAT(IsNear(Health->GetCurrentHealth(), Health->MaxHealth, 0.01f));

				Health->Heal(10.f);

				UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
				ASSERT_THAT(IsNotNull(AnimInstance));
				ASSERT_THAT(IsNull(AnimInstance->GetCurrentActiveMontage()));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
