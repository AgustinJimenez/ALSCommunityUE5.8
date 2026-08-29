#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ALSObjectiveSubsystem.generated.h"

class UALSHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FALSOnEnemyCountChanged, int32, RemainingEnemyCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FALSOnAllEnemiesDefeated);

// Tracks every AALSEnemyAIController-possessed pawn's UALSHealthComponent in
// the world and broadcasts when they've all died - the "defeat all enemies"
// objective this project otherwise had no game-loop concept of at all (no
// GameMode, no score, no win condition anywhere in the codebase before this).
// A UWorldSubsystem rather than a placed actor or a GameMode subclass: it
// needs zero level placement (auto-instantiated per world, including
// FMapTestSpawner's temp test worlds) and sidesteps subclassing
// ALS_GameMode_SP (a vendored Blueprint, not something C++ can inherit from
// directly - see AGENTS.md's "don't touch vendored plugin unless necessary"
// philosophy).
UCLASS()
class ALSHOST_API UALSObjectiveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Fired every time an enemy dies, with the new remaining count.
	UPROPERTY(BlueprintAssignable, Category = "ALS|Objective")
	FALSOnEnemyCountChanged OnEnemyCountChanged;

	// Fired exactly once, the moment the last tracked enemy dies. Never fires
	// on a level with zero enemies to begin with (nothing to defeat).
	UPROPERTY(BlueprintAssignable, Category = "ALS|Objective")
	FALSOnAllEnemiesDefeated OnAllEnemiesDefeated;

	UFUNCTION(BlueprintPure, Category = "ALS|Objective")
	int32 GetRemainingEnemyCount() const { return RemainingEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "ALS|Objective")
	bool HasAnyTrackedEnemies() const { return TrackedEnemyHealthComponents.Num() > 0; }

	// (Re)scans the world for every AALSEnemyAIController-possessed pawn not
	// already tracked and starts listening for its death. Called
	// automatically on world BeginPlay for real gameplay (level-placed
	// enemies already exist by then); tests that spawn enemies dynamically
	// after that point (the normal FMapTestSpawner pattern - see AGENTS.md)
	// need to call this again after spawning, there's no actor-spawned
	// delegate hooked here to do it automatically.
	UFUNCTION(BlueprintCallable, Category = "ALS|Objective")
	void RefreshTrackedEnemies();

protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UFUNCTION()
	void HandleEnemyDeath(AActor* Killer);

	UPROPERTY()
	TArray<TObjectPtr<UALSHealthComponent>> TrackedEnemyHealthComponents;

	int32 RemainingEnemyCount = 0;
	bool bVictoryBroadcast = false;
};
