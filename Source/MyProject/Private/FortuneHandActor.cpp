#include "FortuneHandActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "ProceduralMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Yerel eksenler: X = parmaklarin baktigi yon (ileri), Y = sag, Z = yukari.
// Sag el; basparmak -Y tarafinda (coin'e dogru).

AFortuneHandActor::AFortuneHandActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shape(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	SphereMesh = Sph.Object;
	CylMesh = Cyl.Object;
	ShapeMaterial = Shape.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Skel = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Skel"));
	Skel->SetupAttachment(Root);
	Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skel->SetVisibility(false);
	Wrist = CreateDefaultSubobject<USceneComponent>(TEXT("Wrist"));
	Wrist->SetupAttachment(Root);

	// Kol + yen (kameraya dogru uzanir)
	SkinParts.Add(Part(TEXT("Forearm"), Root, SphereMesh, FVector(-11.f, 0.5f, -0.8f), FVector(16.f, 6.4f, 5.6f)));
	SleeveParts.Add(Part(TEXT("Sleeve"), Root, SphereMesh, FVector(-19.f, 0.6f, -0.8f), FVector(16.f, 8.6f, 7.8f)));
	SleeveParts.Add(Part(TEXT("Cuff"), Root, CylMesh, FVector(-11.5f, 0.6f, -0.8f), FVector(8.8f, 8.f, 2.2f), FRotator(90.f, 0.f, 0.f)));

	// Avuc (yumruk)
	SkinParts.Add(Part(TEXT("Palm"), Wrist, SphereMesh, FVector(0.f, 0.f, 0.f), FVector(9.5f, 8.6f, 5.8f)));
	SkinParts.Add(Part(TEXT("PalmBack"), Wrist, SphereMesh, FVector(1.5f, 0.f, 0.9f), FVector(7.5f, 8.2f, 4.2f)));

	// Dort parmak: kivrik (ust bogum ileri, orta asagi, uc geri)
	const float Ys[4] = { -3.05f, -1.0f, 1.05f, 3.0f };   // isaret, orta, yuzuk, serce
	const float Sc[4] = { 1.f, 1.05f, 0.98f, 0.82f };
	for (int32 i = 0; i < 4; ++i)
	{
		const float Y = Ys[i], S = Sc[i];
		SkinParts.Add(Part(*FString::Printf(TEXT("Knuckle%d"), i), Wrist, SphereMesh, FVector(4.4f, Y, 1.3f), FVector(2.3f, 2.1f, 2.1f) * S));
		SkinParts.Add(Part(*FString::Printf(TEXT("Prox%d"), i), Wrist, SphereMesh, FVector(5.9f, Y, 0.6f), FVector(4.4f, 2.0f, 2.0f) * S, FRotator(-22.f, 0.f, 0.f)));
		SkinParts.Add(Part(*FString::Printf(TEXT("Mid%d"), i), Wrist, SphereMesh, FVector(7.9f * (0.9f + 0.1f * S), Y, -1.5f), FVector(2.0f, 1.9f, 3.7f) * S, FRotator(-8.f, 0.f, 0.f)));
		SkinParts.Add(Part(*FString::Printf(TEXT("Dist%d"), i), Wrist, SphereMesh, FVector(6.6f, Y, -3.2f), FVector(2.8f, 1.75f, 1.75f) * S, FRotator(15.f, 0.f, 0.f)));
	}

	// Basparmak: isaret parmaginin altindan one dogru, fiske icin donen eklem
	ThumbPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ThumbPivot"));
	ThumbPivot->SetupAttachment(Wrist);
	ThumbPivot->SetRelativeLocation(FVector(1.2f, -4.3f, -1.0f));
	SkinParts.Add(Part(TEXT("ThumbBase"), ThumbPivot, SphereMesh, FVector(0.6f, -0.3f, 0.f), FVector(4.f, 3.2f, 3.f)));
	SkinParts.Add(Part(TEXT("Thumb1"), ThumbPivot, SphereMesh, FVector(2.8f, -0.5f, 0.2f), FVector(4.6f, 2.5f, 2.4f), FRotator(0.f, 12.f, 0.f)));
	ThumbTipPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ThumbTipPivot"));
	ThumbTipPivot->SetupAttachment(ThumbPivot);
	ThumbTipPivot->SetRelativeLocation(FVector(4.7f, -0.2f, 0.3f));
	SkinParts.Add(Part(TEXT("Thumb2"), ThumbTipPivot, SphereMesh, FVector(1.4f, 0.f, 0.f), FVector(3.3f, 2.2f, 2.1f)));
	NailParts.Add(Part(TEXT("ThumbNail"), ThumbTipPivot, SphereMesh, FVector(1.9f, 0.f, 0.85f), FVector(1.9f, 1.5f, 0.5f)));

	// Puruzsuz el modeli (Art/Hand/*.bin): yuklenirse yukaridaki kureler gizlenir
	auto MakePMC = [this](const TCHAR* Name, USceneComponent* Parent)
	{
		UProceduralMeshComponent* P = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		P->SetupAttachment(Parent);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->bUseAsyncCooking = true;
		return P;
	};
	FistMesh = MakePMC(TEXT("FistMesh"), Wrist);
	ThumbMesh = MakePMC(TEXT("ThumbMesh"), ThumbPivot);
	ThumbTipMesh = MakePMC(TEXT("ThumbTipMesh"), ThumbTipPivot);
	ThumbNailMesh = MakePMC(TEXT("ThumbNailMesh"), ThumbTipPivot);
	ArmMesh = MakePMC(TEXT("ArmMesh"), Root);

	// Coin noktasi: kivrik isaret parmagi ile basparmak tirnaginin ustu
	CoinPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CoinPoint"));
	CoinPoint->SetupAttachment(Wrist);
	CoinPoint->SetRelativeLocation(FVector(6.4f, -3.6f, 2.15f));
}

