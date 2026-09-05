#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Character/ALSCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Library/ALSCharacterEnumLibrary.h"

// Diagnostic test to read back exact world-space transforms for the axe
// (attached via the repurposed "Torch" slot - see AGENTS.md) relative to
// the hand bone it's attached to, so the AttachToHand Offset/RotationOffset
// values can be computed instead of guessed from screenshots. Not deleted
// after use - the assertions themselves are a real regression check
// (equipping actually attaches a mesh to a valid bone), the UE_LOG lines
// are the diagnostic payload, grepped from Saved/Logs/ALSHost.log after a
// headless run.
TEST_CLASS(ALSHeldObjectTuningTests, "ALSHost.Combat")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	TEST_METHOD(EquippingAxe_LogsHandAndMeshTransformsForTuning)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));
				Character = &Spawner->SpawnActor<AALSCharacter>(FActorSpawnParameters(), CharClass);

				Character->SetOverlayState(EALSOverlayState::Torch);

				ASSERT_THAT(IsNotNull(Character->HeldObjectRoot));
				ASSERT_THAT(IsNotNull(Character->StaticMesh));
				ASSERT_THAT(IsNotNull(Character->StaticMesh->GetStaticMesh()));

				USkeletalMeshComponent* BodyMesh = Character->GetMesh();
				ASSERT_THAT(IsNotNull(BodyMesh));
				const int32 BoneIndex = BodyMesh->GetBoneIndex(TEXT("VB LHS_ik_hand_gun"));
				ASSERT_THAT(IsTrue(BoneIndex != INDEX_NONE));

				const FTransform HandBoneWorld = BodyMesh->GetBoneTransform(BoneIndex);
				const FTransform HeldRootWorld = Character->HeldObjectRoot->GetComponentTransform();
				const FTransform StaticMeshWorld = Character->StaticMesh->GetComponentTransform();
				const FBoxSphereBounds WorldBounds = Character->StaticMesh->Bounds;

				const FVector CharForward = Character->GetActorForwardVector();
				const FVector CharRight = Character->GetActorRightVector();
				const FVector CharUp = Character->GetActorUpVector();

				// SM_Axe's own local-space bounds (from StaticMeshTools.get_bounds,
				// checked directly, not assumed): X[-2.87,3.31] Y[-28.02,41.98] Z[10.67,49.37].
				// "Tail"/"Head" here are just the two Y extremes at the
				// bounds' X/Z center - which physical end of the axe (blade
				// vs handle butt) each one actually is isn't known yet,
				// that's part of what these logged world positions should
				// help determine (whichever one ends up further from the
				// hand bone toward the fingers vs the wrist is the tail).
				const FVector TailLocal(0.22f, -28.02f, 30.02f);
				const FVector HeadLocal(0.22f, 41.98f, 30.02f);
				const FVector TailWorld = StaticMeshWorld.TransformPosition(TailLocal);
				const FVector HeadWorld = StaticMeshWorld.TransformPosition(HeadLocal);

				UE_LOG(LogTemp, Warning, TEXT("AXE_TUNE HandBone Loc=%s Rot=%s"), *HandBoneWorld.GetLocation().ToString(), *HandBoneWorld.Rotator().ToString());
				UE_LOG(LogTemp, Warning, TEXT("AXE_TUNE HeldRoot Loc=%s Rot=%s"), *HeldRootWorld.GetLocation().ToString(), *HeldRootWorld.Rotator().ToString());
				UE_LOG(LogTemp, Warning, TEXT("AXE_TUNE StaticMeshComp Loc=%s Rot=%s"), *StaticMeshWorld.GetLocation().ToString(), *StaticMeshWorld.Rotator().ToString());
				UE_LOG(LogTemp, Warning, TEXT("AXE_TUNE WorldBounds Origin=%s BoxExtent=%s"), *WorldBounds.Origin.ToString(), *WorldBounds.BoxExtent.ToString());
				UE_LOG(LogTemp, Warning, TEXT("AXE_TUNE CharForward=%s CharRight=%s CharUp=%s CharLoc=%s"), *CharForward.ToString(), *CharRight.ToString(), *CharUp.ToString(), *Character->GetActorLocation().ToString());
				UE_LOG(LogTemp, Warning, TEXT("AXE_TUNE TailWorld=%s HeadWorld=%s"), *TailWorld.ToString(), *HeadWorld.ToString());
				UE_LOG(LogTemp, Warning, TEXT("AXE_TUNE HandToTail=%s HandToHead=%s"), *(TailWorld - HandBoneWorld.GetLocation()).ToString(), *(HeadWorld - HandBoneWorld.GetLocation()).ToString());

				// User visually confirmed (screenshot, PIE) that the hand bone's
				// own blue (local Z) debug axis is where the axe's blade-up
				// direction should point - not red or green. RotationOffset's
				// Yaw already aligns the handle (mesh local Y) with the bone's
				// local X (red/forward), which Roll does not disturb (FRotator's
				// Forward vector has no Roll term). So solve for the Roll delta
				// that rotates the mesh's current local-Z (blade) axis onto the
				// bone's local Z (blue), measured in the plane perpendicular to
				// the shared forward axis.
				const FVector BoneX = HandBoneWorld.GetUnitAxis(EAxis::X);
				const FVector BoneZ = HandBoneWorld.GetUnitAxis(EAxis::Z);
				const FVector MeshZ = StaticMeshWorld.GetUnitAxis(EAxis::Z);
				FVector MeshZPerp = MeshZ - FVector::DotProduct(MeshZ, BoneX) * BoneX;
				MeshZPerp.Normalize();
				const float CosAngle = FVector::DotProduct(MeshZPerp, BoneZ);
				const float SinAngle = FVector::DotProduct(FVector::CrossProduct(MeshZPerp, BoneZ), BoneX);
				const float RollDeltaDegrees = FMath::RadiansToDegrees(FMath::Atan2(SinAngle, CosAngle));
				UE_LOG(LogTemp, Warning, TEXT("AXE_TUNE BoneX(red)=%s BoneZ(blue)=%s MeshZ(bladeaxis)=%s RollDeltaDegrees=%f"),
					*BoneX.ToString(), *BoneZ.ToString(), *MeshZ.ToString(), RollDeltaDegrees);
			});
	}

	// Real regression check for the current baked AttachToHand pin values on
	// the axe/Torch case (see AGENTS.md's grip-tuning entries) - asserts
	// HeldObjectRoot's relative transform right after equip exactly matches
	// what's baked into the Blueprint pin, independent of any live
	// ALS.HeldObject.Tune* cvar state (a fresh headless process always starts
	// those at their 0 default, so this reads the pure base value). Update
	// the expected constants here whenever the pin values are re-tuned.
	TEST_METHOD(EquippedAxe_GripTransformMatchesBakedPinValues)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));
				Character = &Spawner->SpawnActor<AALSCharacter>(FActorSpawnParameters(), CharClass);

				Character->SetOverlayState(EALSOverlayState::Torch);

				ASSERT_THAT(IsNotNull(Character->HeldObjectRoot));

				const FVector ExpectedOffset(4.38f, 0.0f, 0.0f);
				const FRotator ExpectedRotationOffset(15.76f, 75.03f, -62.47f);

				const FVector ActualOffset = Character->HeldObjectRoot->GetRelativeLocation();
				const FRotator ActualRotation = Character->HeldObjectRoot->GetRelativeRotation();

				UE_LOG(LogTemp, Warning, TEXT("AXE_TUNE_CHECK ActualOffset=%s ActualRotation=%s"),
					*ActualOffset.ToString(), *ActualRotation.ToString());

				ASSERT_THAT(IsTrue(ActualOffset.Equals(ExpectedOffset, 0.01f)));
				ASSERT_THAT(IsTrue(ActualRotation.Equals(ExpectedRotationOffset, 0.01f)));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
