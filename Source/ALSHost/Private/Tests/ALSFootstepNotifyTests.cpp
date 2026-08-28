#include "CQTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/MapTestSpawner.h"
#include "Components/ActorTestSpawner.h"
#include "Character/Animation/Notify/ALSAnimNotifyFootstep.h"
#include "Character/ALSCharacter.h"
#include "Library/ALSCharacterStructLibrary.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Sound/SoundBase.h"
#include "GameFramework/CharacterMovementComponent.h"

// Static wiring checks: the DataTable is correctly configured and every
// existing ALSAnimNotifyFootstep placement in ALS-Community's locomotion
// set actually points at it. Plain LoadObject/field reads, no world needed
// - reading AnimSequence::Notifies from C++ is unaffected by the Python
// "protected and cannot be read" block documented in AGENTS.md, since that
// block is purely a Python-reflection restriction, not a real access
// specifier.
TEST_CLASS(ALSFootstepNotifyWiringTests, "ALSHost.Animation")
{
	FActorTestSpawner Spawner;

	TEST_METHOD(FootstepHitFXDataTable_HasDefaultRow_WithValidSound)
	{
		UDataTable* DataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/ALSHost/Data/DT_FootstepHitFX.DT_FootstepHitFX"));
		ASSERT_THAT(IsNotNull(DataTable));

		FALSHitFX* Row = DataTable->FindRow<FALSHitFX>(TEXT("Default"), TEXT("ALSFootstepNotifyWiringTests"));
		ASSERT_THAT(IsNotNull(Row));
		ASSERT_THAT(IsTrue(Row->SurfaceType == EPhysicalSurface::SurfaceType_Default));
		ASSERT_THAT(IsNotNull(Row->Sound.LoadSynchronous()));
	}

	TEST_METHOD(LocomotionAnimations_FootstepNotifies_PointAtFootstepDataTable)
	{
		UDataTable* ExpectedTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/ALSHost/Data/DT_FootstepHitFX.DT_FootstepHitFX"));
		ASSERT_THAT(IsNotNull(ExpectedTable));

		const TCHAR* AnimPaths[] = {
			TEXT("/ALSV4_CPP/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Base/Locomotion/ALS_N_Walk_F.ALS_N_Walk_F"),
			TEXT("/ALSV4_CPP/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Base/Locomotion/ALS_N_Run_F.ALS_N_Run_F"),
			TEXT("/ALSV4_CPP/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Base/Locomotion/ALS_N_Sprint_F.ALS_N_Sprint_F"),
		};

		int32 TotalFootstepNotifiesChecked = 0;
		for (const TCHAR* Path : AnimPaths)
		{
			UAnimSequenceBase* Anim = LoadObject<UAnimSequenceBase>(nullptr, Path);
			ASSERT_THAT(IsNotNull(Anim));

			bool bFoundAtLeastOne = false;
			for (const FAnimNotifyEvent& Event : Anim->Notifies)
			{
				UALSAnimNotifyFootstep* Footstep = Cast<UALSAnimNotifyFootstep>(Event.Notify);
				if (!Footstep)
				{
					continue;
				}
				bFoundAtLeastOne = true;
				TotalFootstepNotifiesChecked++;
				ASSERT_THAT(IsTrue(Footstep->HitDataTable == ExpectedTable));
			}
			ASSERT_THAT(IsTrue(bFoundAtLeastOne));
		}
		ASSERT_THAT(IsTrue(TotalFootstepNotifiesChecked >= 3));
	}
};

