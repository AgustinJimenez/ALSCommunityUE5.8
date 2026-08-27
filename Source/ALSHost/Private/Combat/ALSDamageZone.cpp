#include "Combat/ALSDamageZone.h"

#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Camera/PlayerCameraManager.h"

AALSDamageZone::AALSDamageZone()
{
	// Ticks solely to keep LabelText facing the camera (see Tick()).
	PrimaryActorTick.bCanEverTick = true;

	// ApplyDamage with an unset DamageTypeClass doesn't reliably reach
	// UALSHealthComponent's OnTakeAnyDamage handler the same way an explicit
	// UDamageType::StaticClass() does (see ALSHealthComponentTests, which
	// always passes it explicitly) - default to it here so a zone dropped in
	// the editor with no DamageTypeClass set still actually damages.
	DamageTypeClass = UDamageType::StaticClass();

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	// Z extent generous enough to stay overlapping a standing character even
	// as it settles from spawn height onto the floor (a shallower box was
	// losing overlap within the first ~1s as the capsule dropped and
	// resettled slightly, briefly exiting the volume's Z range) - see
	// AGENTS.md.
	TriggerVolume->InitBoxExtent(FVector(150.f, 150.f, 200.f));
	// Same reasoning as AALSPickupBase::TriggerSphere - a dedicated
	// trigger-volume collision profile avoids a one-sided Overlap response
	// still resolving as a physical block against the character capsule's
	// own default Pawn preset.
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = TriggerVolume;

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(RootComponent);
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, 220.f));
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextBottom);
	LabelText->SetWorldSize(28.f);
	LabelText->SetTextRenderColor(FColor::Red);
}

void AALSDamageZone::BeginPlay()
{
	Super::BeginPlay();

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AALSDamageZone::HandleBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AALSDamageZone::HandleEndOverlap);

	GetWorldTimerManager().SetTimer(DamageTimerHandle, this, &AALSDamageZone::ApplyTick, DamageIntervalSeconds, /*bLoop=*/true);

	if (LabelText)
	{
		LabelText->SetText(ZoneLabel);
	}
}

void AALSDamageZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!LabelText)
	{
		return;
	}

	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCamera = CameraManager->GetCameraLocation() - LabelText->GetComponentLocation();
		if (!ToCamera.IsNearlyZero())
		{
			LabelText->SetWorldRotation(ToCamera.Rotation());
		}
	}
}

void AALSDamageZone::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		OverlappingActors.Add(OtherActor);
	}
}

void AALSDamageZone::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	OverlappingActors.Remove(OtherActor);
}

void AALSDamageZone::ApplyTick()
{
	if (DamagePerSecond <= 0.f)
	{
		return;
	}

	const float Damage = DamagePerSecond * DamageIntervalSeconds;
	for (AActor* Actor : OverlappingActors)
	{
		if (IsValid(Actor))
		{
			UGameplayStatics::ApplyDamage(Actor, Damage, nullptr, this, DamageTypeClass);
		}
	}
}
