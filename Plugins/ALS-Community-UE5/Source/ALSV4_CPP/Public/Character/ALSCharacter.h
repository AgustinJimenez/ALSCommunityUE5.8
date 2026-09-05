// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community


#pragma once

#include "CoreMinimal.h"
#include "Character/ALSBaseCharacter.h"
#include "ALSCharacter.generated.h"

/**
 * Specialized character class, with additional features like held object etc.
 */
UCLASS(Blueprintable, BlueprintType)
class ALSV4_CPP_API AALSCharacter : public AALSBaseCharacter
{
	GENERATED_BODY()

public:
	AALSCharacter(const FObjectInitializer& ObjectInitializer);

	/** Implemented on BP to update held objects */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALS|HeldObject")
	void UpdateHeldObject();

	UFUNCTION(BlueprintCallable, Category = "ALS|HeldObject")
	void ClearHeldObject();

	UFUNCTION(BlueprintCallable, Category = "ALS|HeldObject")
	void AttachToHand(UStaticMesh* NewStaticMesh, USkeletalMesh* NewSkeletalMesh,
	                  class UClass* NewAnimClass, bool bLeftHand, FVector Offset,
	                  FRotator RotationOffset = FRotator::ZeroRotator);

	virtual void RagdollStart() override;

	virtual void RagdollEnd() override;

	virtual ECollisionChannel GetThirdPersonTraceParams(FVector& TraceOrigin, float& TraceRadius) override;

	virtual FTransform GetThirdPersonPivotTarget() override;

	virtual FVector GetFirstPersonCameraTarget() override;

	// The Offset/RotationOffset the currently-held object was last equipped
	// with (i.e. what's baked into the Blueprint's AttachToHand call), with
	// no live ALS.HeldObject.Tune* delta applied - lets external code (the
	// grip-tuning widget) show/edit an *absolute* grip value instead of a
	// delta from an invisible baseline. See HeldObjectBaseOffset.
	UFUNCTION(BlueprintCallable, Category = "ALS|HeldObject")
	FVector GetHeldObjectBaseOffset() const { return HeldObjectBaseOffset; }

	UFUNCTION(BlueprintCallable, Category = "ALS|HeldObject")
	FRotator GetHeldObjectBaseRotationOffset() const { return HeldObjectBaseRotationOffset; }

protected:
	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlay() override;

	virtual void OnOverlayStateChanged(EALSOverlayState PreviousState) override;

	/** Implement on BP to update animation states of held objects */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALS|HeldObject")
	void UpdateHeldObjectAnimations();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Component")
	TObjectPtr<USceneComponent> HeldObjectRoot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Component")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Component")
	TObjectPtr<UStaticMeshComponent> StaticMesh = nullptr;

private:
	bool bNeedsColorReset = false;

	// Which bone AttachToHand last attached HeldObjectRoot to, empty if
	// nothing is currently held - lets Tick() keep redrawing the grip debug
	// axes at the hand's current (possibly animating) position every frame,
	// rather than a one-shot draw that goes stale the moment the character
	// moves. See ALS.HeldObject.ShowDebugGripAxes.
	FName HeldObjectAttachBone;

	// The Offset/RotationOffset AttachToHand was last called with - Tick()
	// re-applies these plus the live ALS.HeldObject.Tune* cvar deltas every
	// frame while something is held, so a grip can be nudged from the
	// console in a running PIE session with no rebuild/relaunch needed. See
	// ALS.HeldObject.TuneRollOffset etc.
	FVector HeldObjectBaseOffset = FVector::ZeroVector;
	FRotator HeldObjectBaseRotationOffset = FRotator::ZeroRotator;

	// Accumulates DeltaTime so Tick() can log the current base/tune/final
	// grip values to the output log every couple of seconds while something
	// is held - a way to visually confirm from the console log whether
	// anything is resetting HeldObjectBaseOffset/RotationOffset over time,
	// independent of the on-screen readout (which only shows while a tune
	// delta is non-zero). See ALS.HeldObject.DebugLogIntervalSeconds.
	float HeldObjectDebugLogAccumulatedTime = 0.0f;
};
