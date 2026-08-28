#include "Weapon/ALSMeleeComponent.h"

#include "Inventory/ALSInventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/World.h"

UALSMeleeComponent::UALSMeleeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UALSMeleeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UALSMeleeComponent::HandleControllerChanged);
	}

	TrySetupInput();
}

void UALSMeleeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UALSMeleeComponent::HandleControllerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UALSMeleeComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	TrySetupInput();
}

void UALSMeleeComponent::TrySetupInput()
{
	if (bInputBound || !MeleeInputAction)
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

	EIC->BindAction(MeleeInputAction, ETriggerEvent::Started, this, &UALSMeleeComponent::HandleMeleeInput);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (MeleeInputMappingContext)
		{
			Subsystem->AddMappingContext(MeleeInputMappingContext, 0);
		}
	}

	bInputBound = true;
}

void UALSMeleeComponent::HandleMeleeInput(const FInputActionValue& Value)
{
	TryMeleeAttack();
}

bool UALSMeleeComponent::IsOnCooldown() const
{
	const UWorld* World = GetWorld();
	return World && (World->GetTimeSeconds() - LastAttackWorldTime) < AttackCooldownSeconds;
}

bool UALSMeleeComponent::HasKnifeEquipped() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const UALSInventoryComponent* Inventory = OwnerPawn ? OwnerPawn->FindComponentByClass<UALSInventoryComponent>() : nullptr;
	return Inventory && Inventory->GetItemQuantity(KnifeItemID) > 0;
}

bool UALSMeleeComponent::TryMeleeAttack()
{
	UWorld* World = GetWorld();
	if (!World || IsOnCooldown())
	{
		return false;
	}

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

	LastAttackWorldTime = World->GetTimeSeconds();

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector TraceEnd = CameraLocation + CameraRotation.Vector() * MeleeRange;

	if (SwingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwingSound, OwnerPawn->GetActorLocation());
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ALSMelee), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(OwnerPawn);

	FHitResult Hit;
	const bool bHit = World->SweepSingleByChannel(Hit, CameraLocation, TraceEnd, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(SweepRadius), QueryParams);
	if (!bHit || !Hit.GetActor())
	{
		return true;
	}

	const float Damage = HasKnifeEquipped() ? (FistDamage + KnifeDamageBonus) : FistDamage;
	UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage, CameraRotation.Vector(), Hit, PC, OwnerPawn, DamageTypeClass);

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint);
	}

	return true;
}
