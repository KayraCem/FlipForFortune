#include "FortuneGachaChest.h"
#include "FortuneGachaWidgets.h"
#include "FortuneSubsystem.h"
#include "FortuneSettings.h"
#include "FortuneCoinData.h"
#include "FortuneCoinPreview.h"
#include "FortuneCollectionWidget.h"
#include "FortunePlayerController.h"
#include "FortuneLandFX.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Sandik olculeri (cm): X derinlik, Y genislik, Z yukseklik
	constexpr float ChestDepth = 20.f;
	constexpr float ChestWidth = 30.f;
	constexpr float ChestHeight = 14.f;

	const FLinearColor ChestWood(0.2f, 0.07f, 0.04f);
	const FLinearColor ChestTrim(1.f, 0.72f, 0.25f);
}

AFortuneGachaChest::AFortuneGachaChest()
{
	Action = EFortuneInteraction::Summon;
	Label = FText::FromString(TEXT("Gacha"));
	HoverLift = 1.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shape(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	CylMesh = Cyl.Object;
	SphMesh = Sph.Object;
	ShapeMaterial = Shape.Object;

	// Tiklama kutusu (kok) sandigin ortasinda; Body alt yuzeye hizalanir
	ClickBox->SetBoxExtent(FVector(ChestDepth * 0.5f + 1.f, ChestWidth * 0.5f + 1.f, 14.f));

	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(ClickBox);
	Body->SetRelativeLocation(FVector(0.f, 0.f, -14.f));   // masaya otursun

	const float D = ChestDepth, W = ChestWidth, H = ChestHeight;

	// Govde: taban + 4 duvar (ici bos, coinler gorunsun)
	WoodParts.Add(MakePart(TEXT("Floor"), Body, CubeMesh, FVector(0, 0, 1.f), FVector(D, W, 2.f)));
	WoodParts.Add(MakePart(TEXT("WallFront"), Body, CubeMesh, FVector(-D * 0.5f + 1.f, 0, H * 0.5f), FVector(2.f, W, H)));
	WoodParts.Add(MakePart(TEXT("WallBack"), Body, CubeMesh, FVector(D * 0.5f - 1.f, 0, H * 0.5f), FVector(2.f, W, H)));
	WoodParts.Add(MakePart(TEXT("WallL"), Body, CubeMesh, FVector(0, -W * 0.5f + 1.f, H * 0.5f), FVector(D, 2.f, H)));
	WoodParts.Add(MakePart(TEXT("WallR"), Body, CubeMesh, FVector(0, W * 0.5f - 1.f, H * 0.5f), FVector(D, 2.f, H)));

	// Altin kenarlar ve kose kalkanlari
	GoldParts.Add(MakePart(TEXT("RimFront"), Body, CubeMesh, FVector(-D * 0.5f + 1.f, 0, H), FVector(2.6f, W + 0.6f, 1.2f)));
	GoldParts.Add(MakePart(TEXT("RimBack"), Body, CubeMesh, FVector(D * 0.5f - 1.f, 0, H), FVector(2.6f, W + 0.6f, 1.2f)));
	GoldParts.Add(MakePart(TEXT("RimL"), Body, CubeMesh, FVector(0, -W * 0.5f + 1.f, H), FVector(D + 0.6f, 2.6f, 1.2f)));
	GoldParts.Add(MakePart(TEXT("RimR"), Body, CubeMesh, FVector(0, W * 0.5f - 1.f, H), FVector(D + 0.6f, 2.6f, 1.2f)));
	for (int32 i = 0; i < 2; ++i)
	{
		const float Y = (i == 0 ? -1.f : 1.f) * W * 0.3f;
		GoldParts.Add(MakePart(i == 0 ? TEXT("BandFL") : TEXT("BandFR"), Body, CubeMesh, FVector(-D * 0.5f - 0.2f, Y, H * 0.5f), FVector(0.8f, 2.f, H)));
	}
	GoldParts.Add(MakePart(TEXT("LockPlate"), Body, CubeMesh, FVector(-D * 0.5f - 0.4f, 0, H - 3.f), FVector(1.f, 5.f, 6.f)));
	GoldParts.Add(MakePart(TEXT("Keyhole"), Body, CylMesh, FVector(-D * 0.5f - 0.9f, 0, H - 3.5f), FVector(1.2f, 1.2f, 0.3f), FRotator(90.f, 0.f, 0.f)));
	for (int32 i = 0; i < 4; ++i)
	{
		const float X = (i % 2 == 0 ? -1.f : 1.f) * (D * 0.5f);
		const float Y = (i < 2 ? -1.f : 1.f) * (W * 0.5f);
		GoldParts.Add(MakePart(*FString::Printf(TEXT("Corner%d"), i), Body, CubeMesh, FVector(X, Y, 1.5f), FVector(3.f, 3.f, 3.f)));
	}

	// Ici: renkli coin yigini (sabit "rastgele" dizilim)
	const FVector2D PilePos[] = {
		{-5, -9}, {-2, -4}, {1, 2}, {-4, 6}, {3, 9}, {5, -6}, {0, -11}, {-6, 1}, {4, 4}, {-1, 10},
		{6, 0}, {-3, -1}, {2, -8}, {0, 6}, {-6, -6}, {5, 10}
	};
	const float PileZ[] = { 11.f, 12.5f, 13.4f, 12.2f, 11.6f, 12.f, 11.f, 12.8f, 13.f, 11.8f, 12.4f, 13.6f, 12.1f, 13.2f, 11.4f, 11.2f };
	for (int32 i = 0; i < UE_ARRAY_COUNT(PilePos); ++i)
	{
		const FRotator Tilt(FMath::Fmod(i * 37.f, 50.f) - 25.f, i * 53.f, FMath::Fmod(i * 23.f, 40.f) - 20.f);
		PileCoins.Add(MakePart(*FString::Printf(TEXT("PileCoin%d"), i), Body, CylMesh, FVector(PilePos[i].X, PilePos[i].Y, PileZ[i]), FVector(5.f, 5.f, 0.6f), Tilt));
	}
	// Coinlerin altindaki altin "dolgu" ve birkac mucevher
	GoldParts.Add(MakePart(TEXT("PileBed"), Body, SphMesh, FVector(0, 0, 9.f), FVector(D - 4.f, W - 4.f, 7.f)));
	PileCoins.Add(MakePart(TEXT("GemA"), Body, SphMesh, FVector(-2.f, 7.f, 13.5f), FVector(2.2f, 2.2f, 2.2f)));
	PileCoins.Add(MakePart(TEXT("GemB"), Body, SphMesh, FVector(3.f, -3.f, 13.8f), FVector(1.8f, 1.8f, 1.8f)));

	// Kapak: arka ust kenardan mentese
	LidPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LidPivot"));
	LidPivot->SetupAttachment(Body);
	LidPivot->SetRelativeLocation(FVector(D * 0.5f, 0.f, H + 0.6f));
	WoodParts.Add(MakePart(TEXT("LidTop"), LidPivot, CubeMesh, FVector(-D * 0.5f, 0, 2.f), FVector(D, W, 3.f)));
	// Basik kubbe: dikey capi kucuk tutuldu ki alt yarisi sandigin icine taşmasin
	WoodParts.Add(MakePart(TEXT("LidDome"), LidPivot, CylMesh, FVector(-D * 0.5f, 0, 3.f), FVector(D - 1.f, 8.f, W - 0.5f), FRotator(0.f, 0.f, 90.f)));
	GoldParts.Add(MakePart(TEXT("LidBandL"), LidPivot, CylMesh, FVector(-D * 0.5f, -W * 0.3f, 3.f), FVector(D + 0.4f, 8.6f, 1.8f), FRotator(0.f, 0.f, 90.f)));
	GoldParts.Add(MakePart(TEXT("LidBandR"), LidPivot, CylMesh, FVector(-D * 0.5f, W * 0.3f, 3.f), FVector(D + 0.4f, 8.6f, 1.8f), FRotator(0.f, 0.f, 90.f)));
	GoldParts.Add(MakePart(TEXT("LidLatch"), LidPivot, CubeMesh, FVector(-D - 0.4f, 0, 1.f), FVector(1.f, 4.f, 4.f)));

	// Icten gelen isik
	InnerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("InnerLight"));
	InnerLight->SetupAttachment(Body);
	InnerLight->SetRelativeLocation(FVector(-6.f, 0.f, H + 6.f));
	InnerLight->SetIntensity(250.f);
	InnerLight->SetAttenuationRadius(45.f);
	InnerLight->SetLightColor(FLinearColor(1.f, 0.75f, 0.35f));
	InnerLight->SetCastShadows(false);

	// Nadirlik / sans listesi (uzerine gelince)
	RatesWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Rates"));
	RatesWidget->SetupAttachment(ClickBox);
	RatesWidget->SetRelativeLocation(FVector(0.f, 0.f, 34.f));
	RatesWidget->SetWidgetSpace(EWidgetSpace::Screen);
	RatesWidget->SetDrawAtDesiredSize(true);
	RatesWidget->SetPivot(FVector2D(0.5f, 1.f));
	RatesWidget->SetWidgetClass(UFortuneGachaRatesWidget::StaticClass());
	RatesWidget->SetVisibility(false);
	RatesWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UStaticMeshComponent* AFortuneGachaChest::MakePart(const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm, const FRotator& Rot)
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

void AFortuneGachaChest::BeginPlay()
{
	Super::BeginPlay();
	BodyRestLoc = Body->GetRelativeLocation();

	auto Tint = [this](UStaticMeshComponent* C, const FLinearColor& Color)
	{
		if (C)
		{
			if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, ShapeMaterial))
			{
				MID->SetVectorParameterValue(TEXT("Color"), Color);
			}
		}
	};
	for (UStaticMeshComponent* C : WoodParts) Tint(C, ChestWood);
	for (UStaticMeshComponent* C : GoldParts) Tint(C, ChestTrim);

	// Coin yigini: bronz, gumus, altin, yesil, kirmizi, mor... (cesit cesit para)
	const FLinearColor PileColors[] = {
		FLinearColor(0.62f, 0.36f, 0.18f), FLinearColor(0.85f, 0.86f, 0.9f), FLinearColor(1.f, 0.72f, 0.25f),
		FLinearColor(0.2f, 0.75f, 0.35f), FLinearColor(1.f, 0.3f, 0.08f), FLinearColor(0.6f, 0.35f, 1.f),
		FLinearColor(0.3f, 0.6f, 1.f), FLinearColor(1.f, 0.85f, 0.4f)
	};
	for (int32 i = 0; i < PileCoins.Num(); ++i)
	{
		Tint(PileCoins[i], PileColors[i % UE_ARRAY_COUNT(PileColors)]);
	}

	SetLid(IdleLidAngle);
}

