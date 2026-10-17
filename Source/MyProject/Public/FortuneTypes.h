// Flip for Fortune - ortak tipler (enum ve struct'lar)
#pragma once

#include "CoreMinimal.h"
#include "FortuneTypes.generated.h"

class UFortuneCoinData;
class UAnimMontage;

/** Coin nadirligi (gacha ve UI rengi icin). */
UENUM(BlueprintType)
enum class EFortuneRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary,
	Mythic
};

/** Oyuncunun parayla satin aldigi genel yukseltmeler. */
UENUM(BlueprintType)
enum class EFortuneUpgradeType : uint8
{
	CoinFlipSpeed,
	HeadChance,
	MoneyEarn,        // Seviyeden gelen degerin ustune duz (+) bonus
	CriticalChance,
	CriticalMultiply
};

/** Envanterdeki esya turleri. */
UENUM(BlueprintType)
enum class EFortuneItemType : uint8
{
	EnchantScroll
};

/** Her coinin tek bir benzersiz pasifi olur; PassiveValue ile ne kadar etkiledigi belirlenir. */
UENUM(BlueprintType)
enum class EFortunePassiveType : uint8
{
	None,
	BonusHeadChance,     // +PassiveValue yazi sansi (0.05 = +%5)
	BonusCritChance,     // +PassiveValue kritik sansi
	BonusCritMultiply,   // +PassiveValue kritik carpani
	FlatMoneyBonus,      // +PassiveValue para (duz)
	MoneyMultiplier,     // x(1 + PassiveValue) para
	TailsFullValue,      // Tura da tam deger verir
	BonusScrollChance,   // Scroll dusme sansi x(1 + PassiveValue)
	BonusXP              // Atis basina +PassiveValue ekstra XP
};

/** Coin masaya indiginde cikan hazir efekt. */
UENUM(BlueprintType)
enum class EFortuneLandFX : uint8
{
	None,
	Dust,      // toz bulutu
	Sparks,    // beyaz kivilcim
	GoldRing,  // altin halka dalgasi + parilti
	Leaves,    // yesil yapraklar
	Flame      // alev halkasi + korlar
};

/** Enchant Scroll ile coinlere gelen boost turleri. */
UENUM(BlueprintType)
enum class EFortuneEnchantStat : uint8
{
	CoinFlipSpeed,     // Atis hizi +%
	MoneyEarn,         // Kazanc +%
	Luck,              // Genel sans: gacha, esya dusmesi, enchant tier'i
	HeadChance,        // Yazi sansi +
	CritChance,        // Kritik sansi +
	CriticalMultiply,  // Kritik carpani +
	XPBoost,           // XP +%
	TailsBoost,        // Turanin verdigi oran +
	DoubleFlip,        // Sansla bir atis 2 kez para verir
	Jackpot,           // Cok dusuk sansla dev kazanc
	Streak,            // Art arda yazi geldikce artan bonus
	SummonDiscount,    // Gacha indirimi
	UpgradeDiscount    // Yukseltme indirimi
};

/** Bir coinin uzerindeki enchant. Tier 0 = enchant yok, 1-3 = tier (3 en guclu ve en nadir). */
USTRUCT(BlueprintType)
struct FFortuneEnchant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Enchant")
	EFortuneEnchantStat Stat = EFortuneEnchantStat::MoneyEarn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Enchant", meta = (ClampMin = "0", ClampMax = "3"))
	int32 Tier = 0;

	/** Boost'un degeri (ornek: 0.2 = %20). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Enchant")
	float Value = 0.f;

	bool IsActive() const { return Tier > 0; }
};

/** Bir enchant stat'inin her tier'daki degeri. */
USTRUCT(BlueprintType)
struct FFortuneEnchantStatConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enchant")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enchant")
	float Tier1Value = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enchant")
	float Tier2Value = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enchant")
	float Tier3Value = 0.4f;

	/** true ise UI'da yuzde olarak yazilir (0.2 -> %20). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enchant")
	bool bPercent = true;

	/** Havuzda cikma agirligi (0 = hic cikmaz). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enchant", meta = (ClampMin = "0"))
	float RollWeight = 100.f;

	float GetValue(int32 Tier) const { return Tier >= 3 ? Tier3Value : (Tier == 2 ? Tier2Value : Tier1Value); }
};

/** Coin seviyesine gore acilan flip animasyonu. */
USTRUCT(BlueprintType)
struct FFortuneFlipAnimTier
{
	GENERATED_BODY()

	/** Bu animasyonun acildigi minimum coin seviyesi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flip", meta = (ClampMin = "1"))
	int32 MinLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flip")
	FText DisplayName;

	/** Coinin havaya cikacagi yukseklik (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flip")
	float TossHeight = 40.f;

	/** Havadayken kac tam tur doner (takla). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flip", meta = (ClampMin = "1"))
	int32 Spins = 4;

	/** Dikey eksende ekstra donus (havali animasyonlar icin). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flip")
	int32 YawSpins = 0;

	/** Sure carpani (1 = normal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flip", meta = (ClampMin = "0.1"))
	float DurationScale = 1.f;

	/** Istege bagli: elin bu seviyede oynatacagi montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flip")
	TObjectPtr<UAnimMontage> HandMontage = nullptr;
};

/** Oyuncunun sahip oldugu bir coinin kayitli durumu. */
USTRUCT(BlueprintType)
struct FFortuneOwnedCoin
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Coin")
	FName CoinId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Coin")
	int32 Level = 1;

	/** Mevcut seviyedeki XP (kullandikca artar). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Coin")
	int32 XP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Coin")
	int64 TotalFlips = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Coin")
	FFortuneEnchant Enchant;
};

/** Bir yukseltmenin denge ayarlari. */
USTRUCT(BlueprintType)
struct FFortuneUpgradeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "1"))
	int32 MaxLevel = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	double BaseCost = 10.0;

	/** Her seviyede maliyet bu oranla carpilir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	double CostGrowth = 1.15;

	/** Her seviyenin verdigi bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	float ValuePerLevel = 0.01f;
};

/** Bir atisin sonucu. */
USTRUCT(BlueprintType)
struct FFortuneFlipResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	FName CoinId;

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	bool bHeads = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	bool bCritical = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	double MoneyEarned = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	bool bScrollDropped = false;

	/** DoubleFlip enchant'i tetiklendi (para x2). */
	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	bool bDoubleFlip = false;

	/** Jackpot enchant'i tetiklendi. */
	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	bool bJackpot = false;

	/** Su anki art arda yazi serisi. */
	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	int32 HeadStreak = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	int32 XPGained = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	bool bLeveledUp = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	int32 NewLevel = 1;

	/** Atis animasyonunun suresi (CoinFlipSpeed ve animasyon katmanina gore). */
	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	float FlipDuration = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Flip")
	FFortuneFlipAnimTier AnimTier;
};

/** Coin Collection ekrani icin tek satir. */
USTRUCT(BlueprintType)
struct FFortuneCollectionEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	TObjectPtr<UFortuneCoinData> Coin = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	bool bOwned = false;

	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	bool bEquipped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	int32 Level = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	int32 XP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	int32 XPForNextLevel = 0;

	/** Yazi gelince kazandiracagi deger (kritik haric). */
	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	double CurrentValue = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	bool bEnchanted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	FFortuneEnchant Enchant;

	/** Hazir yazi: "Tier 3 - Money Earn +%60" */
	UPROPERTY(BlueprintReadOnly, Category = "Collection")
	FText EnchantText;
};
