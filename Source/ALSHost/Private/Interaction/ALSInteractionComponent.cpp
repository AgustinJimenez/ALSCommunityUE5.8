#include "Interaction/ALSInteractionComponent.h"

#include "Interaction/ALSInteractable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/World.h"

UALSInteractionComponent::UALSInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

bool UALSInteractionComponent::TryInteract()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		return false;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector TraceEnd = CameraLocation + CameraRotation.Vector() * InteractRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ALSInteract), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(OwnerPawn);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CameraLocation, TraceEnd, TraceChannel, QueryParams);
	if (!bHit || !Hit.GetActor())
	{
		return false;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor->GetClass()->ImplementsInterface(UALSInteractable::StaticClass()))
	{
		return false;
	}

	IALSInteractable::Execute_Interact(HitActor, OwnerPawn);
	return true;
}
