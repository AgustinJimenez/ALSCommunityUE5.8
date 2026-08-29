#include "Inventory/ALSPickupBase.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AALSPickupBase::AALSPickupBase()
{
	// Ticks solely to keep LabelText facing the camera while visible (see
	// Tick()) - UTextRenderComponent has no built-in billboard-to-camera
	// behavior.
	PrimaryActorTick.bCanEverTick = true;

	// Mesh is the root so it can simulate physics - a dropped/placed pickup
	// falls under gravity and rests on the ground like a real object, and
	// the player can bump/kick it, using the engine's stock "PhysicsActor"
	// profile (blocks world geometry and pawns, and blocks the Visibility
	// trace channel IALSInteractable's line-trace-based Interact key uses,
	// so this doubles as the interact hit target with no separate trigger
	// volume needed).
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetRelativeScale3D(FVector(0.5f));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (DefaultMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(DefaultMeshFinder.Object);
	}

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(RootComponent);
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextBottom);
	LabelText->SetWorldSize(24.f);
	LabelText->SetTextRenderColor(FColor::White);
	// Hidden until UALSInteractionComponent marks this pickup as the
	// player's current interact target - see SetInteractPromptVisible_Implementation.
	LabelText->SetVisibility(false);
}

void AALSPickupBase::BeginPlay()
{
	Super::BeginPlay();
}

void AALSPickupBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!LabelText || !LabelText->IsVisible())
	{
		return;
	}

	ALSFaceTextTowardCamera(LabelText, this);
}

void AALSPickupBase::Interact_Implementation(APawn* Interactor)
{
	if (bConsumed || !Interactor)
	{
		return;
	}

	if (OnPickedUp(Interactor))
	{
		bConsumed = true;
		SetActorHiddenInGame(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetLifeSpan(0.01f);
	}
}

FText AALSPickupBase::GetInteractionPrompt_Implementation() const
{
	return PickupLabel.IsEmpty() ? FText::FromString(TEXT("Pick Up")) : FText::Format(NSLOCTEXT("ALSHost", "PickupPrompt", "Pick Up {0}"), PickupLabel);
}

void AALSPickupBase::SetInteractPromptVisible_Implementation(bool bVisible)
{
	if (!LabelText)
	{
		return;
	}

	LabelText->SetVisibility(bVisible);
	if (bVisible)
	{
		LabelText->SetText(GetInteractionPrompt_Implementation());
	}
}
