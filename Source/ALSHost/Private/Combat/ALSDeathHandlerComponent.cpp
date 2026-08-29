#include "Combat/ALSDeathHandlerComponent.h"

#include "Combat/ALSHealthComponent.h"
#include "Inventory/ALSItemPickup.h"
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
	else
	{
		TrySpawnLoot();
	}
}

void UALSDeathHandlerComponent::TrySpawnLoot() const
{
	// FRand() is uniform on [0,1) (can return exactly 0.0) - the skip
	// condition must be ">=", not ">", or a LootDropChance of exactly 0 can
	// still occasionally spawn loot on the rare tick FRand() lands on 0.0.
	// Caught by ZeroDropChance_NeverSpawnsLoot actually failing intermittently.
	if (LootItemID.IsNone() || FMath::FRand() >= LootDropChance)
	{
		return;
	}

	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!OwnerCharacter || !World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AALSItemPickup* Pickup = World->SpawnActorDeferred<AALSItemPickup>(AALSItemPickup::StaticClass(), FTransform(OwnerCharacter->GetActorLocation()));
	if (!Pickup)
	{
		return;
	}

	Pickup->ItemID = LootItemID;
	Pickup->Quantity = LootQuantity;
	Pickup->FinishSpawning(FTransform(OwnerCharacter->GetActorLocation()));
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
