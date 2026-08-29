#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ALSHitReactionComponent.generated.h"

class UAnimSequenceBase;
class UALSHealthComponent;

// Plays a short reaction montage whenever the owning character takes
// damage - previously taking damage had zero animated feedback anywhere in
// this project (only the HUD health bar moved). Binds
// UALSHealthComponent::OnHealthChanged rather than needing a bespoke
// "OnDamaged" event, so it works with anything already routed through
// UGameplayStatics::ApplyDamage the same way UALSHealthComponent itself
// does. Picks a random entry from HitReactionAnimations each time - there's
// no hit-location/bone data available at OnHealthChanged (that's resolved
// deeper inside UALSWeaponFireComponent::Fire(), not surfaced through the
// delegate), so this can't distinguish a head hit from a body hit yet; a
// future refinement could thread bone info through if that's ever wanted.
UCLASS(ClassGroup = (ALSHost), meta = (BlueprintSpawnableComponent))
class ALSHOST_API UALSHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSHitReactionComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|HitReaction")
	TArray<TObjectPtr<UAnimSequenceBase>> HitReactionAnimations;

	// Same full-body slot UALSWeaponFireComponent::Reload() and
	// UALSMeleeComponent's swing animations already use.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|HitReaction")
	FName MontageSlotName = TEXT("Grounded Slot");

	// A dead character shouldn't play a hit-react on top of ragdolling -
	// skip entirely once IsDead() is true.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|HitReaction")
	float MinDamageToReact = 0.f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth, float Delta, AActor* DamageInstigator);

private:
	UPROPERTY()
	TObjectPtr<UALSHealthComponent> Health;
};
