#include "Combat/ALSRestartLevelComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UALSRestartLevelComponent::UALSRestartLevelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UALSRestartLevelComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UALSRestartLevelComponent::HandleControllerChanged);
	}

	TrySetupInput();
}

void UALSRestartLevelComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UALSRestartLevelComponent::HandleControllerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UALSRestartLevelComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	TrySetupInput();
}

void UALSRestartLevelComponent::TrySetupInput()
{
	if (bInputBound || !RestartInputAction)
	{
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		return;
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!EIC)
	{
		return;
	}

	EIC->BindAction(RestartInputAction, ETriggerEvent::Started, this, &UALSRestartLevelComponent::HandleRestartInput);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (RestartInputMappingContext)
		{
			Subsystem->AddMappingContext(RestartInputMappingContext, 0);
		}
	}

	bInputBound = true;
}

void UALSRestartLevelComponent::HandleRestartInput(const FInputActionValue& Value)
{
	RestartLevel();
}

void UALSRestartLevelComponent::RestartLevel() const
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World)));
	}
}
