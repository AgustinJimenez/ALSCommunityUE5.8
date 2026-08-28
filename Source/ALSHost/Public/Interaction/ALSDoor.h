#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/ALSInteractable.h"
#include "ALSDoor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

// Interact() toggles between closed and open, swinging smoothly around the
// actor's own placed location (the hinge point) rather than the mesh's
// center - HingeRoot is the actual RootComponent, and DoorMesh is a child
// offset sideways from it, so rotating HingeRoot's relative yaw swings the
// mesh like a real hinged door instead of spinning it in place. No door
// mesh exists anywhere in this project or its dependencies as of this
// writing (checked directly) - DoorMesh defaults to a plain scaled Cube,
// same "functional placeholder, swap the visual later" approach
// AALSPickupBase already uses for its own Sphere default.
UCLASS()
class ALSHOST_API AALSDoor : public AActor, public IALSInteractable
{
	GENERATED_BODY()

public:
	AALSDoor();

	// Yaw swung through when open, relative to closed (0). Negative swings
	// the other way - flip the sign if it opens through a wall instead of
	// away from one.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Door")
	float OpenYawDegrees = 100.0f;

	// How fast the door swings open/closed, in degrees/second.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Door")
	float SwingSpeedDegreesPerSecond = 180.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Door")
	TObjectPtr<USceneComponent> HingeRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UFUNCTION(BlueprintPure, Category = "ALS|Door")
	bool IsOpen() const { return bIsOpen; }

	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	bool bIsOpen = false;
	float CurrentRelativeYaw = 0.0f;
	float TargetRelativeYaw = 0.0f;
};
