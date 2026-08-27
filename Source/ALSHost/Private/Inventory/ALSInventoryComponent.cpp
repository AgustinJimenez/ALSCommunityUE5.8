#include "Inventory/ALSInventoryComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"

UALSInventoryComponent::UALSInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UALSInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UALSInventoryComponent::HandleControllerChanged);
	}

	TrySetupInput();
}

void UALSInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UALSInventoryComponent::HandleControllerChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UALSInventoryComponent::HandleControllerChanged(APawn* PawnChanged, AController* OldController, AController* NewController)
{
	TrySetupInput();
}

void UALSInventoryComponent::TrySetupInput()
{
	// Same BeginPlay-before-possession gotcha UALSWeaponFireComponent's
	// input binding already documents - retry on possession.
	if (bInputBound || !ToggleInventoryUIInputAction)
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

	EIC->BindAction(ToggleInventoryUIInputAction, ETriggerEvent::Started, this, &UALSInventoryComponent::HandleToggleInventoryUI);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (ToggleInventoryUIInputMappingContext)
		{
			Subsystem->AddMappingContext(ToggleInventoryUIInputMappingContext, 0);
		}
	}

	bInputBound = true;
}

bool UALSInventoryComponent::IsInventoryUIOpen() const
{
	return InventoryWidgetInstance && InventoryWidgetInstance->IsInViewport();
}

void UALSInventoryComponent::HandleToggleInventoryUI(const FInputActionValue& Value)
{
	if (!InventoryWidgetClass)
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}

	if (!InventoryWidgetInstance)
	{
		InventoryWidgetInstance = CreateWidget<UUserWidget>(PC, InventoryWidgetClass);
	}

	if (!InventoryWidgetInstance)
	{
		return;
	}

	if (InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
	}
	else
	{
		InventoryWidgetInstance->AddToViewport();
	}
}

int32 UALSInventoryComponent::AddItem(FName ItemID, FText DisplayName, int32 Quantity, int32 MaxStack)
{
	if (Quantity <= 0 || ItemID.IsNone())
	{
		return 0;
	}

	FALSInventoryItem* Existing = Items.FindByPredicate([ItemID](const FALSInventoryItem& Item) { return Item.ItemID == ItemID; });

	if (!Existing)
	{
		FALSInventoryItem NewItem;
		NewItem.ItemID = ItemID;
		NewItem.DisplayName = DisplayName;
		NewItem.MaxStack = MaxStack;
		NewItem.Quantity = 0;
		Existing = &Items.Add_GetRef(NewItem);
	}

	const int32 SpaceLeft = FMath::Max(Existing->MaxStack - Existing->Quantity, 0);
	const int32 AmountAdded = FMath::Min(Quantity, SpaceLeft);
	Existing->Quantity += AmountAdded;

	if (AmountAdded > 0)
	{
		OnInventoryChanged.Broadcast();
	}

	return AmountAdded;
}

int32 UALSInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return 0;
	}

	const int32 Index = Items.IndexOfByPredicate([ItemID](const FALSInventoryItem& Item) { return Item.ItemID == ItemID; });
	if (Index == INDEX_NONE)
	{
		return 0;
	}

	FALSInventoryItem& Existing = Items[Index];
	const int32 AmountRemoved = FMath::Min(Quantity, Existing.Quantity);
	Existing.Quantity -= AmountRemoved;

	if (Existing.Quantity <= 0)
	{
		Items.RemoveAt(Index);
	}

	if (AmountRemoved > 0)
	{
		OnInventoryChanged.Broadcast();
	}

	return AmountRemoved;
}

int32 UALSInventoryComponent::GetItemQuantity(FName ItemID) const
{
	const FALSInventoryItem* Existing = Items.FindByPredicate([ItemID](const FALSInventoryItem& Item) { return Item.ItemID == ItemID; });
	return Existing ? Existing->Quantity : 0;
}

bool UALSInventoryComponent::HasItem(FName ItemID, int32 Quantity) const
{
	return GetItemQuantity(ItemID) >= Quantity;
}
