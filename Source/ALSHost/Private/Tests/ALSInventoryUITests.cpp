#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Components/InputTestActions.h"
#include "Inventory/ALSInventoryComponent.h"
#include "Character/ALSCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

struct FALSToggleInventoryTestAction : public FTestAction
{
	FALSToggleInventoryTestAction()
	{
		InputActionName = TEXT("IA_ToggleInventory");
		InputActionValue = FInputActionValue(true);
	}
};

class FALSInventoryTestActions : public FInputTestActions
{
public:
	explicit FALSInventoryTestActions(APawn* InPawn) : FInputTestActions(InPawn)
	{
	}

	void PressToggle()
	{
		PerformAction(FALSToggleInventoryTestAction{});
	}
};

// Same real-input pattern as ALSWeaponFireInputTests - see that file and
// docs/testing.md for the FMapTestSpawner-local-player-repossession trick
// FInputTestActions needs.
TEST_CLASS(ALSInventoryUITests, "ALSHost.Inventory")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;
	UALSInventoryComponent* Inventory = nullptr;
	TUniquePtr<FALSInventoryTestActions> InputActions;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	TEST_METHOD(PressingToggleAction_ThroughRealInput_TogglesInventoryWidget)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));

				Character = &Spawner->SpawnActorAt<AALSCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				Inventory = Character->FindComponentByClass<UALSInventoryComponent>();
				ASSERT_THAT(IsNotNull(Inventory));

				APlayerController* PC = UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0);
				ASSERT_THAT(IsNotNull(PC));
				PC->Possess(Character);

				InputActions = MakeUnique<FALSInventoryTestActions>(Character);
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				ASSERT_THAT(IsFalse(Inventory->IsInventoryUIOpen()));
				InputActions->PressToggle();
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				ASSERT_THAT(IsTrue(Inventory->IsInventoryUIOpen()));
				InputActions->PressToggle();
			})
			.WaitDelay(FTimespan::FromSeconds(0.2))
			.Then([this]() {
				ASSERT_THAT(IsFalse(Inventory->IsInventoryUIOpen()));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
