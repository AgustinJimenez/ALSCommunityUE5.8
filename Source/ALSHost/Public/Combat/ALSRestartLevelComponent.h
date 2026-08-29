#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "ALSRestartLevelComponent.generated.h"

class UInputAction;
class UInputMappingContext;

// Lets the player reload the current level from scratch at any time. Added
// alongside UALSObjectiveSubsystem's win condition - once a session actually
// has something to win, there was no way to "play again" short of manually
// reopening the level yourself. Binds its own key/mapping context the same
// self-contained way every other input-driven component in this project
// does (see UALSInteractionComponent) - R was already Reload, so this needs
// its own key, not a reused one (see AGENTS.md's key-collision lesson from
// the Tab/L rebind saga).
UCLASS(ClassGroup = (ALS), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSRestartLevelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSRestartLevelComponent();

	// Reopens the current level fresh. Public/BlueprintCallable so it's
	// usable directly (e.g. from a UI "Restart" button) without needing the
	// key press.
	UFUNCTION(BlueprintCallable, Category = "ALS|Restart")
	void RestartLevel() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleRestartInput(const FInputActionValue& Value);
	void TrySetupInput();

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Restart|Input")
	TObjectPtr<UInputAction> RestartInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Restart|Input")
	TObjectPtr<UInputMappingContext> RestartInputMappingContext;

private:
	bool bInputBound = false;
};
