#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ALSInventoryComponent.generated.h"

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
// entries a pickup actor or gameplay event can Add/Remove by FName. Intended
// as the foundation to build real item types (health packs, ammo, keys) on
// top of once there's content to define them against - see AGENT_TASKS for
// what's still open.
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

private:
	UPROPERTY(VisibleInstanceOnly, Category = "ALS|Inventory")
	TArray<FALSInventoryItem> Items;
};
