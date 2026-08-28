#include "Inventory/ALSPickupBase.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AALSPickupBase::AALSPickupBase()
{
	// Ticks solely to keep LabelText facing the camera (see Tick()) -
	// UTextRenderComponent has no built-in billboard-to-camera behavior.
	PrimaryActorTick.bCanEverTick = true;

	// Mesh is the root (not TriggerSphere) so it can simulate physics - a
	// dropped/placed pickup falls under gravity and rests on the ground
	// like a real object, and the player can bump/kick it, using the
	// engine's stock "PhysicsActor" profile (blocks world geometry and
	// pawns). TriggerSphere and LabelText attach to it as children so they
	// fall and settle together with it; TriggerSphere's own OverlapAllDynamic
	// profile keeps pickup-on-approach working exactly as before regardless
	// of how Mesh's own collision responds.
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

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->InitSphereRadius(75.f);
	// "OverlapAllDynamic" (a stock engine collision profile) rather than
	// hand-picking channel responses - a manually-set Overlap response on
	// just the Pawn channel still resolves as a physical *block* if the
	// other side's response to this component's own object channel isn't
	// also non-blocking, which the character's capsule's default "Pawn"
	// preset is not. This profile is built specifically for trigger/pickup
	// volumes and avoids that mismatch.
	TriggerSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerSphere->SetupAttachment(RootComponent);

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(RootComponent);
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextBottom);
	LabelText->SetWorldSize(24.f);
	LabelText->SetTextRenderColor(FColor::White);
}

void AALSPickupBase::BeginPlay()
{
	Super::BeginPlay();

	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AALSPickupBase::HandleBeginOverlap);

	if (LabelText)
	{
		LabelText->SetText(PickupLabel);
	}
}

void AALSPickupBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!LabelText)
	{
		return;
	}

	// UTextRenderComponent has no built-in camera-facing behavior, unlike a
	// screen-space UWidgetComponent - rotate it manually every frame so the
	// label stays legible from whichever direction the player approaches.
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCamera = CameraManager->GetCameraLocation() - LabelText->GetComponentLocation();
		if (!ToCamera.IsNearlyZero())
		{
			LabelText->SetWorldRotation(ToCamera.Rotation());
		}
	}
}

void AALSPickupBase::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bConsumed)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	if (OnPickedUp(Pawn))
	{
		bConsumed = true;
		// Not an immediate Destroy(): a pickup spawned already overlapping a
		// pawn (e.g. dropped right on top of one, or in a test spawning both
		// at the same location) fires this overlap synchronously from
		// within SpawnActor's own initial-overlap resolution, still on the
		// stack below it - destroying the actor there makes SpawnActor
		// return nullptr to its own caller instead of the actor it just
		// created. Disable immediately so it can't be picked up twice or
		// keep blocking, but defer the actual Destroy() a tick.
		TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetActorHiddenInGame(true);
		SetLifeSpan(0.01f);
	}
}
