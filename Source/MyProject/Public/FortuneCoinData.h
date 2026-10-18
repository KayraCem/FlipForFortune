// Flip for Fortune - Her coin icin bir Data Asset (Content Browser > Miscellaneous > Data Asset > FortuneCoinData)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FortuneTypes.h"
#include "FortuneCoinData.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UTexture2D;
class USoundBase;
class UNiagaraSystem;

UCLASS(BlueprintType)
class MYPROJECT_API UFortuneCoinData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Coinin kimligi = asset adi (ornek: DA_Coin_Gold). */
	UFUNCTION(BlueprintPure, Category = "Coin")
	FName GetCoinId() const { return GetFName(); }

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("FortuneCoin"), GetFName());
	}

	// ---- Tanitim ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FText DisplayName;

	/** Coin Collection'da gorunen tanitim yazisi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	EFortuneRarity Rarity = EFortuneRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Icon yoksa Coin Collection'da bu renkte daire gosterilir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FLinearColor IconColor = FLinearColor(0.9f, 0.7f, 0.3f);

	// ---- Gacha ----
	/** Gacha'da cikma agirligi (buyuk = daha sik). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gacha", meta = (ClampMin = "0"))
	float SummonWeight = 100.f;

	// ---- Deger ve seviye ----
	/** Seviye 1'deki yazi degeri. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Value")
	double BaseValue = 1.0;

	/** Her seviyede eklenen deger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Value")
	double ValuePerLevel = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Value", meta = (ClampMin = "1"))
	int32 MaxLevel = 100;

	// ---- Pasif ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	EFortunePassiveType PassiveType = EFortunePassiveType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive")
	float PassiveValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive", meta = (MultiLine = true))
	FText PassiveDescription;

	// ---- Gorunus ----
	/** Coin'in sekli (yuz dokulari varsa kullanilir). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	EFortuneCoinShape Shape = EFortuneCoinShape::Round;

	/** Yazi yuzu resmi. Doluysa coin bu sekil + resimlerle olusturulur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UTexture2D> HeadsTexture = nullptr;

	/** Tura yuzu resmi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UTexture2D> TailsTexture = nullptr;

	/** Kenar (yan yuz) rengi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor RimColor = FLinearColor(0.75f, 0.5f, 0.15f);

	/** Bos birakilirsa sahnedeki varsayilan coin mesh'i kullanilir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> CoinMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> CoinMaterial = nullptr;

	/** Enchant'liyken kullanilacak materyal (bos = normal materyal). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> EnchantedMaterial = nullptr;

	/** Enchant'liyken coinin uzerinde surekli oynayan efekt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UNiagaraSystem> EnchantVFX = nullptr;

	/** Coin masaya indiginde cikan hazir efekt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	EFortuneLandFX LandFX = EFortuneLandFX::Dust;

	/** Doluysa hazir efekt yerine bu Niagara efekti oynar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UNiagaraSystem> LandVFX = nullptr;

	/** Bu coine ozel kritik efekti (bos = genel kritik efekti). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UNiagaraSystem> CritVFX = nullptr;

	/** Seviyeye gore flip animasyonlari. Bos ise ayarlardaki varsayilan liste kullanilir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TArray<FFortuneFlipAnimTier> FlipAnimTiers;

	// ---- Ses ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> FlipSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> LandSound = nullptr;

	/** Bu coine ozel kritik sesi (bos = genel kritik sesi). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> CritSound = nullptr;
};
