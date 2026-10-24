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
		const int32 Counts[3] = { 38, 24, 16 };
		for (int32 g = 0; g < 3; ++g)
		{
			Flames[g].SetNum(Counts[g]);
			for (FFlame& F : Flames[g])
			{
				RespawnFlame(F, g, true);
				Groups[g]->AddInstance(FTransform(FQuat::Identity, F.Pos, FVector::ZeroVector), false);
			}
		}
		AddLight(FLinearColor(1.f, 0.45f, 0.12f), 1400.f, 55.f);
		break;
	}
	case EFortuneCoinAura::EyeFlames:
	{
		// Kafatasinin goz cukurlarindan mor alev: dis alev, sicak cekirdek, kivilcim (dunya uzayi, ucarken iz birakir)
		if (UMaterialInterface* Fire = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_FX_Fire.M_FX_Fire"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			GlowMaterial = Fire;
		}
		MakeISM(SphereMesh, Color, true, 3.f, true);
		MakeISM(SphereMesh, FLinearColor(1.f, 0.7f, 1.f), true, 5.f, true);
		MakeISM(SphereMesh, FLinearColor(0.85f, 0.4f, 1.f), true, 8.f, true);
		const int32 Counts[3] = { 22, 14, 10 };
		for (int32 g = 0; g < 3; ++g)
		{
			Flames[g].SetNum(Counts[g]);
			for (FFlame& F : Flames[g])
			{
				RespawnFlame(F, g, true);
				Groups[g]->AddInstance(FTransform(FQuat::Identity, F.Pos, FVector::ZeroVector), false);
			}
		}
		GlowMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_FX_Glow.M_FX_Glow"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		// Kafatasinin arkasindan capraz cikan iki korsan kilici (pala)
		{
			UInstancedStaticMeshComponent* Blade = MakeISM(CubeMesh, FLinearColor(0.78f, 0.8f, 0.84f), false, 0.f, false);
			UInstancedStaticMeshComponent* Guard = MakeISM(CubeMesh, FLinearColor(0.95f, 0.68f, 0.2f), false, 0.f, false);
			UInstancedStaticMeshComponent* Grip = MakeISM(CylMesh ? CylMesh.Get() : CubeMesh.Get(), FLinearColor(0.18f, 0.08f, 0.03f), false, 0.f, false, true);
			for (float Deg : { 40.f, -40.f })
			{
				const float Sg = Deg > 0.f ? 1.f : -1.f;
				const float A = FMath::DegreesToRadians(180.f + Deg);   // kabza asagida, uc yukarida
				const FVector Dir(-FMath::Cos(A), -FMath::Sin(A), 0.f);  // kabzadan uca
				const FVector Side(-Dir.Y, Dir.X, 0.f);
				const float Z = -Half - 0.25f - (Sg > 0.f ? 0.f : 0.12f);
				const FVector Hilt = -Dir * (Radius * 1.05f) + FVector(0.f, 0.f, Z);
				// Kavisli bicak: 6 parca, uca dogru incelir ve kivrilir
				FVector P = Hilt + Dir * 0.35f;
				FVector D = Dir;
				for (int32 k = 0; k < 6; ++k)
				{
					const float L = Radius * 0.42f;
					const float W = 0.55f - k * 0.06f;
					Blade->AddInstance(AuraBox(P + D * L * 0.5f, D, Side, FVector(L * 1.05f, W, 0.08f)), false);
					P += D * L;
					D = (D + Side * Sg * 0.07f).GetSafeNormal();
				}
				Guard->AddInstance(AuraBox(Hilt + Dir * 0.3f, Side, Dir, FVector(1.5f, 0.26f, 0.26f)), false);
				Guard->AddInstance(AuraBox(Hilt + Dir * 0.3f + Side * 0.7f - Dir * 0.35f, Dir, Side, FVector(0.8f, 0.18f, 0.18f)), false);
				Grip->AddInstance(FTransform(FRotationMatrix::MakeFromZ(-Dir).ToQuat(), Hilt - Dir * 0.55f, FVector(0.3f, 0.3f, 1.1f) * AuraCm), false);
				Guard->AddInstance(AuraBox(Hilt - Dir * 1.15f, Dir, Side, FVector(0.4f, 0.4f, 0.4f)), false);
			}
		}
		AddLight(Color, 900.f, 40.f);
		break;
	}
	case EFortuneCoinAura::Pages:
	{
		// Ustte spiral cilt halkalari (altin) + kopup ucan takvim yapraklari
		UInstancedStaticMeshComponent* Rings = MakeISM(SphereMesh, FLinearColor(0.95f, 0.7f, 0.25f), false, 0.f, false);
		MakeISM(CubeMesh, FLinearColor(0.93f, 0.89f, 0.76f), false, 0.f, false, true);   // sayfalar
		MakeISM(CubeMesh, FLinearColor(0.7f, 0.08f, 0.1f), false, 0.f, false, true);     // sayfa ust seridi (kirmizi)
		if (Rings)
		{
			for (int32 r = 0; r < 6; ++r)
			{
				const float Y = (-0.5f + r / 5.f) * Radius * 0.95f;
				for (int32 k = 0; k < 9; ++k)
				{
					// Kenarin uzerinden iki yuze sarilan kucuk halka
					const float A = PI * (k / 8.f) - PI * 0.5f;
					const FVector P(Radius * 0.86f + FMath::Cos(A) * 0.42f, Y, FMath::Sin(A) * (Half + 0.25f));
					Rings->AddInstance(FTransform(FQuat::Identity, P, FVector(0.13f) * AuraCm), false);
				}
			}
		}
		for (int32 i = 0; i < 3; ++i)
		{
			Groups[1]->AddInstance(FTransform::Identity, false);
			Groups[2]->AddInstance(FTransform::Identity, false);
		}
		break;
	}
	case EFortuneCoinAura::RoseBloom:
		BuildRose();
		break;
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
		// Parilti yildizi: iki uzun sivri isin (+), iki kisa capraz isin (x), parlak cekirdek
		MakeISM(SphereMesh, Color, true, 16.f, false);
		MakeISM(SphereMesh, FMath::Lerp(Color, FLinearColor::White, 0.6f), true, 22.f, false);
		MakeISM(SphereMesh, FLinearColor::White, true, 30.f, false);
		for (int32 i = 0; i < 14; ++i)
		{
			for (int32 k = 0; k < 2; ++k)
			{
				Groups[0]->AddInstance(FTransform::Identity, false);
				Groups[1]->AddInstance(FTransform::Identity, false);
			}
			Groups[2]->AddInstance(FTransform::Identity, false);
			SparkSeeds.Add(FVector(FMath::FRand(), FMath::FRand(), FMath::FRandRange(0.7f, 1.5f)));
		}
		AddLight(Color, 200.f, 25.f);
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
		MakeISM(SphereMesh, FLinearColor(0.96f, 0.95f, 0.92f), false, 0.f, false, true);  // tuyler (yumusak elips)
		MakeISM(CubeMesh, Color, true, 6.f, false);                                       // hale
		MakeISM(SphereMesh, FLinearColor(0.98f, 0.82f, 0.4f), false, 0.f, false, true);   // altin tuy uclari
		for (int32 i = 0; i < 2 * 21; ++i) Groups[0]->AddInstance(FTransform::Identity, false);
		for (int32 i = 0; i < 2 * 8; ++i) Groups[2]->AddInstance(FTransform::Identity, false);
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
	if (Aura == EFortuneCoinAura::EyeFlames)
	{
		// Yazi yuzundeki iki goz cukuru (doku: merkezden +/-0.31R yanda, hafif yukarida)
		const float E = FMath::RandBool() ? 1.f : -1.f;
		const FVector Eye(0.05f * Radius + FMath::FRandRange(-0.18f, 0.18f), E * 0.31f * Radius + FMath::FRandRange(-0.18f, 0.18f), Half + 0.05f);
		F.Pos = X.TransformPosition(Eye);
		const FVector Out = X.TransformVectorNoScale(FVector::UpVector);     // yuzden disari
		const FVector CoinUp = X.TransformVectorNoScale(FVector::ForwardVector);
		switch (Group)
		{
		case 0: F.Life = FMath::FRandRange(0.4f, 0.7f); F.Size = FMath::FRandRange(0.6f, 1.0f); F.Vel = (Out * 3.f + CoinUp * 2.5f + FVector::UpVector * 3.f) * S; break;
		case 1: F.Life = FMath::FRandRange(0.25f, 0.45f); F.Size = FMath::FRandRange(0.35f, 0.6f); F.Vel = (Out * 2.f + CoinUp * 2.f + FVector::UpVector * 2.f) * S; break;
		default: F.Life = FMath::FRandRange(0.7f, 1.3f); F.Size = FMath::FRandRange(0.1f, 0.18f); F.Vel = (Out * FMath::FRandRange(3.f, 7.f) + FVector::UpVector * 6.f) * S; break;
		}
		F.Phase = FMath::FRandRange(0.f, 2.f * PI);
		F.Age = bRandomAge ? FMath::FRandRange(0.f, F.Life) : 0.f;
		return;
	}
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
		F.Size = FMath::FRandRange(1.5f, 2.4f);
		F.Vel = (FVector::UpVector * FMath::FRandRange(4.f, 8.f) + Out * 1.2f) * S;
		break;
	case 1:
		F.Life = FMath::FRandRange(0.28f, 0.5f);
		F.Size = FMath::FRandRange(0.9f, 1.4f);
		F.Vel = (FVector::UpVector * FMath::FRandRange(3.f, 6.f) + Out * 0.6f) * S;
		break;
	default:
		F.Life = FMath::FRandRange(0.8f, 1.5f);
		F.Size = FMath::FRandRange(0.2f, 0.35f);
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
	if (Aura == EFortuneCoinAura::Fire || Aura == EFortuneCoinAura::EyeFlames)
	{
		TickFire(Dt);
		return;
	}
	if (Aura == EFortuneCoinAura::Sparkle)   { TickSparkle();  return; }
	if (Aura == EFortuneCoinAura::Pages)     { TickPages();    return; }
	if (Aura == EFortuneCoinAura::RoseBloom) { TickRose(Dt);   return; }
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
	UInstancedStaticMeshComponent* Stem = MakeISM(CylMesh ? CylMesh.Get() : CubeMesh.Get(), FLinearColor(0.12f, 0.15f, 0.045f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Leaf = MakeISM(SphereMesh, FLinearColor(0.06f, 0.2f, 0.05f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Curl = MakeISM(SphereMesh, FLinearColor(0.09f, 0.18f, 0.045f), false, 0.f, false, true);
	if (!Stem || !Leaf || !Curl) return;
	const FVector X = FVector::ForwardVector;   // coinin ustu (+X)
	// Balkabagi seklinin ust cukuru ~0.72R: sap coinin icinden baslar (bosluk kalmaz)
	FVector P = X * (Radius * 0.62f);
	FVector D = X;
	for (int32 k = 0; k < 4; ++k)
	{
		D = (D + FVector(0.f, 0.14f, 0.03f)).GetSafeNormal();
		const float L = 0.5f;
		const float Rr = 0.36f - k * 0.05f;
		Stem->AddInstance(FTransform(FRotationMatrix::MakeFromZ(D).ToQuat(), P + D * L * 0.5f, FVector(Rr * 2.f, Rr * 2.f, L * 1.15f) * AuraCm), false);
		P += D * L;
	}
	// Sap ucunda kesik (biraz kalin)
	Stem->AddInstance(FTransform(FRotationMatrix::MakeFromZ(D).ToQuat(), P, FVector(0.5f, 0.5f, 0.12f) * AuraCm), false);
	// Iki yaprak: yassi elips, cukura dogru egik
	Leaf->AddInstance(FTransform(FRotationMatrix::MakeFromXY(FVector(0.35f, -1.f, 0.f).GetSafeNormal(), FVector(1.f, 0.35f, 0.f).GetSafeNormal()).ToQuat(),
		X * (Radius * 0.8f) + FVector(0.f, -0.9f, 0.12f), FVector(1.5f, 0.75f, 0.1f) * AuraCm), false);
	Leaf->AddInstance(FTransform(FRotationMatrix::MakeFromXY(FVector(0.2f, 1.f, 0.f).GetSafeNormal(), FVector(-1.f, 0.2f, 0.f).GetSafeNormal()).ToQuat(),
		X * (Radius * 0.82f) + FVector(0.f, 0.75f, -0.1f), FVector(1.1f, 0.55f, 0.09f) * AuraCm), false);
	// Kivrik dal (spiral)
	for (int32 i = 0; i < 18; ++i)
	{
		const float T = i / 17.f;
		const float A = T * 3.5f * PI;
		const float Rr = 0.5f * (1.f - T * 0.6f);
		const FVector C = X * (Radius * 0.86f) + FVector(FMath::Sin(A) * Rr, 1.25f + FMath::Cos(A) * Rr, 0.f);
		Curl->AddInstance(FTransform(FQuat::Identity, C, FVector(0.12f) * AuraCm), false);
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
	if (Groups.Num() < 3) return;
	TArray<FTransform> Xs, Tips;
	// Cirpma: kanatlar mentese ekseni (coinin ustu, +X) etrafinda one/arkaya acilip kapanir
	const float Beat = FMath::Sin(Time * 3.4f);
	const float Flap = FMath::DegreesToRadians(18.f + 30.f * Beat);
	const float Spread = 0.92f + 0.08f * Beat;
	struct FRow { int32 Count; float A0; float A1; float L0; float L1; float W; float Base; float Lift; };
	// birincil (uzun, ustte), ikincil (orta), ortu tuyleri (kisa, kok)
	const FRow Rows[3] = { {8, 18.f, 84.f, 3.0f, 4.6f, 0.62f, 1.25f, 0.06f}, {7, -22.f, 30.f, 2.1f, 3.0f, 0.6f, 0.75f, 0.12f}, {6, -10.f, 70.f, 1.1f, 1.6f, 0.55f, 0.3f, 0.18f} };
	for (int32 W = 0; W < 2; ++W)
	{
		const float S = W == 0 ? 1.f : -1.f;                    // +Y / -Y yan
		const FVector Root(0.35f * Radius, S * Radius * 0.9f, 0.f);
		const FQuat FlapQ(FVector::ForwardVector, -S * Flap);   // mentese: coinin ustu
		for (int32 r = 0; r < 3; ++r)
		{
			const FRow& R = Rows[r];
			for (int32 f = 0; f < R.Count; ++f)
			{
				const float T = R.Count > 1 ? f / float(R.Count - 1) : 0.f;
				const float Ang = FMath::DegreesToRadians(FMath::Lerp(R.A0, R.A1, T) * Spread);
				// kanat duzleminde (disa = S*Y, yukari = +X)
				const FVector Dir0(FMath::Sin(Ang), S * FMath::Cos(Ang), 0.f);
				const float L = FMath::Lerp(R.L0, R.L1, FMath::Sin(T * PI * 0.5f));
				const FVector Bone0 = FVector(Dir0.X * 0.2f, Dir0.Y * 0.2f, 0.f) * R.Base;
				FVector C0 = Bone0 + Dir0 * (L * 0.5f) + FVector(0.f, 0.f, -R.Lift - 0.03f * f);
				const FVector Dir = FlapQ.RotateVector(Dir0);
				const FVector C = Root + FlapQ.RotateVector(C0);
				const FVector Nrm = FlapQ.RotateVector(FVector::UpVector);
				const FVector Side = FVector::CrossProduct(Nrm, Dir).GetSafeNormal();
				Xs.Add(FTransform(FRotationMatrix::MakeFromXY(Dir, Side).ToQuat(), C, FVector(L, R.W, 0.14f) * AuraCm));
				if (r == 0)
				{
					Tips.Add(FTransform(FRotationMatrix::MakeFromXY(Dir, Side).ToQuat(), C + Dir * (L * 0.42f), FVector(L * 0.22f, R.W * 0.7f, 0.16f) * AuraCm));
				}
			}
		}
	}
	Groups[0]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
	Groups[2]->BatchUpdateInstancesTransforms(0, Tips, false, true, true);

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

// ---------------- Parilti yildizlari (+ isareti) ----------------

void UFortuneCoinAuraComponent::TickSparkle()
{
	if (Groups.Num() < 3) return;
	TArray<FTransform> Long, Short, Core;
	for (int32 i = 0; i < SparkSeeds.Num(); ++i)
	{
		const FVector& Sd = SparkSeeds[i];
		const float Cyc = Time * 0.6f * Sd.Z + Sd.X * 5.f;
		const int32 Gen = FMath::FloorToInt32(Cyc);
		const float T = Cyc - Gen;
		const uint32 H = (uint32(i + 1) * 2654435761u) ^ (uint32(Gen) * 40503u + 0x9e3779b9u);
		FRandomStream Rs(int32(H & 0x7fffffff));
		const float A = Rs.FRandRange(0.f, 2.f * PI);
		const bool bEdge = Rs.FRand() < 0.35f;                    // bazilari kenarda, coinin disinda parlar
		const float Rr = bEdge ? Radius * Rs.FRandRange(1.0f, 1.25f) : Radius * FMath::Sqrt(Rs.FRand()) * 0.85f;
		const float Side = Rs.FRand() < 0.5f ? 1.f : -1.f;
		const FVector P(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, bEdge ? Rs.FRandRange(-Half, Half) : Side * (Half + 0.15f));
		// Hizli parlar, yavas soner + kucuk bir sicrama
		const float Tw = T < 0.25f ? FMath::Sin(T / 0.25f * PI * 0.5f) : FMath::Pow(1.f - (T - 0.25f) / 0.75f, 2.f);
		const float L = (1.4f + 0.8f * Rs.FRand()) * Tw;
		const float Rot = Rs.FRandRange(0.f, PI) + T * 0.8f;
		const FVector N = bEdge ? FVector(FMath::Cos(A), FMath::Sin(A), 0.f) : FVector(0.f, 0.f, Side);
		FVector U = FVector::CrossProduct(N, FVector(0.31f, 0.77f, 0.55f)).GetSafeNormal();
		const FVector V0 = FVector::CrossProduct(N, U);
		const FVector U1 = U * FMath::Cos(Rot) + V0 * FMath::Sin(Rot);
		const FVector V1 = FVector::CrossProduct(N, U1);
		const FVector Lift = N * (0.4f * T);
		Long.Add(FTransform(FRotationMatrix::MakeFromXY(U1, V1).ToQuat(), P + Lift, FVector(L, 0.07f + 0.05f * Tw, 0.07f) * AuraCm));
		Long.Add(FTransform(FRotationMatrix::MakeFromXY(V1, U1).ToQuat(), P + Lift, FVector(L, 0.07f + 0.05f * Tw, 0.07f) * AuraCm));
		const FVector D1 = (U1 + V1).GetSafeNormal(), D2 = (U1 - V1).GetSafeNormal();
		Short.Add(FTransform(FRotationMatrix::MakeFromXY(D1, D2).ToQuat(), P + Lift, FVector(L * 0.45f, 0.05f, 0.05f) * AuraCm));
		Short.Add(FTransform(FRotationMatrix::MakeFromXY(D2, D1).ToQuat(), P + Lift, FVector(L * 0.45f, 0.05f, 0.05f) * AuraCm));
		Core.Add(FTransform(FQuat::Identity, P + Lift, FVector(0.28f * Tw) * AuraCm));
	}
	Groups[0]->BatchUpdateInstancesTransforms(0, Long, false, true, true);
	Groups[1]->BatchUpdateInstancesTransforms(0, Short, false, true, true);
	Groups[2]->BatchUpdateInstancesTransforms(0, Core, false, true, true);
	if (Light) Light->SetIntensity(LightBase * (0.7f + 0.3f * FMath::Sin(Time * 5.f)));
}

// ---------------- Takvim: kopup ucan sayfalar ----------------

void UFortuneCoinAuraComponent::TickPages()
{
	if (Groups.Num() < 3) return;
	TArray<FTransform> Pg, Hd;
	const float Period = 3.f;
	const FVector Hinge(Radius * 0.74f, 0.f, Half + 0.04f);    // sayfanin ust kenari (spiral halkalarin alti)
	const float PW = Radius * 1.05f, PH = Radius * 1.25f;
	for (int32 i = 0; i < 3; ++i)
	{
		const float T = FMath::Fmod(Time / Period + i / 3.f, 1.f);
		FQuat Q = FQuat::Identity;
		FVector Off = FVector::ZeroVector;
		float Sc = 1.f;
		if (T < 0.2f)
		{
			// yuzde yatiyor
		}
		else if (T < 0.55f)
		{
			// ust kenardan kalkip arkaya dogru katlanir
			const float E = (T - 0.2f) / 0.35f;
			Q = FQuat(FVector::RightVector, -FMath::DegreesToRadians(150.f * FMath::InterpEaseInOut(0.f, 1.f, E, 2.f)));
		}
		else
		{
			// kopar, donerek ucar ve kuculur
			const float E = (T - 0.55f) / 0.45f;
			Q = FQuat(FVector::RightVector, -FMath::DegreesToRadians(150.f + 260.f * E)) * FQuat(FVector::ForwardVector, E * 2.2f);
			Off = FVector(Radius * 1.6f * E, (i - 1) * Radius * 0.9f * E, Radius * 1.2f * E * (1.f - E) + 0.8f * E);
			Sc = 1.f - E;
		}
		const FVector PageCenterLocal(-PH * 0.5f, 0.f, 0.f);       // mentese -> sayfa merkezi (asagi dogru)
		const FVector C = Hinge + Off + Q.RotateVector(PageCenterLocal);
		Pg.Add(FTransform(Q, C + FVector(0.f, 0.f, 0.01f * i), FVector(PH, PW, 0.025f) * Sc * AuraCm));
		const FVector HC = Hinge + Off + Q.RotateVector(FVector(-PH * 0.1f, 0.f, 0.02f));
		Hd.Add(FTransform(Q, HC + FVector(0.f, 0.f, 0.01f * i), FVector(PH * 0.2f, PW, 0.03f) * Sc * AuraCm));
	}
	Groups[1]->BatchUpdateInstancesTransforms(0, Pg, false, true, true);
	Groups[2]->BatchUpdateInstancesTransforms(0, Hd, false, true, true);
}

// ---------------- Siyah gul: yumusak yapraklar + sarmasik + dokulen yapraklar ----------------

void UFortuneCoinAuraComponent::BuildVines(bool bLeaves)
{
	UInstancedStaticMeshComponent* Vine = MakeISM(CylMesh ? CylMesh.Get() : CubeMesh.Get(), FLinearColor(0.03f, 0.12f, 0.03f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Thorn = MakeISM(ConeMesh ? ConeMesh.Get() : CubeMesh.Get(), FLinearColor(0.05f, 0.16f, 0.04f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* Leaf = MakeISM(SphereMesh, FLinearColor(0.03f, 0.18f, 0.04f), false, 0.f, false, true);
	if (!Vine || !Thorn || !Leaf) return;
	const FVector Up = FVector::UpVector;
	FRandomStream Rs(4242);
	for (int32 v = 0; v < 2; ++v)
	{
		const int32 N = 48;
		const float Ph = v * 1.7f;
		auto Pt = [&](int32 i)
		{
			const float A = 2.f * PI * i / N;
			const float R = Radius + 0.4f + 0.16f * FMath::Sin(5.f * A + Ph);
			const float Z = (v == 0 ? 1.f : -1.f) * 0.36f * FMath::Sin(3.f * A + Ph);
			return FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, Z);
		};
		for (int32 i = 0; i < N; ++i)
		{
			const FVector P0 = Pt(i), P1 = Pt(i + 1);
			const FVector Dir = (P1 - P0).GetSafeNormal();
			const FVector Radial = FVector(P0.X, P0.Y, 0.f).GetSafeNormal();
			const float Len = (P1 - P0).Size() * 1.2f;
			Vine->AddInstance(FTransform(FRotationMatrix::MakeFromZ(Dir).ToQuat(), (P0 + P1) * 0.5f, FVector(0.2f, 0.2f, Len) * AuraCm), false);
			if (i % 3 == v)
			{
				const FVector TD = (Radial * 0.8f + Up * Rs.FRandRange(-0.7f, 0.7f) + Dir * 0.3f).GetSafeNormal();
				Thorn->AddInstance(FTransform(FRotationMatrix::MakeFromZ(TD).ToQuat(), P0 + TD * 0.24f, FVector(0.15f, 0.15f, 0.5f) * AuraCm), false);
			}
			if (bLeaves && i % 8 == 3 + v)
			{
				const FVector LD = (Radial + Up * (v == 0 ? 0.5f : -0.5f)).GetSafeNormal();
				Leaf->AddInstance(FTransform(FRotationMatrix::MakeFromXY(LD, Dir).ToQuat(), P0 + LD * 0.5f, FVector(1.0f, 0.5f, 0.08f) * AuraCm), false);
			}
		}
	}
}

void UFortuneCoinAuraComponent::BuildRose()
{
	BuildVines(true);
	// 3D gul (yazi yuzunun ortasinda, dokudaki gulun ustunde): yumusak elips yapraklar, ice dogru dikleserek
	UInstancedStaticMeshComponent* PetalA = MakeISM(SphereMesh, FLinearColor(0.07f, 0.0f, 0.012f), false, 0.f, false, true);
	UInstancedStaticMeshComponent* PetalB = MakeISM(SphereMesh, FLinearColor(0.16f, 0.005f, 0.03f), false, 0.f, false, true);
	if (!PetalA || !PetalB) return;
	const FVector Up = FVector::UpVector;
	const FVector Cen(-0.06f * Radius, 0.f, Half);
	struct FRing { int32 Count; float R; float Len; float Wid; float Tilt; float Z; };
	const FRing Rings[] = { {6, 0.95f, 1.2f, 0.95f, 14.f, 0.1f}, {6, 0.72f, 1.0f, 0.85f, 32.f, 0.25f}, {5, 0.5f, 0.82f, 0.7f, 50.f, 0.42f}, {4, 0.3f, 0.62f, 0.55f, 66.f, 0.6f}, {3, 0.14f, 0.45f, 0.42f, 78.f, 0.75f} };
	FRandomStream Rs(777);
	for (int32 k = 0; k < 5; ++k)
	{
		const FRing& R = Rings[k];
		for (int32 j = 0; j < R.Count; ++j)
		{
			const float A = 2.f * PI * j / R.Count + k * 0.6f;
			const FVector Rd(FMath::Cos(A), FMath::Sin(A), 0.f);
			const FVector Td(-Rd.Y, Rd.X, 0.f);
			const float T = FMath::DegreesToRadians(R.Tilt + Rs.FRandRange(-5.f, 5.f));
			const FVector X = (Rd * FMath::Cos(T) + Up * FMath::Sin(T)).GetSafeNormal();
			const FVector Base = Cen + Rd * R.R * 0.6f + Up * R.Z;
			UInstancedStaticMeshComponent* P = ((j + k) % 2 == 0) ? PetalA : PetalB;
			P->AddInstance(FTransform(FRotationMatrix::MakeFromXY(X, Td).ToQuat(), Base + X * R.Len * 0.35f, FVector(R.Len, R.Wid, 0.14f) * AuraCm), false);
		}
	}
	PetalA->AddInstance(FTransform(FRotator(30.f, 40.f, 15.f).Quaternion(), Cen + Up * 0.95f, FVector(0.42f, 0.36f, 0.5f) * AuraCm), false);
	// Dokulen yapraklar (dunya uzayi, ucarken arkada kalir)
	PetalGroup = Groups.Num();
	UInstancedStaticMeshComponent* Fall = MakeISM(SphereMesh, FLinearColor(0.22f, 0.01f, 0.04f), false, 0.f, true, true);
	if (Fall)
	{
		Petals.SetNum(8);
		for (FFlame& F : Petals)
		{
			F.Age = FMath::FRandRange(0.f, 2.5f);
			F.Life = FMath::FRandRange(2.f, 3.f);
			F.Pos = GetComponentLocation();
			Fall->AddInstance(FTransform(FQuat::Identity, F.Pos, FVector::ZeroVector), false);
		}
	}
}

void UFortuneCoinAuraComponent::TickRose(float Dt)
{
	if (!Groups.IsValidIndex(PetalGroup)) return;
	const FTransform& X = GetComponentTransform();
	const float S = X.GetScale3D().GetAbsMax();
	TArray<FTransform> Xs;
	for (int32 i = 0; i < Petals.Num(); ++i)
	{
		FFlame& F = Petals[i];
		F.Age += Dt;
		if (F.Age >= F.Life)
		{
			// Gulden kopar: coinin yuzunden, kenara yakin bir yerden
			const float A = FMath::FRandRange(0.f, 2.f * PI);
			F.Pos = X.TransformPosition(FVector(FMath::Cos(A) * Radius * 0.5f, FMath::Sin(A) * Radius * 0.5f, Half + 0.6f));
			F.Vel = (X.TransformVectorNoScale(FVector(FMath::Cos(A), FMath::Sin(A), 0.6f)) * FMath::FRandRange(2.f, 5.f)) * S;
			F.Age = 0.f;
			F.Life = FMath::FRandRange(2.f, 3.f);
			F.Phase = FMath::FRandRange(0.f, 2.f * PI);
		}
		const float T = F.Age / F.Life;
		F.Vel.Z -= 4.f * S * Dt;
		F.Vel *= FMath::Max(0.f, 1.f - 1.2f * Dt);
		F.Pos += (F.Vel + FVector(FMath::Sin(Time * 2.f + F.Phase), FMath::Cos(Time * 1.7f + F.Phase), 0.f) * 1.5f * S) * Dt;
		const float Sc = S * (T < 0.1f ? T / 0.1f : 1.f - FMath::Max(0.f, (T - 0.7f) / 0.3f));
		const FQuat Q = FQuat(FRotator(Time * 90.f + F.Phase * 50.f, Time * 60.f + i * 40.f, Time * 120.f + F.Phase * 30.f));
		Xs.Add(FTransform(Q, F.Pos, FVector(0.7f, 0.5f, 0.07f) * Sc * AuraCm));
	}
	Groups[PetalGroup]->BatchUpdateInstancesTransforms(0, Xs, false, true, true);
}
