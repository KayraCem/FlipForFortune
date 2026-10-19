#include "FortuneCoinAura.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr float AuraCm = 0.01f;   // BasicShapes 100 cm -> 1 cm

	FTransform AuraBar(const FVector& Center, const FVector& Along, const FVector& Other, float Length, float Thick)
	{
		const FQuat Q = FRotationMatrix::MakeFromXY(Along, Other).ToQuat();
		return FTransform(Q, Center, FVector(Length, Thick, Thick) * AuraCm);
	}

	FTransform AuraBox(const FVector& Center, const FVector& X, const FVector& Y, const FVector& Size)
	{
		const FQuat Q = FRotationMatrix::MakeFromXY(X, Y).ToQuat();
		return FTransform(Q, Center, Size * AuraCm);
	}
}

UFortuneCoinAuraComponent::UFortuneCoinAuraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bTickInEditor = false;
}

void UFortuneCoinAuraComponent::OnUnregister()
{
	SetComponentTickEnabled(false);
	Super::OnUnregister();
}

void UFortuneCoinAuraComponent::OnVisibilityChanged()
{
	Super::OnVisibilityChanged();
	for (UInstancedStaticMeshComponent* G : Groups)
	{
		if (G) G->SetVisibility(IsVisible());
	}
	if (Light) Light->SetVisibility(IsVisible());
}

void UFortuneCoinAuraComponent::Clear()
{
	for (UInstancedStaticMeshComponent* G : Groups)
	{
		if (G) G->DestroyComponent();
	}
	Groups.Reset();
	if (Light)
	{
		Light->DestroyComponent();
		Light = nullptr;
	}
	for (TArray<FFlame>& F : Flames) F.Reset();
	SparkSeeds.Reset();
	SetComponentTickEnabled(false);
}

UInstancedStaticMeshComponent* UFortuneCoinAuraComponent::MakeISM(UStaticMesh* Mesh, const FLinearColor& InColor, bool bGlow, float Glow, bool bWorldSpace, bool bPlain)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Mesh) return nullptr;

	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(Owner);
	ISM->SetMobility(EComponentMobility::Movable);
	ISM->SetupAttachment(this);
	if (bWorldSpace)
	{
		// Ornekler dunya koordinatinda: coin ucarken alevler arkada iz birakir
		ISM->SetUsingAbsoluteLocation(true);
		ISM->SetUsingAbsoluteRotation(true);
		ISM->SetUsingAbsoluteScale(true);
	}
	ISM->SetStaticMesh(Mesh);
	ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ISM->SetCastShadow(false);
	ISM->SetVisibility(IsVisible());
	ISM->RegisterComponent();
	if (bWorldSpace)
	{
		ISM->SetWorldTransform(FTransform::Identity);
	}

	UMaterialInterface* Base = bGlow ? GlowMaterial.Get() : ((bPlain && PlainMaterial) ? PlainMaterial.Get() : MetalMaterial.Get());
	if (!Base) Base = MetalMaterial ? MetalMaterial.Get() : GlowMaterial.Get();
	if (Base)
	{
		if (UMaterialInstanceDynamic* MID = ISM->CreateDynamicMaterialInstance(0, Base))
		{
			MID->SetVectorParameterValue(TEXT("Color"), InColor);
			MID->SetScalarParameterValue(TEXT("Glow"), Glow);
		}
	}
	Groups.Add(ISM);
	return ISM;
}

