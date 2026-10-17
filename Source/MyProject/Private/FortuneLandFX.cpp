#include "FortuneLandFX.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFortuneLandFX::AFortuneLandFX()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
	Light->SetIntensity(0.f);
	Light->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shape(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	SphereMesh = Sph.Object;
	CubeMesh = Cube.Object;
	LitMaterial = Shape.Object;
}

void AFortuneLandFX::SpawnAt(UWorld* World, const FVector& Location, EFortuneLandFX Type, float Power, bool bHeads)
{
	if (!World || Type == EFortuneLandFX::None) return;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AFortuneLandFX* FX = World->SpawnActor<AFortuneLandFX>(AFortuneLandFX::StaticClass(), FTransform(Location), P))
	{
		FX->Play(Type, Power, bHeads);
	}
}

int32 AFortuneLandFX::AddGroup(UStaticMesh* Mesh, const FLinearColor& Color, bool bGlow, float Glow)
{
	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
	ISM->SetMobility(EComponentMobility::Movable);
	ISM->SetupAttachment(Root);
	ISM->SetStaticMesh(Mesh);
	ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ISM->SetCastShadow(false);
	ISM->RegisterComponent();

	UMaterialInterface* Base = (bGlow && GlowMaterial) ? GlowMaterial.Get() : LitMaterial.Get();
	if (UMaterialInstanceDynamic* MID = ISM->CreateDynamicMaterialInstance(0, Base))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Glow"), Glow);
	}
	return Groups.Add(ISM);
}

FVector AFortuneLandFX::RandRing(float MinSpeed, float MaxSpeed, float Up) const
{
	const float A = FMath::FRandRange(0.f, 2.f * PI);
	const float S = FMath::FRandRange(MinSpeed, MaxSpeed);
	return FVector(FMath::Cos(A) * S, FMath::Sin(A) * S, Up);
}

void AFortuneLandFX::Emit(int32 Group, const FVector& Vel, const FVector& Size, float Life, float Gravity, float Drag,
	const FVector& Offset, bool bGrowFirst, bool bSpin, bool bFlicker)
{
	if (!Groups.IsValidIndex(Group)) return;
	FParticle P;
	P.Group = Group;
	P.Pos = GetActorLocation() + Offset;
	P.Vel = Vel;
	P.Size = Size;
	P.Life = FMath::Max(0.05f, Life);
	P.Gravity = Gravity;
	P.Drag = Drag;
	P.bGrowFirst = bGrowFirst;
	P.bFlicker = bFlicker;
	P.Rot = FRotator(FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f));
	if (bSpin)
	{
		P.RotVel = FRotator(FMath::FRandRange(-400.f, 400.f), FMath::FRandRange(-400.f, 400.f), FMath::FRandRange(-400.f, 400.f));
	}
	P.Instance = Groups[Group]->AddInstance(FTransform(P.Rot, P.Pos, FVector::ZeroVector), /*bWorldSpace*/ true);
	Particles.Add(P);
}

void AFortuneLandFX::Flash(const FLinearColor& Color, float Intensity, float Radius)
{
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(Radius);
	LightIntensity = Intensity;
	LightAge = 0.f;
	Light->SetIntensity(Intensity);
}

