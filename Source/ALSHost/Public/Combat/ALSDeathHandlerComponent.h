#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ALSDeathHandlerComponent.generated.h"

class UALSHealthComponent;
class ACharacter;

// Reacts to the owning character's UALSHealthComponent::OnDeath: ragdolls
// the skeletal mesh and disables input/movement. If RespawnDelaySeconds is
// greater than 0, restores the character to its BeginPlay location/rotation
// with full health after that delay (the player); a value of 0 leaves the
// ragdoll down permanently with no timer (enemies - dying and collapsing is
// the whole point, there's nothing to respawn back to).
UCLASS(ClassGroup = (ALS), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSDeathHandlerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSDeathHandlerComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Death")
	float RespawnDelaySeconds = 5.f;

	UFUNCTION(BlueprintPure, Category = "ALS|Death")
	bool IsRagdolling() const { return bIsRagdolling; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDeath(AActor* Killer);

	void Respawn();

private:
	UPROPERTY()
	TObjectPtr<UALSHealthComponent> Health;

	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	FName OriginalMeshCollisionProfile;
	FTransform OriginalMeshRelativeTransform;

	bool bIsRagdolling = false;

	FTimerHandle RespawnTimerHandle;
};
