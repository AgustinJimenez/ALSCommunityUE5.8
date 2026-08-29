#include "Combat/ALSObjectiveSubsystem.h"

#include "AI/ALSEnemyAIController.h"
#include "Combat/ALSHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

void UALSObjectiveSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	RefreshTrackedEnemies();
}

void UALSObjectiveSubsystem::RefreshTrackedEnemies()
{
	TArray<AActor*> Controllers;
	UGameplayStatics::GetAllActorsOfClass(this, AALSEnemyAIController::StaticClass(), Controllers);

	for (AActor* ControllerActor : Controllers)
	{
		const AALSEnemyAIController* Controller = Cast<AALSEnemyAIController>(ControllerActor);
		const APawn* ControlledPawn = Controller ? Controller->GetPawn() : nullptr;
		UALSHealthComponent* Health = ControlledPawn ? ControlledPawn->FindComponentByClass<UALSHealthComponent>() : nullptr;
		if (!Health || Health->IsDead() || TrackedEnemyHealthComponents.Contains(Health))
		{
			continue;
		}

		TrackedEnemyHealthComponents.Add(Health);
		Health->OnDeath.AddDynamic(this, &UALSObjectiveSubsystem::HandleEnemyDeath);
	}

	RemainingEnemyCount = TrackedEnemyHealthComponents.Num();
}

void UALSObjectiveSubsystem::HandleEnemyDeath(AActor* Killer)
{
	RemainingEnemyCount = FMath::Max(0, RemainingEnemyCount - 1);
	OnEnemyCountChanged.Broadcast(RemainingEnemyCount);

	if (RemainingEnemyCount == 0 && !bVictoryBroadcast && TrackedEnemyHealthComponents.Num() > 0)
	{
		bVictoryBroadcast = true;
		OnAllEnemiesDefeated.Broadcast();
	}
}
