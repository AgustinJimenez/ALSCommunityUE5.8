#include "UI/ALSHUDComponent.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UALSHUDComponent::UALSHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UALSHUDComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UALSHUDComponent::HandleControllerChanged);
	}

	TryCreateHUD();
}

void UALSHUDComponent::HandleControllerChanged(APawn* PawnChanged, AController* OldController, AController* NewController)
{
	TryCreateHUD();
}

void UALSHUDComponent::TryCreateHUD()
{
	if (!StatusBarsWidgetClass || StatusBarsWidgetInstance)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (!PC)
	{
		return;
	}

	StatusBarsWidgetInstance = CreateWidget<UUserWidget>(PC, StatusBarsWidgetClass);
	if (StatusBarsWidgetInstance)
	{
		StatusBarsWidgetInstance->AddToViewport();
	}
}
