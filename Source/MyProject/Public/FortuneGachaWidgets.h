// Flip for Fortune - Gacha ile ilgili iki ekran:
//  - UFortuneGachaRatesWidget: sandigin ustunde nadirlik / cikma sansi listesi
//  - UFortuneGachaRevealWidget: coin ciktiginda "YENI!" ekrani (tikla -> kapat)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "FortuneGachaWidgets.generated.h"

class UTextBlock;
class UVerticalBox;
class UFortuneCoinData;
class AFortuneGachaChest;
class UButton;

UCLASS()
class MYPROJECT_API UFortuneGachaRatesWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Listeyi yeniden olustur (para, sans, sahip olunanlar degisince). */
	void Refresh();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTree();

	UPROPERTY() TObjectPtr<UVerticalBox> List;
	UPROPERTY() TObjectPtr<UTextBlock> CostText;
};

class UFortuneSummonWidget;

/** Kac kez summon yapacagini bilen buton (x1 / x5 / x10). */
UCLASS()
class MYPROJECT_API UFortuneSummonCountButton : public UButton
{
	GENERATED_BODY()

public:
	void Setup(UFortuneSummonWidget* InOwner, int32 InCount);
	int32 Count = 1;

private:
	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<UFortuneSummonWidget> Owner;
};

/** Sandiga tiklayinca acilan panel: tum coinlerin nadirlik ve sanslari + SUMMON x1 / x5 / x10 butonlari. */
UCLASS()
class MYPROJECT_API UFortuneSummonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<AFortuneGachaChest> Chest;

	void Summon(int32 Count);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	void Refresh();
	void RebuildList();

	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void HandleMoneyChanged(double NewMoney);
	UFUNCTION() void HandleCoinsChanged();

	UPROPERTY() TObjectPtr<UVerticalBox> List;
	UPROPERTY() TObjectPtr<class UHorizontalBox> DistRow;
	UPROPERTY() TObjectPtr<UTextBlock> MoneyText;
	UPROPERTY() TObjectPtr<UTextBlock> LuckText;
	UPROPERTY() TObjectPtr<UTextBlock> CollectionText;
	UPROPERTY() TObjectPtr<class UWidget> PanelRoot;
	UPROPERTY() TObjectPtr<class UHorizontalBoxSlot> CollFill;
	UPROPERTY() TObjectPtr<class UHorizontalBoxSlot> CollRest;
	UPROPERTY() TObjectPtr<class UBorder> CollFillBorder;

	// x1 / x5 / x10 butonlari
	UPROPERTY() TArray<TObjectPtr<UFortuneSummonCountButton>> SummonButtons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SummonCosts;
	UPROPERTY() TArray<TObjectPtr<class UWidget>> SummonBoxes;

	float Age = 0.f;
	TArray<bool> Affordable;
};

/** Legendary ve ustu cikinca: ekran parlar, buyuk nadirlik yazisi (kendi kendine kaybolur). */
UCLASS()
class MYPROJECT_API UFortuneLegendBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FString& RarityName, const FLinearColor& Color, const FString& CoinName);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY() TObjectPtr<class UImage> Flash;
	UPROPERTY() TObjectPtr<UTextBlock> BigText;
	UPROPERTY() TObjectPtr<UTextBlock> SubText;
	FLinearColor Tint = FLinearColor::White;
	float Age = 0.f;
};

/** Summon sahnesinde sag ustte duran para gostergesi (HUD gizliyken). */
UCLASS()
class MYPROJECT_API UFortuneMoneyChipWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY() TObjectPtr<UTextBlock> MoneyText;
	double Shown = -1.0;
};

/** Summon sonuc ekraninin altindaki butonlar: TEKRAR xN, x10 (veya x1), TAMAM. */
UCLASS()
class MYPROJECT_API UFortuneSummonAgainBar : public UUserWidget
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<AFortuneGachaChest> Chest;

	/** Butonlar bu kadar saniye sonra belirir (yanlislikla basilmasin). */
	float Delay = 0.8f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	UButton* MakeBarButton(const FString& Label, const FLinearColor& Fill, const FLinearColor& Accent, UTextBlock*& OutText, float Width);

	UFUNCTION() void OnAgain();
	UFUNCTION() void OnOther();
	UFUNCTION() void OnOk();

	UPROPERTY() TObjectPtr<UButton> AgainBtn;
	UPROPERTY() TObjectPtr<UButton> OtherBtn;
	UPROPERTY() TObjectPtr<UButton> OkBtn;
	UPROPERTY() TObjectPtr<UTextBlock> AgainText;
	UPROPERTY() TObjectPtr<UTextBlock> OtherText;
	UPROPERTY() TObjectPtr<UTextBlock> OkText;

	int32 AgainCount = 1;
	int32 OtherCount = 10;
	float Age = 0.f;
	bool bUsed = false;
};

/** Coklu summon sonucu: her coinin altinda isim/nadirlik etiketi, en iyisi vurgulu. Tikla -> kapat. */
UCLASS()
class MYPROJECT_API UFortuneGachaMultiRevealWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(AFortuneGachaChest* InChest, const TArray<UFortuneCoinData*>& Coins, const TArray<bool>& Duplicates,
		const TArray<FVector>& WorldPositions, int32 BestIndex, int32 DuplicateXP);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildTree();
	void TryClose();

	UPROPERTY() TObjectPtr<class UCanvasPanel> Canvas;
	UPROPERTY() TObjectPtr<UTextBlock> HeadlineText;
	UPROPERTY() TObjectPtr<UTextBlock> SummaryText;
	UPROPERTY() TObjectPtr<UTextBlock> HintText;
	UPROPERTY() TArray<TObjectPtr<class UWidget>> Labels;

	TArray<FVector> Positions;
	TWeakObjectPtr<AFortuneGachaChest> Chest;
	float Age = 0.f;
	bool bClosed = false;
	bool bPlaced = false;
};

UCLASS()
class MYPROJECT_API UFortuneGachaRevealWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(AFortuneGachaChest* InChest, const UFortuneCoinData* Coin, bool bDuplicate, int32 DuplicateXP);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildTree();
	void TryClose();

	UPROPERTY() TObjectPtr<UTextBlock> HeadlineText;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> RarityText;
	UPROPERTY() TObjectPtr<UTextBlock> PassiveText;
	UPROPERTY() TObjectPtr<UTextBlock> HintText;

	TWeakObjectPtr<AFortuneGachaChest> Chest;
	float Age = 0.f;
	bool bClosed = false;
};
