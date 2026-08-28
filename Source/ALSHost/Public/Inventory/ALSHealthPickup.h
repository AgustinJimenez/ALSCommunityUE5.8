#pragma once

#include "CoreMinimal.h"
#include "Inventory/ALSPickupBase.h"
#include "ALSHealthPickup.generated.h"

class UStaticMeshComponent;

// Heals whatever pawn's UALSHealthComponent overlaps it. Not consumed (left
// in the world) if the pawn has no health component, or is already dead
// (a health pack shouldn't revive a corpse) or already at full health.
UCLASS()
class ALSHOST_API AALSHealthPickup : public AALSPickupBase
{
	GENERATED_BODY()

public:
	AALSHealthPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	float HealAmount = 50.f;

	// The inherited Mesh (base's placeholder Sphere) is repurposed here for
	// the medkit's box body; this is the box's separate lid/cover part -
	// the source FBX split into many sub-meshes (box, cover, plus dozens of
	// tiny interior contents - syringes, pills, cotton, etc. - not worth
	// displaying individually since they're hidden when the kit is closed).
	// Both parts already share one material slot/texture set, and their
	// vertex data is already positioned correctly relative to each other,
	// so attaching both at identity relative transform reconstructs the
	// assembled closed kit.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Pickup")
	TObjectPtr<UStaticMeshComponent> CoverMesh;

protected:
	virtual bool OnPickedUp(APawn* Pawn) override;
};