void UFortuneCoinAuraComponent::SetAura(EFortuneCoinAura InAura, const FLinearColor& InColor, float InRadius, float InHalfThickness)
{
	Clear();
	Aura = InAura;
	Color = InColor;
	Radius = InRadius;
	Half = InHalfThickness;
	Time = 0.f;
	if (Aura == EFortuneCoinAura::None || !GetOwner()) return;

	SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	GlowMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_FX_Glow.M_FX_Glow"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	MetalMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_AuraMetal.M_AuraMetal"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!MetalMaterial)
	{
		MetalMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	PlainMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_AuraPlain.M_AuraPlain"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!PlainMaterial)
	{
		PlainMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	CylMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	auto AddLight = [this](const FLinearColor& C, float Intensity, float Range)
	{
		Light = NewObject<UPointLightComponent>(GetOwner());
		Light->SetupAttachment(this);
		Light->SetCastShadows(false);
		Light->SetLightColor(C);
		Light->SetAttenuationRadius(Range);
		Light->SetIntensity(Intensity);
		Light->SetVisibility(IsVisible());
		Light->RegisterComponent();
		LightBase = Intensity;
	};

	switch (Aura)
	{
	case EFortuneCoinAura::Fire:
	{
		// Alevler isik ekleyen (additive) materyalle: ust uste binince parlar, hic siyah gorunmez
		if (UMaterialInterface* Fire = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_FX_Fire.M_FX_Fire"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			GlowMaterial = Fire;
		}
		MakeISM(SphereMesh, Color, true, 2.2f, true);                                  // dis alev
		MakeISM(SphereMesh, FLinearColor(1.f, 0.75f, 0.3f), true, 3.f, true);         // sicak cekirdek
		MakeISM(SphereMesh, FLinearColor(1.f, 0.55f, 0.15f), true, 6.f, true);        // korlar
		const int32 Counts[3] = { 26, 16, 12 };
		for (int32 g = 0; g < 3; ++g)
		{
			Flames[g].SetNum(Counts[g]);
			for (FFlame& F : Flames[g])
			{
				RespawnFlame(F, g, true);
				Groups[g]->AddInstance(FTransform(FQuat::Identity, F.Pos, FVector::ZeroVector), false);
			}
		}
		AddLight(FLinearColor(1.f, 0.45f, 0.12f), 900.f, 45.f);
		break;
	}
	case EFortuneCoinAura::Chains:
		BuildChains();
		break;
	case EFortuneCoinAura::Halo:
	{
		UInstancedStaticMeshComponent* Ring = MakeISM(SphereMesh, Color, true, 7.f, false);
		UInstancedStaticMeshComponent* Motes = MakeISM(SphereMesh, FLinearColor(1.f, 0.97f, 0.85f), true, 10.f, false);
		for (int32 i = 0; i < 28; ++i) Ring->AddInstance(FTransform::Identity, false);
		for (int32 i = 0; i < 10; ++i)
		{
			Motes->AddInstance(FTransform::Identity, false);
			SparkSeeds.Add(FVector(FMath::FRand(), FMath::FRand(), FMath::FRandRange(0.6f, 1.4f)));
		}
		AddLight(Color, 350.f, 30.f);
		break;
	}
	case EFortuneCoinAura::Orbit:
	{
		UInstancedStaticMeshComponent* Planets = MakeISM(SphereMesh, Color, true, 14.f, false);
		UInstancedStaticMeshComponent* Stars = MakeISM(SphereMesh, FLinearColor(0.95f, 0.9f, 1.f), true, 18.f, false);
		for (int32 i = 0; i < 3 * 8; ++i) Planets->AddInstance(FTransform::Identity, false);
		for (int32 i = 0; i < 36; ++i)
		{
			Stars->AddInstance(FTransform::Identity, false);
			SparkSeeds.Add(FVector(FMath::FRand(), FMath::FRand(), FMath::FRandRange(0.6f, 1.5f)));
		}
		AddLight(Color, 700.f, 40.f);
		break;
	}
	case EFortuneCoinAura::Sparkle:
	{
		UInstancedStaticMeshComponent* Spark = MakeISM(CubeMesh, Color, true, 20.f, false);
		for (int32 i = 0; i < 12; ++i)
		{
			Spark->AddInstance(FTransform::Identity, false);
			Spark->AddInstance(FTransform::Identity, false);
			SparkSeeds.Add(FVector(FMath::FRand(), FMath::FRand(), FMath::FRandRange(0.7f, 1.6f)));
		}
		break;
	}
	case EFortuneCoinAura::Thorns:
		BuildThorns();
		break;
	case EFortuneCoinAura::Lock:
	{
		BuildChains();
		// Yazi yuzunun ortasinda buyuk asma kilit
		const float S = 2.1f;
		BuildPadlock(FVector(0.f, -0.72f * S - 0.1f, 0.f), FVector::ForwardVector, FVector::RightVector, FVector::UpVector, S);
		break;
	}
	case EFortuneCoinAura::Kelp:
	{
		MakeISM(CubeMesh, FLinearColor(0.06f, 0.32f, 0.12f), false, 0.f, false, true);   // yosun seritleri
		MakeISM(SphereMesh, Color, true, 4.f, false);                                  // kabarciklar
		for (int32 i = 0; i < 8 * 7; ++i) Groups[0]->AddInstance(FTransform::Identity, false);
		for (int32 i = 0; i < 12; ++i)
		{
			Groups[1]->AddInstance(FTransform::Identity, false);
			SparkSeeds.Add(FVector(FMath::FRand(), FMath::FRand(), FMath::FRandRange(0.6f, 1.4f)));
		}
		break;
	}
	case EFortuneCoinAura::Zone:
	{
		MakeISM(CubeMesh, Color, true, 7.f, false);     // donen kesik halkalar
		MakeISM(SphereMesh, Color, true, 9.f, false);   // yayilan dalga
		for (int32 i = 0; i < 16 + 24; ++i) Groups[0]->AddInstance(FTransform::Identity, false);
		for (int32 i = 0; i < 36; ++i) Groups[1]->AddInstance(FTransform::Identity, false);
		AddLight(Color, 300.f, 30.f);
		break;
	}
	case EFortuneCoinAura::Vault:
	{
		MakeISM(CylMesh ? CylMesh.Get() : SphereMesh.Get(), FLinearColor(0.75f, 0.76f, 0.8f), false, 0.f, false);   // gobek + kollar
		MakeISM(SphereMesh, FLinearColor(0.95f, 0.68f, 0.2f), false, 0.f, false);                                    // kol uclari (altin)
		for (int32 i = 0; i < 1 + 3; ++i) Groups[0]->AddInstance(FTransform::Identity, false);
		for (int32 i = 0; i < 3 + 1; ++i) Groups[1]->AddInstance(FTransform::Identity, false);
		break;
	}
	case EFortuneCoinAura::Tape:  BuildTape();  break;
	case EFortuneCoinAura::Bolts: BuildBolts(); break;
	case EFortuneCoinAura::Stem:  BuildStem();  break;
	case EFortuneCoinAura::Bones: BuildBones(); break;
	case EFortuneCoinAura::Wings:
	{
		MakeISM(CubeMesh, FLinearColor(0.95f, 0.94f, 0.9f), false, 0.f, false, true);   // tuyler
		MakeISM(CubeMesh, Color, true, 6.f, false);                                      // hale
		for (int32 i = 0; i < 2 * 7; ++i) Groups[0]->AddInstance(FTransform::Identity, false);
		for (int32 i = 0; i < 24; ++i) Groups[1]->AddInstance(FTransform::Identity, false);
		AddLight(Color, 250.f, 25.f);
		break;
	}
	case EFortuneCoinAura::ClockHands:
	{
		MakeISM(CubeMesh, FLinearColor(0.95f, 0.72f, 0.25f), false, 0.f, false);   // akrep + yelkovan (altin)
		MakeISM(CylMesh ? CylMesh.Get() : SphereMesh.Get(), FLinearColor(0.35f, 0.22f, 0.06f), false, 0.f, false);
		for (int32 i = 0; i < 2; ++i) Groups[0]->AddInstance(FTransform::Identity, false);
		Groups[1]->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, Half + 0.12f), FVector(0.45f, 0.45f, 0.2f) * AuraCm), false);
		break;
	}
	case EFortuneCoinAura::Compass:
	{
		MakeISM(CubeMesh, FLinearColor(0.85f, 0.08f, 0.06f), false, 0.f, false, true);   // kuzey (kirmizi)
		MakeISM(CubeMesh, FLinearColor(0.9f, 0.9f, 0.92f), false, 0.f, false);          // guney (gumus)
		MakeISM(CylMesh ? CylMesh.Get() : SphereMesh.Get(), FLinearColor(0.3f, 0.3f, 0.34f), false, 0.f, false);
		for (int32 g = 0; g < 2; ++g) for (int32 i = 0; i < 2; ++i) Groups[g]->AddInstance(FTransform::Identity, false);
		Groups[2]->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, Half + 0.15f), FVector(0.3f, 0.3f, 0.3f) * AuraCm), false);
		break;
	}
	default:
		break;
	}

	const bool bStatic = Aura == EFortuneCoinAura::Chains || Aura == EFortuneCoinAura::Thorns || Aura == EFortuneCoinAura::Lock
		|| Aura == EFortuneCoinAura::Tape || Aura == EFortuneCoinAura::Bolts || Aura == EFortuneCoinAura::Stem || Aura == EFortuneCoinAura::Bones;
	SetComponentTickEnabled(!bStatic);
	if (!bStatic)
	{
		TickLocal(0.f);
	}
}

// ---------------- Zincirler ----------------

