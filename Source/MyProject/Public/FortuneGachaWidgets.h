// Flip for Fortune - Gacha ile ilgili iki ekran:
//  - UFortuneGachaRatesWidget: sandigin ustunde nadirlik / cikma sansi listesi
//  - UFortuneGachaRevealWidget: coin ciktiginda "YENI!" ekrani (tikla -> kapat)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FortuneGachaWidgets.generated.h"

class UTextBlock;
class UVerticalBox;
class UFortuneCoinData;
class AFortuneGachaChest;

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
