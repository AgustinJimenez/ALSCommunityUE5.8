#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ALSInventoryComponent.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

USTRUCT(BlueprintType)
struct FALSInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Inventory")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Inventory")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Inventory")
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Inventory")
	int32 MaxStack = 99;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FALSOnInventoryChanged);

// Minimal generic stack-based inventory: no item data assets yet (there's no
// item content in the project to back them), just ID/DisplayName/Quantity
// entries a pickup actor or gameplay event can Add/Remove by FName. Also
// owns showing/hiding a simple list widget on its own input action, the
// same "component binds its own input" pattern UALSWeaponFireComponent
// already uses - so no separate UI-only component is needed just to toggle
// visibility.
UCLASS(ClassGroup = (ALS), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "ALS|Inventory")
	FALSOnInventoryChanged OnInventoryChanged;

	// Adds up to Quantity of ItemID, respecting MaxStack (first time an item
	// is added, its DisplayName/MaxStack are recorded; later adds of the same
	// ItemID reuse the stored MaxStack regardless of what's passed).
	// Returns how many were actually added (may be less than Quantity if the
	// stack filled up).
	UFUNCTION(BlueprintCallable, Category = "ALS|Inventory")
	int32 AddItem(FName ItemID, FText DisplayName, int32 Quantity, int32 MaxStack = 99);

	// Removes up to Quantity of ItemID. Returns how many were actually
	// removed (may be less than Quantity if fewer were held).
	UFUNCTION(BlueprintCallable, Category = "ALS|Inventory")
	int32 RemoveItem(FName ItemID, int32 Quantity);

	UFUNCTION(BlueprintPure, Category = "ALS|Inventory")
	int32 GetItemQuantity(FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "ALS|Inventory")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

	UFUNCTION(BlueprintPure, Category = "ALS|Inventory")
	const TArray<FALSInventoryItem>& GetItems() const { return Items; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Inventory|UI")
	TObjectPtr<UInputAction> ToggleInventoryUIInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Inventory|UI")
	TObjectPtr<UInputMappingContext> ToggleInventoryUIInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Inventory|UI")
	TSubclassOf<class UUserWidget> InventoryWidgetClass;

	// Exposed for testability (and any Blueprint/UI code that wants to
	// check current state) rather than just an internal HandleToggleInventoryUI
	// implementation detail.
	UFUNCTION(BlueprintPure, Category = "ALS|Inventory|UI")
	bool IsInventoryUIOpen() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleControllerChanged(APawn* PawnChanged, AController* OldController, AController* NewController);

	void TrySetupInput();
	void HandleToggleInventoryUI(const FInputActionValue& Value);

private:
	UPROPERTY(VisibleInstanceOnly, Category = "ALS|Inventory")
	TArray<FALSInventoryItem> Items;

	UPROPERTY()
	TObjectPtr<class UUserWidget> InventoryWidgetInstance;

	bool bInputBound = false;
};
