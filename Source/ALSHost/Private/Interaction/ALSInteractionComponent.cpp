#include "Interaction/ALSInteractionComponent.h"

#include "Interaction/ALSInteractable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"

// Same pattern as ALS.Weapon.ShowDebugTrace - purely visual, does not affect
// hit detection. Defaulted to true for the same reason: the editor gets
// restarted often during development, and this is the only way to visually
// confirm the Interact key is actually registering a press at all (as
// opposed to registering but missing its trace, or not registering at all)
// without attaching a debugger.
static TAutoConsoleVariable<bool> CVarALSInteractShowDebugTrace(
	TEXT("ALS.Interact.ShowDebugTrace"),
	true,
	TEXT("Draw the interact trace and show an on-screen message every time the Interact key is pressed, reporting whether it hit anything interactable. Purely visual - does not affect hit detection."),
	ECVF_Default);

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
		if (CVarALSInteractShowDebugTrace.GetValueOnGameThread())
		{
			DrawDebugLine(GetWorld(), CameraLocation, TraceEnd, FColor::Yellow, false, 1.0f, 0, 0.5f);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("Interact: nothing in range/aim"));
			}
		}
		return false;
	}

	AActor* HitActor = Hit.GetActor();
	const bool bIsInteractable = HitActor->GetClass()->ImplementsInterface(UALSInteractable::StaticClass());

	if (CVarALSInteractShowDebugTrace.GetValueOnGameThread())
	{
		DrawDebugLine(GetWorld(), CameraLocation, Hit.ImpactPoint, bIsInteractable ? FColor::Green : FColor::Red, false, 1.0f, 0, 0.5f);
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 8.0f, bIsInteractable ? FColor::Green : FColor::Red, false, 1.0f);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, bIsInteractable ? FColor::Green : FColor::Red,
				FString::Printf(TEXT("Interact: hit %s (%s)"), *HitActor->GetName(), bIsInteractable ? TEXT("interactable") : TEXT("not interactable")));
		}
	}

	if (!bIsInteractable)
	{
		return false;
	}

	IALSInteractable::Execute_Interact(HitActor, OwnerPawn);
	return true;
}
