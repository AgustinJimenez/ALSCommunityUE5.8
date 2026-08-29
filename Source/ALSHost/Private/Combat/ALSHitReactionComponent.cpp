#include "Combat/ALSHitReactionComponent.h"

#include "Combat/ALSHealthComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"

UALSHitReactionComponent::UALSHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UALSHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = GetOwner() ? GetOwner()->FindComponentByClass<UALSHealthComponent>() : nullptr;
	if (Health)
	{
		Health->OnHealthChanged.AddDynamic(this, &UALSHitReactionComponent::HandleHealthChanged);
	}
}

void UALSHitReactionComponent::HandleHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageInstigator)
{
	// Delta < 0 means damage (healing/regen is Delta >= 0) - only react to
	// actually getting hit, and never on top of a death ragdoll.
	if (Delta >= 0.f || -Delta < MinDamageToReact || HitReactionAnimations.IsEmpty() || (Health && Health->IsDead()))
	{
		return;
	}

	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* BodyMesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = BodyMesh ? BodyMesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	UAnimSequenceBase* Chosen = HitReactionAnimations[FMath::RandHelper(HitReactionAnimations.Num())];
	if (Chosen)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Chosen, MontageSlotName, 0.1f, 0.1f, 1.0f, 1);
	}
}