UStaticMeshComponent* AFortuneHandActor::Part(const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm, const FRotator& Rot)
{
	UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	C->SetupAttachment(Parent);
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocation(Loc);
	C->SetRelativeRotation(Rot);
	C->SetRelativeScale3D(SizeCm / 100.f);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetMobility(EComponentMobility::Movable);
	return C;
}

void AFortuneHandActor::BeginPlay()
{
	Super::BeginPlay();
	WristRest = Wrist->GetRelativeLocation();
	auto Tint = [this](UStaticMeshComponent* C, const FLinearColor& Color)
	{
		if (UMaterialInstanceDynamic* MID = C ? C->CreateDynamicMaterialInstance(0, ShapeMaterial) : nullptr)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	};
	for (UStaticMeshComponent* C : SkinParts) Tint(C, SkinColor);
	for (UStaticMeshComponent* C : NailParts) Tint(C, NailColor);
	for (UStaticMeshComponent* C : SleeveParts) Tint(C, SleeveColor);

	if (HandMesh)
	{
		SetupSkeletal();
	}
	if (!bSkeletal)
	{
		LoadSmoothHand();
	}
}

void AFortuneHandActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Mesh verisi haritaya kaydedilmesin diye el sadece oyunda (BeginPlay) yuklenir
}

bool AFortuneHandActor::LoadBinMesh(UProceduralMeshComponent* PMC, const FString& File, const FLinearColor& Color)
{
	if (!PMC) return false;
	TArray<uint8> Data;
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/Hand"), File);
	if (!FFileHelper::LoadFileToArray(Data, *Path, FILEREAD_Silent) || Data.Num() < 12) return false;
	if (FMemory::Memcmp(Data.GetData(), "FHM1", 4) != 0) return false;
	int32 NV = 0, NT = 0;
	FMemory::Memcpy(&NV, Data.GetData() + 4, 4);
	FMemory::Memcpy(&NT, Data.GetData() + 8, 4);
	const int64 Need = 12 + (int64)NV * 24 + (int64)NT * 12;
	if (NV <= 0 || NT <= 0 || Data.Num() < Need) return false;

	const float* Pos = reinterpret_cast<const float*>(Data.GetData() + 12);
	const float* Nrm = Pos + NV * 3;
	const int32* Idx = reinterpret_cast<const int32*>(Nrm + NV * 3);
	TArray<FVector> V; V.SetNumUninitialized(NV);
	TArray<FVector> N; N.SetNumUninitialized(NV);
	for (int32 i = 0; i < NV; ++i)
	{
		V[i] = FVector(Pos[i * 3], Pos[i * 3 + 1], Pos[i * 3 + 2]);
		N[i] = -FVector(Nrm[i * 3], Nrm[i * 3 + 1], Nrm[i * 3 + 2]);   // dosyadaki normaller iceri bakiyor -> disari cevir
	}
	// Ucgen sirasini ters cevir: UE'de on yuz bu sarimla gorunur (yoksa el iceriden gorunuyordu)
	TArray<int32> T; T.SetNumUninitialized(NT * 3);
	for (int32 t = 0; t < NT; ++t)
	{
		T[t * 3 + 0] = Idx[t * 3 + 0];
		T[t * 3 + 1] = Idx[t * 3 + 2];
		T[t * 3 + 2] = Idx[t * 3 + 1];
	}

	PMC->ClearAllMeshSections();
	PMC->CreateMeshSection(0, V, T, N, TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_AuraPlain.M_AuraPlain"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mat) Mat = ShapeMaterial;
	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, PMC))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		PMC->SetMaterial(0, MID);
	}
	PMC->SetVisibility(true);
	return true;
}

