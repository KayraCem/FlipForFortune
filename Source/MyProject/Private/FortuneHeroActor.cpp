#include "FortuneHeroActor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "UObject/ObjectSaveContext.h"
#include "Misc/Paths.h"

// Eklem hiyerarsisi (Python hero.py ile ayni). Karakter +X'e bakar, Z yukari, cm.
const TArray<AFortuneHeroActor::FJointDef>& AFortuneHeroActor::Defs()
{
	static const TArray<FJointDef> D = {
		{ TEXT("Pelvis"),     -1, FVector(0, 0, 95),       TEXT("pelvis") },     // 0
		{ TEXT("Chest"),       0, FVector(0, 0, 5),        TEXT("chest") },      // 1
		{ TEXT("Head"),        1, FVector(1, 0, 45),       TEXT("head") },       // 2
		{ TEXT("UpperArmR"),   1, FVector(0, -19.5f, 38),  TEXT("upperarm") },   // 3
		{ TEXT("ForearmR"),    3, FVector(0, 0, -29),      TEXT("forearm") },    // 4
		{ TEXT("HandR"),       4, FVector(0, 0, -25),      TEXT("hand_r") },     // 5
		{ TEXT("UpperArmL"),   1, FVector(0, 19.5f, 38),   TEXT("upperarm") },   // 6
		{ TEXT("ForearmL"),    6, FVector(0, 0, -29),      TEXT("forearm") },    // 7
		{ TEXT("HandL"),       7, FVector(0, 0, -25),      TEXT("hand_l") },     // 8
		{ TEXT("ThighR"),      0, FVector(0, -9, -3),      TEXT("thigh") },      // 9
		{ TEXT("ShinR"),       9, FVector(0, 0, -44),      TEXT("shin") },       // 10
		{ TEXT("FootR"),      10, FVector(0, 0, -40),      TEXT("foot") },       // 11
		{ TEXT("ThighL"),      0, FVector(0, 9, -3),       TEXT("thigh") },      // 12
		{ TEXT("ShinL"),      12, FVector(0, 0, -44),      TEXT("shin") },       // 13
		{ TEXT("FootL"),      13, FVector(0, 0, -40),      TEXT("foot") },       // 14
	};
	return D;
}

AFortuneHeroActor::AFortuneHeroActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	for (const FJointDef& J : Defs())
	{
		USceneComponent* C = CreateDefaultSubobject<USceneComponent>(J.Name);
		C->SetupAttachment(J.Parent < 0 ? Root.Get() : Joints[J.Parent].Get());
		C->SetRelativeLocation(J.Loc);
		Joints.Add(C);
		UProceduralMeshComponent* P = CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("%sMesh"), J.Name));
		P->SetupAttachment(C);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Meshes.Add(P);
	}
}

int32 AFortuneHeroActor::JointIndex(const TCHAR* Name) const
{
	const TArray<FJointDef>& D = Defs();
	for (int32 i = 0; i < D.Num(); ++i)
	{
		if (FCString::Strcmp(D[i].Name, Name) == 0) return i;
	}
	return INDEX_NONE;
}

void AFortuneHeroActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LoadParts();
	ApplyPose();
}

void AFortuneHeroActor::BeginPlay()
{
	Super::BeginPlay();
	if (!bLoaded) LoadParts();
	ApplyPose();
}

