#include "FortuneCoinPreview.h"
#include "FortuneCoinData.h"
#include "FortuneCoinShape.h"
#include "FortuneCoinAura.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void PreviewTint(UStaticMeshComponent* C, UMaterialInterface* Base, const FLinearColor& Color)
	{
		if (!C) return;
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, Base))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
}

AFortuneCoinPreview::AFortuneCoinPreview()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Spinner = CreateDefaultSubobject<USceneComponent>(TEXT("Spinner"));
	Spinner->SetupAttachment(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	DefaultCoinMesh = Cyl.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	ShapeMaterial = ShapeMat.Object;

	auto Make = [this](const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Spinner);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetStaticMesh(Mesh);
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Scale);
		C->SetCastShadow(false);
		return C;
	};
	CoinMesh  = Make(TEXT("CoinMesh"),  Cyl.Object, FVector::ZeroVector, FVector(0.06f, 0.06f, 0.006f));
	HeadsFace = Make(TEXT("HeadsFace"), Cyl.Object, FVector(0.f, 0.f, 0.32f), FVector(0.042f, 0.042f, 0.0008f));
	HeadsGem  = Make(TEXT("HeadsGem"),  Sph.Object, FVector(0.f, 0.f, 0.36f), FVector(0.016f, 0.016f, 0.004f));
	TailsFace = Make(TEXT("TailsFace"), Cyl.Object, FVector(0.f, 0.f, -0.32f), FVector(0.046f, 0.046f, 0.0008f));
	TailsRing = Make(TEXT("TailsRing"), Cyl.Object, FVector(0.f, 0.f, -0.34f), FVector(0.022f, 0.022f, 0.0008f));

	ShapedCoin = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ShapedCoin"));
	ShapedCoin->SetupAttachment(Spinner);
	ShapedCoin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShapedCoin->SetVisibility(false);

	CoinAura = CreateDefaultSubobject<UFortuneCoinAuraComponent>(TEXT("CoinAura"));
	CoinAura->SetupAttachment(Spinner);

	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(Root);
	KeyLight->SetRelativeLocation(FVector(-12.f, -8.f, 10.f));
	KeyLight->SetIntensity(1200.f);
	KeyLight->SetAttenuationRadius(60.f);
	KeyLight->SetLightColor(FLinearColor(1.f, 0.92f, 0.8f));
	KeyLight->SetCastShadows(false);
}

void AFortuneCoinPreview::PlaceInFrontOf(const FVector& CamLoc, const FRotator& CamRot, float Distance, float SideOffset)
{
	const FRotationMatrix M(CamRot);
	const FVector Fwd = M.GetUnitAxis(EAxis::X);
	const FVector Right = M.GetUnitAxis(EAxis::Y);
	const FVector Up = M.GetUnitAxis(EAxis::Z);

	SetActorLocation(CamLoc + Fwd * Distance + Right * SideOffset);
	// Coinin yuzu (Z ekseni) kameraya baksin; kameranin "yukari" ekseni etrafinda donsun
	BaseRot = FRotationMatrix::MakeFromZX(-Fwd, Up).ToQuat();
	SpinAxis = Up;
	SetActorRotation(FRotator::ZeroRotator);
	Spinner->SetWorldRotation(BaseRot);
	// Isik kameraya gore konumlansin
	KeyLight->SetWorldLocation(GetActorLocation() - Fwd * 12.f + Up * 8.f - Right * 8.f);
}

void AFortuneCoinPreview::PlaceAt(const FVector& Location, const FVector& ViewFrom, const FVector& ViewUp, float Scale)
{
	const FVector ToCam = (ViewFrom - Location).GetSafeNormal();
	SetActorLocation(Location);
	SetActorRotation(FRotator::ZeroRotator);
	SetActorScale3D(FVector(Scale));
	BaseRot = FRotationMatrix::MakeFromZX(ToCam, ViewUp).ToQuat();
	SpinAxis = ViewUp;
	Spinner->SetWorldRotation(BaseRot);
	const FVector Right = FVector::CrossProduct(ViewUp, -ToCam).GetSafeNormal();
	KeyLight->SetWorldLocation(Location + ToCam * 12.f * Scale + ViewUp * 8.f * Scale - Right * 8.f * Scale);
	KeyLight->SetAttenuationRadius(60.f * Scale);
	KeyLight->SetIntensity(1200.f * Scale);
}