bool AFortuneHandActor::LoadSmoothHand()
{
	if (!LoadBinMesh(FistMesh, TEXT("hand_fist.bin"), SkinColor)) return false;
	LoadBinMesh(ThumbMesh, TEXT("hand_thumb.bin"), SkinColor);
	LoadBinMesh(ThumbTipMesh, TEXT("hand_thumbtip.bin"), SkinColor);
	LoadBinMesh(ThumbNailMesh, TEXT("hand_nail.bin"), NailColor);
	LoadBinMesh(ArmMesh, TEXT("hand_arm.bin"), SkinColor);
	// Eski kure el parcalarini gizle (yen kalir)
	for (UStaticMeshComponent* C : SkinParts) if (C) C->SetVisibility(false);
	for (UStaticMeshComponent* C : NailParts) if (C) C->SetVisibility(false);
	return true;
}

// ---------------- Iskeletli el ----------------

namespace
{
	void HandComputeCS(const FReferenceSkeleton& Ref, const TArray<FTransform>& Local, TArray<FTransform>& OutCS)
	{
		OutCS.SetNum(Local.Num());
		for (int32 i = 0; i < Local.Num(); ++i)
		{
			const int32 P = Ref.GetParentIndex(i);
			OutCS[i] = (P == INDEX_NONE) ? Local[i] : Local[i] * OutCS[P];
		}
	}
}

void AFortuneHandActor::SetupSkeletal()
{
	Skel->SetSkinnedAssetAndUpdate(HandMesh, true);
	const FReferenceSkeleton& Ref = HandMesh->GetRefSkeleton();
	const int32 IHand = Ref.FindBoneIndex(TEXT("hand_r"));
	const int32 IElbow = Ref.FindBoneIndex(TEXT("lowerarm_r"));
	const int32 IIndex = Ref.FindBoneIndex(TEXT("index_01_r"));
	const int32 IPinky = Ref.FindBoneIndex(TEXT("pinky_01_r"));
	const int32 IMiddle = Ref.FindBoneIndex(TEXT("middle_01_r"));
	if (IHand == INDEX_NONE || IElbow == INDEX_NONE || IIndex == INDEX_NONE || IPinky == INDEX_NONE || IMiddle == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("FortuneHand: %s modelinde hand_r / parmak kemikleri yok, kodla kurulan el kullaniliyor."), *HandMesh->GetName());
		Skel->SetVisibility(false);
		return;
	}

	bSkeletal = true;
	Skel->SetVisibility(true);
	for (UStaticMeshComponent* C : SkinParts) if (C) C->SetVisibility(false);
	for (UStaticMeshComponent* C : NailParts) if (C) C->SetVisibility(false);
	for (UStaticMeshComponent* C : SleeveParts) if (C) C->SetVisibility(false);

	// Yumruk pozunda kolu yerlestir: kol +X'e (coin'e) uzansin, isaret parmagi ustte kalsin
	ApplySkeletalPose();
	TArray<FTransform> CS;
	HandComputeCS(Ref, Skel->BoneSpaceTransforms, CS);
	const FVector Hand = CS[IHand].GetLocation();
	const FVector ArmDir = (Hand - CS[IElbow].GetLocation()).GetSafeNormal();
	FVector Across = (CS[IIndex].GetLocation() - CS[IPinky].GetLocation());
	Across = (Across - ArmDir * FVector::DotProduct(Across, ArmDir)).GetSafeNormal();
	const FQuat ToCS = FRotationMatrix::MakeFromXZ(ArmDir, Across).ToQuat();
	const FQuat Final = MeshRotationOffset.Quaternion() * ToCS.Inverse();
	const FVector Palm = (Hand + CS[IMiddle].GetLocation()) * 0.5f;

	Skel->SetRelativeRotation(Final);
	Skel->SetRelativeScale3D(FVector(MeshScale));
	SkelBaseLoc = WristRest - Final.RotateVector(Palm * MeshScale);
	Skel->SetRelativeLocation(SkelBaseLoc);
}

