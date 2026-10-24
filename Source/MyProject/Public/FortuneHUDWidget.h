// Flip for Fortune - Ekranin altindaki para gostergesi (tamamen C++ ile kurulur, WBP gerekmez).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FortuneTypes.h"
#include "FortuneHUDWidget.generated.h"

class UTextBlock;
class UButton;
class UBorder;
class UFortuneSubsystem;

UCLASS()
class MYPROJECT_API UFortuneHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 1234 -> "1.23K", 2500000 -> "2.50M" */
	UFUNCTION(BlueprintPure, Category = "Fortune")
	static FText FormatMoney(double Value);

	/** Birinci sahis nisangahi + baktigin esyanin adi ("[E] Yukseltmeler"). */
	void SetCrosshair(bool bShow, const FText& Target);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	void RefreshScrolls();
	void RefreshAutoFlip();

	UFUNCTION()
	void HandleAutoClicked();

	UFUNCTION()
	void HandleAutoChanged();
	UFortuneSubsystem* GetFortune() const;

	UFUNCTION()
	void HandleMoneyChanged(double NewMoney);

	UFUNCTION()
	void HandleFlip(const FFortuneFlipResult& Result);

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION() void HandleCollectionIcon();
	UFUNCTION() void HandleInventoryIcon();

	/** Sol kenar: ikonlu buton (Coin Collection / Envanter). */
	class UButton* MakeSideButton(const TCHAR* IconPath, const FString& Label, const FString& Key, const FLinearColor& Accent);

	UPROPERTY()
	TObjectPtr<UTextBlock> MoneyText;

	UPROPERTY()
	TObjectPtr<UWidget> CrosshairDot;

	UPROPERTY()
	TObjectPtr<UTextBlock> AimText;

	bool bCrosshairShown = false;
	FString AimLabel;

	UPROPERTY()
	TObjectPtr<UTextBlock> ScrollText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PopupText;

	/** "LEVEL 2!" yazisi (para renginden farkli renkte). */
	UPROPERTY()
	TObjectPtr<UTextBlock> LevelUpText;

	float LevelUpAge = 99.f;

	UPROPERTY()
	TObjectPtr<UButton> AutoButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> AutoText;

	UPROPERTY()
	TObjectPtr<UBorder> AutoLight;

	double DisplayedMoney = 0.0;
	double TargetMoney = 0.0;
	double HeldBack = 0.0;        // coin inene kadar gosterilmeyen kazanc
	float PendingTime = -1.f;     // coin inisine kalan sure
	FFortuneFlipResult Pending;
	float PopupAge = 99.f;
};
