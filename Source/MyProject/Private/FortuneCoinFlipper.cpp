#include "FortuneCoinFlipper.h"
#include "FortuneSubsystem.h"
#include "FortuneCoinData.h"
#include "FortuneSettings.h"
#include "FortuneLandFX.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFortuneCoinFlipper::AFortuneCoinFlipper()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CoinPivot = CreateDefaultSubobject<USceneComponent>(TEXT("CoinPivot"));
	CoinPivot->SetupAttachment(Root);
	CoinPivot->SetRelativeLocation(FVector(0.f, 0.f, 0.3f)); // coin kalinliginin yarisi: masaya oturur

	CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
	CoinMesh->SetupAttachment(CoinPivot);
	CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Varsayilan coin: motorun silindiri, cap 6 cm, kalinlik 0.6 cm
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	ShapeMaterial = ShapeMat.Object;
	if (Cyl.Succeeded())
	{
		CoinMesh->SetStaticMesh(Cyl.Object);
		CoinMesh->SetRelativeScale3D(FVector(0.06f, 0.06f, 0.006f));
	}

	auto MakeFace = [this](const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(CoinPivot);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetStaticMesh(Mesh);
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Scale);
		C->SetCastShadow(false);
		return C;
	};
	// Yazi: ustte buyuk parlak daire + ortada kubbe. Tura: altta koyu daire + halka.
	HeadsFace = MakeFace(TEXT("HeadsFace"), Cyl.Object, FVector(0.f, 0.f, 0.32f), FVector(0.042f, 0.042f, 0.0008f));
	HeadsGem  = MakeFace(TEXT("HeadsGem"),  Sph.Object, FVector(0.f, 0.f, 0.36f), FVector(0.016f, 0.016f, 0.004f));
	TailsFace = MakeFace(TEXT("TailsFace"), Cyl.Object, FVector(0.f, 0.f, -0.32f), FVector(0.046f, 0.046f, 0.0008f));
	TailsRing = MakeFace(TEXT("TailsRing"), Cyl.Object, FVector(0.f, 0.f, -0.34f), FVector(0.022f, 0.022f, 0.0008f));

	EnchantFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EnchantFX"));
	EnchantFX->SetupAttachment(CoinPivot);
	EnchantFX->SetAutoActivate(false);

	HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	HandMesh->SetupAttachment(Root);
	HandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UFortuneSubsystem* AFortuneCoinFlipper::GetFortune() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
}

void AFortuneCoinFlipper::BeginPlay()
{
	Super::BeginPlay();
	RestTransform = CoinPivot->GetRelativeTransform();

	// Yuz renkleri (motorun BasicShapeMaterial'inin "Color" parametresi)
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
	Tint(HeadsFace, HeadsColor);
	Tint(HeadsGem, FLinearColor(1.f, 1.f, 0.85f));
	Tint(TailsFace, TailsColor);
	Tint(TailsRing, FLinearColor(0.6f, 0.35f, 0.12f));
	if (UFortuneSubsystem* F = GetFortune())
	{
		F->OnCoinsChanged.AddDynamic(this, &AFortuneCoinFlipper::HandleCoinsChanged);
	}
	RefreshCoinVisuals();
}

void AFortuneCoinFlipper::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UFortuneSubsystem* F = GetFortune())
	{
		F->OnCoinsChanged.RemoveDynamic(this, &AFortuneCoinFlipper::HandleCoinsChanged);
	}
	Super::EndPlay(Reason);
}

void AFortuneCoinFlipper::HandleCoinsChanged()
{
	if (!bIsFlipping)
	{
		RefreshCoinVisuals();
	}
}

void AFortuneCoinFlipper::RefreshCoinVisuals()
{
	UFortuneSubsystem* F = GetFortune();
	if (!F) return;
	const UFortuneCoinData* Coin = F->GetEquippedCoinData();
	if (!Coin) return;

	FFortuneOwnedCoin Owned;
	F->GetOwnedCoin(Coin->GetCoinId(), Owned);

	if (Coin->CoinMesh)
	{
		CoinMesh->SetStaticMesh(Coin->CoinMesh);
	}
	// Kendi mesh'i olan coinlerde gecici yuz amblemlerini gizle
	const bool bShowFaces = (Coin->CoinMesh == nullptr);
	HeadsFace->SetVisibility(bShowFaces);
	HeadsGem->SetVisibility(bShowFaces);
	TailsFace->SetVisibility(bShowFaces);
	TailsRing->SetVisibility(bShowFaces);
	UMaterialInterface* Mat = (Owned.Enchant.IsActive() && Coin->EnchantedMaterial) ? Coin->EnchantedMaterial.Get() : Coin->CoinMaterial.Get();
	if (Mat)
	{
		CoinMesh->SetMaterial(0, Mat);
	}

	if (Owned.Enchant.IsActive() && Coin->EnchantVFX)
	{
		EnchantFX->SetAsset(Coin->EnchantVFX);
		EnchantFX->Activate(true);
	}
	else
	{
		EnchantFX->Deactivate();
	}
}

