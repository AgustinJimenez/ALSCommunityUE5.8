#include "Inventory/ALSHealthPickup.h"

#include "Combat/ALSHealthComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AALSHealthPickup::AALSHealthPickup()
{
	// Base's inherited scale-down (0.5x, sized for the generic placeholder
	// Sphere) doesn't apply here - the medkit meshes are already correctly
	// sized real-world assets.
	Mesh->SetRelativeScale3D(FVector(1.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxMeshFinder(TEXT("/Game/ALSHost/Props/MedKit/Medic_Kit_MetalBox"));
	if (BoxMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(BoxMeshFinder.Object);
	}

	CoverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoverMesh"));
	CoverMesh->SetupAttachment(RootComponent);
	CoverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CoverMeshFinder(TEXT("/Game/ALSHost/Props/MedKit/Medic_Kit_MetalCover"));
	if (CoverMeshFinder.Succeeded())
	{
		CoverMesh->SetStaticMesh(CoverMeshFinder.Object);
	}
}

bool AALSHealthPickup::OnPickedUp(APawn* Pawn)
{
	UALSHealthComponent* Health = Pawn->FindComponentByClass<UALSHealthComponent>();
	if (!Health || Health->IsDead() || Health->GetCurrentHealth() >= Health->MaxHealth)
	{
		return false;
	}

	Health->Heal(HealAmount);
	return true;
}