void AFortuneGachaChest::SetLid(float Angle)
{
	LidAngle = Angle;
	// Negatif pitch: on kenar yukari kalkar (kapak acilir)
	LidPivot->SetRelativeRotation(FRotator(-Angle, 0.f, 0.f));
}

void AFortuneGachaChest::NotifyActorBeginCursorOver()
{
	Super::NotifyActorBeginCursorOver();
	// Nadirlikler artik tiklayinca acilan Summon panelinde
}

void AFortuneGachaChest::NotifyActorEndCursorOver()
{
	Super::NotifyActorEndCursorOver();
	ShowRates(false);
}

void AFortuneGachaChest::ShowRates(bool bShow)
{
	if (!RatesWidget) return;
	if (bShow)
	{
		if (UFortuneGachaRatesWidget* W = Cast<UFortuneGachaRatesWidget>(RatesWidget->GetUserWidgetObject()))
		{
			W->Refresh();
		}
	}
	RatesWidget->SetVisibility(bShow);
}

void AFortuneGachaChest::Interact()
{
	// Tiklayinca once Summon paneli acilir (nadirlikler + SUMMON butonu)
	if (State != EState::Idle) return;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	AFortunePlayerController* FPC = Cast<AFortunePlayerController>(PC);
	if (!PC || (FPC && FPC->IsMenuOpen())) return;

	ShowRates(false);
	if (UFortuneSummonWidget* W = CreateWidget<UFortuneSummonWidget>(PC, UFortuneSummonWidget::StaticClass()))
	{
		W->Chest = this;
		W->AddToViewport(15);
	}
	OnInteracted();
}

