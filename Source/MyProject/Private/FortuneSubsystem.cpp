#include "FortuneSubsystem.h"
#include "FortuneCoinData.h"
#include "FortuneSaveGame.h"
#include "FortuneSettings.h"
#include "Kismet/GameplayStatics.h"

void UFortuneSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadCoins();
	LoadOrCreateSave();
}

void UFortuneSubsystem::Deinitialize()
{
	SaveProgress();
	Super::Deinitialize();
}

void UFortuneSubsystem::LoadCoins()
{
	LoadedCoins.Reset();
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	for (const TSoftObjectPtr<UFortuneCoinData>& Soft : S->Coins)
	{
		if (UFortuneCoinData* Coin = Soft.LoadSynchronous())
		{
			LoadedCoins.AddUnique(Coin);
		}
	}
	if (UFortuneCoinData* Start = S->StartingCoin.LoadSynchronous())
	{
		LoadedCoins.AddUnique(Start);
	}
}

void UFortuneSubsystem::LoadOrCreateSave()
{
	const FString& Slot = GetDefault<UFortuneSettings>()->SaveSlotName;
	if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		Save = Cast<UFortuneSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	}
	if (!Save)
	{
		CreateNewSave();
	}
	// Kayitta hic coin yoksa (coinler eklenmeden once olusmus kayit) baslangic coinini ver
	if (Save->OwnedCoins.Num() == 0)
	{
		UFortuneCoinData* Start = GetDefault<UFortuneSettings>()->StartingCoin.LoadSynchronous();
		if (!Start && LoadedCoins.Num() > 0)
		{
			Start = LoadedCoins[0];
		}
		if (Start)
		{
			GiveCoin(Start);
			Save->EquippedCoin = Start->GetCoinId();
		}
	}
	// Kusanili coin yoksa ilk sahip olunani kusan
	if (!FindOwned(Save->EquippedCoin) && Save->OwnedCoins.Num() > 0)
	{
		Save->EquippedCoin = Save->OwnedCoins[0].CoinId;
	}
}

void UFortuneSubsystem::CreateNewSave()
{
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	Save = Cast<UFortuneSaveGame>(UGameplayStatics::CreateSaveGameObject(UFortuneSaveGame::StaticClass()));
	Save->Money = S->StartingMoney;
	UFortuneCoinData* Start = S->StartingCoin.LoadSynchronous();
	if (!Start && LoadedCoins.Num() > 0)
	{
		Start = LoadedCoins[0];
	}
	if (Start)
	{
		GiveCoin(Start);
		Save->EquippedCoin = Start->GetCoinId();
	}
}

void UFortuneSubsystem::SaveProgress()
{
	if (Save)
	{
		UGameplayStatics::SaveGameToSlot(Save, GetDefault<UFortuneSettings>()->SaveSlotName, 0);
	}
}

void UFortuneSubsystem::ResetProgress()
{
	UGameplayStatics::DeleteGameInSlot(GetDefault<UFortuneSettings>()->SaveSlotName, 0);
	CreateNewSave();
	SaveProgress();
	OnMoneyChanged.Broadcast(Save->Money);
	OnInventoryChanged.Broadcast();
	OnCoinsChanged.Broadcast();
	OnUpgradesChanged.Broadcast();
}

// ---------------- Para ----------------

double UFortuneSubsystem::GetMoney() const
{
	return Save ? Save->Money : 0.0;
}

void UFortuneSubsystem::AddMoney(double Amount)
{
	if (!Save) return;
	Save->Money = FMath::Max(0.0, Save->Money + Amount);
	OnMoneyChanged.Broadcast(Save->Money);
}

// ---------------- Yardimcilar ----------------

FFortuneOwnedCoin* UFortuneSubsystem::FindOwned(FName CoinId)
{
	if (!Save || CoinId.IsNone()) return nullptr;
	return Save->OwnedCoins.FindByPredicate([CoinId](const FFortuneOwnedCoin& C) { return C.CoinId == CoinId; });
}

