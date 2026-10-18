// Flip for Fortune - Oyunun beyni: para, coinler, seviye, envanter, yukseltmeler, gacha, kayit.
// Blueprint'ten: Get Game Instance Subsystem (Fortune Subsystem)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FortuneTypes.h"
#include "FortuneSubsystem.generated.h"

class UFortuneCoinData;
class UFortuneSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFortuneMoneyChanged, double, NewMoney);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFortuneSimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFortuneFlipEvent, const FFortuneFlipResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFortuneLevelUpEvent, FName, CoinId, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFortuneSummonEvent, UFortuneCoinData*, Coin, bool, bDuplicate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFortuneEnchantEvent, FName, CoinId, const FFortuneEnchant&, Enchant);

UCLASS()
class MYPROJECT_API UFortuneSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---------------- Olaylar (UI bunlara baglanir) ----------------
	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneMoneyChanged OnMoneyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneSimpleEvent OnInventoryChanged;

	/** Coin seviyesi, enchant, kusanma veya yeni coin degisince. */
	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneSimpleEvent OnCoinsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneSimpleEvent OnUpgradesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneFlipEvent OnFlipResolved;

	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneSimpleEvent OnEnchantScrollDropped;

	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneLevelUpEvent OnCoinLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneSummonEvent OnCoinSummoned;

	/** AutoFlip kilidi acildiginda veya acilip kapandiginda. */
	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneSimpleEvent OnAutoFlipChanged;

	/** Enchant'a basilip yeni boost geldiginde (tier'a gore efekt/ses icin). */
	UPROPERTY(BlueprintAssignable, Category = "Fortune|Events")
	FFortuneEnchantEvent OnCoinEnchanted;

	// ---------------- Para ----------------
	UFUNCTION(BlueprintPure, Category = "Fortune|Money")
	double GetMoney() const;

	UFUNCTION(BlueprintCallable, Category = "Fortune|Money")
	void AddMoney(double Amount);

	// ---------------- Atis ----------------
	/** Kusanili coini atar: sonucu hesaplar, parayi/XP'yi/scroll'u uygular ve kaydeder. */
	UFUNCTION(BlueprintCallable, Category = "Fortune|Flip")
	FFortuneFlipResult FlipCoin(bool bManual = true);

	// ---------------- AutoFlip ----------------
	UFUNCTION(BlueprintPure, Category = "Fortune|AutoFlip")
	bool IsAutoFlipUnlocked() const;

	UFUNCTION(BlueprintPure, Category = "Fortune|AutoFlip")
	bool IsAutoFlipEnabled() const;

	/** Kilit ilerlemesi: OutCurrent / OutRequired elle atis. */
	UFUNCTION(BlueprintPure, Category = "Fortune|AutoFlip")
	void GetAutoFlipProgress(int32& OutCurrent, int32& OutRequired) const;

	UFUNCTION(BlueprintCallable, Category = "Fortune|AutoFlip")
	bool SetAutoFlipEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Fortune|AutoFlip")
	bool ToggleAutoFlip();

	UFUNCTION(BlueprintPure, Category = "Fortune|Flip")
	float GetHeadChance() const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Flip")
	float GetCritChance() const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Flip")
	float GetCritMultiplier() const;

	/** Seviye animasyonu haric temel atis suresi (CoinFlipSpeed'e gore). */
	UFUNCTION(BlueprintPure, Category = "Fortune|Flip")
	float GetBaseFlipDuration() const;

	// ---------------- Coinler ----------------
	UFUNCTION(BlueprintPure, Category = "Fortune|Coins")
	TArray<UFortuneCoinData*> GetAllCoins() const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Coins")
	UFortuneCoinData* FindCoinData(FName CoinId) const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Coins")
	bool IsCoinOwned(FName CoinId) const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Coins")
	bool GetOwnedCoin(FName CoinId, FFortuneOwnedCoin& OutCoin) const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Coins")
	FName GetEquippedCoinId() const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Coins")
	UFortuneCoinData* GetEquippedCoinData() const;

	UFUNCTION(BlueprintCallable, Category = "Fortune|Coins")
	bool EquipCoin(FName CoinId);

	/** Yazi gelince kazandiracagi deger (seviye + MoneyEarn + pasif + enchant, kritik haric). */
	UFUNCTION(BlueprintPure, Category = "Fortune|Coins")
	double GetCoinValue(FName CoinId) const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Coins")
	int32 GetXPRequiredForLevel(int32 Level) const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Coins")
	FFortuneFlipAnimTier GetFlipAnimTier(FName CoinId) const;

	/** Coin Collection sekmesi icin tum coinler (sahip olunmayanlar dahil). */
	UFUNCTION(BlueprintCallable, Category = "Fortune|Coins")
	TArray<FFortuneCollectionEntry> GetCollection() const;

	// ---------------- Envanter ----------------
	UFUNCTION(BlueprintPure, Category = "Fortune|Inventory")
	int32 GetItemCount(EFortuneItemType Item) const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Inventory")
	TMap<EFortuneItemType, int32> GetInventory() const;

	UFUNCTION(BlueprintCallable, Category = "Fortune|Inventory")
	void AddItem(EFortuneItemType Item, int32 Count = 1);

	/** Bir Enchant Scroll harcar ve coine rastgele boost + tier atar (eski enchant'in yerine gecer). */
	UFUNCTION(BlueprintCallable, Category = "Fortune|Enchant")
	bool UseEnchantScroll(FName CoinId, FFortuneEnchant& OutEnchant);

	/** Kusanili coinin enchant'indan gelen bonus (o stat degilse 0). */
	UFUNCTION(BlueprintPure, Category = "Fortune|Enchant")
	float GetEnchantBonus(EFortuneEnchantStat Stat) const;

	/** Genel sans: gacha, esya dusmesi ve enchant tier'ini etkiler. */
	UFUNCTION(BlueprintPure, Category = "Fortune|Enchant")
	float GetLuck() const;

	/** Su anki Luck ile Tier 2 ve Tier 3 gelme sansi. */
	UFUNCTION(BlueprintPure, Category = "Fortune|Enchant")
	void GetEnchantTierChances(float& OutTier1, float& OutTier2, float& OutTier3) const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Enchant")
	FFortuneEnchantStatConfig GetEnchantStatConfig(EFortuneEnchantStat Stat) const;

	/** UI icin hazir yazi: "Tier 3 - Money Earn +%60". */
	UFUNCTION(BlueprintPure, Category = "Fortune|Enchant")
	FText FormatEnchant(const FFortuneEnchant& Enchant) const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Flip")
	int32 GetHeadStreak() const { return HeadStreak; }

	// ---------------- Yukseltmeler ----------------
	UFUNCTION(BlueprintPure, Category = "Fortune|Upgrades")
	int32 GetUpgradeLevel(EFortuneUpgradeType Upgrade) const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Upgrades")
	FFortuneUpgradeConfig GetUpgradeConfig(EFortuneUpgradeType Upgrade) const;

	/** Mevcut seviyenin toplam bonusu. */
	UFUNCTION(BlueprintPure, Category = "Fortune|Upgrades")
	float GetUpgradeBonus(EFortuneUpgradeType Upgrade) const;

	/** Bir sonraki seviyenin fiyati (max ise -1). */
	UFUNCTION(BlueprintPure, Category = "Fortune|Upgrades")
	double GetUpgradeCost(EFortuneUpgradeType Upgrade) const;

	UFUNCTION(BlueprintCallable, Category = "Fortune|Upgrades")
	bool BuyUpgrade(EFortuneUpgradeType Upgrade);

	// ---------------- Gacha ----------------
	UFUNCTION(BlueprintPure, Category = "Fortune|Gacha")
	double GetSummonCost() const;

	UFUNCTION(BlueprintPure, Category = "Fortune|Gacha")
	static FLinearColor GetRarityColor(EFortuneRarity Rarity);

	/** Enchant boost'unun ne yaptigi (UI icin kisa aciklama). */
	UFUNCTION(BlueprintPure, Category = "Fortune|Enchant")
	static FString GetEnchantDescription(EFortuneEnchantStat Stat);

	UFUNCTION(BlueprintPure, Category = "Fortune|Gacha")
	static FString GetRarityName(EFortuneRarity Rarity);

	/** Bu coinin gacha'dan cikma olasiligi (0-1, Luck dahil). */
	UFUNCTION(BlueprintPure, Category = "Fortune|Gacha")
	float GetSummonChance(const UFortuneCoinData* Coin) const;

	/** Para harcayip agirliga gore rastgele coin cikarir. */
	UFUNCTION(BlueprintCallable, Category = "Fortune|Gacha")
	bool SummonCoin(UFortuneCoinData*& OutCoin, bool& bOutDuplicate);

	// ---------------- Kayit ----------------
	UFUNCTION(BlueprintCallable, Category = "Fortune|Save")
	void SaveProgress();

	UFUNCTION(BlueprintCallable, Category = "Fortune|Save")
	void ResetProgress();

private:
	void LoadCoins();
	void LoadOrCreateSave();
	void CreateNewSave();
	FFortuneOwnedCoin* FindOwned(FName CoinId);
	const FFortuneOwnedCoin* FindOwned(FName CoinId) const;
	float GetPassive(const UFortuneCoinData* Coin, EFortunePassiveType Type) const;
	bool AddXP(FFortuneOwnedCoin& Owned, const UFortuneCoinData* Coin, int32 Amount);
	bool GiveCoin(UFortuneCoinData* Coin);
	float GetCoinEnchantBonus(const FFortuneOwnedCoin* Owned, EFortuneEnchantStat Stat) const;
	FFortuneEnchant RollEnchant() const;
	static int32 RandomRound(float X);

	/** Art arda yazi sayisi (kayda yazilmaz). */
	int32 HeadStreak = 0;

	UPROPERTY()
	TArray<TObjectPtr<UFortuneCoinData>> LoadedCoins;

	UPROPERTY()
	TObjectPtr<UFortuneSaveGame> Save;
};
