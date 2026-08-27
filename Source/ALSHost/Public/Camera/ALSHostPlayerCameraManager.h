#pragma once

#include "CoreMinimal.h"
#include "Character/ALSPlayerCameraManager.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "ALSHostPlayerCameraManager.generated.h"

// ALS computes the third-person camera's location/rotation/FOV entirely in
// C++ (AALSPlayerCameraManager::CustomCameraBehavior), driven by curves on
// the CameraBehavior AnimBP - there's no simple "TargetArmLength" to expose
// like a stock SpringArm-based third-person setup. Rather than touch that
// vendored curve-driven pipeline, this just runs after it every frame and
// pushes the already-computed camera location further along its own
// backward vector by ZoomDistanceOffset, which mouse wheel input adjusts.
UCLASS(Blueprintable, BlueprintType)
class ALSHOST_API AALSHostPlayerCameraManager : public AALSPlayerCameraManager
{
	GENERATED_BODY()

public:
	// Positive AxisValue (scroll up) zooms in (closer); negative zooms out.
	UFUNCTION(BlueprintCallable, Category = "ALS|Camera|Zoom")
	void AddZoomInput(float AxisValue);

protected:
	virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Camera|Zoom")
	float ZoomStepDistance = 25.0f;

	// 0 = ALS's normal computed distance. Negative pulls the camera closer
	// (zoomed in); clamped so it can't be pushed through the character.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Camera|Zoom")
	float MinZoomDistanceOffset = -150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Camera|Zoom")
	float MaxZoomDistanceOffset = 400.0f;

private:
	float ZoomDistanceOffset = 0.0f;
};
