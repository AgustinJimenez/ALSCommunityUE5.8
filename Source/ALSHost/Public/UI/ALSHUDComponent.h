#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ALSHUDComponent.generated.h"

// Owns the lifetime of the always-on player HUD (status bars for now).
// Only creates it for a locally-controlled pawn, so attaching this same
// component to an AI-controlled enemy character (for its own
// UALSHealthComponent) doesn't spawn a HUD nobody's meant to see.
UCLASS(ClassGroup = (ALS), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSHUDComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|UI")
	TSubclassOf<class UUserWidget> StatusBarsWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> StatusBarsWidgetInstance;

	// A pawn's BeginPlay commonly runs before its PlayerController actually
	// possesses it (see AGENTS.md, same gotcha UALSWeaponFireComponent's
	// input binding hit) - retry HUD creation on possession too.
	UFUNCTION()
	void HandleControllerChanged(APawn* PawnChanged, AController* OldController, AController* NewController);

	void TryCreateHUD();
};