void AFortuneLandFX::Play(EFortuneLandFX Type, float Power, bool bHeads)
{
	// Isik saçan parcaciklar icin projedeki M_FX_Glow (Python betigi olusturur), yoksa normal materyal
	GlowMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_FX_Glow.M_FX_Glow"), nullptr, LOAD_NoWarn | LOAD_Quiet);

	const float P = FMath::Max(0.3f, Power) * (bHeads ? 1.f : 0.7f);
	auto N = [P](int32 Base) { return FMath::Max(1, FMath::RoundToInt32(Base * P)); };
	const float S = 0.01f;   // Sphere/Cube 100 cm -> 1 cm

	switch (Type)
	{
	case EFortuneLandFX::Dust:
	{
		const int32 G = AddGroup(SphereMesh, FLinearColor(0.42f, 0.36f, 0.3f), false);
		for (int32 i = 0; i < N(16); ++i)
		{
			const float Sz = FMath::FRandRange(0.8f, 1.6f) * S;
			Emit(G, RandRing(15.f, 45.f, FMath::FRandRange(5.f, 20.f)), FVector(Sz), FMath::FRandRange(0.45f, 0.8f), -20.f, 3.f,
				FVector(0.f, 0.f, 0.5f), /*grow*/ true);
		}
		break;
	}
	case EFortuneLandFX::Sparks:
	{
		const int32 G = AddGroup(CubeMesh, FLinearColor(1.f, 0.95f, 0.8f), true, 12.f);
		for (int32 i = 0; i < N(22); ++i)
		{
			Emit(G, RandRing(50.f, 140.f, FMath::FRandRange(80.f, 180.f)), FVector(0.6f, 0.12f, 0.12f) * S, FMath::FRandRange(0.3f, 0.6f),
				-700.f, 1.f, FVector(0.f, 0.f, 1.f), false, true);
		}
		Flash(FLinearColor(1.f, 0.95f, 0.85f), 900.f * P, 70.f);
		break;
	}
	case EFortuneLandFX::GoldRing:
	{
		const int32 Ring = AddGroup(SphereMesh, FLinearColor(1.f, 0.72f, 0.2f), true, 8.f);
		const int32 Count = N(30);
		for (int32 i = 0; i < Count; ++i)
		{
			const float A = 2.f * PI * i / Count;
			const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
			Emit(Ring, Dir * 90.f * FMath::Sqrt(P), FVector(0.7f * S), 0.5f, 0.f, 2.5f, Dir * 2.f + FVector(0.f, 0.f, 0.3f));
		}
		const int32 Glit = AddGroup(SphereMesh, FLinearColor(1.f, 0.9f, 0.5f), true, 14.f);
		for (int32 i = 0; i < N(14); ++i)
		{
			Emit(Glit, RandRing(5.f, 30.f, FMath::FRandRange(60.f, 130.f)), FVector(0.35f * S), FMath::FRandRange(0.5f, 0.9f), -150.f, 1.5f,
				FVector(0.f, 0.f, 1.f), false, false, /*flicker*/ true);
		}
		Flash(FLinearColor(1.f, 0.75f, 0.3f), 1200.f * P, 90.f);
		break;
	}
	case EFortuneLandFX::Leaves:
	{
		const int32 Leaf = AddGroup(CubeMesh, FLinearColor(0.15f, 0.65f, 0.2f), false);
		for (int32 i = 0; i < N(14); ++i)
		{
			Emit(Leaf, RandRing(25.f, 70.f, FMath::FRandRange(60.f, 120.f)), FVector(1.4f, 0.7f, 0.06f) * S, FMath::FRandRange(1.f, 1.5f),
				-160.f, 2.5f, FVector(0.f, 0.f, 1.f), false, true);
		}
		const int32 Clover = AddGroup(SphereMesh, FLinearColor(0.35f, 1.f, 0.4f), true, 5.f);
		for (int32 i = 0; i < N(6); ++i)
		{
			Emit(Clover, RandRing(10.f, 30.f, FMath::FRandRange(40.f, 80.f)), FVector(0.5f * S), FMath::FRandRange(0.7f, 1.1f), -120.f, 2.f,
				FVector(0.f, 0.f, 1.f), false, false, true);
		}
		Flash(FLinearColor(0.4f, 1.f, 0.45f), 500.f * P, 60.f);
		break;
	}
	case EFortuneLandFX::Flame:
	{
		const int32 Ring = AddGroup(SphereMesh, FLinearColor(1.f, 0.35f, 0.05f), true, 10.f);
		const int32 Count = N(24);
		for (int32 i = 0; i < Count; ++i)
		{
			const float A = 2.f * PI * i / Count;
			const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
			Emit(Ring, Dir * 70.f + FVector(0.f, 0.f, 15.f), FVector(1.1f * S), 0.45f, 30.f, 3.f, Dir * 2.f, true, false, true);
		}
		const int32 Ember = AddGroup(SphereMesh, FLinearColor(1.f, 0.6f, 0.15f), true, 14.f);
		for (int32 i = 0; i < N(16); ++i)
		{
			Emit(Ember, RandRing(5.f, 25.f, FMath::FRandRange(50.f, 110.f)), FVector(0.3f * S), FMath::FRandRange(0.7f, 1.3f), 40.f, 1.5f,
				FVector(FMath::FRandRange(-2.f, 2.f), FMath::FRandRange(-2.f, 2.f), 1.f), false, false, true);
		}
		Flash(FLinearColor(1.f, 0.45f, 0.1f), 1600.f * P, 100.f);
		break;
	}
	default:
		break;
	}

	// Kritik: her coinde ekstra altin kivilcim patlamasi
	if (Power > 1.2f)
	{
		const int32 Crit = AddGroup(CubeMesh, FLinearColor(1.f, 0.6f, 0.15f), true, 16.f);
		for (int32 i = 0; i < 26; ++i)
		{
			Emit(Crit, RandRing(80.f, 200.f, FMath::FRandRange(100.f, 220.f)), FVector(0.8f, 0.14f, 0.14f) * S, FMath::FRandRange(0.4f, 0.7f),
				-650.f, 0.8f, FVector(0.f, 0.f, 1.f), false, true);
		}
		if (LightIntensity < 1500.f)
		{
			Flash(FLinearColor(1.f, 0.6f, 0.2f), 1800.f, 110.f);
		}
	}

	if (Particles.Num() == 0)
	{
		SetLifeSpan(0.1f);
	}
}

void AFortuneLandFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TotalAge += DeltaSeconds;

	bool bAnyAlive = false;
	for (FParticle& P : Particles)
	{
		if (P.Age >= P.Life) continue;
		P.Age += DeltaSeconds;
		const float T = FMath::Clamp(P.Age / P.Life, 0.f, 1.f);

		P.Vel *= FMath::Max(0.f, 1.f - P.Drag * DeltaSeconds);
		P.Vel.Z += P.Gravity * DeltaSeconds;
		P.Pos += P.Vel * DeltaSeconds;
		P.Rot += P.RotVel * DeltaSeconds;

		// Boyut: ya once buyuyup sonra kuculur (toz/alev) ya da dogrudan kuculur
		float Scale = P.bGrowFirst ? FMath::Sin(T * PI) * 1.4f : (1.f - T * T);
		if (P.bFlicker)
		{
			Scale *= 0.7f + 0.3f * FMath::Abs(FMath::Sin((P.Age + P.Instance) * 40.f));
		}
		if (P.Age >= P.Life) Scale = 0.f;
		bAnyAlive |= (P.Age < P.Life);

		Groups[P.Group]->UpdateInstanceTransform(P.Instance, FTransform(P.Rot, P.Pos, P.Size * Scale), true, false, true);
	}
	for (UInstancedStaticMeshComponent* G : Groups)
	{
		if (G) G->MarkRenderStateDirty();
	}

	// Isik flasi hizla soner
	if (LightIntensity > 0.f)
	{
		LightAge += DeltaSeconds;
		const float L = FMath::Max(0.f, 1.f - LightAge / 0.35f);
		Light->SetIntensity(LightIntensity * L * L);
		if (L <= 0.f) LightIntensity = 0.f;
	}

	if ((!bAnyAlive && LightIntensity <= 0.f) || TotalAge > 4.f)
	{
		Destroy();
	}
}