const FFortuneOwnedCoin* UFortuneSubsystem::FindOwned(FName CoinId) const
{
	if (!Save || CoinId.IsNone()) return nullptr;
	return Save->OwnedCoins.FindByPredicate([CoinId](const FFortuneOwnedCoin& C) { return C.CoinId == CoinId; });
}

float UFortuneSubsystem::GetPassive(const UFortuneCoinData* Coin, EFortunePassiveType Type) const
{
	return (Coin && Coin->PassiveType == Type) ? Coin->PassiveValue : 0.f;
}

bool UFortuneSubsystem::GiveCoin(UFortuneCoinData* Coin)
{
	if (!Coin || !Save || FindOwned(Coin->GetCoinId())) return false;
	FFortuneOwnedCoin New;
	New.CoinId = Coin->GetCoinId();
	Save->OwnedCoins.Add(New);
	return true;
}

int32 UFortuneSubsystem::GetXPRequiredForLevel(int32 Level) const
{
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	const double Req = S->BaseXPPerLevel * FMath::Pow((double)S->XPGrowth, (double)FMath::Max(0, Level - 1));
	return FMath::Max(1, (int32)FMath::RoundToInt32(Req));
}

bool UFortuneSubsystem::AddXP(FFortuneOwnedCoin& Owned, const UFortuneCoinData* Coin, int32 Amount)
{
	const int32 MaxLevel = Coin ? Coin->MaxLevel : 100;
	bool bLeveled = false;
	Owned.XP += Amount;
	while (Owned.Level < MaxLevel && Owned.XP >= GetXPRequiredForLevel(Owned.Level))
	{
		Owned.XP -= GetXPRequiredForLevel(Owned.Level);
		Owned.Level++;
		bLeveled = true;
	}
	if (Owned.Level >= MaxLevel)
	{
		Owned.XP = 0;
	}
	if (bLeveled)
	{
		OnCoinLevelUp.Broadcast(Owned.CoinId, Owned.Level);
	}
	return bLeveled;
}

// ---------------- Degerler ----------------

double UFortuneSubsystem::GetCoinValue(FName CoinId) const
{
	const UFortuneCoinData* Coin = FindCoinData(CoinId);
	if (!Coin) return 0.0;
	const FFortuneOwnedCoin* Owned = FindOwned(CoinId);
	const int32 Level = Owned ? Owned->Level : 1;

	double Value = Coin->BaseValue + Coin->ValuePerLevel * (Level - 1);   // Seviyeden gelen deger
	Value += GetUpgradeBonus(EFortuneUpgradeType::MoneyEarn);            // MoneyEarn: duz (+) bonus
	Value += GetPassive(Coin, EFortunePassiveType::FlatMoneyBonus);
	Value *= 1.0 + GetPassive(Coin, EFortunePassiveType::MoneyMultiplier);
	Value *= 1.0 + GetCoinEnchantBonus(Owned, EFortuneEnchantStat::MoneyEarn);  // Enchant: kazanc +%
	return Value;
}

float UFortuneSubsystem::GetHeadChance() const
{
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	const float C = S->BaseHeadChance + GetUpgradeBonus(EFortuneUpgradeType::HeadChance)
		+ GetPassive(GetEquippedCoinData(), EFortunePassiveType::BonusHeadChance)
		+ GetEnchantBonus(EFortuneEnchantStat::HeadChance);
	return FMath::Clamp(C, 0.f, S->MaxHeadChance);
}

float UFortuneSubsystem::GetCritChance() const
{
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	const float C = S->BaseCritChance + GetUpgradeBonus(EFortuneUpgradeType::CriticalChance)
		+ GetPassive(GetEquippedCoinData(), EFortunePassiveType::BonusCritChance)
		+ GetEnchantBonus(EFortuneEnchantStat::CritChance);
	return FMath::Clamp(C, 0.f, S->MaxCritChance);
}

