// Flip for Fortune - Ortada acilan Yukseltmeler paneli (orse tiklayinca).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "FortuneTypes.h"
#include "FortuneUpgradeWidget.generated.h"

class UTextBlock;
class UFortuneSubsystem;
class UFortuneUpgradeWidget;

/** Hangi yukseltmeye ait oldugunu bilen "AL" butonu. */
UCLASS()
class MYPROJECT_API UFortuneUpgradeBuyButton : public UButton
{
	GENERATED_BODY()

public:
	void Setup(UFortuneUpgradeWidget* InOwner, EFortuneUpgradeType InType);
	EFortuneUpgradeType Type = EFortuneUpgradeType::MoneyEarn;

private:
	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<UFortuneUpgradeWidget> Owner;
};

UCLASS()
class MYPROJECT_API UFortuneUpgradeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Buy(EFortuneUpgradeType Type);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	void Refresh();
	void SetBuyMode(int32 Mode);
	UFortuneSubsystem* GetFortune() const;

	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnModeX1();
	UFUNCTION() void OnModeX10();
	UFUNCTION() void OnModeMax();
	UFUNCTION() void HandleChanged();
	UFUNCTION() void HandleMoneyChanged(double NewMoney);

	struct FRow
	{
		EFortuneUpgradeType Type;
		FLinearColor Color;
		TObjectPtr<class UBorder> RowBorder;
		TObjectPtr<UTextBlock> Level;
		TObjectPtr<UTextBlock> Value;
		TObjectPtr<UTextBlock> Cost;
		TObjectPtr<UTextBlock> CostSub;
		TObjectPtr<UFortuneUpgradeBuyButton> Button;
		TObjectPtr<class UHorizontalBoxSlot> BarFill;
		TObjectPtr<class UHorizontalBoxSlot> BarRest;
		TObjectPtr<class UBorder> BarFillBorder;
		float Flash = 0.f;
	};
	TArray<FRow> Rows;

	// Satirlardaki widget'lar WidgetTree'ye ait; GC icin ayrica tutulur
	UPROPERTY() TArray<TObjectPtr<UObject>> KeepAlive;

	UPROPERTY() TObjectPtr<UTextBlock> MoneyText;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> StatChips;
	UPROPERTY() TArray<TObjectPtr<UButton>> ModeButtons;
	UPROPERTY() TObjectPtr<UWidget> PanelRoot;

	/** 0 = x1, 1 = x10, 2 = MAKS */
	int32 BuyMode = 0;
	float Age = 0.f;
	float MoneyPop = 0.f;
};