bool AFortuneHeroActor::LoadPart(UProceduralMeshComponent* PMC, const FString& File)
{
	TArray<uint8> Data;
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), PartsFolder, File);
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
	for (int32 i = 0; i < NV; ++i)
	{
		V[i] = FVector(Pos[i * 3], Pos[i * 3 + 1], Pos[i * 3 + 2]);
		N[i] = -FVector(Nrm[i * 3], Nrm[i * 3 + 1], Nrm[i * 3 + 2]);   // dosyadaki normaller iceri bakiyor -> disari cevir
		C[i] = FColor(Col[i * 4], Col[i * 4 + 1], Col[i * 4 + 2], Col[i * 4 + 3]);
	}
	// UE on yuz sarimi icin ucgen sirasi ters (eldeki modelle ayni kural)
	TArray<int32> T; T.SetNumUninitialized(NT * 3);
	for (int32 t = 0; t < NT; ++t)
	{
		T[t * 3 + 0] = Idx[t * 3 + 0];
		T[t * 3 + 1] = Idx[t * 3 + 2];
		T[t * 3 + 2] = Idx[t * 3 + 1];
	}
	PMC->ClearAllMeshSections();
	PMC->CreateMeshSection(0, V, T, N, TArray<FVector2D>(), C, TArray<FProcMeshTangent>(), false);

	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_VertexColor.M_VertexColor"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mat) Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	PMC->SetMaterial(0, Mat);
	return true;
}

FQuat AFortuneHeroActor::GetBodyFrame() const
{
	return Joints.Num() > 1 && Joints[1] ? Joints[1]->GetComponentQuat() : GetActorQuat();
}

FVector AFortuneHeroActor::GetEyePoint() const
{
	return Joints.Num() > 2 ? Joints[2]->GetComponentTransform().TransformPosition(FVector(10.f, 0.f, 20.f)) : GetActorLocation();
}

bool AFortuneHeroActor::LoadParts()
{
	const TArray<FJointDef>& D = Defs();
	bool bAll = true;
	for (int32 i = 0; i < D.Num(); ++i)
	{
		bAll &= LoadPart(Meshes[i], FString::Printf(TEXT("hero_%s.bin"), D[i].Part));
	}
	bLoaded = bAll;
	// Birinci sahis: kamera bas icinde oldugu icin bas/govde/bacaklar gizli, kollar gorunur
	static const int32 BodyParts[] = { 0, 1, 2, 9, 10, 11, 12, 13, 14 };
	for (int32 I : BodyParts)
	{
		if (Meshes.IsValidIndex(I)) Meshes[I]->SetVisibility(!bFirstPerson);
	}
	if (!bAll)
	{
		UE_LOG(LogTemp, Warning, TEXT("FortuneHero: %s klasorunde hero_*.bin dosyalari eksik."), *PartsFolder);
	}
	return bAll;
}

FVector AFortuneHeroActor::GetCoinPoint() const
{
	const int32 I = JointIndex(bCoinArmPlusY ? TEXT("HandL") : TEXT("HandR"));
	if (I == INDEX_NONE) return GetActorLocation();
	// Avuc ortasinin biraz ustu (el yerel uzayi: avuc +X'e bakar, parmaklar -Z)
	return Joints[I]->GetComponentTransform().TransformPosition(FVector(5.2f, 0.f, -9.f));
}

void AFortuneHeroActor::SetPose(float InWindup, float InFlick)
{
	Windup = FMath::Clamp(InWindup, 0.f, 1.f);
	Flick = FMath::Clamp(InFlick, 0.f, 1.f);
}

void AFortuneHeroActor::SetCatch(float InCatch)
{
	Catch = FMath::Clamp(InCatch, 0.f, 1.f);
}

void AFortuneHeroActor::SetLookTarget(const FVector& WorldPoint)
{
	LookTarget = WorldPoint;
}

void AFortuneHeroActor::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);
	for (UProceduralMeshComponent* P : Meshes)
	{
		if (P) P->ClearAllMeshSections();
	}
	bLoaded = false;
	bReloadAfterSave = true;
}

void AFortuneHeroActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bReloadAfterSave)
	{
		bReloadAfterSave = false;
		LoadParts();   // editor: kayittan sonra model geri gelsin
		ApplyPose();
	}
	if (GetWorld() && !GetWorld()->IsGameWorld()) return;   // editorde animasyon yok
	Time += DeltaSeconds;
	LookBlend = FMath::FInterpTo(LookBlend, LookTarget.IsZero() ? 0.f : 1.f, DeltaSeconds, 8.f);
	ApplyPose();
}

