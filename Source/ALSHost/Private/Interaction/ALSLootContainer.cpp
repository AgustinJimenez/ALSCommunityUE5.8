#include "Interaction/ALSLootContainer.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Inventory/ALSInventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AALSLootContainer::AALSLootContainer()
{
	PrimaryActorTick.bCanEverTick = true;

	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
	RootComponent = ContainerMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxMeshFinder(
		TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Props/Meshes/Box.Box"));
	if (BoxMeshFinder.Succeeded())
	{
		ContainerMesh->SetStaticMesh(BoxMeshFinder.Object);
	}

	PromptText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PromptText"));
	PromptText->SetupAttachment(RootComponent);
	PromptText->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	PromptText->SetHorizontalAlignment(EHTA_Center);
	PromptText->SetVerticalAlignment(EVRTA_TextBottom);
	PromptText->SetWorldSize(24.f);
	PromptText->SetTextRenderColor(FColor::White);
	PromptText->SetVisibility(false);
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

void AALSLootContainer::SetInteractPromptVisible_Implementation(bool bVisible)
{
	if (!PromptText)
	{
		return;
	}

	PromptText->SetVisibility(bVisible);
	if (bVisible)
	{
		PromptText->SetText(GetInteractionPrompt_Implementation());
	}
}

void AALSLootContainer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PromptText && PromptText->IsVisible())
	{
		ALSFaceTextTowardCamera(PromptText, this);
	}
}
