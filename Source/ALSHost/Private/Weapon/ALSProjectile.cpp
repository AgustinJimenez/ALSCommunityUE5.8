#include "Weapon/ALSProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Weapon/ALSWeaponFireComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AALSProjectile::AALSProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(5.f);
	// No stock "Projectile" collision profile actually ships in vanilla
	// UE5 (checked BaseEngine.ini directly - it isn't there), so set
	// responses explicitly rather than relying on one by name, which fails
	// silently (logs a warning, leaves collision unconfigured) and let the
	// projectile fly straight through anything it should have hit.
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->OnComponentHit.AddDynamic(this, &AALSProjectile::HandleHit);
	RootComponent = CollisionComponent;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.08f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (DefaultMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(DefaultMeshFinder.Object);
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 8000.f;
	ProjectileMovement->MaxSpeed = 8000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.3f;
	ProjectileMovement->bShouldBounce = false;

	InitialLifeSpan = 0.f;
}

void AALSProjectile::InitializeProjectile(UALSWeaponFireComponent* InSourceWeapon, AController* InInstigatorController, AActor* InDamageCauser, TSubclassOf<UDamageType> InDamageTypeClass, const FVector& StartLocation)
{
	SourceWeapon = InSourceWeapon;
	InstigatorController = InInstigatorController;
	DamageCauser = InDamageCauser;
	DamageTypeClass = InDamageTypeClass;
	SpawnLocation = StartLocation;

	if (DamageCauser)
	{
		CollisionComponent->IgnoreActorWhenMoving(DamageCauser, true);
	}
}

void AALSProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(MaxLifetimeSeconds);
}

void AALSProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasHit || !OtherActor || OtherActor == DamageCauser)
	{
		return;
	}
	bHasHit = true;

	if (SourceWeapon)
	{
		const float DistanceTraveled = FVector::Dist(SpawnLocation, Hit.ImpactPoint);
		const float FinalDamage = SourceWeapon->ComputeDamageForHit(Hit.BoneName, DistanceTraveled);
		UGameplayStatics::ApplyPointDamage(OtherActor, FinalDamage, GetVelocity().GetSafeNormal(), Hit, InstigatorController, DamageCauser, DamageTypeClass);
	}

	Destroy();
}