void UFortuneCoinAuraComponent::AddLink(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& U, const FVector& V, float Len, float Wid, float Bar)
{
	// Dikdortgen halka: iki uzun + iki kisa cubuk
	const FVector Off = V * (Wid * 0.5f - Bar * 0.5f);
	ISM->AddInstance(AuraBar(Center + Off, U, V, Len, Bar), false);
	ISM->AddInstance(AuraBar(Center - Off, U, V, Len, Bar), false);
	const FVector OffU = U * (Len * 0.5f - Bar * 0.5f);
	ISM->AddInstance(AuraBar(Center + OffU, V, U, Wid, Bar), false);
	ISM->AddInstance(AuraBar(Center - OffU, V, U, Wid, Bar), false);
}

void UFortuneCoinAuraComponent::BuildChains()
{
	UInstancedStaticMeshComponent* Metal = MakeISM(CubeMesh, FLinearColor(0.16f, 0.17f, 0.2f), false, 0.f, false);   // koyu demir (parlayip beyaz gorunmesin)
	if (!Metal) return;

	const float Len = 0.95f;
	const float Wid = 0.48f;
	const float Bar = 0.11f;
	const float Step = 0.7f;
	const FVector Up = FVector::UpVector;

	for (int32 Face = 0; Face < 2; ++Face)
	{
		const float S = Face == 0 ? 1.f : -1.f;
		for (int32 d = 0; d < 2; ++d)
		{
			const FVector Dir = (d == 0 ? FVector(1.f, 1.f, 0.f) : FVector(1.f, -1.f, 0.f)).GetSafeNormal();
			const FVector Side = FVector::CrossProduct(Up, Dir);
			const float Lift = d == 0 ? 0.f : 0.14f;   // ikinci zincir ustten gecer
			const float Span = Radius + 0.25f;
			const int32 N = FMath::FloorToInt32(2.f * Span / Step) + 1;
			const float Start = -0.5f * Step * (N - 1);
			for (int32 i = 0; i < N; ++i)
			{
				const float T = Start + i * Step;
				if (i % 2 == 0)
				{
					const FVector C = Dir * T + Up * S * (Half + Bar * 0.5f + 0.03f + Lift);
					AddLink(Metal, C, Dir, Side, Len, Wid, Bar);
				}
				else
				{
					const FVector C = Dir * T + Up * S * (Half + Wid * 0.5f - 0.06f + Lift);
					AddLink(Metal, C, Dir, Up, Len, Wid, Bar);
				}
			}
		}
	}

	// Asma kilitler: biri yazi yuzunde, biri tura yuzunde
	const FVector D0 = FVector(1.f, 1.f, 0.f).GetSafeNormal();
	const FVector D1 = FVector(1.f, -1.f, 0.f).GetSafeNormal();
	BuildPadlock(D0 * (Radius * 0.52f), D0, FVector::CrossProduct(Up, D0), Up);
	BuildPadlock(D1 * (-Radius * 0.48f), D1, FVector::CrossProduct(-Up, D1), -Up);
}

void UFortuneCoinAuraComponent::BuildPadlock(const FVector& Center, const FVector& U, const FVector& V, const FVector& N, float Scale)
{
	const float S = Scale;
	UInstancedStaticMeshComponent* Metal = Groups.Num() > 0 ? Groups[0].Get() : nullptr;
	if (!Metal) return;
	if (Groups.Num() < 2)
	{
		MakeISM(CubeMesh, FLinearColor(0.95f, 0.66f, 0.18f), false, 0.f, false);   // altin govde
		MakeISM(CubeMesh, Color, true, 8.f, false);                                 // isikli anahtar deligi
	}
	UInstancedStaticMeshComponent* Gold = Groups[1];
	UInstancedStaticMeshComponent* Hole = Groups[2];

	// Kilit coinin yuzune yatik durur; govde zincirin yaninda, kanca zincire takili
	const FVector BodySize = FVector(1.05f, 0.9f, 0.34f) * S;   // U, V, N
	const float Base = Half + BodySize.Z * 0.5f + 0.25f;        // zincirin ustunde
	const FVector BodyC = Center + V * 0.72f * S + N * Base;
	Gold->AddInstance(AuraBox(BodyC, U, V, BodySize), false);
	// Govde kenar seridi
	Gold->AddInstance(AuraBox(BodyC - V * 0.42f * S, U, V, FVector(1.15f, 0.1f, 0.4f) * S), false);

	// U kanca: iki dik cubuk + ust cubuk (zincirin uzerinden gecer)
	const float Bar = 0.12f * S;
	const float Reach = 0.62f * S;
	for (float Sgn : { -1.f, 1.f })
	{
		const FVector C = BodyC - V * (BodySize.Y * 0.5f + Reach * 0.5f) + U * Sgn * 0.3f * S;
		Metal->AddInstance(AuraBar(C, V, U, Reach, Bar), false);
	}
	Metal->AddInstance(AuraBar(BodyC - V * (BodySize.Y * 0.5f + Reach - Bar * 0.5f), U, V, 0.72f * S, Bar), false);

	// Anahtar deligi (isikli)
	const FVector HoleC = BodyC + N * (BodySize.Z * 0.5f + 0.01f);
	Hole->AddInstance(AuraBox(HoleC + V * 0.06f * S, U, V, FVector(0.16f, 0.16f, 0.04f) * S), false);
	Hole->AddInstance(AuraBox(HoleC - V * 0.1f * S, U, V, FVector(0.07f, 0.26f, 0.04f) * S), false);
}

// ---------------- Alev ----------------

void UFortuneCoinAuraComponent::RespawnFlame(FFlame& F, int32 Group, bool bRandomAge)
{
	const FTransform& X = GetComponentTransform();
	const float S = X.GetScale3D().GetAbsMax();
	const float A = FMath::FRandRange(0.f, 2.f * PI);
	const FVector Radial(FMath::Cos(A), FMath::Sin(A), 0.f);
	const float R = Group == 2 ? FMath::FRandRange(0.3f, 1.f) * Radius : Radius * FMath::FRandRange(0.88f, 1.0f);
	const FVector Local = Radial * R + FVector(0.f, 0.f, FMath::FRandRange(-Half, Half));
	F.Pos = X.TransformPosition(Local);
	const FVector Out = X.TransformVectorNoScale(Radial);

	switch (Group)
	{
	case 0:
		F.Life = FMath::FRandRange(0.45f, 0.8f);
		F.Size = FMath::FRandRange(1.0f, 1.6f);
		F.Vel = (FVector::UpVector * FMath::FRandRange(4.f, 8.f) + Out * 1.2f) * S;
		break;
	case 1:
		F.Life = FMath::FRandRange(0.28f, 0.5f);
		F.Size = FMath::FRandRange(0.55f, 0.9f);
		F.Vel = (FVector::UpVector * FMath::FRandRange(3.f, 6.f) + Out * 0.6f) * S;
		break;
	default:
		F.Life = FMath::FRandRange(0.8f, 1.5f);
		F.Size = FMath::FRandRange(0.14f, 0.26f);
		F.Vel = (FVector::UpVector * FMath::FRandRange(8.f, 14.f) + Out * FMath::FRandRange(0.5f, 2.5f)) * S;
		break;
	}
	F.Phase = FMath::FRandRange(0.f, 2.f * PI);
	F.Age = bRandomAge ? FMath::FRandRange(0.f, F.Life) : 0.f;
}