bool AFortuneGachaChest::StartSummon(int32 Count)
{
	if (State != EState::Idle) return false;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	AFortunePlayerController* FPC = Cast<AFortunePlayerController>(PC);

	const UGameInstance* GI = GetGameInstance();
	UFortuneSubsystem* F = GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!F) return false;

	TArray<UFortuneCoinData*> Coins;
	TArray<bool> Dups;
	if (!F->SummonCoins(FMath::Max(1, Count), Coins, Dups) || Coins.Num() == 0)
	{
		return false;
	}

	// Nadirlige gore sirala: en iyisi en son cikar (ayni nadirlikte yeni coin tekrardan sonra)
	TArray<int32> Order;
	for (int32 i = 0; i < Coins.Num(); ++i) Order.Add(i);
	Order.StableSort([&Coins, &Dups](int32 A, int32 B)
	{
		const uint8 RA = (uint8)Coins[A]->Rarity;
		const uint8 RB = (uint8)Coins[B]->Rarity;
		if (RA != RB) return RA < RB;
		return Dups[A] && !Dups[B];
	});
	WonCoins.Reset();
	WonDups.Reset();
	for (int32 Idx : Order)
	{
		WonCoins.Add(Coins[Idx]);
		WonDups.Add(Dups[Idx]);
	}
	BestIndex = WonCoins.Num() - 1;
	bMulti = WonCoins.Num() > 1;
	MultiCoins.Reset();
	MultiSlots.Reset();
	MultiLanded.Reset();
	BurstTime = -1.f;

	WonCoin = WonCoins[BestIndex];
	bWonDuplicate = WonDups[BestIndex];
	RarityColor = UFortuneSubsystem::GetRarityColor(WonCoin->Rarity);
	if (FPC) FPC->SetMenuOpen(true);

	State = EState::Shake;
	StateTime = 0.f;
	if (ShakeSound) UGameplayStatics::PlaySound2D(this, ShakeSound);
	return true;
}

