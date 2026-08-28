#include "Combat/ALSMedkitComponent.h"

#include "Character/ALSCharacter.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Combat/ALSHealthComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

UALSMedkitComponent::UALSMedkitComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MedkitMeshFinder(TEXT("/Game/ALSHost/Props/MedKit/Medic_Kit_MetalBox"));
	if (MedkitMeshFinder.Succeeded())
	{
		MedkitHeldMesh = MedkitMeshFinder.Object;
	}
}

void UALSMedkitComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UALSMedkitComponent::HandleControllerChanged);
	}
	TrySetupInput();

	if (const AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwner()))
	{
		if (UALSHealthComponent* Health = ALSChar->FindComponentByClass<UALSHealthComponent>())
		{
			Health->OnHealthChanged.AddDynamic(this, &UALSMedkitComponent::HandleHealthChanged);
		}
	}
}

void UALSMedkitComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UALSMedkitComponent::HandleControllerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UALSMedkitComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	TrySetupInput();
}

void UALSMedkitComponent::TrySetupInput()
{
	if (bInputBound || !ApplyInputAction)
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

	EIC->BindAction(ApplyInputAction, ETriggerEvent::Started, this, &UALSMedkitComponent::HandleApplyInput);

	// No AddMappingContext call here - UALSWeaponFireComponent's own
	// BeginPlay already adds IMC_Fire, and ApplyInputAction reuses that same
	// mapping context's IA_Fire action, so adding it a second time would be
	// redundant.
	bInputBound = true;
}

void UALSMedkitComponent::HandleApplyInput(const FInputActionValue& Value)
{
	TryStartApplyingMedkit();
}

bool UALSMedkitComponent::TryStartApplyingMedkit()
{
	if (bIsApplying)
	{
		return false;
	}

	AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwner());
	if (!ALSChar)
	{
		return false;
	}

	// Box is currently only ever used for the medkit's held pose in this
	// project (see ALSMedkitComponent.h) - if another item ever also uses
	// Box, this check will need to distinguish which one is actually
	// equipped, not just the overlay state.
	if (ALSChar->GetOverlayState() != EALSOverlayState::Box)
	{
		return false;
	}

	const UALSInventoryComponent* Inventory = ALSChar->FindComponentByClass<UALSInventoryComponent>();
	if (!Inventory || !Inventory->HasItem(MedkitItemID, 1))
	{
		return false;
	}

	const UALSHealthComponent* Health = ALSChar->FindComponentByClass<UALSHealthComponent>();
	if (!Health || Health->IsDead() || Health->GetCurrentHealth() >= Health->MaxHealth)
	{
		return false;
	}

	bIsApplying = true;
	ApplyElapsedSeconds = 0.0f;
	ApplyStartLocation = ALSChar->GetActorLocation();
	OnMedkitApplyStarted.Broadcast();
	OnMedkitApplyProgress.Broadcast(0.0f);
	return true;
}

void UALSMedkitComponent::CancelApplyingMedkit()
{
	if (!bIsApplying)
	{
		return;
	}
	FinishApplyingMedkit(/*bCompleted=*/false);
}

void UALSMedkitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsApplying)
	{
		return;
	}

	const AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwner());
	if (ALSChar && FVector::DistSquared(ALSChar->GetActorLocation(), ApplyStartLocation) > FMath::Square(MovementCancelDistance))
	{
		CancelApplyingMedkit();
		return;
	}

	ApplyElapsedSeconds += DeltaTime;
	const float Progress = ApplyDurationSeconds > 0.0f ? FMath::Clamp(ApplyElapsedSeconds / ApplyDurationSeconds, 0.0f, 1.0f) : 1.0f;
	OnMedkitApplyProgress.Broadcast(Progress);

	if (ApplyElapsedSeconds >= ApplyDurationSeconds)
	{
		FinishApplyingMedkit(/*bCompleted=*/true);
	}
}

void UALSMedkitComponent::HandleHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageInstigator)
{
	if (bIsApplying && Delta < 0.0f)
	{
		CancelApplyingMedkit();
	}
}

void UALSMedkitComponent::FinishApplyingMedkit(bool bCompleted)
{
	bIsApplying = false;
	ApplyElapsedSeconds = 0.0f;

	if (bCompleted)
	{
		if (AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwner()))
		{
			if (UALSHealthComponent* Health = ALSChar->FindComponentByClass<UALSHealthComponent>())
			{
				Health->Heal(HealAmount);
			}
			if (UALSInventoryComponent* Inventory = ALSChar->FindComponentByClass<UALSInventoryComponent>())
			{
				Inventory->RemoveItem(MedkitItemID, 1);
			}
		}
	}

	OnMedkitApplyEnded.Broadcast(bCompleted);
}

void UALSMedkitComponent::EquipMedkit()
{
	AALSCharacter* ALSChar = Cast<AALSCharacter>(GetOwner());
	if (!ALSChar)
	{
		return;
	}

	ALSChar->SetOverlayState(EALSOverlayState::Box);
	ALSChar->AttachToHand(MedkitHeldMesh, nullptr, nullptr, /*bLeftHand=*/false, FVector::ZeroVector);
}