float UFortuneSubsystem::GetCritMultiplier() const
{
	return GetDefault<UFortuneSettings>()->BaseCritMultiplier
		+ GetUpgradeBonus(EFortuneUpgradeType::CriticalMultiply)
		+ GetPassive(GetEquippedCoinData(), EFortunePassiveType::BonusCritMultiply)
		+ GetEnchantBonus(EFortuneEnchantStat::CriticalMultiply);
}

float UFortuneSubsystem::GetBaseFlipDuration() const
{
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	const float Speed = 1.f + GetUpgradeBonus(EFortuneUpgradeType::CoinFlipSpeed) + GetEnchantBonus(EFortuneEnchantStat::CoinFlipSpeed);
	return FMath::Max(S->MinFlipDuration, S->BaseFlipDuration / Speed);
}

FFortuneFlipAnimTier UFortuneSubsystem::GetFlipAnimTier(FName CoinId) const
{
	const UFortuneCoinData* Coin = FindCoinData(CoinId);
	const FFortuneOwnedCoin* Owned = FindOwned(CoinId);
	const int32 Level = Owned ? Owned->Level : 1;

	const TArray<FFortuneFlipAnimTier>& Tiers = (Coin && Coin->FlipAnimTiers.Num() > 0)
		? Coin->FlipAnimTiers : GetDefault<UFortuneSettings>()->DefaultFlipAnimTiers;

	FFortuneFlipAnimTier Best;
	int32 BestMin = -1;
	for (const FFortuneFlipAnimTier& T : Tiers)
	{
		if (T.MinLevel <= Level && T.MinLevel > BestMin)
		{
			Best = T;
			BestMin = T.MinLevel;
		}
	}
	return Best;
}

// ---------------- Atis ----------------

FFortuneFlipResult UFortuneSubsystem::FlipCoin(bool bManual)
{
	FFortuneFlipResult R;
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	UFortuneCoinData* Coin = GetEquippedCoinData();
	FFortuneOwnedCoin* Owned = Save ? FindOwned(Save->EquippedCoin) : nullptr;
	if (!Coin || !Owned)
	{
		return R;
	}

	R.bSuccess = true;
	R.CoinId = Owned->CoinId;
	R.AnimTier = GetFlipAnimTier(R.CoinId);
	R.FlipDuration = GetBaseFlipDuration() * R.AnimTier.DurationScale;

	// Yazi / tura
	R.bHeads = FMath::FRand() < GetHeadChance();
	double Value = GetCoinValue(R.CoinId);
	if (R.bHeads)
	{
		HeadStreak++;
		// Streak enchant'i: her art arda yazi icin +%
		const int32 Stacks = FMath::Min(HeadStreak - 1, S->MaxStreakStacks);
		Value *= 1.0 + GetEnchantBonus(EFortuneEnchantStat::Streak) * Stacks;
	}
	else
	{
		HeadStreak = 0;
		if (Coin->PassiveType != EFortunePassiveType::TailsFullValue)
		{
			const float TailsMult = FMath::Min(1.f, S->TailsValueMultiplier + GetEnchantBonus(EFortuneEnchantStat::TailsBoost));
			Value *= TailsMult;
		}
	}
	R.HeadStreak = HeadStreak;

	// Kritik
	R.bCritical = FMath::FRand() < GetCritChance();
	if (R.bCritical)
	{
		Value *= GetCritMultiplier();
	}

	// DoubleFlip ve Jackpot enchant'lari
	R.bDoubleFlip = FMath::FRand() < GetEnchantBonus(EFortuneEnchantStat::DoubleFlip);
	if (R.bDoubleFlip)
	{
		Value *= 2.0;
	}
	R.bJackpot = FMath::FRand() < GetEnchantBonus(EFortuneEnchantStat::Jackpot);
	if (R.bJackpot)
	{
		Value *= S->JackpotMultiplier;
	}
	R.MoneyEarned = Value;

	// Enchant Scroll (1/800), Luck ve pasif artirir
	const float ScrollChance = S->EnchantScrollDropChance * (1.f + GetLuck())
		* (1.f + GetPassive(Coin, EFortunePassiveType::BonusScrollChance));
	R.bScrollDropped = FMath::FRand() < ScrollChance;

	// Uygula
	Save->Money += Value;
	Save->TotalFlips++;
	Owned->TotalFlips++;
	if (bManual)
	{
		Save->ManualFlips++;
		if (!Save->bAutoFlipUnlocked && Save->ManualFlips >= S->AutoFlipRequiredFlips)
		{
			Save->bAutoFlipUnlocked = true;
			SaveProgress();
		}
		OnAutoFlipChanged.Broadcast();   // ilerleme sayaci (x/500) guncellensin
	}
	const float RawXP = (1.f + GetPassive(Coin, EFortunePassiveType::BonusXP)) * (1.f + GetEnchantBonus(EFortuneEnchantStat::XPBoost));
	R.XPGained = RandomRound(RawXP);
	R.bLeveledUp = AddXP(*Owned, Coin, R.XPGained);
	R.NewLevel = Owned->Level;

	if (R.bScrollDropped)
	{
		AddItem(EFortuneItemType::EnchantScroll, 1);
		OnEnchantScrollDropped.Broadcast();
	}

	OnMoneyChanged.Broadcast(Save->Money);
	OnCoinsChanged.Broadcast();
	OnFlipResolved.Broadcast(R);

	// Her atista diske yazmamak icin 10 atista bir veya onemli olayda kaydet
	if (R.bLeveledUp || R.bScrollDropped || Save->TotalFlips % 10 == 0)
	{
		SaveProgress();
	}
	return R;
}

