#include "Interaction/ALSInteractable.h"

#include "Components/TextRenderComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

// SetInteractPromptVisible_Implementation needs no definition here - UHT
// auto-generates a no-op default body for a BlueprintNativeEvent declared
// on an interface (same as Interact/GetInteractionPrompt above, which have
// never needed one either); AALSPickupBase/AALSDoor/AALSLootContainer each
// override it with a real implementation.

void ALSFaceTextTowardCamera(UTextRenderComponent* Text, const UObject* WorldContextObject)
{
	if (!Text)
	{
		return;
	}

	const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(WorldContextObject, 0);
	if (!CameraManager)
	{
		return;
	}

	const FVector ToCamera = CameraManager->GetCameraLocation() - Text->GetComponentLocation();
	if (!ToCamera.IsNearlyZero())
	{
		Text->SetWorldRotation(ToCamera.Rotation());
	}
}