bool AFortuneCoinFlipper::Flip(bool bManual)
{
	if (bIsFlipping) return false;
	UFortuneSubsystem* F = GetFortune();
	if (!F) return false;

	bIsFlipping = true;   // FlipCoin olaylari gorunumu ortasinda yenilemesin
	Current = F->FlipCoin(bManual);
	if (!Current.bSuccess)
	{
		bIsFlipping = false;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Red, TEXT("Coin atilamadi: kusanili coin yok (Project Settings > Flip for Fortune > Coins)"));
		}
		return false;
	}
	Elapsed = 0.f;
	Delay = 0.f;
	LandOffset = bLandInPlace ? FVector::ZeroVector : FVector(FMath::FRandRange(-4.f, 4.f), FMath::FRandRange(-4.f, 4.f), 0.f);

	// El animasyonu (varsa)
	if (UAnimMontage* Montage = Current.AnimTier.HandMontage)
	{
		if (UAnimInstance* Anim = HandMesh->GetAnimInstance())
		{
			Anim->Montage_Play(Montage);
			Delay = HandReleaseDelay;
		}
	}

	const UFortuneCoinData* Coin = F->FindCoinData(Current.CoinId);
	if (Coin && Coin->FlipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Coin->FlipSound, CoinPivot->GetComponentLocation());
	}

	OnFlipStarted(Current);
	return true;
}

void AFortuneCoinFlipper::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bIsFlipping)
	{
		// AutoFlip: coin indikten kisa sure sonra kendiliginden at
		const UFortuneSubsystem* F = GetFortune();
		if (F && F->IsAutoFlipEnabled())
		{
			AutoTimer -= DeltaSeconds;
			if (AutoTimer <= 0.f && !Flip(false))
			{
				AutoTimer = 1.f;   // atilamadiysa her karede denemesin
			}
		}
		return;
	}

	if (Delay > 0.f)
	{
		Delay -= DeltaSeconds;
		return;
	}

	Elapsed += DeltaSeconds;
	const float Dur = FMath::Max(0.05f, Current.FlipDuration);
	const float T = FMath::Clamp(Elapsed / Dur, 0.f, 1.f);

	const FFortuneFlipAnimTier& Tier = Current.AnimTier;

	// Yukseklik: parabol (0 -> tepe -> 0)
	const float Height = 4.f * Tier.TossHeight * T * (1.f - T);

	// Takla: baslangic yuzunden bitis yuzune tam Spins tur + gerekirse yarim tur
	const float StartRoll = bRestIsHeads ? 0.f : 180.f;
	const float EndRoll = Tier.Spins * 360.f + (Current.bHeads ? 0.f : 180.f);
	const float Ease = 1.f - FMath::Pow(1.f - T, 2.f);      // havada hizli baslayip yavaslar
	const float Roll = FMath::Lerp(StartRoll, EndRoll, Ease);
	const float Yaw = Tier.YawSpins * 360.f * Ease;

	FTransform X = RestTransform;
	X.AddToTranslation(FVector(0.f, 0.f, Height) + LandOffset * T);
	X.SetRotation((RestTransform.GetRotation() * FRotator(0.f, Yaw, Roll).Quaternion()));
	CoinPivot->SetRelativeTransform(X);

	if (T >= 1.f)
	{
		Land();
	}
}

void AFortuneCoinFlipper::Land()
{
	bIsFlipping = false;
	bRestIsHeads = Current.bHeads;
	AutoTimer = GetDefault<UFortuneSettings>()->AutoFlipDelay;

	// Son pozu temizle: yazi = duz, tura = ters
	FTransform X = RestTransform;
	X.AddToTranslation(LandOffset);
	X.SetRotation(RestTransform.GetRotation() * FRotator(0.f, 0.f, Current.bHeads ? 0.f : 180.f).Quaternion());
	CoinPivot->SetRelativeTransform(X);

	UFortuneSubsystem* F = GetFortune();
	const UFortuneCoinData* Coin = F ? F->FindCoinData(Current.CoinId) : nullptr;
	if (Coin && Coin->LandSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Coin->LandSound, CoinPivot->GetComponentLocation());
	}

	// Masaya carpma efekti: Niagara atanmissa o, yoksa coinin hazir efekti (kritikte daha buyuk)
	const FVector LandLoc = CoinPivot->GetComponentLocation();
	if (Coin && Coin->LandVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Coin->LandVFX, LandLoc);
	}
	else
	{
		const EFortuneLandFX FXType = Coin ? Coin->LandFX : EFortuneLandFX::Dust;
		const float Power = Current.bJackpot ? 2.2f : (Current.bCritical ? 1.6f : 1.f);
		AFortuneLandFX::SpawnAt(GetWorld(), LandLoc, FXType, Power, Current.bHeads);
	}

	if (Current.bCritical)
	{
		PlayCriticalEffects();
		OnCriticalHit(Current);
	}

	if (Current.bJackpot)
	{
		const UFortuneSettings* S = GetDefault<UFortuneSettings>();
		const FVector Loc = CoinPivot->GetComponentLocation();
		if (UNiagaraSystem* FX = S->JackpotVFX.LoadSynchronous())
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FX, Loc);
		}
		if (USoundBase* Snd = S->JackpotSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySoundAtLocation(this, Snd, Loc);
		}
		OnJackpot(Current);
	}

	OnFlipLanded(Current);

	// Seviye atladiysa gorunum degismis olabilir
	RefreshCoinVisuals();
}

void AFortuneCoinFlipper::PlayCriticalEffects()
{
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	UFortuneSubsystem* F = GetFortune();
	const UFortuneCoinData* Coin = F ? F->FindCoinData(Current.CoinId) : nullptr;
	const FVector Loc = CoinPivot->GetComponentLocation();

	UNiagaraSystem* FX = (Coin && Coin->CritVFX) ? Coin->CritVFX.Get() : S->DefaultCritVFX.LoadSynchronous();
	if (FX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FX, Loc);
	}

	USoundBase* Snd = (Coin && Coin->CritSound) ? Coin->CritSound.Get() : S->DefaultCritSound.LoadSynchronous();
	if (Snd)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Snd, Loc);
	}
}
