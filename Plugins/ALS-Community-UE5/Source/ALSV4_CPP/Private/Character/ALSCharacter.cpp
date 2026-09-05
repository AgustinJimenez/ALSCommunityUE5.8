// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community


#include "Character/ALSCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "AI/ALSAIController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ALSCharacter)

// Draws the gripping hand bone's own local coordinate axes (X=red, Y=green,
// Z=blue, standard UE gizmo convention) whenever something gets attached to
// it - lets a human visually identify, live in PIE, which of the bone's
// local axes actually points "up, out between the thumb and index finger"
// (the direction a naturally-held object's length should run), rather than
// guessing a RotationOffset from bone-transform math alone. See AGENTS.md's
// axe-grip-tuning entry for why: the bone's own rotation for this rig
// (VB LHS_ik_hand_gun) is a compound, non-axis-aligned rotation with no
// obvious "forward" by inspection.
static TAutoConsoleVariable<bool> CVarALSHeldObjectShowDebugGripAxes(
	TEXT("ALS.HeldObject.ShowDebugGripAxes"),
	true,
	TEXT("Draw the gripping hand bone's local X/Y/Z axes (red/green/blue) whenever a held object is attached - purely visual, for tuning AttachToHand's RotationOffset."),
	ECVF_Default);

// Live grip-tuning deltas, added on top of whatever Offset/RotationOffset
// AttachToHand was last called with, re-applied every Tick while something
// is held - lets a grip be nudged from the in-game console during a running
// PIE session (`ALS.HeldObject.TuneRollOffset 30`, etc.) with no
// rebuild/relaunch needed, then the final numbers read off the on-screen
// debug text and baked into the Blueprint's AttachToHand pin defaults.
static TAutoConsoleVariable<float> CVarALSHeldObjectTuneOffsetX(TEXT("ALS.HeldObject.TuneOffsetX"), 0.0f, TEXT("Live delta added to the held object's grip Offset.X, for tuning."), ECVF_Default);
static TAutoConsoleVariable<float> CVarALSHeldObjectTuneOffsetY(TEXT("ALS.HeldObject.TuneOffsetY"), 0.0f, TEXT("Live delta added to the held object's grip Offset.Y, for tuning."), ECVF_Default);
static TAutoConsoleVariable<float> CVarALSHeldObjectTuneOffsetZ(TEXT("ALS.HeldObject.TuneOffsetZ"), 0.0f, TEXT("Live delta added to the held object's grip Offset.Z, for tuning."), ECVF_Default);
static TAutoConsoleVariable<float> CVarALSHeldObjectTunePitchOffset(TEXT("ALS.HeldObject.TunePitchOffset"), 0.0f, TEXT("Live delta added to the held object's grip RotationOffset Pitch, for tuning."), ECVF_Default);
static TAutoConsoleVariable<float> CVarALSHeldObjectTuneYawOffset(TEXT("ALS.HeldObject.TuneYawOffset"), 0.0f, TEXT("Live delta added to the held object's grip RotationOffset Yaw, for tuning."), ECVF_Default);
static TAutoConsoleVariable<float> CVarALSHeldObjectTuneRollOffset(TEXT("ALS.HeldObject.TuneRollOffset"), 0.0f, TEXT("Live delta added to the held object's grip RotationOffset Roll, for tuning."), ECVF_Default);

// Periodically logs the current base/tune/final grip values to the output
// log while something is held - lets a human watch the console/log file
// directly to confirm whether anything is silently resetting
// HeldObjectBaseOffset/RotationOffset over time, independent of the
// on-screen readout (which only shows while a tune delta is non-zero) and
// independent of the debug menu being open at all. 0 disables logging.
static TAutoConsoleVariable<float> CVarALSHeldObjectDebugLogIntervalSeconds(
	TEXT("ALS.HeldObject.DebugLogIntervalSeconds"),
	2.0f,
	TEXT("How often (seconds) to log the held object's current base/tune/final grip values while something is held. 0 disables."),
	ECVF_Default);

AALSCharacter::AALSCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HeldObjectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HeldObjectRoot"));
	HeldObjectRoot->SetupAttachment(GetMesh());

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(HeldObjectRoot);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(HeldObjectRoot);

	AIControllerClass = AALSAIController::StaticClass();
}

void AALSCharacter::ClearHeldObject()
{
	StaticMesh->SetStaticMesh(nullptr);
	SkeletalMesh->SetSkeletalMesh(nullptr);
	SkeletalMesh->SetAnimInstanceClass(nullptr);
	HeldObjectAttachBone = NAME_None;
}

void AALSCharacter::AttachToHand(UStaticMesh* NewStaticMesh, USkeletalMesh* NewSkeletalMesh, UClass* NewAnimClass,
                                 bool bLeftHand, FVector Offset, FRotator RotationOffset)
{
	ClearHeldObject();

	if (IsValid(NewStaticMesh))
	{
		StaticMesh->SetStaticMesh(NewStaticMesh);
	}
	else if (IsValid(NewSkeletalMesh))
	{
		SkeletalMesh->SetSkeletalMesh(NewSkeletalMesh);
		if (IsValid(NewAnimClass))
		{
			SkeletalMesh->SetAnimInstanceClass(NewAnimClass);
		}
	}

	FName AttachBone;
	if (bLeftHand)
	{
		AttachBone = TEXT("VB LHS_ik_hand_gun");
	}
	else
	{
		AttachBone = TEXT("VB RHS_ik_hand_gun");
	}

	HeldObjectRoot->AttachToComponent(GetMesh(),
	                                  FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachBone);
	HeldObjectRoot->SetRelativeLocation(Offset);
	HeldObjectRoot->SetRelativeRotation(RotationOffset);

	HeldObjectBaseOffset = Offset;
	HeldObjectBaseRotationOffset = RotationOffset;
	HeldObjectAttachBone = (IsValid(NewStaticMesh) || IsValid(NewSkeletalMesh)) ? AttachBone : NAME_None;
}

