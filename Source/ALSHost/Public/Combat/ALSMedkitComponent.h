#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "InputActionValue.h"
#include "ALSMedkitComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UStaticMesh;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FALSOnMedkitApplyStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FALSOnMedkitApplyProgress, float, Progress01);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FALSOnMedkitApplyEnded, bool, bCompleted);

// "Hold still and apply a medkit" - equipping the medkit from the inventory
// panel (see EquipMedkit(), called from UALSInventoryWidget::EquipItem)
// reuses ALS-Community's own Box overlay state (a real two-handed carry
// pose, otherwise unused in this project) and swaps in the real medkit
// mesh via AttachToHand, so no new overlay-state enum value or animation
// content is needed. With the medkit equipped, the same left-click (IA_Fire)
// that would normally fire a weapon starts a timed heal instead -
// UALSWeaponFireComponent::StartFiring() already no-ops on its own when no
// skeletal weapon is attached (which AttachToHand ensures here, since only
// a StaticMesh is passed), so no change was needed there.
UCLASS(ClassGroup = (ALSHost), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSMedkitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSMedkitComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Medkit")
	FName MedkitItemID = TEXT("Item_Medkit");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Medkit")
	float HealAmount = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Medkit")
	float ApplyDurationSeconds = 3.0f;

	// Cancels the in-progress apply once the character has physically moved
	// this far (cm) from where the apply started. Checked as accumulated
	// displacement rather than instantaneous velocity - GetVelocity() can
	// blip nonzero for a frame or two from spawn overlap resolution or
	// animation root motion even while genuinely standing still, which
	// false-cancels a legitimate "not moving" apply; displacement only
	// grows if the character actually ends up somewhere else. Naturally
	// covers sprinting too, since sprinting is just faster displacement.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Medkit")
	float MovementCancelDistance = 30.0f;

	// Real medkit mesh shown in-hand once equipped (see EquipMedkit()) -
	// defaults to the migrated medkit box mesh already used by
	// AALSHealthPickup.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Medkit")
	TObjectPtr<UStaticMesh> MedkitHeldMesh;

	// Reuses the existing IA_Fire/IMC_Fire assets (both are also bound by
	// UALSWeaponFireComponent already) rather than introducing a new key -
	// "click to use" while the medkit is equipped is the whole point.
	// TrySetupInput() binds the action but deliberately does NOT add its
	// own mapping context, since UALSWeaponFireComponent's BeginPlay
	// already adds IMC_Fire once; adding it twice would be redundant.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Medkit|Input")
	TObjectPtr<UInputAction> ApplyInputAction;

	UPROPERTY(BlueprintAssignable, Category = "ALS|Medkit")
	FALSOnMedkitApplyStarted OnMedkitApplyStarted;

	UPROPERTY(BlueprintAssignable, Category = "ALS|Medkit")
	FALSOnMedkitApplyProgress OnMedkitApplyProgress;

	UPROPERTY(BlueprintAssignable, Category = "ALS|Medkit")
	FALSOnMedkitApplyEnded OnMedkitApplyEnded;

	// What pressing the input action does - exposed directly so it's
	// testable/callable without needing real input injection, matching
	// StartFiring()/TryInteract()'s existing testability pattern. Returns
	// false (no-op) if the medkit isn't the equipped item, isn't in the
	// inventory, or health is already full.
	UFUNCTION(BlueprintCallable, Category = "ALS|Medkit")
	bool TryStartApplyingMedkit();

	UFUNCTION(BlueprintCallable, Category = "ALS|Medkit")
	void CancelApplyingMedkit();

	UFUNCTION(BlueprintPure, Category = "ALS|Medkit")
	bool IsApplyingMedkit() const { return bIsApplying; }

	// Called by UALSInventoryWidget::EquipItem when the clicked item's ID
	// matches MedkitItemID. Sets the Box overlay pose and attaches
	// MedkitHeldMesh, overriding whatever default mesh ALS_CharacterBP's
	// OnUpdateHeldObject switch would otherwise show for Box.
	UFUNCTION(BlueprintCallable, Category = "ALS|Medkit")
	void EquipMedkit();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void TrySetupInput();
	void HandleApplyInput(const FInputActionValue& Value);

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageInstigator);

private:
	void FinishApplyingMedkit(bool bCompleted);

	bool bInputBound = false;
	bool bIsApplying = false;
	float ApplyElapsedSeconds = 0.0f;
	FVector ApplyStartLocation = FVector::ZeroVector;
};
