// Flip for Fortune - Tam ekran Coin Collection (albume tiklayinca acilir).
// Ortada donen 3D coin, altta coin seridi, sagda bilgi paneli, TAK / GERI / ENCHANT.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "FortuneTypes.h"
#include "FortuneCollectionWidget.generated.h"

class UTextBlock;
class UImage;
class UBorder;
class UHorizontalBox;
class UProgressBar;
class UFortuneSubsystem;
class AFortuneCoinPreview;
class UFortuneCollectionWidget;

/** Seritteki tek coin kutusu (hangi index oldugunu bilen buton). */
UCLASS()
class MYPROJECT_API UFortuneCollectionSlot : public UButton
{
	GENERATED_BODY()

public:
	void Setup(UFortuneCollectionWidget* InOwner, int32 InIndex);

	int32 Index = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UImage> Icon;

	UPROPERTY()
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY()
	TObjectPtr<UImage> EquippedBadge;

	UPROPERTY()
	TObjectPtr<UImage> EnchantBadge;

private:
	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<UFortuneCollectionWidget> Owner;
};

UCLASS()
class MYPROJECT_API UFortuneCollectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SelectIndex(int32 NewIndex);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	void BuildSlots();
	void RefreshAll();
	void RefreshSlots();
	void RefreshDetails();
	void ShowToast(const FText& Msg, const FLinearColor& Color);
	UFortuneSubsystem* GetFortune() const;

	UFUNCTION() void OnEquipClicked();
	UFUNCTION() void OnBackClicked();
	UFUNCTION() void OnEnchantClicked();
	UFUNCTION() void OnPrevPage();
	UFUNCTION() void OnNextPage();
	UFUNCTION() void HandleCoinsChanged();

	UPROPERTY() TObjectPtr<UHorizontalBox> SlotRow;
	UPROPERTY() TArray<TObjectPtr<UFortuneCollectionSlot>> Slots;

	UPROPERTY() TObjectPtr<UTextBlock> CountText;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> EarnText;
	UPROPERTY() TObjectPtr<UTextBlock> OddsText;
	UPROPERTY() TObjectPtr<UTextBlock> RarityText;
	UPROPERTY() TObjectPtr<UTextBlock> PageTitle;
	UPROPERTY() TObjectPtr<UTextBlock> PageBody;
	UPROPERTY() TObjectPtr<UTextBlock> LevelText;
	UPROPERTY() TObjectPtr<UTextBlock> XPText;
	UPROPERTY() TObjectPtr<UProgressBar> XPBar;
	UPROPERTY() TObjectPtr<UButton> EquipButton;
	UPROPERTY() TObjectPtr<UTextBlock> EquipText;
	UPROPERTY() TObjectPtr<UButton> EnchantButton;
	UPROPERTY() TObjectPtr<UTextBlock> EnchantText;
	UPROPERTY() TObjectPtr<UTextBlock> ToastText;

	UPROPERTY() TObjectPtr<AFortuneCoinPreview> Preview;

	TArray<FFortuneCollectionEntry> Entries;
	int32 Selected = 0;
	int32 Page = 0;
	float ToastAge = 99.f;
	FName LastPreviewKey;
	TWeakObjectPtr<AActor> PrevViewTarget;
};
