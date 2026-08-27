#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ALSEnemyAIController.generated.h"

// Simple chase+melee-attack controller for a basic enemy, driven by a plain
// Tick FSM rather than a Behavior Tree - ClaudeUnrealMCP has tools to create
// an empty BehaviorTree/Blackboard and read one back
// (create_behavior_tree/create_blackboard/read_behavior_tree), but no tool
// to actually add composite/task/decorator nodes into one, which a real
// chase-vs-attack tree needs. See AGENTS.md/AGENT_TASKS for the note on
// building that tool if a proper BT-driven version is wanted later; this
// gets a working enemy without it.
UCLASS()
class ALSHOST_API AALSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AALSEnemyAIController();

	// Distance at which the enemy starts chasing the player.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Enemy")
	float SightRange = 1500.f;

	// Distance at which the enemy stops moving and starts attacking.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Enemy")
	float AttackRange = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Enemy")
	float AttackDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Enemy")
	float AttackIntervalSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Enemy")
	TSubclassOf<class UDamageType> DamageTypeClass;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<APawn> TargetPawn;

	float TimeSinceLastAttack = 0.f;

	void TryAcquireTarget();
	void FaceTarget(float DeltaTime) const;
};