void AFortuneHandActor::ApplySkeletalPose()
{
	if (!HandMesh || !Skel) return;
	const FReferenceSkeleton& Ref = HandMesh->GetRefSkeleton();
	const TArray<FTransform>& RefPose = Ref.GetRefBonePose();
	TArray<FTransform>& L = Skel->BoneSpaceTransforms;
	L = RefPose;

	for (const FName& B : HiddenBones)
	{
		const int32 I = Ref.FindBoneIndex(B);
		if (I != INDEX_NONE) L[I].SetScale3D(FVector(0.001f));
	}

	auto Curl = [&](const TCHAR* Bone, const FVector& Axis, float Deg)
	{
		const int32 I = Ref.FindBoneIndex(Bone);
		if (I == INDEX_NONE) return;
		L[I].SetRotation(L[I].GetRotation() * FQuat(Axis.GetSafeNormal(), FMath::DegreesToRadians(Deg)));
	};

	// Dort parmak yumruk
	const TCHAR* Fingers[4][3] = {
		{ TEXT("index_01_r"), TEXT("index_02_r"), TEXT("index_03_r") },
		{ TEXT("middle_01_r"), TEXT("middle_02_r"), TEXT("middle_03_r") },
		{ TEXT("ring_01_r"), TEXT("ring_02_r"), TEXT("ring_03_r") },
		{ TEXT("pinky_01_r"), TEXT("pinky_02_r"), TEXT("pinky_03_r") } };
	const float FingerDeg[3] = { 72.f, 95.f, 60.f };
	for (int32 f = 0; f < 4; ++f)
	{
		for (int32 j = 0; j < 3; ++j)
		{
			// isaret parmagi biraz daha acik: coin onun ustunde durur
			const float D = FingerDeg[j] * (f == 0 ? 0.8f : 1.f);
			Curl(Fingers[f][j], FingerCurlAxis, FingerCurlSign * D);
		}
	}

	// Basparmak: gerilirken isaret parmaginin altina kivrilir, fiskede firlar
	const float W = PoseWindup, F = PoseFlick;
	Curl(TEXT("thumb_01_r"), ThumbCurlAxis, ThumbCurlSign * (15.f + 20.f * W - 35.f * F));
	Curl(TEXT("thumb_02_r"), ThumbCurlAxis, ThumbCurlSign * (25.f + 35.f * W - 55.f * F));
	Curl(TEXT("thumb_03_r"), ThumbCurlAxis, ThumbCurlSign * (15.f + 30.f * W - 40.f * F));
	// Bilek
	Curl(TEXT("hand_r"), FingerCurlAxis, FingerCurlSign * (8.f * W - 14.f * F));

	Skel->MarkRefreshTransformDirty();

	// Coin noktasi: isaret parmaginin orta bogumunun ustu
	TArray<FTransform> CS;
	HandComputeCS(Ref, L, CS);
	const int32 IIdx = Ref.FindBoneIndex(TEXT("index_02_r"));
	const int32 IIndex = Ref.FindBoneIndex(TEXT("index_01_r"));
	const int32 IPinky = Ref.FindBoneIndex(TEXT("pinky_01_r"));
	if (IIdx != INDEX_NONE && IIndex != INDEX_NONE && IPinky != INDEX_NONE)
	{
		const FVector Across = (CS[IIndex].GetLocation() - CS[IPinky].GetLocation()).GetSafeNormal();
		CoinLocalCS = CS[IIdx].GetLocation() + Across * 2.2f;
	}
}

FVector AFortuneHandActor::GetCoinPoint() const
{
	if (bSkeletal && Skel)
	{
		return Skel->GetComponentTransform().TransformPosition(CoinLocalCS);
	}
	return CoinPoint->GetComponentLocation();
}

void AFortuneHandActor::SetPose(float Windup, float Flick)
{
	PoseWindup = FMath::Clamp(Windup, 0.f, 1.f);
	PoseFlick = FMath::Clamp(Flick, 0.f, 1.f);
}

void AFortuneHandActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	if (bSkeletal)
	{
		ApplySkeletalPose();
		const float SBob = FMath::Sin(Time * 1.6f) * 0.15f;
		Skel->SetRelativeLocation(SkelBaseLoc + FVector(0.f, 0.f, SBob + 1.2f * PoseFlick - 0.4f * PoseWindup));
		return;
	}

	// Basparmak: gerilirken asagi kivrilir, fiskede yukari-ileri firlar
	const float ThumbPitch = -16.f * PoseWindup + 62.f * PoseFlick;
	const float TipPitch = -25.f * PoseWindup + 20.f * PoseFlick;
	ThumbPivot->SetRelativeRotation(FRotator(ThumbPitch, 0.f, 0.f));
	ThumbTipPivot->SetRelativeRotation(FRotator(TipPitch, 0.f, 0.f));

	// Bilek: fiskede hafif yukari zipla, bosta yavasca nefes al
	const float Bob = FMath::Sin(Time * 1.6f) * 0.15f;
	Wrist->SetRelativeLocation(WristRest + FVector(0.f, 0.f, Bob + 1.2f * PoseFlick - 0.4f * PoseWindup));
	Wrist->SetRelativeRotation(FRotator(7.f * PoseFlick - 3.f * PoseWindup, 0.f, 0.f));
}