// ---------------- AutoFlip ----------------

bool UFortuneSubsystem::IsAutoFlipUnlocked() const
{
	return Save && (Save->bAutoFlipUnlocked || GetDefault<UFortuneSettings>()->bUnlockAutoFlipForTesting);
}

bool UFortuneSubsystem::IsAutoFlipEnabled() const
{
	return Save && Save->bAutoFlipEnabled && IsAutoFlipUnlocked();
}

void UFortuneSubsystem::GetAutoFlipProgress(int32& OutCurrent, int32& OutRequired) const
{
	OutRequired = GetDefault<UFortuneSettings>()->AutoFlipRequiredFlips;
	OutCurrent = Save ? (int32)FMath::Min<int64>(Save->ManualFlips, OutRequired) : 0;
}

bool UFortuneSubsystem::SetAutoFlipEnabled(bool bEnabled)
{
	if (!Save || (bEnabled && !IsAutoFlipUnlocked())) return false;
	Save->bAutoFlipEnabled = bEnabled;
	OnAutoFlipChanged.Broadcast();
	SaveProgress();
	return true;
}

bool UFortuneSubsystem::ToggleAutoFlip()
{
	return SetAutoFlipEnabled(!IsAutoFlipEnabled());
}

// ---------------- Coinler ----------------

TArray<UFortuneCoinData*> UFortuneSubsystem::GetAllCoins() const
{
	TArray<UFortuneCoinData*> Out;
	for (const TObjectPtr<UFortuneCoinData>& C : LoadedCoins)
	{
		Out.Add(C);
	}
	return Out;
}

UFortuneCoinData* UFortuneSubsystem::FindCoinData(FName CoinId) const
{
	for (const TObjectPtr<UFortuneCoinData>& C : LoadedCoins)
	{
		if (C && C->GetCoinId() == CoinId)
		{
			return C;
		}
	}
	return nullptr;
}

bool UFortuneSubsystem::IsCoinOwned(FName CoinId) const
{
	return FindOwned(CoinId) != nullptr;
}

bool UFortuneSubsystem::GetOwnedCoin(FName CoinId, FFortuneOwnedCoin& OutCoin) const
{
	if (const FFortuneOwnedCoin* O = FindOwned(CoinId))
	{
		OutCoin = *O;
		return true;
	}
	return false;
}

FName UFortuneSubsystem::GetEquippedCoinId() const
{
	return Save ? Save->EquippedCoin : NAME_None;
}

UFortuneCoinData* UFortuneSubsystem::GetEquippedCoinData() const
{
	return FindCoinData(GetEquippedCoinId());
}