void UFortuneCoinAuraComponent::TickFire(float Dt)
{
	const float S = GetComponentTransform().GetScale3D().GetAbsMax();
	TArray<FTransform> Xs;
	for (int32 g = 0; g < 3 && Groups.IsValidIndex(g); ++g)
	{
		Xs.Reset(Flames[g].Num());
		for (FFlame& F : Flames[g])
		{
			F.Age += Dt;
			if (F.Age >= F.Life) RespawnFlame(F, g, false);
			const float T = F.Age / F.Life;
			// Yukselirken hizlanir, hafif kivrilir
			F.Vel.Z += (g == 2 ? 4.f : 7.f) * S * Dt;
			const FVector Wiggle(FMath::Sin(Time * 9.f + F.Phase), FMath::Cos(Time * 7.f + F.Phase * 1.3f), 0.f);
			F.Pos += (F.Vel + Wiggle * (g == 2 ? 2.5f : 1.2f) * S) * Dt;

			float Profile = FMath::Sin(PI * FMath::Min(1.f, T * 1.15f));
			if (g == 2) Profile = (T < 0.15f) ? T / 0.15f : 1.f - (T - 0.15f) / 0.85f;
			Profile = FMath::Max(0.f, Profile);
			const float Sz = F.Size * Profile * S * AuraCm;
			const FVector Scale = g == 2 ? FVector(Sz) : FVector(Sz * 0.85f, Sz * 0.85f, Sz * 1.7f);
			Xs.Add(FTransform(FQuat::Identity, F.Pos, Scale));
		}
		Groups[g]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
	}

	if (Light)
	{
		const float Flick = 0.75f + 0.15f * FMath::Sin(Time * 23.f) + 0.1f * FMath::Sin(Time * 37.f + 1.3f);
		Light->SetWorldLocation(GetComponentLocation() + FVector(0.f, 0.f, 3.f * S));
		Light->SetIntensity(LightBase * Flick);
		Light->SetAttenuationRadius(45.f * S);
	}
}

// ---------------- Hale / yorunge / isilti ----------------

void UFortuneCoinAuraComponent::TickLocal(float Dt)
{
	if (Aura == EFortuneCoinAura::Fire)
	{
		TickFire(Dt);
		return;
	}
	if (Aura == EFortuneCoinAura::Kelp)
	{
		TickKelp();
		return;
	}
	if (Aura == EFortuneCoinAura::Zone)
	{
		TickZone();
		return;
	}
	if (Aura == EFortuneCoinAura::Wings)   { TickWings();   return; }
	if (Aura == EFortuneCoinAura::ClockHands) { TickClock(); return; }
	if (Aura == EFortuneCoinAura::Compass) { TickCompass(); return; }
	if (Aura == EFortuneCoinAura::Vault)   { TickVault();   return; }

	TArray<FTransform> Xs;
	const float S = GetComponentTransform().GetScale3D().GetAbsMax();

	if (Aura == EFortuneCoinAura::Halo && Groups.Num() >= 2)
	{
		const int32 N = Groups[0]->GetInstanceCount();
		const float Rr = Radius + 0.6f;
		for (int32 i = 0; i < N; ++i)
		{
			const float A = 2.f * PI * i / N + Time * 0.9f;
			const float Pulse = 0.75f + 0.25f * FMath::Sin(Time * 4.f + i * 0.9f);
			const FVector P(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, 0.f);
			Xs.Add(FTransform(FQuat::Identity, P, FVector(0.26f * Pulse * AuraCm)));
		}
		Groups[0]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);

		Xs.Reset();
		for (int32 i = 0; i < SparkSeeds.Num(); ++i)
		{
			FVector& Sd = SparkSeeds[i];
			const float Cycle = FMath::Fmod(Time * 0.5f * Sd.Z + Sd.X, 1.f);
			const float A = Sd.Y * 2.f * PI + Time * 0.4f;
			const float Rr2 = Radius * (0.9f + 0.4f * Cycle);
			const float Side = (i % 2 == 0) ? 1.f : -1.f;
			const FVector P(FMath::Cos(A) * Rr2, FMath::Sin(A) * Rr2, Side * (Half + 0.3f + 1.6f * Cycle));
			const float Sz = 0.16f * FMath::Sin(PI * Cycle);
			Xs.Add(FTransform(FQuat::Identity, P, FVector(Sz * AuraCm)));
		}
		Groups[1]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
		if (Light) Light->SetIntensity(LightBase * (0.85f + 0.15f * FMath::Sin(Time * 2.f)));
	}
	else if (Aura == EFortuneCoinAura::Orbit && Groups.Num() >= 2)
	{
		const FRotator Tilts[3] = { FRotator(25.f, 0.f, 10.f), FRotator(-35.f, 60.f, 0.f), FRotator(10.f, 120.f, -40.f) };
		const float Speeds[3] = { 1.6f, -1.1f, 0.8f };
		const float Sizes[3] = { 0.8f, 0.6f, 0.7f };
		for (int32 b = 0; b < 3; ++b)
		{
			const float Rr = Radius * (1.28f + 0.16f * b);
			const FQuat Q = Tilts[b].Quaternion();
			for (int32 k = 0; k < 8; ++k)
			{
				const float A = Time * Speeds[b] + b * 2.1f - k * 0.11f * FMath::Sign(Speeds[b]);
				const FVector P = Q.RotateVector(FVector(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, 0.f));
				const float Sz = (k == 0 ? Sizes[b] : Sizes[b] * 0.55f * (1.f - k / 8.f));
				Xs.Add(FTransform(FQuat::Identity, P, FVector(Sz * AuraCm)));
			}
		}
		Groups[0]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);

		Xs.Reset();
		for (int32 i = 0; i < SparkSeeds.Num(); ++i)
		{
			const FVector& Sd = SparkSeeds[i];
			const float A = Sd.X * 2.f * PI + Time * 0.25f * Sd.Z;
			// Coinin etrafinda donen yildiz tozu diski (sarmal)
			const float Rr = Radius * (1.05f + 0.9f * Sd.Y);
			const FVector P(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, (Sd.Y - 0.5f) * 1.2f + FMath::Sin(A * 2.f) * 0.3f);
			const float Tw = 0.45f + 0.55f * FMath::Max(0.f, FMath::Sin(Time * 3.f * Sd.Z + Sd.X * 20.f));
			Xs.Add(FTransform(FQuat::Identity, P, FVector(0.26f * Tw * AuraCm)));
		}
		Groups[1]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
	}
	else if (Aura == EFortuneCoinAura::Sparkle && Groups.Num() >= 1)
	{
		for (int32 i = 0; i < SparkSeeds.Num(); ++i)
		{
			FVector& Sd = SparkSeeds[i];
			const float Cyc = Time * 0.7f * Sd.Z + Sd.X * 5.f;
			const int32 Gen = FMath::FloorToInt32(Cyc);
			const float T = Cyc - Gen;
			// Her dongude yeni rastgele nokta (tohum + dongu sayisindan)
			const uint32 H = (uint32(i + 1) * 2654435761u) ^ (uint32(Gen) * 40503u + 0x9e3779b9u);
			FRandomStream Rs(int32(H & 0x7fffffff));
			const float A = Rs.FRandRange(0.f, 2.f * PI);
			const float Rr = Radius * FMath::Sqrt(Rs.FRand()) * 0.85f;
			const float Side = Rs.FRand() < 0.5f ? 1.f : -1.f;
			const FVector P(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, Side * (Half + 0.12f));
			const float Tw = FMath::Pow(FMath::Max(0.f, FMath::Sin(PI * T)), 3.f);
			const float L = 1.5f * Tw;
			const FQuat Spin(FVector::UpVector, Time * 1.5f + i);
			const FVector X = Spin.RotateVector(FVector::ForwardVector);
			const FVector Y = Spin.RotateVector(FVector::RightVector);
			Xs.Add(AuraBox(P, X, Y, FVector(L, 0.12f * Tw + 0.001f, 0.06f)));
			Xs.Add(AuraBox(P, Y, X, FVector(L, 0.12f * Tw + 0.001f, 0.06f)));
		}
		Groups[0]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
	}
	(void)S;
}

void UFortuneCoinAuraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Time += DeltaTime;
	TickLocal(FMath::Min(DeltaTime, 0.05f));
}

// ---------------- Dikenli sarmasik + low-poly siyah gul ----------------

void UFortuneCoinAuraComponent::BuildThorns()
{
	UInstancedStaticMeshComponent* Vine = MakeISM(CubeMesh, FLinearColor(0.035f, 0.2f, 0.05f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Thorn = MakeISM(ConeMesh ? ConeMesh.Get() : CubeMesh.Get(), FLinearColor(0.06f, 0.32f, 0.08f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Leaf = MakeISM(CubeMesh, FLinearColor(0.03f, 0.26f, 0.06f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* PetalA = MakeISM(CubeMesh, FLinearColor(0.025f, 0.025f, 0.03f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* PetalB = MakeISM(CubeMesh, FLinearColor(0.06f, 0.06f, 0.07f), false, 0.f, false, true);
	if (!Vine || !Thorn || !Leaf || !PetalA || !PetalB) return;

	const FVector Up = FVector::UpVector;
	FRandomStream Rs(1337);

	// Iki sarmasik coinin kenarini dolanir, birbirinin ustunden alttan gecer
	for (int32 v = 0; v < 2; ++v)
	{
		const int32 N = 44;
		const float Ph = v * 1.7f;
		auto Pt = [&](int32 i)
		{
			const float A = 2.f * PI * i / N;
			const float R = Radius + 0.45f + 0.18f * FMath::Sin(5.f * A + Ph);
			const float Z = (v == 0 ? 1.f : -1.f) * 0.38f * FMath::Sin(3.f * A + Ph);
			return FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, Z);
		};
		for (int32 i = 0; i < N; ++i)
		{
			const FVector P0 = Pt(i), P1 = Pt(i + 1);
			const FVector Dir = (P1 - P0).GetSafeNormal();
			const FVector Radial = FVector(P0.X, P0.Y, 0.f).GetSafeNormal();
			const float Len = (P1 - P0).Size() * 1.15f;
			Vine->AddInstance(AuraBox((P0 + P1) * 0.5f, Dir, Radial, FVector(Len, 0.22f, 0.22f)), false);

			if (i % 3 == v)
			{
				// Diken: disa ve yukari/asagi bakan koni
				const FVector TD = (Radial * 0.8f + Up * Rs.FRandRange(-0.7f, 0.7f) + Dir * 0.3f).GetSafeNormal();
				const FQuat Q = FRotationMatrix::MakeFromZ(TD).ToQuat();
				Thorn->AddInstance(FTransform(Q, P0 + TD * 0.28f, FVector(0.17f, 0.17f, 0.6f) * AuraCm), false);
			}
			if (i % 7 == 3 + v)
			{
				// Yaprak: yassi, disa egik kutu
				const FVector LD = (Radial + Up * (v == 0 ? 0.5f : -0.5f)).GetSafeNormal();
				Leaf->AddInstance(AuraBox(P0 + LD * 0.45f, LD, Dir, FVector(0.95f, 0.5f, 0.06f)), false);
				Leaf->AddInstance(AuraBox(P0 + LD * 0.45f + Dir * 0.15f, LD, Dir, FVector(0.6f, 0.3f, 0.09f)), false);
			}
		}
	}

	// Yazi yuzunde low-poly siyah gul: ice dogru dikleşen yaprak halkalari
	struct FRing { int32 Count; float R; float Len; float Wid; float Tilt; };
	const FRing Rings[] = { {5, 0.95f, 1.15f, 0.8f, 18.f}, {6, 0.7f, 0.95f, 0.7f, 35.f}, {5, 0.45f, 0.78f, 0.58f, 52.f}, {4, 0.22f, 0.58f, 0.46f, 68.f} };
	for (int32 k = 0; k < 4; ++k)
	{
		const FRing& R = Rings[k];
		for (int32 j = 0; j < R.Count; ++j)
		{
			const float A = 2.f * PI * j / R.Count + k * 0.55f;
			const FVector Rd(FMath::Cos(A), FMath::Sin(A), 0.f);
			const FVector Td(-Rd.Y, Rd.X, 0.f);
			const float T = FMath::DegreesToRadians(R.Tilt + Rs.FRandRange(-6.f, 6.f));
			const FVector X = (Rd * FMath::Cos(T) + Up * FMath::Sin(T)).GetSafeNormal();
			const FVector Base = Rd * R.R + Up * (Half + 0.12f + k * 0.16f);
			UInstancedStaticMeshComponent* P = ((j + k) % 2 == 0) ? PetalA : PetalB;
			P->AddInstance(AuraBox(Base + X * R.Len * 0.4f, X, Td, FVector(R.Len, R.Wid, 0.09f)), false);
		}
	}
	// Ortadaki tomurcuk
	PetalA->AddInstance(FTransform(FRotator(35.f, 45.f, 20.f).Quaternion(), Up * (Half + 0.85f), FVector(0.42f) * AuraCm), false);
	PetalB->AddInstance(FTransform(FRotator(-20.f, 10.f, 40.f).Quaternion(), Up * (Half + 0.7f), FVector(0.55f, 0.55f, 0.3f) * AuraCm), false);
	// Gulun altindaki iki yaprak
	Leaf->AddInstance(AuraBox(FVector(1.2f, 0.6f, Half + 0.1f), FVector(0.9f, 0.45f, 0.f).GetSafeNormal(), FVector(-0.45f, 0.9f, 0.f).GetSafeNormal(), FVector(1.3f, 0.65f, 0.07f)), false);
	Leaf->AddInstance(AuraBox(FVector(-1.1f, -0.75f, Half + 0.1f), FVector(-0.85f, -0.55f, 0.f).GetSafeNormal(), FVector(0.55f, -0.85f, 0.f).GetSafeNormal(), FVector(1.2f, 0.6f, 0.07f)), false);
}

// ---------------- Yosun ----------------

void UFortuneCoinAuraComponent::TickKelp()
{
	if (Groups.Num() < 2) return;
	TArray<FTransform> Xs;
	const int32 Strands = 8, Segs = 7;
	const FVector Up = FVector::UpVector;
	for (int32 s = 0; s < Strands; ++s)
	{
		const float A = 2.f * PI * s / Strands + 0.3f;
		const FVector Rd(FMath::Cos(A), FMath::Sin(A), 0.f);
		const FVector Td(-Rd.Y, Rd.X, 0.f);
		const float Side = (s % 2 == 0) ? 1.f : -1.f;   // yarisi yazi, yarisi tura tarafina
		FVector P = Rd * Radius * 0.96f;
		float Bend = FMath::DegreesToRadians(25.f);
		for (int32 k = 0; k < Segs; ++k)
		{
			const float Sway = FMath::Sin(Time * 1.7f + s * 1.3f + k * 0.55f) * FMath::DegreesToRadians(9.f + 3.f * k);
			Bend += FMath::DegreesToRadians(9.f) + Sway * 0.35f;
			const FVector Dir = (Rd * FMath::Cos(Bend) + Up * Side * FMath::Sin(Bend) + Td * FMath::Sin(Sway) * 0.5f).GetSafeNormal();
			const float Len = 0.55f;
			const float W = 0.34f * (1.f - 0.55f * k / Segs);
			Xs.Add(AuraBox(P + Dir * Len * 0.5f, Dir, Td, FVector(Len * 1.1f, W, 0.05f)));
			P += Dir * Len;
		}
	}
	Groups[0]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);

	Xs.Reset();
	for (int32 i = 0; i < SparkSeeds.Num(); ++i)
	{
		const FVector& Sd = SparkSeeds[i];
		const float T = FMath::Fmod(Time * 0.45f * Sd.Z + Sd.X, 1.f);
		const float A = Sd.Y * 2.f * PI;
		const float Side = (i % 2 == 0) ? 1.f : -1.f;
		const FVector P(FMath::Cos(A) * Radius * (0.7f + 0.3f * Sd.X) + FMath::Sin(Time * 3.f + i) * 0.15f,
			FMath::Sin(A) * Radius * (0.7f + 0.3f * Sd.X), Side * (Half + 0.2f + T * 4.f));
		const float Sz = (0.12f + 0.12f * Sd.Z) * FMath::Sin(PI * T);
		Xs.Add(FTransform(FQuat::Identity, P, FVector(Sz * AuraCm)));
	}
	Groups[1]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
}

// ---------------- Tehlike bolgesi ----------------

void UFortuneCoinAuraComponent::TickZone()
{
	if (Groups.Num() < 2) return;
	TArray<FTransform> Xs;
	auto Ring = [&](int32 N, float R, float Speed, float Thick, float Z)
	{
		for (int32 i = 0; i < N; ++i)
		{
			const float A = 2.f * PI * i / N + Time * Speed;
			const FVector Rd(FMath::Cos(A), FMath::Sin(A), 0.f);
			const FVector Td(-Rd.Y, Rd.X, 0.f);
			const float Arc = 2.f * PI * R / N * 0.55f;
			Xs.Add(AuraBox(Rd * R + FVector(0.f, 0.f, Z), Td, Rd, FVector(Arc, Thick, Thick)));
		}
	};
	Ring(16, Radius + 0.7f, 0.9f, 0.14f, 0.f);
	Ring(24, Radius + 1.25f, -0.6f, 0.09f, 0.f);
	Groups[0]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);

	// Kenardan disa yayilan dalga (1.6 sn'de bir)
	Xs.Reset();
	const float T = FMath::Fmod(Time / 1.6f, 1.f);
	const float R = Radius + 0.3f + T * 3.f;
	const float Sz = 0.16f * (1.f - T);
	const int32 N = Groups[1]->GetInstanceCount();
	for (int32 i = 0; i < N; ++i)
	{
		const float A = 2.f * PI * i / N;
		Xs.Add(FTransform(FQuat::Identity, FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f), FVector(Sz * AuraCm)));
	}
	Groups[1]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
	if (Light) Light->SetIntensity(LightBase * (0.7f + 0.3f * (1.f - T)));
}

// ---------------- Bant + etiketler (kenara sarili) ----------------

void UFortuneCoinAuraComponent::BuildTape()
{
	UInstancedStaticMeshComponent* Tape = MakeISM(CubeMesh, FLinearColor(0.86f, 0.8f, 0.58f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Label = MakeISM(CubeMesh, FLinearColor(0.98f, 0.56f, 0.08f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Sticker = MakeISM(CubeMesh, FLinearColor(0.95f, 0.9f, 0.3f), false, 0.f, false, true);
	if (!Tape || !Label || !Sticker) return;
	const FVector Up = FVector::UpVector;

	// Kenarin uzerinden iki yuze katlanan bant: ust parca, kenar parcasi, alt parca
	auto Wrap = [&](UInstancedStaticMeshComponent* ISM, float AngDeg, float Width, float Reach, float Th)
	{
		const float A = FMath::DegreesToRadians(AngDeg);
		const FVector Rd(FMath::Cos(A), FMath::Sin(A), 0.f);
		const FVector Td(-Rd.Y, Rd.X, 0.f);
		const float Rr = Radius * 0.98f;
		ISM->AddInstance(AuraBox(Rd * (Rr - Reach * 0.5f) + Up * (Half + Th * 0.5f), Rd, Td, FVector(Reach, Width, Th)), false);
		ISM->AddInstance(AuraBox(Rd * (Rr - Reach * 0.5f) - Up * (Half + Th * 0.5f), Rd, Td, FVector(Reach, Width, Th)), false);
		ISM->AddInstance(AuraBox(Rd * (Rr + Th * 0.5f), Up, Td, FVector(Half * 2.f + Th * 2.f, Width, Th)), false);
	};
	Wrap(Tape, 95.f, 0.9f, 1.0f, 0.05f);     // ust
	Wrap(Tape, 205.f, 0.75f, 0.8f, 0.05f);   // sol alt
	Wrap(Label, 160.f, 1.1f, 1.4f, 0.07f);   // turuncu etiket
	Wrap(Sticker, 345.f, 0.55f, 0.6f, 0.08f); // kucuk sari etiket
}

// ---------------- Civata basliklari ----------------

void UFortuneCoinAuraComponent::BuildBolts()
{
	UInstancedStaticMeshComponent* Bolt = MakeISM(CylMesh ? CylMesh.Get() : SphereMesh.Get(), FLinearColor(0.45f, 0.25f, 0.12f), false, 0.f, false);
	UInstancedStaticMeshComponent* Slot = MakeISM(CubeMesh, FLinearColor(0.08f, 0.04f, 0.02f), false, 0.f, false, true);
	if (!Bolt || !Slot) return;
	for (int32 Face = 0; Face < 2; ++Face)
	{
		const float S = Face == 0 ? 1.f : -1.f;
		for (int32 i = 0; i < 6; ++i)
		{
			const float A = PI / 3.f * i;
			const FVector P(FMath::Cos(A) * Radius * 0.74f, FMath::Sin(A) * Radius * 0.74f, S * (Half + 0.08f));
			Bolt->AddInstance(FTransform(FRotator(0.f, 30.f, 0.f), P, FVector(0.5f, 0.5f, 0.16f) * AuraCm), false);   // 6 kenarli gibi
			Slot->AddInstance(AuraBox(P + FVector(0.f, 0.f, S * 0.09f), FVector(FMath::Cos(A + 0.6f), FMath::Sin(A + 0.6f), 0.f), FVector(-FMath::Sin(A + 0.6f), FMath::Cos(A + 0.6f), 0.f), FVector(0.4f, 0.07f, 0.03f)), false);
		}
	}
}

// ---------------- Balkabagi sapi ----------------

void UFortuneCoinAuraComponent::BuildStem()
{
	UInstancedStaticMeshComponent* Stem = MakeISM(CubeMesh, FLinearColor(0.22f, 0.32f, 0.08f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Leaf = MakeISM(CubeMesh, FLinearColor(0.12f, 0.45f, 0.1f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Curl = MakeISM(SphereMesh, FLinearColor(0.18f, 0.4f, 0.08f), false, 0.f, false, true);
	if (!Stem || !Leaf || !Curl) return;
	const FVector X = FVector::ForwardVector;   // coinin ustu (+X)
	// Kalin, hafif egik sap: 3 parca
	FVector P = X * (Radius * 0.9f);
	FVector D = X;
	for (int32 k = 0; k < 3; ++k)
	{
		D = (D + FVector(0.f, 0.18f, 0.05f)).GetSafeNormal();
		const float L = 0.55f;
		Stem->AddInstance(AuraBox(P + D * L * 0.5f, D, FVector::RightVector, FVector(L * 1.1f, 0.5f - k * 0.08f, 0.5f - k * 0.08f)), false);
		P += D * L;
	}
	// Yaprak
	Leaf->AddInstance(AuraBox(X * (Radius + 0.3f) + FVector(0.f, -0.9f, 0.15f), FVector(0.3f, -1.f, 0.f).GetSafeNormal(), FVector(1.f, 0.3f, 0.f).GetSafeNormal(), FVector(1.4f, 0.8f, 0.07f)), false);
	Leaf->AddInstance(AuraBox(X * (Radius + 0.3f) + FVector(0.f, -0.9f, 0.18f), FVector(0.3f, -1.f, 0.f).GetSafeNormal(), FVector(1.f, 0.3f, 0.f).GetSafeNormal(), FVector(1.0f, 0.12f, 0.1f)), false);
	// Kivrik dal (spiral)
	for (int32 i = 0; i < 14; ++i)
	{
		const float T = i / 13.f;
		const float A = T * 3.5f * PI;
		const float Rr = 0.55f * (1.f - T * 0.6f);
		const FVector C = X * (Radius + 0.4f) + FVector(FMath::Sin(A) * Rr, 0.7f + FMath::Cos(A) * Rr, 0.f);
		Curl->AddInstance(FTransform(FQuat::Identity, C, FVector(0.14f) * AuraCm), false);
	}
}

// ---------------- Capraz kemikler ----------------

void UFortuneCoinAuraComponent::BuildBones()
{
	UInstancedStaticMeshComponent* Shaft = MakeISM(CylMesh ? CylMesh.Get() : CubeMesh.Get(), FLinearColor(0.88f, 0.84f, 0.74f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Knob = MakeISM(SphereMesh, FLinearColor(0.9f, 0.86f, 0.76f), false, 0.f, false, true);
	if (!Shaft || !Knob) return;
	for (float Deg : { 45.f, -45.f })
	{
		const float A = FMath::DegreesToRadians(Deg);
		const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
		const FVector Side(-Dir.Y, Dir.X, 0.f);
		const float Len = Radius * 2.f + 2.2f;
		const float Z = Deg > 0.f ? -0.05f : 0.05f;
		const FQuat Q = FRotationMatrix::MakeFromZ(Dir).ToQuat();   // silindir ekseni Z
		Shaft->AddInstance(FTransform(Q, FVector(0.f, 0.f, Z), FVector(0.42f, 0.42f, Len) * AuraCm), false);
		for (float E : { -1.f, 1.f })
		{
			const FVector End = Dir * (Len * 0.5f) * E + FVector(0.f, 0.f, Z);
			Knob->AddInstance(FTransform(FQuat::Identity, End + Side * 0.3f, FVector(0.62f) * AuraCm), false);
			Knob->AddInstance(FTransform(FQuat::Identity, End - Side * 0.3f, FVector(0.62f) * AuraCm), false);
		}
	}
}

// ---------------- Kanatlar + hale ----------------

void UFortuneCoinAuraComponent::TickWings()
{
	if (Groups.Num() < 2) return;
	TArray<FTransform> Xs;
	const float Flap = FMath::Sin(Time * 2.2f) * 12.f;
	for (int32 W = 0; W < 2; ++W)
	{
		const float S = W == 0 ? 1.f : -1.f;   // +Y / -Y yan
		const FVector Root(0.4f, S * (Radius * 0.85f), -0.15f);
		for (int32 f = 0; f < 7; ++f)
		{
			// tuyler yelpaze gibi yukari-disa acilir, en usttekiler en uzun
			const float Ang = FMath::DegreesToRadians(-35.f + f * 17.f);
			const float Back = FMath::DegreesToRadians(15.f + Flap);
			const FVector Dir = FVector(FMath::Sin(Ang), S * FMath::Cos(Ang) * FMath::Cos(Back), -FMath::Sin(Back) * 0.6f).GetSafeNormal();
			const float L = 1.4f + 0.35f * f;
			const FVector Sd = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
			Xs.Add(AuraBox(Root + Dir * L * 0.5f, Dir, Sd, FVector(L, 0.42f, 0.09f)));
		}
	}
	Groups[0]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);

	// Hale: coinin tepesinde (+X) yatay donen isik halkasi
	Xs.Reset();
	const FVector C(Radius + 1.0f + FMath::Sin(Time * 2.f) * 0.12f, 0.f, 0.f);
	const int32 N = Groups[1]->GetInstanceCount();
	for (int32 i = 0; i < N; ++i)
	{
		const float A = 2.f * PI * i / N + Time * 0.8f;
		const FVector P = C + FVector(0.f, FMath::Cos(A) * 1.3f, FMath::Sin(A) * 0.45f);
		const FVector T(0.f, -FMath::Sin(A), FMath::Cos(A) * 0.35f);
		Xs.Add(AuraBox(P, T.GetSafeNormal(), FVector::ForwardVector, FVector(0.4f, 0.14f, 0.14f)));
	}
	Groups[1]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
}

// ---------------- Saat akrep/yelkovan ----------------

void UFortuneCoinAuraComponent::TickClock()
{
	if (Groups.Num() < 1) return;
	TArray<FTransform> Xs;
	auto Hand = [&](float AngRad, float Len, float W, float Z)
	{
		const FVector D(FMath::Cos(AngRad), FMath::Sin(AngRad), 0.f);
		const FVector S(-D.Y, D.X, 0.f);
		Xs.Add(AuraBox(D * Len * 0.4f + FVector(0.f, 0.f, Half + Z), D, S, FVector(Len, W, 0.07f)));
	};
	Hand(-Time * 0.25f, Radius * 0.5f, 0.28f, 0.06f);    // akrep
	Hand(-Time * 1.6f, Radius * 0.72f, 0.16f, 0.14f);    // yelkovan
	Groups[0]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
}

// ---------------- Pusula ignesi ----------------

void UFortuneCoinAuraComponent::TickCompass()
{
	if (Groups.Num() < 2) return;
	// Kuzeye (+X) bakar, hafifce titrer
	const float A = FMath::DegreesToRadians(8.f * FMath::Sin(Time * 2.3f) + 3.f * FMath::Sin(Time * 7.1f));
	const FVector D(FMath::Cos(A), FMath::Sin(A), 0.f);
	const FVector S(-D.Y, D.X, 0.f);
	const float L = Radius * 0.62f;
	const float Z = Half + 0.12f;
	for (int32 g = 0; g < 2; ++g)
	{
		const float Sg = g == 0 ? 1.f : -1.f;
		TArray<FTransform> Xs;
		// Elmas uc: iki ince kutu caprazlanir
		for (int32 k = 0; k < 2; ++k)
		{
			const float Tilt = (k == 0 ? 1.f : -1.f) * 0.16f;
			const FVector Dk = (D * Sg + S * Tilt).GetSafeNormal();
			Xs.Add(AuraBox(Dk * L * 0.5f + FVector(0.f, 0.f, Z), Dk, FVector(-Dk.Y, Dk.X, 0.f), FVector(L, 0.16f, 0.06f)));
		}
		Groups[g]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
	}
}

// ---------------- Kasa kapisi tekerlegi ----------------

void UFortuneCoinAuraComponent::TickVault()
{
	if (Groups.Num() < 2) return;
	TArray<FTransform> Xs;
	const float Z = Half + 0.35f;
	// Yavas don, arada bir tik tik kilit acar gibi geri sek
	const float Spin = Time * 0.6f + 0.15f * FMath::Sin(Time * 5.f) * (FMath::Sin(Time * 0.7f) > 0.6f ? 1.f : 0.f);
	Xs.Add(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, Half + 0.2f), FVector(0.95f, 0.95f, 0.45f) * AuraCm));   // gobek
	const float Arm = Radius * 0.62f;
	for (int32 k = 0; k < 3; ++k)
	{
		const float A = Spin + k * 2.f * PI / 3.f;
		const FVector D(FMath::Cos(A), FMath::Sin(A), 0.f);
		const FQuat Q = FRotationMatrix::MakeFromZ(D).ToQuat();
		Xs.Add(FTransform(Q, D * Arm * 0.5f + FVector(0.f, 0.f, Z), FVector(0.26f, 0.26f, Arm) * AuraCm));
	}
	Groups[0]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);

	Xs.Reset();
	for (int32 k = 0; k < 3; ++k)
	{
		const float A = Spin + k * 2.f * PI / 3.f;
		const FVector D(FMath::Cos(A), FMath::Sin(A), 0.f);
		Xs.Add(FTransform(FQuat::Identity, D * Arm + FVector(0.f, 0.f, Z), FVector(0.55f) * AuraCm));
	}
	Xs.Add(FTransform(FQuat::Identity, FVector(0.f, 0.f, Half + 0.5f), FVector(0.5f, 0.5f, 0.3f) * AuraCm));
	Groups[1]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
}