// Full runtime check: firing the real, already-placed notify instance from
// ALS_N_Walk_F against a real character standing on real collision
// actually spawns an attached audio component with the configured sound -
// not just that the data is wired, but that wiring produces the actual
// gameplay effect.
TEST_CLASS(ALSFootstepNotifyRuntimeTests, "ALSHost.Animation")
{
	TUniquePtr<FMapTestSpawner> Spawner;
	AALSCharacter* Character = nullptr;

	BEFORE_EACH()
	{
		Spawner = FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder);
		ASSERT_THAT(IsNotNull(Spawner.Get()));
		Spawner->AddWaitUntilLoadedCommand(TestRunner);
	}

	TEST_METHOD(FootstepNotify_Fired_SpawnsAttachedAudioComponentWithConfiguredSound)
	{
		TestCommandBuilder
			.StartWhen([this]() { return Spawner.IsValid(); })
			.Then([this]() {
				// CQTest's temp level has no floor at all (see
				// docs/testing.md / ALSDamageZoneTests) - the footstep
				// notify's downward trace needs real collision to hit.
				AStaticMeshActor* Floor = Spawner->GetWorld().SpawnActor<AStaticMeshActor>(FVector(0.f, 0.f, -50.f), FRotator::ZeroRotator);
				ASSERT_THAT(IsNotNull(Floor));
				UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
				ASSERT_THAT(IsNotNull(CubeMesh));
				UStaticMeshComponent* FloorMesh = Floor->GetStaticMeshComponent();
				// A StaticMeshActor's mesh component defaults to Static
				// mobility, which warns (and may no-op collision updates)
				// on runtime SetStaticMesh/collision calls - switch to
				// Movable first so the trace-collision setup below actually
				// takes effect.
				FloorMesh->SetMobility(EComponentMobility::Movable);
				FloorMesh->SetStaticMesh(CubeMesh);
				FloorMesh->SetRelativeScale3D(FVector(20.f, 20.f, 1.f));
				FloorMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				FloorMesh->SetCollisionResponseToAllChannels(ECR_Block);

				UClass* CharClass = LoadClass<AALSCharacter>(nullptr, TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_CharacterBP.ALS_CharacterBP_C"));
				ASSERT_THAT(IsNotNull(CharClass));
				Character = &Spawner->SpawnActorAt<AALSCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters(), CharClass);
				// Pin the character exactly at the spawn location - the
				// footstep trace only needs a known foot position relative
				// to the floor, not real locomotion physics, and CQTest's
				// temp level has no ambient floor to fall onto otherwise.
				Character->GetCharacterMovement()->DisableMovement();
			})
			.WaitDelay(FTimespan::FromSeconds(0.3))
			.Then([this]() {
				USkeletalMeshComponent* Mesh = Character->GetMesh();
				ASSERT_THAT(IsNotNull(Mesh));

				UAnimSequenceBase* WalkAnim = LoadObject<UAnimSequenceBase>(nullptr,
					TEXT("/ALSV4_CPP/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Base/Locomotion/ALS_N_Walk_F.ALS_N_Walk_F"));
				ASSERT_THAT(IsNotNull(WalkAnim));

				// Pull the real, already-wired notify instance straight off
				// the production animation rather than constructing a fresh
				// one, so this exercises the actual shipped configuration
				// (trace channel, foot socket, etc.), not test-only defaults.
				UAnimNotify* FootstepNotify = nullptr;
				for (const FAnimNotifyEvent& Event : WalkAnim->Notifies)
				{
					if (Cast<UALSAnimNotifyFootstep>(Event.Notify))
					{
						FootstepNotify = Event.Notify;
						break;
					}
				}
				ASSERT_THAT(IsNotNull(FootstepNotify));

				// UAnimNotify::Notify is public on the base class even
				// though UALSAnimNotifyFootstep re-declares it under a
				// private section - C++ access control follows the static
				// (base-class) type at the call site, not the override's
				// own access specifier, so calling through this
				// UAnimNotify* pointer is legal.
				FAnimNotifyEventReference EventRef;
				FootstepNotify->Notify(Mesh, WalkAnim, EventRef);

				TArray<USceneComponent*> Children;
				Mesh->GetChildrenComponents(/*bIncludeAllDescendants=*/true, Children);
				UAudioComponent* SpawnedAudio = nullptr;
				for (USceneComponent* Child : Children)
				{
					if (UAudioComponent* AudioComp = Cast<UAudioComponent>(Child))
					{
						SpawnedAudio = AudioComp;
						break;
					}
				}
				ASSERT_THAT(IsNotNull(SpawnedAudio));
				ASSERT_THAT(IsNotNull(SpawnedAudio->Sound));
			});
	}
};

#endif // WITH_AUTOMATION_TESTS
