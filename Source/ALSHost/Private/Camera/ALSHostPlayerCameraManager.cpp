#include "Camera/ALSHostPlayerCameraManager.h"
#include "Character/ALSBaseCharacter.h"

void AALSHostPlayerCameraManager::AddZoomInput(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	// First person has no concept of camera distance - ALS's own
	// CustomCameraBehavior already collapses Location to the head/eye
	// position in that mode, so a zoom offset would just push the view
	// off-eye instead of "zooming". Ignoring the input here (rather than
	// zeroing ZoomDistanceOffset) means the third-person zoom level is
	// preserved across a mode switch instead of resetting.
	if (ControlledCharacter && ControlledCharacter->GetViewMode() == EALSViewMode::FirstPerson)
	{
		return;
	}

	// Scroll up (positive) = zoom in = smaller (more negative) offset.
	ZoomDistanceOffset = FMath::Clamp(ZoomDistanceOffset - AxisValue * ZoomStepDistance,
		MinZoomDistanceOffset, MaxZoomDistanceOffset);
}

void AALSHostPlayerCameraManager::UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTargetInternal(OutVT, DeltaTime);

	if (FMath::IsNearlyZero(ZoomDistanceOffset))
	{
		return;
	}

	if (ControlledCharacter && ControlledCharacter->GetViewMode() == EALSViewMode::FirstPerson)
	{
		return;
	}

	OutVT.POV.Location -= OutVT.POV.Rotation.Vector() * ZoomDistanceOffset;
}