bool UFortuneSubsystem::EquipCoin(FName CoinId)
{
	if (!FindOwned(CoinId)) return false;
	Save->EquippedCoin = CoinId;
	OnCoinsChanged.Broadcast();
	SaveProgress();
	return true;
}

TArray<FFortuneCollectionEntry> UFortuneSubsystem::GetCollection() const
{
	TArray<FFortuneCollectionEntry> Out;
	for (const TObjectPtr<UFortuneCoinData>& Coin : LoadedCoins)
	{
		if (!Coin) continue;
		FFortuneCollectionEntry E;
		E.Coin = Coin;
		if (const FFortuneOwnedCoin* O = FindOwned(Coin->GetCoinId()))
		{
			E.bOwned = true;
			E.bEquipped = (O->CoinId == GetEquippedCoinId());
			E.Level = O->Level;
			E.XP = O->XP;
			E.XPForNextLevel = (O->Level >= Coin->MaxLevel) ? 0 : GetXPRequiredForLevel(O->Level);
			E.bEnchanted = O->Enchant.IsActive();
			E.Enchant = O->Enchant;
			E.EnchantText = FormatEnchant(O->Enchant);
		}
		E.CurrentValue = GetCoinValue(Coin->GetCoinId());
		Out.Add(E);
	}
	// Nadirlige gore sirala (yuksek nadirlik once)
	Out.Sort([](const FFortuneCollectionEntry& A, const FFortuneCollectionEntry& B)
	{
		return (uint8)A.Coin->Rarity > (uint8)B.Coin->Rarity;
	});
	return Out;
}

// ---------------- Envanter ----------------

int32 UFortuneSubsystem::GetItemCount(EFortuneItemType Item) const
{
	if (!Save) return 0;
	const int32* N = Save->Items.Find(Item);
	return N ? *N : 0;
}

TMap<EFortuneItemType, int32> UFortuneSubsystem::GetInventory() const
{
	return Save ? Save->Items : TMap<EFortuneItemType, int32>();
}

void UFortuneSubsystem::AddItem(EFortuneItemType Item, int32 Count)
{
	if (!Save || Count == 0) return;
	int32& N = Save->Items.FindOrAdd(Item);
	N = FMath::Max(0, N + Count);
	OnInventoryChanged.Broadcast();
}

bool UFortuneSubsystem::UseEnchantScroll(FName CoinId, FFortuneEnchant& OutEnchant)
{
	FFortuneOwnedCoin* Owned = FindOwned(CoinId);
	if (!Owned || GetItemCount(EFortuneItemType::EnchantScroll) <= 0)
	{
		return false;
	}
	AddItem(EFortuneItemType::EnchantScroll, -1);
	OutEnchant = RollEnchant();
	Owned->Enchant = OutEnchant;
	OnCoinEnchanted.Broadcast(CoinId, OutEnchant);
	OnCoinsChanged.Broadcast();
	OnUpgradesChanged.Broadcast();  // indirimler fiyatlari degistirebilir
	SaveProgress();
	return true;
}

// ---------------- Enchant ----------------

int32 UFortuneSubsystem::RandomRound(float X)
{
	const int32 Whole = FMath::FloorToInt32(X);
	return Whole + ((FMath::FRand() < (X - Whole)) ? 1 : 0);
}

float UFortuneSubsystem::GetCoinEnchantBonus(const FFortuneOwnedCoin* Owned, EFortuneEnchantStat Stat) const
{
	return (Owned && Owned->Enchant.IsActive() && Owned->Enchant.Stat == Stat) ? Owned->Enchant.Value : 0.f;
}

float UFortuneSubsystem::GetEnchantBonus(EFortuneEnchantStat Stat) const
{
	return GetCoinEnchantBonus(FindOwned(GetEquippedCoinId()), Stat);
}

float UFortuneSubsystem::GetLuck() const
{
	return GetEnchantBonus(EFortuneEnchantStat::Luck);
}

