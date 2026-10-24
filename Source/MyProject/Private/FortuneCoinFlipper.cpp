#include "FortuneCoinFlipper.h"
#include "FortuneSubsystem.h"
#include "FortuneCoinData.h"
#include "FortuneSettings.h"
#include "FortuneLandFX.h"
#include "FortuneHandActor.h"
#include "FortuneHeroActor.h"
#include "EngineUtils.h"
#include "FortuneCoinShape.h"
#include "FortuneCoinAura.h"
#include "ProceduralMeshComponent.h"
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

	ShapedCoin = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ShapedCoin"));
	ShapedCoin->SetupAttachment(CoinPivot);
	ShapedCoin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShapedCoin->SetVisibility(false);

	CoinAura = CreateDefaultSubobject<UFortuneCoinAuraComponent>(TEXT("CoinAura"));
	CoinAura->SetupAttachment(CoinPivot);

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
	if (!Hand)
	{
		for (TActorIterator<AFortuneHandActor> It(GetWorld()); It; ++It)
		{
			Hand = *It;
			break;
		}
	}
	if (!Hero)
	{
		for (TActorIterator<AFortuneHeroActor> It(GetWorld()); It; ++It)
		{
			Hero = *It;
			break;
		}
	}
	if (Hero)
	{
		HeroStartFrame = Hero->GetBodyFrame();   // coinin tasarlandigi govde yonu
	}
	if (Hero && Hand)
	{
		Hand->SetActorHiddenInGame(true);   // karakter varken eski yuzen el gizlenir
		Hand = nullptr;
	}
	if (HasHand())
	{
		HandCoin = EHandCoin::InHand;   // oyun basinda coin elde hazir
	}

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
	// Yuz resmi olan coin: sekilli model; yoksa eski silindir + gecici amblemler
	const bool bShaped = !Coin->CoinMesh && FFortuneCoinShapeBuilder::BuildForCoin(ShapedCoin, Coin, true, Owned.Enchant.IsActive());
	ShapedCoin->SetVisibility(bShaped);
	CoinMesh->SetVisibility(!bShaped);
	if (CoinAura)
	{
		CoinAura->SetAura(bShaped ? Coin->Aura : EFortuneCoinAura::None, Coin->AuraColor);
	}
	const bool bShowFaces = (Coin->CoinMesh == nullptr) && !bShaped;
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
	PreTime = 0.f;
	if (HasHand())
	{
		// Hizli atislarda el hareketleri de kisalir
		const float S = FMath::Clamp(Current.FlipDuration / 1.2f, 0.35f, 1.f);
		ReturnDur = (HandCoin == EHandCoin::InHand) ? 0.f : 0.22f * S;
		WindupDur = 0.16f * S;
		ReturnFrom = CoinPivot->GetComponentLocation();
		HandCoin = EHandCoin::Returning;
	}
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
		// El varsa: sonuc gorunduktan sonra coin ele doner ve elde bekler
		if (HasHand())
		{
			if (HandCoin == EHandCoin::OnTable)
			{
				HandTimer -= DeltaSeconds;
				if (HandTimer <= 0.f)
				{
					HandCoin = EHandCoin::Returning;
					HandTimer = 0.f;
					ReturnFrom = CoinPivot->GetComponentLocation();
				}
			}
			else if (HandCoin == EHandCoin::Returning)
			{
				HandTimer += DeltaSeconds;
				const float T = FMath::Clamp(HandTimer / 0.25f, 0.f, 1.f);
				const float E = T * T * (3.f - 2.f * T);
				const FVector P = FMath::Lerp(ReturnFrom, HandPoint(), E) + FVector(0.f, 0.f, FMath::Sin(T * PI) * 6.f);
				CoinPivot->SetWorldLocationAndRotation(P, FaceQuat(bRestIsHeads));
				if (T >= 1.f) HandCoin = EHandCoin::InHand;
				return;
			}
			else
			{
				PlaceCoinInHand();
				if (Hero && CatchTimer >= 0.f)
				{
					// Yakalama esnemesi: el asagi iner, sonra toparlanir
					CatchTimer += DeltaSeconds;
					const float C = CatchTimer < 0.08f ? 0.4f + 0.6f * CatchTimer / 0.08f : FMath::Max(0.f, 1.f - (CatchTimer - 0.08f) / 0.3f);
					Hero->SetCatch(C);
					if (CatchTimer > 0.4f) { CatchTimer = -1.f; Hero->SetCatch(0.f); }
				}
			}
			if (HandCoin != EHandCoin::InHand) return;   // AutoFlip coin elde olunca
		}

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

	// El: once coin ele doner, sonra basparmak gerilir
	if (HasHand() && PreTime < ReturnDur + WindupDur)
	{
		PreTime += DeltaSeconds;
		if (PreTime < ReturnDur)
		{
			const float T = PreTime / FMath::Max(0.01f, ReturnDur);
			const float E = T * T * (3.f - 2.f * T);
			const FVector P = FMath::Lerp(ReturnFrom, HandPoint(), E) + FVector(0.f, 0.f, FMath::Sin(T * PI) * 5.f);
			CoinPivot->SetWorldLocationAndRotation(P, FaceQuat(bRestIsHeads));
			HandPose(0.f, 0.f);
		}
		else
		{
			const float W = FMath::Clamp((PreTime - ReturnDur) / FMath::Max(0.01f, WindupDur), 0.f, 1.f);
			HandPose(W, 0.f);
			PlaceCoinInHand();
		}
		if (PreTime >= ReturnDur + WindupDur)
		{
			TossStart = HandPoint();
			HandCoin = EHandCoin::OnTable;
		}
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

	if (HasHand())
	{
		// Fiske: basparmak hizla firlar, sonra yavasca geri gelir
		const float Flick = (T < 0.08f) ? T / 0.08f : FMath::Max(0.f, 1.f - (T - 0.08f) / 0.35f);
		HandPose(0.f, Flick);

		const FTransform RW = RestWorld();
		// Karakter varsa coin yine avucuna duser (avuc hareket etse de takip eder)
		const FVector End = Hero ? HandPoint() : RW.GetLocation() + GetActorTransform().TransformVector(LandOffset);
		if (Hero)
		{
			Hero->SetLookTarget(CoinPivot->GetComponentLocation());
			Hero->SetCatch(T > 0.85f ? (T - 0.85f) / 0.15f * 0.4f : 0.f);   // yakalamaya hazirlanir
		}
		const FVector P = FMath::Lerp(TossStart, End, T) + FVector(0.f, 0.f, Height);
		CoinPivot->SetWorldLocationAndRotation(P, RestRotation() * FRotator(0.f, Yaw, Roll).Quaternion());
	}
	else
	{
		FTransform X = RestTransform;
		X.AddToTranslation(FVector(0.f, 0.f, Height) + LandOffset * T);
		X.SetRotation((RestTransform.GetRotation() * FRotator(0.f, Yaw, Roll).Quaternion()));
		CoinPivot->SetRelativeTransform(X);
	}

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
	if (Hero)
	{
		// Avuca duser: el coinle birlikte esner, sonuc avucta gorunur
		HandPose(0.f, 0.f);
		HandCoin = EHandCoin::InHand;
		CatchTimer = 0.f;
		Hero->SetLookTarget(FVector::ZeroVector);
		PlaceCoinInHand();
	}
	else
	{
		if (Hand)
		{
			HandPose(0.f, 0.f);
			HandCoin = EHandCoin::OnTable;
			HandTimer = ShowResultTime;
		}
		// Son pozu temizle: yazi = duz, tura = ters
		FTransform X = RestTransform;
		X.AddToTranslation(LandOffset);
		X.SetRotation(RestTransform.GetRotation() * FRotator(0.f, 0.f, Current.bHeads ? 0.f : 180.f).Quaternion());
		CoinPivot->SetRelativeTransform(X);
	}

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
		float Power = Current.bJackpot ? 2.2f : (Current.bCritical ? 1.6f : 1.f);
		if (Hero) Power *= 0.8f;   // avucta (kameraya yakin) biraz kucuk
		FLinearColor Tint = Coin ? (Coin->Aura != EFortuneCoinAura::None ? Coin->AuraColor : Coin->IconColor) : FLinearColor(1.f, 0.8f, 0.4f);
		// Ozel renkli patlama sadece Epic ve ustu coinlerde (digerleri kendi sade efektiyle)
		Tint.A = (Coin && (uint8)Coin->Rarity >= (uint8)EFortuneRarity::Epic) ? 1.f : 0.f;
		AFortuneLandFXActor::SpawnAt(GetWorld(), LandLoc, FXType, Power, Current.bHeads, Tint);
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

FTransform AFortuneCoinFlipper::RestWorld() const
{
	return RestTransform * GetActorTransform();
}

FQuat AFortuneCoinFlipper::FaceQuat(bool bHeads) const
{
	return RestRotation() * FRotator(0.f, 0.f, bHeads ? 0.f : 180.f).Quaternion();
}

FQuat AFortuneCoinFlipper::RestRotation() const
{
	if (Hero)
	{
		// Karakterin baslangic yonune gore fark kadar dondur: coin avucla birlikte kalir
		const FQuat Body = Hero->GetBodyFrame();
		const FQuat BodyStart = HeroStartFrame.IsIdentity() ? Body : HeroStartFrame;
		return Body * BodyStart.Inverse() * RestWorld().GetRotation();
	}
	return RestWorld().GetRotation();
}

void AFortuneCoinFlipper::PlaceCoinInHand()
{
	if (HasHand())
	{
		CoinPivot->SetWorldLocationAndRotation(HandPoint(), FaceQuat(bRestIsHeads));
	}
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

FVector AFortuneCoinFlipper::HandPoint() const
{
	if (Hero) return Hero->GetCoinPoint();
	if (Hand) return Hand->GetCoinPoint();
	return CoinPivot->GetComponentLocation();
}

void AFortuneCoinFlipper::HandPose(float InWindup, float InFlick)
{
	if (Hero) Hero->SetPose(InWindup, InFlick);
	else if (Hand) Hand->SetPose(InWindup, InFlick);
}
