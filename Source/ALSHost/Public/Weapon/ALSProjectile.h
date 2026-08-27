#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALSProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UALSWeaponFireComponent;

// A real traveling projectile (spawned by UALSWeaponFireComponent when
// bUseProjectilePhysics is set) rather than an instant hitscan trace - has
// actual flight time and gravity drop via UProjectileMovementComponent,
// so a shot at range visibly arcs and takes time to arrive instead of
// hitting the instant the trigger is pulled. Reuses
// UALSWeaponFireComponent::ComputeDamageForHit for the actual damage
// number (falloff by distance traveled, headshot multiplier), so the two
// fire modes deal identical damage at identical distances/hit zones - only
// the timing/trajectory differs.
UCLASS()
class ALSHOST_API AALSProjectile : public AActor
{
	GENERATED_BODY()

public:
	AALSProjectile();

	// Called by UALSWeaponFireComponent right after spawning, before the
	// projectile has had a chance to move or hit anything.
	void InitializeProjectile(UALSWeaponFireComponent* InSourceWeapon, AController* InInstigatorController, AActor* InDamageCauser, TSubclassOf<class UDamageType> InDamageTypeClass, const FVector& StartLocation);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// Safety net in case it never hits anything (flies off into the sky).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Projectile")
	float MaxLifetimeSeconds = 5.0f;

private:
	UPROPERTY()
	TObjectPtr<UALSWeaponFireComponent> SourceWeapon;

	UPROPERTY()
	TObjectPtr<AController> InstigatorController;

	UPROPERTY()
	TObjectPtr<AActor> DamageCauser;

	UPROPERTY()
	TSubclassOf<UDamageType> DamageTypeClass;

	FVector SpawnLocation = FVector::ZeroVector;

	bool bHasHit = false;
};
