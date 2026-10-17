#include "FortuneSettings.h"

#define LOCTEXT_NAMESPACE "FortuneSettings"

static FFortuneUpgradeConfig MakeUpgrade(const FText& Name, const FText& Desc, int32 MaxLevel, double BaseCost, double Growth, float PerLevel)
{
	FFortuneUpgradeConfig C;
	C.DisplayName = Name;
	C.Description = Desc;
	C.MaxLevel = MaxLevel;
	C.BaseCost = BaseCost;
	C.CostGrowth = Growth;
	C.ValuePerLevel = PerLevel;
	return C;
}

UFortuneSettings::UFortuneSettings()
{
	Upgrades.Add(EFortuneUpgradeType::CoinFlipSpeed,
		MakeUpgrade(LOCTEXT("SpeedN", "Coin Flip Speed"), LOCTEXT("SpeedD", "Coin daha hızlı atılır."), 30, 15.0, 1.18, 0.05f));
	Upgrades.Add(EFortuneUpgradeType::HeadChance,
		MakeUpgrade(LOCTEXT("HeadN", "Head Chance"), LOCTEXT("HeadD", "Yazı gelme şansı artar."), 40, 25.0, 1.2, 0.01f));
	Upgrades.Add(EFortuneUpgradeType::MoneyEarn,
		MakeUpgrade(LOCTEXT("EarnN", "Money Earn"), LOCTEXT("EarnD", "Her atışta kazandığın paraya düz (+) bonus ekler."), 100, 10.0, 1.15, 1.0f));
	Upgrades.Add(EFortuneUpgradeType::CriticalChance,
		MakeUpgrade(LOCTEXT("CritN", "Critical Chance"), LOCTEXT("CritD", "Kritik vurma şansı artar."), 35, 40.0, 1.22, 0.01f));
	Upgrades.Add(EFortuneUpgradeType::CriticalMultiply,
		MakeUpgrade(LOCTEXT("CritMN", "Critical Multiply"), LOCTEXT("CritMD", "Kritik vuruşun çarpanı artar."), 50, 60.0, 1.2, 0.1f));

	// Enchant havuzu: (isim, T1, T2, T3, yuzde mi, agirlik)
	auto AddStat = [this](EFortuneEnchantStat Stat, const FText& Name, float T1, float T2, float T3, bool bPercent, float Weight)
	{
		FFortuneEnchantStatConfig C;
		C.DisplayName = Name;
		C.Tier1Value = T1;
		C.Tier2Value = T2;
		C.Tier3Value = T3;
		C.bPercent = bPercent;
		C.RollWeight = Weight;
		EnchantStats.Add(Stat, C);
	};
	AddStat(EFortuneEnchantStat::CoinFlipSpeed,    LOCTEXT("E_Speed", "Coin Flip Speed"),    0.10f, 0.20f, 0.35f, true, 100.f);
	AddStat(EFortuneEnchantStat::MoneyEarn,        LOCTEXT("E_Money", "Money Earn"),         0.15f, 0.30f, 0.60f, true, 100.f);
	AddStat(EFortuneEnchantStat::Luck,             LOCTEXT("E_Luck", "Luck"),                0.10f, 0.25f, 0.50f, true, 70.f);
	AddStat(EFortuneEnchantStat::HeadChance,       LOCTEXT("E_Head", "Head Chance"),         0.02f, 0.04f, 0.08f, true, 90.f);
	AddStat(EFortuneEnchantStat::CritChance,       LOCTEXT("E_Crit", "Crit Chance"),         0.02f, 0.05f, 0.10f, true, 90.f);
	AddStat(EFortuneEnchantStat::CriticalMultiply, LOCTEXT("E_CritM", "Critical Multiply"),  0.25f, 0.50f, 1.00f, false, 90.f);
	AddStat(EFortuneEnchantStat::XPBoost,          LOCTEXT("E_XP", "XP Boost"),              0.25f, 0.50f, 1.00f, true, 100.f);
	AddStat(EFortuneEnchantStat::TailsBoost,       LOCTEXT("E_Tails", "Tails Boost"),        0.10f, 0.20f, 0.35f, true, 90.f);
	AddStat(EFortuneEnchantStat::DoubleFlip,       LOCTEXT("E_Double", "Double Flip"),       0.03f, 0.06f, 0.12f, true, 70.f);
	AddStat(EFortuneEnchantStat::Jackpot,          LOCTEXT("E_Jackpot", "Jackpot"),          0.002f, 0.005f, 0.01f, true, 40.f);
	AddStat(EFortuneEnchantStat::Streak,           LOCTEXT("E_Streak", "Streak"),            0.02f, 0.04f, 0.08f, true, 70.f);
	AddStat(EFortuneEnchantStat::SummonDiscount,   LOCTEXT("E_SumDisc", "Summon Discount"),  0.05f, 0.10f, 0.20f, true, 60.f);
	AddStat(EFortuneEnchantStat::UpgradeDiscount,  LOCTEXT("E_UpgDisc", "Upgrade Discount"), 0.05f, 0.10f, 0.20f, true, 60.f);

	// Varsayilan seviye animasyonlari
	auto AddTier = [this](int32 MinLevel, const FText& Name, float Height, int32 Spins, int32 YawSpins, float DurScale)
	{
		FFortuneFlipAnimTier T;
		T.MinLevel = MinLevel;
		T.DisplayName = Name;
		T.TossHeight = Height;
		T.Spins = Spins;
		T.YawSpins = YawSpins;
		T.DurationScale = DurScale;
		DefaultFlipAnimTiers.Add(T);
	};
	AddTier(1,  LOCTEXT("T1", "Basic Flip"),   35.f, 3, 0, 1.0f);
	AddTier(10, LOCTEXT("T2", "High Flip"),    55.f, 5, 0, 1.0f);
	AddTier(25, LOCTEXT("T3", "Spiral Flip"),  60.f, 6, 1, 1.05f);
	AddTier(50, LOCTEXT("T4", "Tornado Flip"), 80.f, 8, 3, 1.1f);
	AddTier(75, LOCTEXT("T5", "Royal Flip"),  100.f, 10, 4, 1.15f);
}

#undef LOCTEXT_NAMESPACE