void AFortuneHeroActor::ApplyPose()
{
	if (Joints.Num() < 15) return;
	auto Set = [this](int32 I, float Pitch, float Yaw, float Roll)
	{
		Joints[I]->SetRelativeRotation(FRotator(Pitch, Yaw, Roll));
	};
	const float Breath = FMath::Sin(Time * 1.7f);
	const float Sway = FMath::Sin(Time * 0.6f);

	// Govde: hafif nefes + agirlik aktarimi
	Set(0, 0.f, Sway * 1.5f, Sway * 1.2f);
	Set(1, -2.f + Breath * 0.8f, -Sway * 1.f, 0.f);
	if (bFirstPerson)
	{
		// Kollar bakisi takip eder: gogus goz noktasi etrafinda doner, boylece el hep ekranin altinda kalir
		const FVector Eye(11.f, 0.f, 65.f);
		const FRotator ChestRot(-2.f + Breath * 0.8f + ViewPitch, -Sway * 1.f, 0.f);
		Joints[1]->SetRelativeRotation(ChestRot);
		Joints[1]->SetRelativeLocation(FVector(0.f, 0.f, 5.f) + Eye - ChestRot.RotateVector(Eye));
	}
	Joints[1]->SetRelativeScale3D(FVector(1.f + Breath * 0.006f, 1.f + Breath * 0.01f, 1.f));

	// Coin kolu: dirsek bukuk, avuc yukari, govdenin onunde
	const int32 UA = bCoinArmPlusY ? 6 : 3, FA = UA + 1, HA = UA + 2;
	const float S = bCoinArmPlusY ? 1.f : -1.f;
	// Birinci sahista el biraz daha onde ve yuksekte durur ki kamerada gorunsun
	const float FP = bFirstPerson ? 1.f : 0.f;
	const float ForearmPitch = 68.f + 10.f * FP - 14.f * Windup + 34.f * Flick - 12.f * Catch;
	Set(UA, 22.f + 18.f * FP + 4.f * Flick - 4.f * Catch, 0.f, S * (6.f - 2.f * FP));
	Set(FA, ForearmPitch, -S * (26.f - 12.f * FP), 0.f);
	Set(HA, 6.f * Windup + 22.f * Flick - 6.f * Catch, 0.f, 0.f);

	// Diger kol: yanda gevsek, hafif sallanir
	const int32 UB = bCoinArmPlusY ? 3 : 6;
	Set(UB, 4.f + Breath * 0.6f, 0.f, -S * 7.f);
	Set(UB + 1, 14.f, 0.f, 0.f);
	Set(UB + 2, 5.f, 0.f, 0.f);

	// Bas: avucuna ya da havadaki coine bakar
	float HeadPitch = -22.f, HeadYaw = S * 12.f;
	if (LookBlend > 0.f)
	{
		const FTransform HeadW = Joints[2]->GetComponentTransform();
		const FVector Eye = HeadW.GetLocation() + GetActorUpVector() * 20.f;
		const FVector Dir = GetActorTransform().InverseTransformVectorNoScale((LookTarget - Eye).GetSafeNormal());
		const float P = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Dir.Z, -1.f, 1.f)));
		const float Y = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
		HeadPitch = FMath::Lerp(HeadPitch, FMath::Clamp(P, -40.f, 45.f), LookBlend);
		HeadYaw = FMath::Lerp(HeadYaw, FMath::Clamp(Y, -40.f, 40.f), LookBlend);
	}
	Set(2, HeadPitch + 2.f, HeadYaw, 0.f);

	// Bacaklar
	Set(9, 0.f, 0.f, -2.f);  Set(10, 0.f, 0.f, 0.f);  Set(11, 0.f, -8.f, 0.f);
	Set(12, 0.f, 0.f, 2.f);  Set(13, 0.f, 0.f, 0.f);  Set(14, 0.f, 8.f, 0.f);
}
