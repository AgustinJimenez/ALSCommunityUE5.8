#include "Camera/ALSHostPlayerCameraManager.h"

void AALSHostPlayerCameraManager::AddZoomInput(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
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

	OutVT.POV.Location -= OutVT.POV.Rotation.Vector() * ZoomDistanceOffset;
}
