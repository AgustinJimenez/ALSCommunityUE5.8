#include "Interaction/ALSLootContainer.h"

#include "Components/StaticMeshComponent.h"
#include "Inventory/ALSInventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AALSLootContainer::AALSLootContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
	RootComponent = ContainerMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxMeshFinder(
		TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Props/Meshes/Box.Box"));
	if (BoxMeshFinder.Succeeded())
	{
		ContainerMesh->SetStaticMesh(BoxMeshFinder.Object);
	}
}

void AALSLootContainer::Interact_Implementation(APawn* Interactor)
{
	if (bOpened || !Interactor)
	{
		return;
	}

	UALSInventoryComponent* Inventory = Interactor->FindComponentByClass<UALSInventoryComponent>();
	if (!Inventory)
	{
		return;
	}

	for (const FALSLootEntry& Entry : LootItems)
	{
		Inventory->AddItem(Entry.ItemID, Entry.DisplayName, Entry.Quantity, Entry.MaxStack);
	}

	bOpened = true;
}

FText AALSLootContainer::GetInteractionPrompt_Implementation() const
{
	return bOpened ? FText::FromString(TEXT("(Empty)")) : FText::FromString(TEXT("Open"));
}
