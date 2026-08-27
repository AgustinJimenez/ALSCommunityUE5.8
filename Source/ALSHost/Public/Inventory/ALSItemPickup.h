#pragma once

#include "CoreMinimal.h"
#include "Inventory/ALSPickupBase.h"
#include "ALSItemPickup.generated.h"

// Generic inventory-item pickup: adds ItemID/DisplayName/Quantity to
// whatever pawn's UALSInventoryComponent overlaps it, respecting that
// item's MaxStack (leaves the pickup in the world - not consumed - if the
// pawn has no inventory component at all, or the stack is already full).
UCLASS()
class ALSHOST_API AALSItemPickup : public AALSPickupBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	int32 MaxStack = 99;

protected:
	virtual bool OnPickedUp(APawn* Pawn) override;
};
