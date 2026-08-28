#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALSPickupBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// Common base for a world pickup: overlap-triggered, destroys itself once
// picked up. Subclasses only implement OnPickedUp() - what actually happens
// (add to inventory, heal, etc.) - everything else (trigger volume, mesh,
// overlap wiring, one-pickup-only guard) lives here.
UCLASS(Abstract)
class ALSHOST_API AALSPickupBase : public AActor
{
	GENERATED_BODY()

public:
	AALSPickupBase();

	// Floating text shown above the pickup (e.g. "Rifle", "Health Pack") -
	// set per-instance in the level, purely cosmetic/identification, not
	// tied to any inventory ItemID.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Pickup")
	FText PickupLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Pickup")
	TObjectPtr<UTextRenderComponent> LabelText;

	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Return true if the pawn actually consumed the pickup (destroys the
	// actor and prevents further overlaps from doing anything). Return
	// false to leave the pickup in the world (e.g. inventory full).
	virtual bool OnPickedUp(APawn* Pawn) PURE_VIRTUAL(AALSPickupBase::OnPickedUp, return false;);

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Pickup")
	TObjectPtr<USphereComponent> TriggerSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	bool bConsumed = false;
};
