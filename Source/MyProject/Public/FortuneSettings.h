// Flip for Fortune - Denge ayarlari: Project Settings > Game > Flip for Fortune
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FortuneTypes.h"
#include "FortuneSettings.generated.h"

class UFortuneCoinData;
class UNiagaraSystem;
class USoundBase;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Flip for Fortune"))
class MYPROJECT_API UFortuneSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFortuneSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// ---- Coinler ----
	/** Oyundaki tum coinler (Coin Collection ve gacha bu listeyi kullanir). */
	UPROPERTY(Config, EditAnywhere, Category = "Coins")
	TArray<TSoftObjectPtr<UFortuneCoinData>> Coins;

	/** Yeni oyunda verilen ilk coin. */
	UPROPERTY(Config, EditAnywhere, Category = "Coins")
	TSoftObjectPtr<UFortuneCoinData> StartingCoin;

	UPROPERTY(Config, EditAnywhere, Category = "Coins")
	double StartingMoney = 0.0;

	// ---- Atis ----
	UPROPERTY(Config, EditAnywhere, Category = "Flip", meta = (ClampMin = "0", ClampMax = "1"))
	float BaseHeadChance = 0.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Flip", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxHeadChance = 0.95f;

	/** Tura gelince degerin kaci verilir (0.5 = yarisi). */
	UPROPERTY(Config, EditAnywhere, Category = "Flip")
	float TailsValueMultiplier = 0.5f;

	/** CoinFlipSpeed yukseltmesi olmadan bir atisin suresi (sn). */
	UPROPERTY(Config, EditAnywhere, Category = "Flip", meta = (ClampMin = "0.1"))
	float BaseFlipDuration = 1.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Flip", meta = (ClampMin = "0.05"))
	float MinFlipDuration = 0.25f;

	// ---- Kritik ----
	UPROPERTY(Config, EditAnywhere, Category = "Critical", meta = (ClampMin = "0", ClampMax = "1"))
	float BaseCritChance = 0.05f;

	UPROPERTY(Config, EditAnywhere, Category = "Critical", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxCritChance = 0.75f;

	/** Kritik vurusun temel carpani (1.5x). CriticalMultiply yukseltmesi buna eklenir. */
	UPROPERTY(Config, EditAnywhere, Category = "Critical")
	float BaseCritMultiplier = 1.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Critical")
	TSoftObjectPtr<UNiagaraSystem> DefaultCritVFX;

	UPROPERTY(Config, EditAnywhere, Category = "Critical")
	TSoftObjectPtr<USoundBase> DefaultCritSound;

	// ---- Seviye ----
	/** Seviye 1 -> 2 icin gereken XP (her atis 1 XP). */
	UPROPERTY(Config, EditAnywhere, Category = "Level", meta = (ClampMin = "1"))
	int32 BaseXPPerLevel = 10;

	/** Her seviyede gereken XP bu oranla artar. */
	UPROPERTY(Config, EditAnywhere, Category = "Level", meta = (ClampMin = "1"))
	float XPGrowth = 1.15f;

	/** Coin kendi listesi bos birakilirsa kullanilan seviye animasyonlari. */
	UPROPERTY(Config, EditAnywhere, Category = "Level")
	TArray<FFortuneFlipAnimTier> DefaultFlipAnimTiers;

	// ---- Enchant ----
	/** Her atista Enchant Scroll dusme sansi (1/800). */
	UPROPERTY(Config, EditAnywhere, Category = "Enchant", meta = (ClampMin = "0", ClampMax = "1"))
	float EnchantScrollDropChance = 1.f / 800.f;

	/** Enchant'a basinca tier sanslari (Luck 0 iken). Kalan sans Tier 1'dir. */
	UPROPERTY(Config, EditAnywhere, Category = "Enchant", meta = (ClampMin = "0", ClampMax = "1"))
	float Tier2Chance = 0.25f;

	UPROPERTY(Config, EditAnywhere, Category = "Enchant", meta = (ClampMin = "0", ClampMax = "1"))
	float Tier3Chance = 0.05f;

	/** Her stat'in tier degerleri ve cikma agirligi. */
	UPROPERTY(Config, EditAnywhere, Category = "Enchant")
	TMap<EFortuneEnchantStat, FFortuneEnchantStatConfig> EnchantStats;

	/** Jackpot tetiklenince kazanc carpani. */
	UPROPERTY(Config, EditAnywhere, Category = "Enchant")
	float JackpotMultiplier = 10.f;

	/** Streak bonusunun en fazla kac yaziya kadar birikecegi. */
	UPROPERTY(Config, EditAnywhere, Category = "Enchant", meta = (ClampMin = "1"))
	int32 MaxStreakStacks = 10;

	/** Indirimlerin ust siniri (0.6 = en fazla %60). */
	UPROPERTY(Config, EditAnywhere, Category = "Enchant", meta = (ClampMin = "0", ClampMax = "0.95"))
	float MaxDiscount = 0.6f;

	UPROPERTY(Config, EditAnywhere, Category = "Enchant")
	TSoftObjectPtr<USoundBase> JackpotSound;

	UPROPERTY(Config, EditAnywhere, Category = "Enchant")
	TSoftObjectPtr<UNiagaraSystem> JackpotVFX;

	// ---- Gacha ----
	UPROPERTY(Config, EditAnywhere, Category = "Gacha")
	double SummonCost = 100.0;

	/** Zaten sahip olunan coin cikarsa o coine verilen XP. */
	UPROPERTY(Config, EditAnywhere, Category = "Gacha")
	int32 DuplicateXP = 25;

	// ---- Yukseltmeler ----
	UPROPERTY(Config, EditAnywhere, Category = "Upgrades")
	TMap<EFortuneUpgradeType, FFortuneUpgradeConfig> Upgrades;

	// ---- AutoFlip ----
	/** AutoFlip'in acilmasi icin gereken elle atis sayisi. */
	UPROPERTY(Config, EditAnywhere, Category = "AutoFlip", meta = (ClampMin = "0"))
	int32 AutoFlipRequiredFlips = 500;

	/** Iki otomatik atis arasindaki bekleme (sn). */
	UPROPERTY(Config, EditAnywhere, Category = "AutoFlip", meta = (ClampMin = "0"))
	float AutoFlipDelay = 0.35f;

	/** TEST: true iken AutoFlip baslangicta acik gelir. Oyun bitince false yap. */
	UPROPERTY(Config, EditAnywhere, Category = "AutoFlip")
	bool bUnlockAutoFlipForTesting = true;

	// ---- Kayit ----
	UPROPERTY(Config, EditAnywhere, Category = "Save")
	FString SaveSlotName = TEXT("FlipForFortune");
};
