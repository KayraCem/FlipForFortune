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
	UFortuneSubsystem* GetFortune() const;

	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void HandleChanged();
	UFUNCTION() void HandleMoneyChanged(double NewMoney);

	struct FRow
	{
		EFortuneUpgradeType Type;
		TObjectPtr<UTextBlock> Level;
		TObjectPtr<UTextBlock> Value;
		TObjectPtr<UTextBlock> Cost;
		TObjectPtr<UFortuneUpgradeBuyButton> Button;
		float Flash = 0.f;
	};
	TArray<FRow> Rows;

	// Satirlardaki widget'lar WidgetTree'ye ait; GC icin ayrica tutulur
	UPROPERTY() TArray<TObjectPtr<UObject>> KeepAlive;

	UPROPERTY() TObjectPtr<UTextBlock> MoneyText;
	UPROPERTY() TObjectPtr<UTextBlock> StatsText;
};
