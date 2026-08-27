#include "AI/ALSEnemyAIController.h"

#include "Combat/ALSHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

AALSEnemyAIController::AALSEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AALSEnemyAIController::BeginPlay()
{
	Super::BeginPlay();
	TryAcquireTarget();
}

void AALSEnemyAIController::TryAcquireTarget()
{
	if (TargetPawn)
	{
		return;
	}

	TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
}

void AALSEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (const UALSHealthComponent* SelfHealth = ControlledPawn->FindComponentByClass<UALSHealthComponent>())
	{
		if (SelfHealth->IsDead())
		{
			StopMovement();
			return;
		}
	}

	TryAcquireTarget();
	if (!TargetPawn)
	{
		return;
	}

	if (const UALSHealthComponent* TargetHealth = TargetPawn->FindComponentByClass<UALSHealthComponent>())
	{
		if (TargetHealth->IsDead())
		{
			StopMovement();
			return;
		}
	}

	TimeSinceLastAttack += DeltaTime;

	const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetPawn->GetActorLocation());

	if (Distance <= AttackRange)
	{
		StopMovement();
		FaceTarget(DeltaTime);

		if (TimeSinceLastAttack >= AttackIntervalSeconds)
		{
			TimeSinceLastAttack = 0.f;
			UGameplayStatics::ApplyDamage(TargetPawn, AttackDamage, this, ControlledPawn, DamageTypeClass);
		}
	}
	else if (Distance <= SightRange)
	{
		MoveToActor(TargetPawn, AttackRange * 0.8f);
	}
	else
	{
		StopMovement();
	}
}

void AALSEnemyAIController::FaceTarget(float DeltaTime) const
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !TargetPawn)
	{
		return;
	}

	FVector ToTarget = TargetPawn->GetActorLocation() - ControlledPawn->GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRotation = ToTarget.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(ControlledPawn->GetActorRotation(), DesiredRotation, DeltaTime, 8.f);
	ControlledPawn->SetActorRotation(NewRotation);
}