void UFortuneSubsystem::GetEnchantTierChances(float& OutTier1, float& OutTier2, float& OutTier3) const
{
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	const float Luck = GetLuck();
	OutTier3 = FMath::Clamp(S->Tier3Chance * (1.f + Luck), 0.f, 1.f);
	OutTier2 = FMath::Clamp(S->Tier2Chance * (1.f + Luck * 0.5f), 0.f, 1.f - OutTier3);
	OutTier1 = 1.f - OutTier2 - OutTier3;
}

FFortuneEnchantStatConfig UFortuneSubsystem::GetEnchantStatConfig(EFortuneEnchantStat Stat) const
{
	const FFortuneEnchantStatConfig* C = GetDefault<UFortuneSettings>()->EnchantStats.Find(Stat);
	return C ? *C : FFortuneEnchantStatConfig();
}

FFortuneEnchant UFortuneSubsystem::RollEnchant() const
{
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	FFortuneEnchant E;

	// Tier
	float T1, T2, T3;
	GetEnchantTierChances(T1, T2, T3);
	const float TierRoll = FMath::FRand();
	E.Tier = (TierRoll < T3) ? 3 : (TierRoll < T3 + T2 ? 2 : 1);

	// Stat (agirliga gore)
	float Total = 0.f;
	for (const TPair<EFortuneEnchantStat, FFortuneEnchantStatConfig>& P : S->EnchantStats)
	{
		Total += FMath::Max(0.f, P.Value.RollWeight);
	}
	float Roll = FMath::FRandRange(0.f, Total);
	bool bPicked = false;
	for (const TPair<EFortuneEnchantStat, FFortuneEnchantStatConfig>& P : S->EnchantStats)
	{
		const float W = FMath::Max(0.f, P.Value.RollWeight);
		if (W <= 0.f) continue;
		Roll -= W;
		if (Roll <= 0.f)
		{
			E.Stat = P.Key;
			E.Value = P.Value.GetValue(E.Tier);
			bPicked = true;
			break;
		}
	}
	if (!bPicked)
	{
		E.Stat = EFortuneEnchantStat::MoneyEarn;
		E.Value = GetEnchantStatConfig(E.Stat).GetValue(E.Tier);
	}
	return E;
}

FText UFortuneSubsystem::FormatEnchant(const FFortuneEnchant& Enchant) const
{
	if (!Enchant.IsActive())
	{
		return NSLOCTEXT("Fortune", "NoEnchant", "Enchant yok");
	}
	const FFortuneEnchantStatConfig C = GetEnchantStatConfig(Enchant.Stat);
	FNumberFormattingOptions Opt;
	Opt.MaximumFractionalDigits = 1;
	const FText Amount = C.bPercent
		? FText::Format(NSLOCTEXT("Fortune", "Pct", "+%{0}"), FText::AsNumber(Enchant.Value * 100.f, &Opt))
		: FText::Format(NSLOCTEXT("Fortune", "Flat", "+{0}"), FText::AsNumber(Enchant.Value, &Opt));
	return FText::Format(NSLOCTEXT("Fortune", "EnchantFmt", "Tier {0} - {1} {2}"),
		FText::AsNumber(Enchant.Tier), C.DisplayName, Amount);
}

// ---------------- Yukseltmeler ----------------

int32 UFortuneSubsystem::GetUpgradeLevel(EFortuneUpgradeType Upgrade) const
{
	if (!Save) return 0;
	const int32* L = Save->UpgradeLevels.Find(Upgrade);
	return L ? *L : 0;
}

FFortuneUpgradeConfig UFortuneSubsystem::GetUpgradeConfig(EFortuneUpgradeType Upgrade) const
{
	const FFortuneUpgradeConfig* C = GetDefault<UFortuneSettings>()->Upgrades.Find(Upgrade);
	return C ? *C : FFortuneUpgradeConfig();
}

float UFortuneSubsystem::GetUpgradeBonus(EFortuneUpgradeType Upgrade) const
{
	return GetUpgradeConfig(Upgrade).ValuePerLevel * GetUpgradeLevel(Upgrade);
}

