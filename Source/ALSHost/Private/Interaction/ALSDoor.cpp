#include "Interaction/ALSDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"

AALSDoor::AALSDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	HingeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HingeRoot"));
	RootComponent = HingeRoot;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(HingeRoot);
	// Offset sideways from the hinge so the mesh reads as a door leaf
	// swinging from one edge, not spinning around its own center. Scale is
	// a plain Engine cube stretched into door-like proportions (~1m wide,
	// 2.1m tall, 5cm thick) - matches a standard door's rough footprint
	// closely enough to place sensibly without a real door asset.
	DoorMesh->SetRelativeLocation(FVector(0.f, 50.f, 105.f));
	DoorMesh->SetRelativeScale3D(FVector(0.05f, 1.0f, 2.1f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMeshFinder.Succeeded())
	{
		DoorMesh->SetStaticMesh(DefaultMeshFinder.Object);
	}
}

void AALSDoor::Interact_Implementation(APawn* Interactor)
{
	bIsOpen = !bIsOpen;
	TargetRelativeYaw = bIsOpen ? OpenYawDegrees : 0.0f;
}

FText AALSDoor::GetInteractionPrompt_Implementation() const
{
	return bIsOpen ? FText::FromString(TEXT("Close Door")) : FText::FromString(TEXT("Open Door"));
}

void AALSDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FMath::IsNearlyEqual(CurrentRelativeYaw, TargetRelativeYaw, 0.1f))
	{
		return;
	}

	CurrentRelativeYaw = FMath::FixedTurn(CurrentRelativeYaw, TargetRelativeYaw, SwingSpeedDegreesPerSecond * DeltaSeconds);
	HingeRoot->SetRelativeRotation(FRotator(0.f, CurrentRelativeYaw, 0.f));
}
