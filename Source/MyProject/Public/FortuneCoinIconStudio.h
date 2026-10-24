// Flip for Fortune - Coin ikonlari: her coin oyunun basinda gorunmez bir "stüdyoda" 3D olarak (aurasiyla) cekilir.
// UI'da duz resim yerine coinin 3/4 acidan gercek gorunusu, arka plan seffaf.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneCoinIconStudio.generated.h"

class UFortuneCoinData;
class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UImage;
class AFortuneCoinPreview;

UCLASS(NotBlueprintable)
class MYPROJECT_API AFortuneCoinIconStudio : public AActor
{
	GENERATED_BODY()

public:
	AFortuneCoinIconStudio();
	virtual void Tick(float DeltaSeconds) override;

	/** Coinin ikonu (ilk istekte cekilir, birkac kare sonra dolar). */
	static UTextureRenderTarget2D* GetIcon(UObject* WorldContext, const UFortuneCoinData* Coin);

	/** Tum coinlerin ikonlarini simdiden cek (oyun basinda). */
	static void Prewarm(UObject* WorldContext);

	/** UImage'a coin ikonunu koy. bOwned false: karanlik siluet. */
	static void ApplyIcon(UImage* Image, UObject* WorldContext, const UFortuneCoinData* Coin, bool bOwned, float Size);

private:
	static AFortuneCoinIconStudio* Get(UObject* WorldContext);
	UTextureRenderTarget2D* Request(const UFortuneCoinData* Coin);

	struct FJob
	{
		TWeakObjectPtr<const UFortuneCoinData> Coin;
		TObjectPtr<UTextureRenderTarget2D> Target;
		TObjectPtr<AFortuneCoinPreview> Preview;
		TObjectPtr<USceneCaptureComponent2D> Capture;
		TObjectPtr<UPointLightComponent> Fill;
		float Wait = 0.f;
		int32 Slot = 0;
	};

	UPROPERTY()
	TMap<FName, TObjectPtr<UTextureRenderTarget2D>> Icons;

	UPROPERTY()
	TArray<TObjectPtr<UObject>> KeepAlive;

	UPROPERTY()
	TObjectPtr<class UMaterialInterface> IconMaterial;

	TArray<FJob> Jobs;
	TMap<FName, FVector2f> Calib;   // x = parlaklik carpani, y = alfa ters mi (1/0)
	TMap<FName, TArray<TWeakObjectPtr<class UMaterialInstanceDynamic>>> Pending;
	void Calibrate(const UFortuneCoinData* Coin, UTextureRenderTarget2D* RT);
	int32 NextSlot = 0;
};
