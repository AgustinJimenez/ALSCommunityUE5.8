#pragma once

#include "CoreMinimal.h"
#include "Inventory/ALSPickupBase.h"
#include "ALSHealthPickup.generated.h"

// Heals whatever pawn's UALSHealthComponent overlaps it. Not consumed (left
// in the world) if the pawn has no health component, or is already dead
// (a health pack shouldn't revive a corpse) or already at full health.
UCLASS()
class ALSHOST_API AALSHealthPickup : public AALSPickupBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	float HealAmount = 50.f;

protected:
	virtual bool OnPickedUp(APawn* Pawn) override;
};