void AFortuneGachaChest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (State == EState::Idle)
	{
		// Bosta: hafif nefes alan isik
		InnerLight->SetIntensity(220.f + 60.f * FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f));
		return;
	}

	StateTime += DeltaSeconds;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	switch (State)
	{
	case EState::Shake:
	{
		// Sallanma: nadirlik ne kadar yuksekse o kadar guclu ve renkli
		const float Dur = bMulti ? 1.1f : 0.6f;
		const float Strength = (bMulti ? 0.9f : 0.6f) + 0.25f * (float)(uint8)WonCoin->Rarity;
		const float T = FMath::Min(1.f, StateTime / Dur);
		Body->SetRelativeLocation(BodyRestLoc + FVector(0.f, FMath::Sin(StateTime * 60.f) * Strength * (1.f - T * 0.5f), FMath::Abs(FMath::Sin(StateTime * 30.f)) * 0.6f));
		SetLid(IdleLidAngle * (1.f - T) + FMath::Abs(FMath::Sin(StateTime * 45.f)) * 8.f);
		if (bMulti && T < 0.8f)
		{
			// Coklu acilim: sandigin icinde cikacak coinlerin renkleri yanip soner
			const int32 Idx = FMath::FloorToInt32(StateTime * 12.f) % WonCoins.Num();
			InnerLight->SetLightColor(UFortuneSubsystem::GetRarityColor(WonCoins[Idx]->Rarity));
		}
		else
		{
			InnerLight->SetLightColor(FMath::Lerp(FLinearColor(1.f, 0.75f, 0.35f), RarityColor, T));
		}
		InnerLight->SetIntensity(FMath::Lerp(250.f, bMulti ? 2500.f : 1500.f, T));
		if (StateTime >= Dur)
		{
			Body->SetRelativeLocation(BodyRestLoc);
			State = EState::Open;
			StateTime = 0.f;
			if (OpenSound) UGameplayStatics::PlaySound2D(this, OpenSound);
			AFortuneLandFXActor::SpawnAt(GetWorld(), Body->GetComponentLocation() + FVector(0.f, 0.f, ChestHeight + 2.f),
				EFortuneLandFX::GoldRing, 1.f + 0.25f * (float)(uint8)WonCoin->Rarity, true);
			if (bMulti)
			{
				AFortuneLandFXActor::SpawnAt(GetWorld(), Body->GetComponentLocation() + FVector(0.f, 0.f, ChestHeight + 2.f),
					EFortuneLandFX::Sparks, 2.f, true);
			}
		}
		break;
	}
	case EState::Open:
	{
		const float T = FMath::Clamp(StateTime / 0.3f, 0.f, 1.f);
		const float Ease = 1.f - FMath::Pow(1.f - T, 3.f);
		SetLid(FMath::Lerp(0.f, 115.f, Ease));
		InnerLight->SetIntensity(FMath::Lerp(1500.f, 6000.f, Ease));
		InnerLight->SetAttenuationRadius(FMath::Lerp(45.f, 160.f, Ease));
		if (T >= 1.f && bMulti)
		{
			// Coklu acilim: coinler sirayla firlayip kameranin onunde izgaraya dizilir
			MultiCoins.Init(nullptr, WonCoins.Num());
			MultiLanded.Init(false, WonCoins.Num());
			State = EState::Fly;
			StateTime = 0.f;
		}
		else if (T >= 1.f)
		{
			// Coin sandiktan cikip kameraya ucacak
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			FlyingCoin = GetWorld()->SpawnActor<AFortuneCoinPreview>(AFortuneCoinPreview::StaticClass(), FTransform(Body->GetComponentLocation()), P);
			if (FlyingCoin)
			{
				FlyingCoin->SetCoin(WonCoin, true, false);
			}
			State = EState::Fly;
			StateTime = 0.f;
		}
		break;
	}
	case EState::Fly:
	{
		if (bMulti)
		{
			TickMultiFly(DeltaSeconds, PC);
			break;
		}
		const float T = FMath::Clamp(StateTime / 0.8f, 0.f, 1.f);
		const float Ease = 1.f - FMath::Pow(1.f - T, 3.f);
		InnerLight->SetIntensity(FMath::Lerp(6000.f, 1200.f, T));
		if (FlyingCoin && PC && PC->PlayerCameraManager)
		{
			const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
			const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
			const FVector Fwd = CamRot.Vector();
			const FVector Up = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Z);
			const FVector Start = Body->GetComponentLocation() + FVector(0.f, 0.f, ChestHeight + 4.f);
			const FVector End = CamLoc + Fwd * 28.f + Up * 1.5f;
			// Kavisli yol: once yukari firlar, sonra kameraya gelir
			FVector Pos = FMath::Lerp(Start, End, Ease);
			Pos.Z += FMath::Sin(T * PI) * 25.f;
			FlyingCoin->PlaceAt(Pos, CamLoc, Up, FMath::Lerp(0.7f, 1.f, Ease));
		}
		if (T >= 1.f)
		{
			State = EState::Reveal;
			StateTime = 0.f;
			if (RevealSound) UGameplayStatics::PlaySound2D(this, RevealSound);
			if (PC)
			{
				UFortuneGachaRevealWidget* W = CreateWidget<UFortuneGachaRevealWidget>(PC, UFortuneGachaRevealWidget::StaticClass());
				if (W)
				{
					W->AddToViewport(20);
					W->Setup(this, WonCoin, bWonDuplicate, GetDefault<UFortuneSettings>()->DuplicateXP);
					W->SetKeyboardFocus();
					RevealWidget = W;
				}
			}
		}
		break;
	}
	case EState::Reveal:
		// Oyuncu tiklayana kadar coin kameranin onunde doner (onizleme kendi Tick'inde doner)
		InnerLight->SetIntensity(1000.f + 300.f * FMath::Sin(StateTime * 4.f));
		break;

	case EState::Closing:
	{
		const float T = FMath::Clamp(StateTime / 0.35f, 0.f, 1.f);
		SetLid(FMath::Lerp(115.f, IdleLidAngle, T));
		InnerLight->SetIntensity(FMath::Lerp(1000.f, 250.f, T));
		InnerLight->SetAttenuationRadius(FMath::Lerp(160.f, 45.f, T));
		InnerLight->SetLightColor(FMath::Lerp(RarityColor, FLinearColor(1.f, 0.75f, 0.35f), T));
		if (T >= 1.f)
		{
			State = EState::Idle;
			StateTime = 0.f;
		}
		break;
	}
	default:
		break;
	}
}