void AALSCharacter::RagdollStart()
{
	ClearHeldObject();
	Super::RagdollStart();
}

void AALSCharacter::RagdollEnd()
{
	Super::RagdollEnd();
	UpdateHeldObject();
}

ECollisionChannel AALSCharacter::GetThirdPersonTraceParams(FVector& TraceOrigin, float& TraceRadius)
{
	const FName CameraSocketName = bRightShoulder ? TEXT("TP_CameraTrace_R") : TEXT("TP_CameraTrace_L");
	TraceOrigin = GetMesh()->GetSocketLocation(CameraSocketName);
	TraceRadius = 15.0f;
	return ECC_Camera;
}

FTransform AALSCharacter::GetThirdPersonPivotTarget()
{
	return FTransform(GetActorRotation(),
	                  (GetMesh()->GetSocketLocation(TEXT("Head")) + GetMesh()->GetSocketLocation(TEXT("root"))) / 2.0f,
	                  FVector::OneVector);
}

FVector AALSCharacter::GetFirstPersonCameraTarget()
{
	return GetMesh()->GetSocketLocation(TEXT("FP_Camera"));
}

void AALSCharacter::OnOverlayStateChanged(EALSOverlayState PreviousState)
{
	Super::OnOverlayStateChanged(PreviousState);
	UpdateHeldObject();
}

void AALSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHeldObjectAnimations();

	if (!HeldObjectAttachBone.IsNone())
	{
		const FVector TuneOffset(CVarALSHeldObjectTuneOffsetX.GetValueOnGameThread(),
		                         CVarALSHeldObjectTuneOffsetY.GetValueOnGameThread(),
		                         CVarALSHeldObjectTuneOffsetZ.GetValueOnGameThread());
		const FRotator TuneRotation(CVarALSHeldObjectTunePitchOffset.GetValueOnGameThread(),
		                            CVarALSHeldObjectTuneYawOffset.GetValueOnGameThread(),
		                            CVarALSHeldObjectTuneRollOffset.GetValueOnGameThread());
		// Always re-apply base+tune, not just while the tune deltas are
		// non-zero - this used to be gated behind
		// "!TuneOffset.IsNearlyZero() || !TuneRotation.IsNearlyZero()",
		// which meant clicking Reset (zeroing the cvars) stopped this block
		// from running at all, leaving HeldObjectRoot stuck at whatever the
		// last non-zero-tune transform was instead of snapping back to the
		// base Offset/RotationOffset. Only the on-screen readout stays
		// conditional, so it doesn't linger forever once nothing is
		// actually being tuned.
		const FVector FinalOffset = HeldObjectBaseOffset + TuneOffset;
		const FRotator FinalRotation = HeldObjectBaseRotationOffset + TuneRotation;
		HeldObjectRoot->SetRelativeLocation(FinalOffset);
		HeldObjectRoot->SetRelativeRotation(FinalRotation);
		// Every AALSCharacter with something held runs this same Tick() code
		// - in a level with dozens of AI characters also holding weapons,
		// gating this debug output to the player only avoids two real bugs:
		// AddOnScreenDebugMessage's hardcoded key was shared across every
		// instance, so whichever character ticked last each frame silently
		// stomped every other character's on-screen readout (confirmed live -
		// the player's tuning sliders and the on-screen text showed
		// completely different numbers because an NPC's Tick() had
		// overwritten the message); and the periodic log line below would
		// otherwise spam once per AI character every interval, drowning out
		// the one actually being tuned.
		const bool bIsPlayerControlled = IsPlayerControlled();

		if (bIsPlayerControlled && GEngine && (!TuneOffset.IsNearlyZero() || !TuneRotation.IsNearlyZero()))
		{
			GEngine->AddOnScreenDebugMessage(4471001, 0.0f, FColor::Yellow,
				FString::Printf(TEXT("Grip Offset=%s RotationOffset=%s"), *FinalOffset.ToString(), *FinalRotation.ToString()));
		}

		const float DebugLogInterval = CVarALSHeldObjectDebugLogIntervalSeconds.GetValueOnGameThread();
		if (bIsPlayerControlled && DebugLogInterval > 0.0f)
		{
			HeldObjectDebugLogAccumulatedTime += DeltaTime;
			if (HeldObjectDebugLogAccumulatedTime >= DebugLogInterval)
			{
				HeldObjectDebugLogAccumulatedTime = 0.0f;
				UE_LOG(LogTemp, Log, TEXT("HeldObjectGrip Base=(Offset=%s Rotation=%s) Tune=(Offset=%s Rotation=%s) Final=(Offset=%s Rotation=%s)"),
					*HeldObjectBaseOffset.ToString(), *HeldObjectBaseRotationOffset.ToString(),
					*TuneOffset.ToString(), *TuneRotation.ToString(),
					*FinalOffset.ToString(), *FinalRotation.ToString());
			}
		}

		if (CVarALSHeldObjectShowDebugGripAxes.GetValueOnGameThread())
		{
			const int32 BoneIndex = GetMesh()->GetBoneIndex(HeldObjectAttachBone);
			if (BoneIndex != INDEX_NONE)
			{
				const FTransform BoneWorld = GetMesh()->GetBoneTransform(BoneIndex);
				DrawDebugCoordinateSystem(GetWorld(), BoneWorld.GetLocation(), BoneWorld.Rotator(), 25.0f,
				                          false, -1.0f, 0, 2.0f);
			}
		}
	}
}

void AALSCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHeldObject();
}
