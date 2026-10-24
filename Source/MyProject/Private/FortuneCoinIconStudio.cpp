#include "FortuneCoinIconStudio.h"
#include "FortuneCoinPreview.h"
#include "FortuneCoinData.h"
#include "FortuneSubsystem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TextureResource.h"

namespace
{
	const FVector StudioOrigin(0.f, 0.f, -40000.f);   // oyuncunun hic gormedigi yer
	constexpr float IconCamDist = 19.f;   // coin cerceveyi doldursun
	constexpr int32 IconRes = 256;
}

AFortuneCoinIconStudio::AFortuneCoinIconStudio()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

AFortuneCoinIconStudio* AFortuneCoinIconStudio::Get(UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World) return nullptr;
	for (TActorIterator<AFortuneCoinIconStudio> It(World); It; ++It)
	{
		return *It;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFortuneCoinIconStudio* S = World->SpawnActor<AFortuneCoinIconStudio>(AFortuneCoinIconStudio::StaticClass(), FTransform(StudioOrigin), P);
	if (S)
	{
		S->IconMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_UI_CoinIcon.M_UI_CoinIcon"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	return S;
}

UTextureRenderTarget2D* AFortuneCoinIconStudio::GetIcon(UObject* WorldContext, const UFortuneCoinData* Coin)
{
	AFortuneCoinIconStudio* S = Get(WorldContext);
	return S ? S->Request(Coin) : nullptr;
}

void AFortuneCoinIconStudio::Prewarm(UObject* WorldContext)
{
	AFortuneCoinIconStudio* S = Get(WorldContext);
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UFortuneSubsystem* F = GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!S || !F) return;
	for (UFortuneCoinData* C : F->GetAllCoins())
	{
		S->Request(C);
	}
}

UTextureRenderTarget2D* AFortuneCoinIconStudio::Request(const UFortuneCoinData* Coin)
{
	if (!Coin) return nullptr;
	if (TObjectPtr<UTextureRenderTarget2D>* Found = Icons.Find(Coin->GetCoinId()))
	{
		return Found->Get();
	}
	UTextureRenderTarget2D* RT = UKismetRenderingLibrary::CreateRenderTarget2D(this, IconRes, IconRes, RTF_RGBA16f, FLinearColor(0.f, 0.f, 0.f, 1.f), false);
	if (!RT) return nullptr;
	Icons.Add(Coin->GetCoinId(), RT);

	// Her coin kendi kosesinde: coin + kamera + dolgu isigi
	const int32 Slot = NextSlot++;
	const FVector Base = StudioOrigin + FVector((Slot % 8) * 300.f, (Slot / 8) * 300.f, 0.f);
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFortuneCoinPreview* Prev = GetWorld()->SpawnActor<AFortuneCoinPreview>(AFortuneCoinPreview::StaticClass(), FTransform(Base), P);
	if (!Prev) return RT;
	Prev->SetCoin(Coin, true, false);
	const FVector CamLoc = Base - FVector(IconCamDist, 0.f, 0.f);
	Prev->PlaceAt(Base, CamLoc, FVector::UpVector, 1.f);
	Prev->SetFixedPose(-32.f);   // 3/4 aci: kenar ve kalinlik gorunsun

	USceneCaptureComponent2D* Cap = NewObject<USceneCaptureComponent2D>(this);
	Cap->SetupAttachment(GetRootComponent());
	Cap->RegisterComponent();
	Cap->SetWorldLocationAndRotation(CamLoc, FRotator::ZeroRotator);
	Cap->FOVAngle = 26.f;
	Cap->TextureTarget = RT;
	Cap->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	Cap->bCaptureEveryFrame = false;
	Cap->bCaptureOnMovement = false;
	Cap->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Cap->ShowOnlyActors.Add(Prev);
	Cap->ShowFlags.SetFog(false);
	Cap->ShowFlags.SetAtmosphere(false);
	Cap->ShowFlags.SetBloom(false);
	Cap->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Cap->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Cap->PostProcessSettings.bOverride_AutoExposureBias = true;
	Cap->PostProcessSettings.AutoExposureBias = 0.f;
	Cap->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Cap->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;   // ham parlaklik; UI materyali tonlar (Exposure)

	UPointLightComponent* Fill = NewObject<UPointLightComponent>(this);
	Fill->SetupAttachment(GetRootComponent());
	Fill->RegisterComponent();
	Fill->SetWorldLocation(Base + FVector(-20.f, 18.f, 16.f));
	Fill->SetIntensity(900.f);
	Fill->SetAttenuationRadius(80.f);
	Fill->SetCastShadows(false);

	FJob J;
	J.Coin = Coin;
	J.Target = RT;
	J.Preview = Prev;
	J.Capture = Cap;
	J.Fill = Fill;
	J.Wait = 0.35f + 0.02f * Jobs.Num();   // aura bir kac kare otursun
	J.Slot = Slot;
	Jobs.Add(J);
	KeepAlive.Add(Prev);
	KeepAlive.Add(Cap);
	KeepAlive.Add(Fill);
	return RT;
}