void AFortuneGachaChest::FinishReveal()
{
	if (FlyingCoin)
	{
		FlyingCoin->Destroy();
		FlyingCoin = nullptr;
	}
	for (AFortuneCoinPreview* P : MultiCoins)
	{
		if (P) P->Destroy();
	}
	MultiCoins.Reset();
	bMulti = false;
	RevealWidget = nullptr;
	State = EState::Closing;
	StateTime = 0.f;

	// Summon bitince oyuna geri don (Coin Collection otomatik acilmaz)
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(PC))
	{
		FPC->SetMenuOpen(false);
	}
}

// ---------------- Coklu acilim ----------------

float AFortuneGachaChest::MultiStartTime(int32 Index) const
{
	// Her coin bir oncekinden kisa sure sonra firlar; nadir (Epic+) en iyi coin icin dramatik bekleme
	float T = Index * 0.13f;
	if (Index == BestIndex && WonCoins.IsValidIndex(Index) && (uint8)WonCoins[Index]->Rarity >= (uint8)EFortuneRarity::Epic)
	{
		T += 0.55f;
	}
	return T;
}

void AFortuneGachaChest::TickMultiFly(float DeltaSeconds, APlayerController* PC)
{
	if (!PC || !PC->PlayerCameraManager) return;
	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
	const FRotationMatrix CamM(CamRot);
	const FVector Fwd = CamM.GetUnitAxis(EAxis::X);
	const FVector Right = CamM.GetUnitAxis(EAxis::Y);
	const FVector Up = CamM.GetUnitAxis(EAxis::Z);
	const int32 N = WonCoins.Num();
	const int32 Cols = FMath::Min(N, 5);
	const int32 Rows = FMath::DivideAndRoundUp(N, 5);
	const float Dist = 48.f;
	const float Spacing = 9.5f;
	const float RowGap = 11.f;

	// Izgara yerleri (kameraya gore)
	MultiSlots.SetNum(N);
	for (int32 i = 0; i < N; ++i)
	{
		const int32 R = i / 5;
		const int32 InRow = (R == Rows - 1) ? (N - R * 5) : Cols;
		const int32 C = i % 5;
		const float X = (C - (InRow - 1) * 0.5f) * Spacing;
		const float Y = ((Rows - 1) * 0.5f - R) * RowGap + 2.f;
		MultiSlots[i] = CamLoc + Fwd * Dist + Right * X + Up * Y;
	}

	const FVector Start = Body->GetComponentLocation() + FVector(0.f, 0.f, ChestHeight + 4.f);
	const float FlyDur = 0.7f;
	bool bAllLanded = true;
	float LastStart = 0.f;
	for (int32 i = 0; i < N; ++i)
	{
		const float T0 = MultiStartTime(i);
		LastStart = FMath::Max(LastStart, T0);
		const float Local = (StateTime - T0) / FlyDur;
		if (Local < 0.f)
		{
			bAllLanded = false;
			continue;
		}
		const bool bBest = i == BestIndex;
		const float Scale = bBest ? 1.2f : 1.f;
		if (!MultiCoins[i])
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			MultiCoins[i] = GetWorld()->SpawnActor<AFortuneCoinPreview>(AFortuneCoinPreview::StaticClass(), FTransform(Start), P);
			if (MultiCoins[i]) MultiCoins[i]->SetCoin(WonCoins[i], true, false);
			InnerLight->SetLightColor(UFortuneSubsystem::GetRarityColor(WonCoins[i]->Rarity));
			if (bBest && (uint8)WonCoins[i]->Rarity >= (uint8)EFortuneRarity::Epic)
			{
				// En iyi coin: isik patlamasi + alev/altin halka
				BurstTime = StateTime;
				const float Pow = 1.5f + 0.35f * (float)(uint8)WonCoins[i]->Rarity;
				AFortuneLandFXActor::SpawnAt(GetWorld(), Start, EFortuneLandFX::GoldRing, Pow, true);
				if ((uint8)WonCoins[i]->Rarity >= (uint8)EFortuneRarity::Legendary)
				{
					AFortuneLandFXActor::SpawnAt(GetWorld(), Start, EFortuneLandFX::Flame, Pow, true);
				}
				if (RevealSound) UGameplayStatics::PlaySound2D(this, RevealSound);
			}
			else if (OpenSound && i % 3 == 0)
			{
				UGameplayStatics::PlaySound2D(this, OpenSound, 0.5f, 1.3f);
			}
		}
		if (!MultiCoins[i]) continue;
		if (MultiLanded[i]) continue;

		const float T = FMath::Clamp(Local, 0.f, 1.f);
		const float Ease = 1.f - FMath::Pow(1.f - T, 3.f);
		FVector Pos = FMath::Lerp(Start, MultiSlots[i], Ease);
		Pos += Up * FMath::Sin(T * PI) * 14.f;
		const float S = Scale * (FMath::Lerp(0.5f, 1.f, Ease) + 0.18f * FMath::Sin(T * PI));
		MultiCoins[i]->PlaceAt(Pos, CamLoc, Up, S);
		if (T >= 1.f)
		{
			MultiLanded[i] = true;
		}
		else
		{
			bAllLanded = false;
		}
	}

	// Sandik isigi: her firlayista parlar, en iyi coinde patlar
	float Intensity = 1800.f + 600.f * FMath::Sin(StateTime * 18.f);
	if (BurstTime >= 0.f)
	{
		const float B = FMath::Clamp(1.f - (StateTime - BurstTime) / 0.7f, 0.f, 1.f);
		Intensity += 9000.f * B;
		if (B > 0.f) InnerLight->SetLightColor(RarityColor);
	}
	InnerLight->SetIntensity(Intensity);

	if (bAllLanded && StateTime >= LastStart + FlyDur + 0.2f)
	{
		State = EState::Reveal;
		StateTime = 0.f;
		if (UFortuneGachaMultiRevealWidget* W = CreateWidget<UFortuneGachaMultiRevealWidget>(PC, UFortuneGachaMultiRevealWidget::StaticClass()))
		{
			TArray<UFortuneCoinData*> Coins;
			TArray<FVector> LabelPos;
			for (int32 i = 0; i < N; ++i)
			{
				Coins.Add(WonCoins[i]);
				LabelPos.Add(MultiSlots[i] - Up * ((i == BestIndex) ? 4.6f : 3.9f));
			}
			W->AddToViewport(20);
			W->Setup(this, Coins, WonDups, LabelPos, BestIndex, GetDefault<UFortuneSettings>()->DuplicateXP);
			W->SetKeyboardFocus();
			RevealWidget = W;
		}
	}
}
