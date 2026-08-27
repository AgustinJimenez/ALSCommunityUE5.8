#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALSDamageZone.generated.h"

class UBoxComponent;

// A simple hazard volume: applies DamagePerSecond (via ApplyDamage, so it
// reaches anything with a UALSHealthComponent the same way weapon fire does)
// to any actor standing inside it, ticking once per DamageIntervalSeconds
// rather than every frame. Exists purely as a way to reliably knock the
// player's health down for testing UALSHealthPickup/regen without needing
// an enemy nearby - see AGENTS.md.
UCLASS()
class ALSHOST_API AALSDamageZone : public AActor
{
	GENERATED_BODY()

public:
	AALSDamageZone();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Damage")
	float DamagePerSecond = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Damage")
	float DamageIntervalSeconds = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Damage")
	TSubclassOf<class UDamageType> DamageTypeClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Damage")
	TObjectPtr<UBoxComponent> TriggerVolume;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyTick();

private:
	UPROPERTY()
	TSet<TObjectPtr<AActor>> OverlappingActors;

	FTimerHandle DamageTimerHandle;
};