void AFortuneCoinIconStudio::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	for (int32 i = Jobs.Num() - 1; i >= 0; --i)
	{
		FJob& J = Jobs[i];
		J.Wait -= DeltaSeconds;
		if (J.Wait > 0.f) continue;
		if (J.Capture) J.Capture->CaptureScene();
		if (J.Coin.IsValid()) Calibrate(J.Coin.Get(), J.Target);
		// Cekildi: sahneyi temizle (ikon render target'ta kalir)
		if (J.Preview) { KeepAlive.Remove(J.Preview); J.Preview->Destroy(); }
		if (J.Capture) { KeepAlive.Remove(J.Capture); J.Capture->DestroyComponent(); }
		if (J.Fill) { KeepAlive.Remove(J.Fill); J.Fill->DestroyComponent(); }
		Jobs.RemoveAt(i);
	}
}

// Cekilen goruntuden parlaklik ve seffaflik yonunu olc (isik birimleri sahneye gore degisebilir)
void AFortuneCoinIconStudio::Calibrate(const UFortuneCoinData* Coin, UTextureRenderTarget2D* RT)
{
	FVector2f C(1.f, 1.f);
	FTextureRenderTargetResource* Res = RT ? RT->GameThread_GetRenderTargetResource() : nullptr;
	TArray<FLinearColor> Px;
	if (Res && Res->ReadLinearColorPixels(Px, FReadSurfaceDataFlags(RCM_MinMax)) && Px.Num() > 0)
	{
		const bool bInvert = Px[0].A > 0.5f;   // kose = arka plan
		TArray<float> Lum;
		for (const FLinearColor& P : Px)
		{
			const bool bCoin = bInvert ? (P.A < 0.5f) : (P.A > 0.5f);
			if (bCoin) Lum.Add(FMath::Max3(P.R, P.G, P.B));
		}
		float Ref = 1.f;
		Lum.Sort();
		if (Lum.Num() > 20)
		{
			Ref = FMath::Max(Lum[FMath::Clamp(int32(Lum.Num() * 0.92f), 0, Lum.Num() - 1)], 1e-5f);
		}
		const float Med = FMath::Max(Lum.Num() > 0 ? Lum[Lum.Num() / 2] : Ref, 1e-5f);
		C = FVector2f(FMath::Min(0.5f / Med, 1.1f / Ref), bInvert ? 1.f : 0.f);   // beyaza patlamasin
	}
	Calib.Add(Coin->GetCoinId(), C);
	if (TArray<TWeakObjectPtr<UMaterialInstanceDynamic>>* L = Pending.Find(Coin->GetCoinId()))
	{
		for (const TWeakObjectPtr<UMaterialInstanceDynamic>& M : *L)
		{
			if (M.IsValid())
			{
				M->SetScalarParameterValue(TEXT("Exposure"), C.X);
				M->SetScalarParameterValue(TEXT("Invert"), C.Y);
			}
		}
		Pending.Remove(Coin->GetCoinId());
	}
}

void AFortuneCoinIconStudio::ApplyIcon(UImage* Image, UObject* WorldContext, const UFortuneCoinData* Coin, bool bOwned, float Size)
{
	if (!Image || !Coin) return;
	AFortuneCoinIconStudio* S = Get(WorldContext);
	UTextureRenderTarget2D* RT = S ? S->Request(Coin) : nullptr;
	if (!RT) return;
	const FLinearColor Tint = bOwned ? FLinearColor::White : FLinearColor(0.02f, 0.02f, 0.03f, 1.f);
	if (S->IconMaterial)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(S->IconMaterial, Image);
		MID->SetTextureParameterValue(TEXT("Icon"), RT);
		MID->SetVectorParameterValue(TEXT("Tint"), Tint);
		if (const FVector2f* C = S->Calib.Find(Coin->GetCoinId()))
		{
			MID->SetScalarParameterValue(TEXT("Exposure"), C->X);
			MID->SetScalarParameterValue(TEXT("Invert"), C->Y);
		}
		else
		{
			S->Pending.FindOrAdd(Coin->GetCoinId()).Add(MID);   // cekim bitince ayarlanir
		}
		FSlateBrush Br;
		Br.SetResourceObject(MID);
		Br.ImageSize = FVector2f(Size, Size);
		Image->SetBrush(Br);
		Image->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		FSlateBrush Br;
		Br.SetResourceObject(RT);
		Br.ImageSize = FVector2f(Size, Size);
		Image->SetBrush(Br);
		Image->SetColorAndOpacity(Tint);
	}
}