double UFortuneSubsystem::GetUpgradeCost(EFortuneUpgradeType Upgrade) const
{
	const FFortuneUpgradeConfig C = GetUpgradeConfig(Upgrade);
	const int32 L = GetUpgradeLevel(Upgrade);
	if (L >= C.MaxLevel) return -1.0;
	const float Discount = FMath::Min(GetEnchantBonus(EFortuneEnchantStat::UpgradeDiscount), GetDefault<UFortuneSettings>()->MaxDiscount);
	return FMath::FloorToDouble(C.BaseCost * FMath::Pow(C.CostGrowth, (double)L) * (1.0 - Discount) + 0.5);
}

bool UFortuneSubsystem::BuyUpgrade(EFortuneUpgradeType Upgrade)
{
	const double Cost = GetUpgradeCost(Upgrade);
	if (!Save || Cost < 0.0 || Save->Money < Cost) return false;
	Save->Money -= Cost;
	Save->UpgradeLevels.FindOrAdd(Upgrade)++;
	OnMoneyChanged.Broadcast(Save->Money);
	OnUpgradesChanged.Broadcast();
	OnCoinsChanged.Broadcast();   // MoneyEarn coin degerlerini degistirir
	SaveProgress();
	return true;
}

// ---------------- Gacha ----------------

double UFortuneSubsystem::GetSummonCost() const
{
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	const float Discount = FMath::Min(GetEnchantBonus(EFortuneEnchantStat::SummonDiscount), S->MaxDiscount);
	return FMath::FloorToDouble(S->SummonCost * (1.0 - Discount) + 0.5);
}

float UFortuneSubsystem::GetSummonChance(const UFortuneCoinData* Coin) const
{
	if (!Coin) return 0.f;
	const float Luck = GetLuck();
	auto Weight = [Luck](const UFortuneCoinData* C)
	{
		return FMath::Max(0.f, C->SummonWeight) * (1.f + Luck * (float)(uint8)C->Rarity);
	};
	float Total = 0.f;
	for (const TObjectPtr<UFortuneCoinData>& C : LoadedCoins)
	{
		if (C) Total += Weight(C);
	}
	return Total > 0.f ? Weight(Coin) / Total : 0.f;
}

bool UFortuneSubsystem::SummonCoin(UFortuneCoinData*& OutCoin, bool& bOutDuplicate)
{
	OutCoin = nullptr;
	bOutDuplicate = false;
	const double Cost = GetSummonCost();
	if (!Save || Save->Money < Cost || LoadedCoins.Num() == 0) return false;

	// Luck nadir coinlerin agirligini artirir (nadirlik basamagi basina)
	const float Luck = GetLuck();
	auto Weight = [Luck](const UFortuneCoinData* C)
	{
		return FMath::Max(0.f, C->SummonWeight) * (1.f + Luck * (float)(uint8)C->Rarity);
	};

	float Total = 0.f;
	for (const TObjectPtr<UFortuneCoinData>& C : LoadedCoins)
	{
		if (C) Total += Weight(C);
	}
	if (Total <= 0.f) return false;

	float Roll = FMath::FRandRange(0.f, Total);
	for (const TObjectPtr<UFortuneCoinData>& C : LoadedCoins)
	{
		if (!C) continue;
		Roll -= Weight(C);
		if (Roll <= 0.f)
		{
			OutCoin = C;
			break;
		}
	}
	if (!OutCoin)
	{
		OutCoin = LoadedCoins.Last();
	}

	Save->Money -= Cost;
	if (FFortuneOwnedCoin* Owned = FindOwned(OutCoin->GetCoinId()))
	{
		bOutDuplicate = true;
		AddXP(*Owned, OutCoin, GetDefault<UFortuneSettings>()->DuplicateXP);
	}
	else
	{
		GiveCoin(OutCoin);
	}

	OnMoneyChanged.Broadcast(Save->Money);
	OnCoinsChanged.Broadcast();
	OnCoinSummoned.Broadcast(OutCoin, bOutDuplicate);
	SaveProgress();
	return true;
}