void AFortuneCoinPreview::SetCoin(const UFortuneCoinData* Coin, bool bOwned, bool bEnchanted)
{
	PopIn = 0.f;

	// Yuz resimleri olan coin: sekilli model (bulunmamissa siyah siluet)
	const bool bShaped = Coin && !Coin->CoinMesh && FFortuneCoinShapeBuilder::BuildForCoin(ShapedCoin, Coin, bOwned, bEnchanted);
	ShapedCoin->SetVisibility(bShaped);
	if (CoinAura)
	{
		CoinAura->SetAura((bShaped && bOwned) ? Coin->Aura : EFortuneCoinAura::None, Coin ? Coin->AuraColor : FLinearColor::White);
	}
	if (bShaped)
	{
		CoinMesh->SetVisibility(false);
		HeadsFace->SetVisibility(false);
		HeadsGem->SetVisibility(false);
		TailsFace->SetVisibility(false);
		TailsRing->SetVisibility(false);
		return;
	}
	CoinMesh->SetVisibility(true);
	const bool bCustomMesh = Coin && Coin->CoinMesh;
	CoinMesh->SetStaticMesh(bCustomMesh ? Coin->CoinMesh.Get() : DefaultCoinMesh.Get());

	HeadsFace->SetVisibility(!bCustomMesh);
	HeadsGem->SetVisibility(!bCustomMesh);
	TailsFace->SetVisibility(!bCustomMesh);
	TailsRing->SetVisibility(!bCustomMesh);

	if (!bOwned)
	{
		// Bulunmamis coin: siyah siluet
		const FLinearColor Shadow(0.01f, 0.01f, 0.012f);
		PreviewTint(CoinMesh, ShapeMaterial, Shadow);
		PreviewTint(HeadsFace, ShapeMaterial, Shadow);
		PreviewTint(HeadsGem, ShapeMaterial, Shadow);
		PreviewTint(TailsFace, ShapeMaterial, Shadow);
		PreviewTint(TailsRing, ShapeMaterial, Shadow);
		return;
	}

	UMaterialInterface* Mat = nullptr;
	if (Coin)
	{
		Mat = (bEnchanted && Coin->EnchantedMaterial) ? Coin->EnchantedMaterial.Get() : Coin->CoinMaterial.Get();
	}
	if (Mat)
	{
		CoinMesh->SetMaterial(0, Mat);
	}
	else
	{
		PreviewTint(CoinMesh, ShapeMaterial, FLinearColor(0.8f, 0.6f, 0.25f));
	}
	PreviewTint(HeadsFace, ShapeMaterial, FLinearColor(1.f, 0.93f, 0.55f));
	PreviewTint(HeadsGem, ShapeMaterial, FLinearColor(1.f, 1.f, 0.85f));
	PreviewTint(TailsFace, ShapeMaterial, FLinearColor(0.12f, 0.06f, 0.02f));
	PreviewTint(TailsRing, ShapeMaterial, FLinearColor(0.6f, 0.35f, 0.12f));
}

void AFortuneCoinPreview::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bFixedPose)
	{
		Angle += DeltaSeconds * 50.f;
		PopIn = FMath::Min(1.f, PopIn + DeltaSeconds * 4.f);
	}
	const float Ease = 1.f - FMath::Pow(1.f - PopIn, 3.f);

	// Hafif sallanma + kendi etrafinda yavas donus (iki yuzu de gorunur)
	const FQuat Spin(SpinAxis, FMath::DegreesToRadians(Angle));
	Spinner->SetWorldRotation(Spin * BaseRot);
	Spinner->SetRelativeScale3D(FVector(FMath::Lerp(0.6f, 1.f, Ease)));
	Spinner->SetRelativeLocation(bFixedPose ? FVector::ZeroVector : FVector(0.f, 0.f, FMath::Sin(Angle * 0.05f) * 0.4f));
}
