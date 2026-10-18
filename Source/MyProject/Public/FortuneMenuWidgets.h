// Flip for Fortune - Ana menu ve Enchant ekrani (C++ ile kurulur).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FortuneTypes.h"
#include "FortuneMenuWidgets.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UBorder;
class UFortuneSubsystem;

/** Oyun acilinca gelen ana menu: logo + OYNA + CIKIS. */
UCLASS()
class MYPROJECT_API UFortuneMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();

	UFUNCTION() void OnPlayClicked();
	UFUNCTION() void OnQuitClicked();

	UPROPERTY() TObjectPtr<UImage> Logo;
	UPROPERTY() TObjectPtr<UTextBlock> FallbackTitle;
	UPROPERTY() TObjectPtr<UTextBlock> MoneyText;

	float Age = 0.f;
	float FadeOut = -1.f;
	TWeakObjectPtr<AActor> GameViewTarget;
	TWeakObjectPtr<AActor> MenuCamera;
};

/** Enchant ekrani: tier nadirlikleri, sanslar, mevcut enchant, donen cark ve sonuc. */
UCLASS()
class MYPROJECT_API UFortuneEnchantWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** AddToViewport'tan once ayarla. */
	FName CoinId;

	/** Masadaki buyu kitabindan acildiysa true: menu modunu kendisi acar/kapatir. */
	bool bStandalone = false;

	/** Tier -> nadirlik adi ve rengi (1 = RARE mavi, 2 = EPIC mor, 3 = LEGENDARY altin). */
	static FString TierName(int32 Tier);
	static FLinearColor TierColor(int32 Tier);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	void Refresh();
	void ShowEnchant(UBorder* Box, UTextBlock* TierT, UTextBlock* StatT, const FFortuneEnchant& E, bool bBig);
	UFortuneSubsystem* GetFortune() const;

	UFUNCTION() void OnRollClicked();
	UFUNCTION() void OnPrevCoin();
	UFUNCTION() void OnNextCoin();
	void CycleCoin(int32 Dir);
	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void HandleChanged();

	UPROPERTY() TObjectPtr<UTextBlock> CoinNameText;
	UPROPERTY() TObjectPtr<UTextBlock> ScrollText;
	UPROPERTY() TObjectPtr<UBorder> CurrentBox;
	UPROPERTY() TObjectPtr<UTextBlock> CurrentTier;
	UPROPERTY() TObjectPtr<UTextBlock> CurrentStat;
	UPROPERTY() TObjectPtr<UBorder> ResultBox;
	UPROPERTY() TObjectPtr<UTextBlock> ResultTier;
	UPROPERTY() TObjectPtr<UTextBlock> ResultStat;
	UPROPERTY() TObjectPtr<UTextBlock> ResultHint;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> ChanceTexts;
	UPROPERTY() TObjectPtr<UButton> RollButton;
	UPROPERTY() TObjectPtr<UTextBlock> RollText;

	bool bRolling = false;
	float RollTime = 0.f;
	float NextTick = 0.f;
	float ResultPop = 0.f;
	FFortuneEnchant RollResult;
};
