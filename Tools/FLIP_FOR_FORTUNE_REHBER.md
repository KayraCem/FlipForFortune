# Flip for Fortune - C++ cekirdek kurulum rehberi

## 1) Derle
1. Unreal Editor'u kapat.
2. Gerekli: Visual Studio 2022 + "Game development with C++" is yuku (Unreal'in istedigi surum).
3. `MyProject.uproject` dosyasina cift tikla -> "Missing modules... rebuild?" sorusuna **Yes**.
   (Alternatif: .uproject'e sag tik > Generate Visual Studio project files > .sln'i ac > Development Editor ile Build.)

## 2) Coin olustur
Content Browser > sag tik > Miscellaneous > Data Asset > **FortuneCoinData**. Ornek: `DA_Coin_Bronze`.
Doldur: DisplayName, Description (tanitim), Rarity, SummonWeight, BaseValue, ValuePerLevel,
Passive (tur + deger + aciklama), CoinMaterial / EnchantedMaterial / EnchantVFX, CritVFX, sesler.
FlipAnimTiers bos kalirsa varsayilan seviye animasyonlari kullanilir (Lv1, 10, 25, 50, 75).

## 3) Ayarlar
Project Settings > Game > **Flip for Fortune**:
- Coins listesine tum coin asset'lerini ekle, StartingCoin'i sec.
- DefaultCritVFX / DefaultCritSound: kritik efekti ve sesi.
- Scroll sansi (1/800), enchant carpani, XP egrisi, yukseltme fiyatlari burada.

## 4) Sahne
CoinRoom haritasindaki eski "Coin" aktorunu sil, yerine **FortuneCoinFlipper** surukle,
masanin ustune (Z = 80) koy. Istersen BP_CoinFlipper diye Blueprint'ini olustur.
- Atmak icin: `Flip` fonksiyonunu bir butona veya tusa bagla.
- OnFlipLanded / OnCriticalHit eventleri ile UI yazisi, kamera sarsintisi vb. ekle.
- El: HandMesh bilesenine riglenmis el mesh'i + Anim Blueprint ver (DefaultSlot olmali),
  FlipAnimTier.HandMontage'a montage ata. HandReleaseDelay = fiske ani.

## 5) UI (Blueprint / UMG)
Her yerde: **Get Game Instance Subsystem -> Fortune Subsystem**.
- Ust bar: GetMoney + OnMoneyChanged.
- Envanter sekmesi: GetInventory / GetItemCount(EnchantScroll) + OnInventoryChanged.
- Coin Collection sekmesi: GetCollection -> her satirda Coin (isim, ikon, tanitim, pasif),
  Level, XP / XPForNextLevel, CurrentValue, bEnchanted, bOwned. Butonlar: EquipCoin, UseEnchantScroll.
  OnCoinsChanged'de listeyi yenile.
- Yukseltmeler: GetUpgradeLevel / GetUpgradeCost / BuyUpgrade (CoinFlipSpeed, HeadChance,
  MoneyEarn, CriticalChance, CriticalMultiply) + OnUpgradesChanged.
- Gacha: GetSummonCost / SummonCoin + OnCoinSummoned.

## Formul
Deger = (BaseValue + ValuePerLevel*(Level-1) + MoneyEarn + duz pasif) * para carpani pasifi * (1 + enchant MoneyEarn%)
Yazi = Deger * Streak bonusu, Tura = Deger * (0.5 + TailsBoost), Kritik = x (1.5 + CriticalMultiply).
DoubleFlip = x2, Jackpot = x10.
Her atis = 1 XP; Lv1->2 = 10 XP, sonra her seviye %15 artar.

## Enchant (3 tier)
Envanterdeki Enchant Scroll -> UseEnchantScroll(CoinId): 1 scroll = 1 rastgele boost, eski enchant'in yerine gecer.
Tier sansi: T1 %70, T2 %25, T3 %5 (Luck T2/T3'u artirir). GetEnchantTierChances ile UI'da goster.
Havuz: CoinFlipSpeed, MoneyEarn, Luck, HeadChance, CritChance, CriticalMultiply, XPBoost,
TailsBoost, DoubleFlip, Jackpot, Streak, SummonDiscount, UpgradeDiscount.
Degerler: Project Settings > Flip for Fortune > Enchant > EnchantStats (her stat icin T1/T2/T3 ve agirlik).
Luck: scroll dusme sansi, gacha'da nadir coin agirligi ve enchant tier sansi.
Sadece kusanili coinin enchant'i aktiftir. UI icin FormatEnchant -> "Tier 3 - Money Earn +%60".
Olay: OnCoinEnchanted (tier'a gore efekt/ses). Flipper'da OnJackpot eventi var.
