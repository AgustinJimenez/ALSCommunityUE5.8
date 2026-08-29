#include "Combat/ALSDeathHandlerComponent.h"

#include "Combat/ALSHealthComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"

UALSDeathHandlerComponent::UALSDeathHandlerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UALSDeathHandlerComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	SpawnLocation = OwnerCharacter->GetActorLocation();
	SpawnRotation = OwnerCharacter->GetActorRotation();

	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
	{
		OriginalMeshCollisionProfile = Mesh->GetCollisionProfileName();
		OriginalMeshRelativeTransform = Mesh->GetRelativeTransform();
	}

	Health = OwnerCharacter->FindComponentByClass<UALSHealthComponent>();
	if (Health)
	{
		Health->OnDeath.AddDynamic(this, &UALSDeathHandlerComponent::HandleDeath);
	}
}

void UALSDeathHandlerComponent::HandleDeath(AActor* Killer)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || bIsRagdolling)
	{
		return;
	}

	bIsRagdolling = true;

	OwnerCharacter->DisableInput(nullptr);

	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
	{
		Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
		Mesh->SetSimulatePhysics(true);
		Mesh->SetAllBodiesBelowSimulatePhysics(NAME_None, true);
	}

	if (RespawnDelaySeconds > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this, &UALSDeathHandlerComponent::Respawn, RespawnDelaySeconds, false);
	}
}

void UALSDeathHandlerComponent::Respawn()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
	{
		// SetAllBodiesBelowSimulatePhysics(..., true) put every body below
		// the root into a simulating state on death - SetSimulatePhysics(false)
		// alone only touches the component's own root body, leaving the rest
		// of the ragdoll still simulating. Needs the same below-root call,
		// symmetrically, to actually turn it all off.
		Mesh->SetAllBodiesBelowSimulatePhysics(NAME_None, false);
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionProfileName(OriginalMeshCollisionProfile);
		Mesh->SetRelativeTransform(OriginalMeshRelativeTransform);
	}

	if (UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	OwnerCharacter->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}

	OwnerCharacter->EnableInput(nullptr);

	if (Health)
	{
		Health->ResetHealth();
	}

	bIsRagdolling = false;
}
