#include "FortuneFountain.h"
#include "ProceduralMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/ObjectSaveContext.h"

AFortuneFountain::AFortuneFountain()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	auto MakePMC = [this](const TCHAR* Name)
	{
		UProceduralMeshComponent* P = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		P->SetupAttachment(Root);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // yurume carpismasi: Python'daki gorunmez silindir
		return P;
	};
	StoneMesh = MakePMC(TEXT("Stone"));
	GlowMesh = MakePMC(TEXT("Glow"));
	WaterMesh = MakePMC(TEXT("Water"));
	FallsMesh = MakePMC(TEXT("Falls"));
	FallsMesh->SetTranslucentSortPriority(2);   // akan su, su yuzeyinin onunde cizilsin (kesik gorunmesin)
	WaterMesh->SetTranslucentSortPriority(1);

	OrbLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OrbLight"));
	OrbLight->SetupAttachment(Root);
	OrbLight->SetRelativeLocation(FVector(0.f, 0.f, 189.f));
	OrbLight->SetIntensity(1200.f);
	OrbLight->SetAttenuationRadius(380.f);
	OrbLight->SetCastShadows(false);

	PoolLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PoolLight"));
	PoolLight->SetupAttachment(Root);
	PoolLight->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	PoolLight->SetIntensity(900.f);
	PoolLight->SetAttenuationRadius(300.f);
	PoolLight->SetCastShadows(false);
}

void AFortuneFountain::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LoadAll();   // editorde de gorunsun
}

void AFortuneFountain::BeginPlay()
{
	Super::BeginPlay();
	if (!bLoaded) LoadAll();
	OrbLight->SetLightColor(SpiritColor);
	PoolLight->SetLightColor(SpiritColor);
}

void AFortuneFountain::LoadAll()
{
	bLoaded = LoadFountainPart(StoneMesh, TEXT("fountain_stone.bin"), TEXT("M_Fountain_Stone.M_Fountain_Stone"), true);
	LoadFountainPart(GlowMesh, TEXT("fountain_glow.bin"), TEXT("M_Fountain_Glow.M_Fountain_Glow"), false);
	LoadFountainPart(WaterMesh, TEXT("fountain_water.bin"), TEXT("M_Fountain_Water.M_Fountain_Water"), false);
	LoadFountainPart(FallsMesh, TEXT("fountain_falls.bin"), TEXT("M_Fountain_Falls.M_Fountain_Falls"), false);
}

bool AFortuneFountain::LoadFountainPart(UProceduralMeshComponent* PMC, const TCHAR* File, const TCHAR* MaterialPath, bool bShadow)
{
	if (!PMC) return false;
	TArray<uint8> Data;
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), ModelFolder, File);
	if (!FFileHelper::LoadFileToArray(Data, *Path, FILEREAD_Silent) || Data.Num() < 12) return false;
	if (FMemory::Memcmp(Data.GetData(), "FHM2", 4) != 0) return false;
	int32 NV = 0, NT = 0;
	FMemory::Memcpy(&NV, Data.GetData() + 4, 4);
	FMemory::Memcpy(&NT, Data.GetData() + 8, 4);
	const int64 Need = 12 + (int64)NV * (24 + 4) + (int64)NT * 12;
	if (NV <= 0 || NT <= 0 || Data.Num() < Need) return false;

	const float* Pos = reinterpret_cast<const float*>(Data.GetData() + 12);
	const float* Nrm = Pos + NV * 3;
	const uint8* Col = reinterpret_cast<const uint8*>(Nrm + NV * 3);
	const int32* Idx = reinterpret_cast<const int32*>(Col + NV * 4);

	TArray<FVector> V; V.SetNumUninitialized(NV);
	TArray<FVector> N; N.SetNumUninitialized(NV);
	TArray<FColor> C; C.SetNumUninitialized(NV);
	TArray<FVector2D> UV; UV.SetNumUninitialized(NV);
	for (int32 i = 0; i < NV; ++i)
	{
		V[i] = FVector(Pos[i * 3], Pos[i * 3 + 1], Pos[i * 3 + 2]);
		N[i] = -FVector(Nrm[i * 3], Nrm[i * 3 + 1], Nrm[i * 3 + 2]);   // dosyada ic normal -> disari cevir
		C[i] = FColor(Col[i * 4], Col[i * 4 + 1], Col[i * 4 + 2], Col[i * 4 + 3]);
		UV[i] = FVector2D(0.f, Col[i * 4 + 3] / 255.f);   // akan su: V = akis yonu
	}
	TArray<int32> T; T.SetNumUninitialized(NT * 3);
	for (int32 t = 0; t < NT; ++t)
	{
		T[t * 3 + 0] = Idx[t * 3 + 0];
		T[t * 3 + 1] = Idx[t * 3 + 2];
		T[t * 3 + 2] = Idx[t * 3 + 1];
	}
	PMC->ClearAllMeshSections();
	PMC->CreateMeshSection(0, V, T, N, UV, C, TArray<FProcMeshTangent>(), false);
	PMC->SetCastShadow(bShadow);

	const FString MatPath = FString(TEXT("/Game/FlipForFortune/Materials/")) + MaterialPath;
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, *MatPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mat) Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_VertexColor.M_VertexColor"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Mat) PMC->SetMaterial(0, Mat);
	return true;
}

void AFortuneFountain::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);
	for (UProceduralMeshComponent* P : { StoneMesh.Get(), GlowMesh.Get(), WaterMesh.Get(), FallsMesh.Get() })
	{
		if (P) P->ClearAllMeshSections();
	}
	bLoaded = false;
	bReloadAfterSave = true;
}

void AFortuneFountain::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bReloadAfterSave)
	{
		bReloadAfterSave = false;
		LoadAll();   // editor: kayittan sonra model geri gelsin
	}
	if (GetWorld() && !GetWorld()->IsGameWorld()) return;
	Time += DeltaSeconds;
	// Ruh kuresi nefes alir gibi parlar, havuz isigi suyla titrer
	GlowShown = FMath::FInterpTo(GlowShown, GlowAmount, DeltaSeconds, 6.f);
	const float G = GlowShown;
	OrbLight->SetIntensity(1100.f + 350.f * FMath::Sin(Time * (2.f + G * 6.f)) + G * 7000.f);
	PoolLight->SetIntensity(850.f + 120.f * FMath::Sin(Time * 3.1f) * FMath::Sin(Time * 1.7f + 1.f) + G * 5000.f);
	PoolLight->SetAttenuationRadius(300.f + G * 250.f);
	const FLinearColor C = FMath::Lerp(SpiritColor, GlowColor, FMath::Clamp(G, 0.f, 1.f));
	OrbLight->SetLightColor(C);
	PoolLight->SetLightColor(C);
}
