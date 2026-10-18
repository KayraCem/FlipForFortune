// Flip for Fortune - Envanter ekrani (sol taraftaki sandiga tiklayinca).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "FortuneTypes.h"
#include "FortuneInventoryWidget.generated.h"

class UTextBlock;
class UImage;
class UUniformGridPanel;
class UFortuneInventoryWidget;

UCLASS()
class MYPROJECT_API UFortuneInventorySlot : public UButton
{
	GENERATED_BODY()

public:
	void Setup(UFortuneInventoryWidget* InOwner, int32 InIndex);
	int32 Index = 0;

	UPROPERTY() TObjectPtr<UImage> Icon;
	UPROPERTY() TObjectPtr<UImage> IconLines;
	UPROPERTY() TObjectPtr<UTextBlock> CountText;

private:
	UFUNCTION() void HandleClicked();
	TWeakObjectPtr<UFortuneInventoryWidget> Owner;
};

UCLASS()
class MYPROJECT_API UFortuneInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SelectSlot(int32 Index);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	struct FItemView
	{
		EFortuneItemType Type;
		int32 Count = 0;
	};

	void BuildTree();
	void Refresh();
	static FString ItemName(EFortuneItemType T);
	static FString ItemDesc(EFortuneItemType T);
	static FString ItemRarity(EFortuneItemType T);
	static FLinearColor ItemColor(EFortuneItemType T);

	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void HandleChanged();

	UPROPERTY() TArray<TObjectPtr<UFortuneInventorySlot>> Slots;
	UPROPERTY() TObjectPtr<UTextBlock> CapacityText;
	UPROPERTY() TObjectPtr<UTextBlock> MoneyText;
	UPROPERTY() TObjectPtr<UTextBlock> DetailName;
	UPROPERTY() TObjectPtr<UTextBlock> DetailRarity;
	UPROPERTY() TObjectPtr<UTextBlock> DetailCount;
	UPROPERTY() TObjectPtr<UTextBlock> DetailDesc;
	UPROPERTY() TObjectPtr<UImage> DetailIcon;

	TArray<FItemView> Items;
	int32 Selected = 0;
	static constexpr int32 Columns = 6;
	static constexpr int32 Rows = 4;
};
