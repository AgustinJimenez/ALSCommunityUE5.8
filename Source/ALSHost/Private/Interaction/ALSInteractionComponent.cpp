#include "Interaction/ALSInteractionComponent.h"

#include "Interaction/ALSInteractable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UALSInteractionComponent::UALSInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UALSInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UALSInteractionComponent::HandleControllerChanged);
	}

	TrySetupInput();
}

void UALSInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UALSInteractionComponent::HandleControllerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UALSInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshCurrentInteractable();
}

void UALSInteractionComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	TrySetupInput();
}

void UALSInteractionComponent::TrySetupInput()
{
	// Same BeginPlay-before-possession gotcha every other input-binding
	// component in this project already documents - retry on possession.
	if (bInputBound || !InteractInputAction)
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

	EIC->BindAction(InteractInputAction, ETriggerEvent::Started, this, &UALSInteractionComponent::HandleInteractInput);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (InteractInputMappingContext)
		{
			Subsystem->AddMappingContext(InteractInputMappingContext, 0);
		}
	}

	bInputBound = true;
}

void UALSInteractionComponent::HandleInteractInput(const FInputActionValue& Value)
{
	TryInteract();
}

void UALSInteractionComponent::RefreshCurrentInteractable()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	const FVector PawnLocation = OwnerPawn->GetActorLocation();
	const FVector Forward = OwnerPawn->GetActorForwardVector();

	TArray<AActor*> Candidates;
	UGameplayStatics::GetAllActorsWithInterface(this, UALSInteractable::StaticClass(), Candidates);

	AActor* Best = nullptr;
	float BestDistSq = FMath::Square(InteractRange);

	for (AActor* Candidate : Candidates)
	{
		if (!Candidate || Candidate == OwnerPawn)
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - PawnLocation;
		const float DistSq = ToCandidate.SizeSquared();
		if (DistSq > BestDistSq)
		{
			continue;
		}

		if (!ToCandidate.IsNearlyZero() && (Forward | ToCandidate.GetSafeNormal()) < InteractFacingCosineThreshold)
		{
			continue;
		}

		Best = Candidate;
		BestDistSq = DistSq;
	}

	if (Best != CurrentInteractable)
	{
		if (CurrentInteractable && CurrentInteractable->GetClass()->ImplementsInterface(UALSInteractable::StaticClass()))
		{
			IALSInteractable::Execute_SetInteractPromptVisible(CurrentInteractable, false);
		}
		CurrentInteractable = Best;
	}

	if (CurrentInteractable)
	{
		// Called every tick, not just on change, so a prompt-text change on
		// the SAME target (e.g. a loot container's prompt going from "Open"
		// to "(Empty)" the instant it's looted) stays current - each
		// implementer re-fetches GetInteractionPrompt() itself when told to
		// show, see ALSInteractable.h.
		IALSInteractable::Execute_SetInteractPromptVisible(CurrentInteractable, true);
	}
}

bool UALSInteractionComponent::TryInteract()
{
	if (!CurrentInteractable)
	{
		return false;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return false;
	}

	IALSInteractable::Execute_Interact(CurrentInteractable, OwnerPawn);
	return true;
}
