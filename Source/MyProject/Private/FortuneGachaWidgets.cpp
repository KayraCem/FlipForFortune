#include "FortuneGachaWidgets.h"
#include "FortuneGachaChest.h"
#include "FortuneSubsystem.h"
#include "FortuneCoinData.h"
#include "FortuneHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Engine/GameInstance.h"

namespace FortuneGachaUI
{
	FSlateBrush GachaRounded(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float OutlineWidth, FVector2D Size = FVector2D(32.f, 32.f))
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.ImageSize = FVector2f(Size);
		B.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	UTextBlock* GachaText(UWidgetTree* Tree, const FString& Str, int32 Size, const FLinearColor& Color, bool bBold = true)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		F.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		T->SetFont(F);
		T->SetText(FText::FromString(Str));
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetShadowOffset(FVector2D(1.5f, 1.5f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		return T;
	}

	FString CommaNum(int64 N)
	{
		FString S = FString::Printf(TEXT("%lld"), N);
		for (int32 i = S.Len() - 3; i > 0; i -= 3)
		{
			S.InsertAt(i, TEXT(","));
		}
		return S;
	}
}

using namespace FortuneGachaUI;

// ============================================================ Sans listesi

TSharedRef<SWidget> UFortuneGachaRatesWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneGachaRatesWidget::BuildTree()
{
	UWidgetTree* T = WidgetTree;
	UBorder* Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrush(GachaRounded(FLinearColor(0.04f, 0.03f, 0.03f, 0.88f), FLinearColor(1.f, 0.72f, 0.25f, 0.8f), 14.f, 2.f));
	Panel->SetPadding(FMargin(16.f, 10.f));
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	T->RootWidget = Panel;

	UVerticalBox* Col = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);

	UHorizontalBox* Head = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(Head)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	Head->AddChildToHorizontalBox(GachaText(T, TEXT("HAZİNE SANDIĞI"), 18, FLinearColor(1.f, 0.78f, 0.3f)));
	Head->AddChildToHorizontalBox(T->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	CostText = GachaText(T, TEXT("$ 100"), 18, FLinearColor::White);
	UHorizontalBoxSlot* CostSlot = Head->AddChildToHorizontalBox(CostText);
	CostSlot->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));

	List = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Col->AddChildToVerticalBox(List);

	Col->AddChildToVerticalBox(GachaText(T, TEXT("Tıkla ve aç!"), 14, FLinearColor(1.f, 1.f, 1.f, 0.6f), false))
		->SetHorizontalAlignment(HAlign_Center);
}

void UFortuneGachaRatesWidget::Refresh()
{
	if (!WidgetTree || !List) return;
	const UGameInstance* GI = GetGameInstance();
	UFortuneSubsystem* F = GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!F) return;

	const double Cost = F->GetSummonCost();
	CostText->SetText(FText::FromString(FString::Printf(TEXT("$ %s"), *UFortuneHUDWidget::FormatMoney(Cost).ToString())));
	CostText->SetColorAndOpacity(FSlateColor(F->GetMoney() >= Cost ? FLinearColor(0.5f, 1.f, 0.5f) : FLinearColor(1.f, 0.45f, 0.45f)));

	List->ClearChildren();
	TArray<UFortuneCoinData*> Coins = F->GetAllCoins();
	Coins.Sort([](const UFortuneCoinData& A, const UFortuneCoinData& B) { return (uint8)A.Rarity < (uint8)B.Rarity; });

	for (const UFortuneCoinData* C : Coins)
	{
		if (!C) continue;
		const FLinearColor RC = UFortuneSubsystem::GetRarityColor(C->Rarity);
		const float Chance = F->GetSummonChance(C);
		const bool bOwned = F->IsCoinOwned(C->GetCoinId());

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		List->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 2.f));

		UImage* Dot = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Dot->SetBrush(GachaRounded(RC, FLinearColor(0.f, 0.f, 0.f, 0.5f), 6.f, 1.f, FVector2D(12.f, 12.f)));
		UHorizontalBoxSlot* DotSlot = Row->AddChildToHorizontalBox(Dot);
		DotSlot->SetVerticalAlignment(VAlign_Center);
		DotSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

		USizeBox* RarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		RarBox->SetWidthOverride(112.f);
		RarBox->SetContent(GachaText(WidgetTree, UFortuneSubsystem::GetRarityName(C->Rarity), 14, RC));
		Row->AddChildToHorizontalBox(RarBox)->SetVerticalAlignment(VAlign_Center);

		const FString Name = bOwned ? C->DisplayName.ToString() : FString(TEXT("???"));
		UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(GachaText(WidgetTree, Name, 14, FLinearColor(1.f, 1.f, 1.f, bOwned ? 0.95f : 0.5f), false));
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);

		const int64 OneIn = Chance > 0.f ? (int64)FMath::FloorToDouble(1.0 / Chance + 0.5) : 0;
		const FString Odds = Chance > 0.f ? FString::Printf(TEXT("%%%.1f   1/%s"), Chance * 100.f, *CommaNum(FMath::Max<int64>(1, OneIn))) : FString(TEXT("-"));
		UHorizontalBoxSlot* OddsSlot = Row->AddChildToHorizontalBox(GachaText(WidgetTree, Odds, 14, FLinearColor(1.f, 0.95f, 0.5f)));
		OddsSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
		OddsSlot->SetVerticalAlignment(VAlign_Center);
	}
}

// ============================================================ YENI! ekrani

TSharedRef<SWidget> UFortuneGachaRevealWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneGachaRevealWidget::BuildTree()
{
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;

	// Tum ekrani kaplayan (tiklamayi yakalayan) hafif karartma
	UImage* Dim = T->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Dim"));
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.35f));
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	// Ust: YENI! / TEKRAR!
	HeadlineText = GachaText(T, TEXT("YENİ COIN!"), 46, FLinearColor(1.f, 0.85f, 0.3f));
	HeadlineText->SetJustification(ETextJustify::Center);
	UCanvasPanelSlot* HeadSlot = Root->AddChildToCanvas(HeadlineText);
	HeadSlot->SetAnchors(FAnchors(0.5f, 0.f));
	HeadSlot->SetAlignment(FVector2D(0.5f, 0.f));
	HeadSlot->SetPosition(FVector2D(0.f, 60.f));
	HeadSlot->SetAutoSize(true);

	// Alt: isim, nadirlik, pasif, ipucu
	UVerticalBox* Bottom = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UCanvasPanelSlot* BottomSlot = Root->AddChildToCanvas(Bottom);
	BottomSlot->SetAnchors(FAnchors(0.5f, 1.f));
	BottomSlot->SetAlignment(FVector2D(0.5f, 1.f));
	BottomSlot->SetPosition(FVector2D(0.f, -50.f));
	BottomSlot->SetAutoSize(true);

	RarityText = GachaText(T, TEXT("COMMON"), 22, FLinearColor::White);
	Bottom->AddChildToVerticalBox(RarityText)->SetHorizontalAlignment(HAlign_Center);
	NameText = GachaText(T, TEXT("Coin"), 52, FLinearColor::White);
	Bottom->AddChildToVerticalBox(NameText)->SetHorizontalAlignment(HAlign_Center);
	PassiveText = GachaText(T, TEXT(""), 18, FLinearColor(1.f, 1.f, 1.f, 0.8f), false);
	PassiveText->SetJustification(ETextJustify::Center);
	Bottom->AddChildToVerticalBox(PassiveText)->SetHorizontalAlignment(HAlign_Center);
	HintText = GachaText(T, TEXT("Kapatmak için tıkla"), 20, FLinearColor(1.f, 1.f, 1.f, 0.85f));
	UVerticalBoxSlot* HintSlot = Bottom->AddChildToVerticalBox(HintText);
	HintSlot->SetHorizontalAlignment(HAlign_Center);
	HintSlot->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
	HintText->SetRenderOpacity(0.f);
}

void UFortuneGachaRevealWidget::Setup(AFortuneGachaChest* InChest, const UFortuneCoinData* Coin, bool bDuplicate, int32 DuplicateXP)
{
	Chest = InChest;
	if (!Coin || !HeadlineText) return;
	const FLinearColor RC = UFortuneSubsystem::GetRarityColor(Coin->Rarity);

	HeadlineText->SetText(FText::FromString(bDuplicate ? FString::Printf(TEXT("TEKRAR!  +%d XP"), DuplicateXP) : FString(TEXT("YENİ COIN!"))));
	HeadlineText->SetColorAndOpacity(FSlateColor(bDuplicate ? FLinearColor(0.5f, 0.85f, 1.f) : FLinearColor(1.f, 0.85f, 0.3f)));
	NameText->SetText(Coin->DisplayName);
	NameText->SetColorAndOpacity(FSlateColor(RC));
	RarityText->SetText(FText::FromString(UFortuneSubsystem::GetRarityName(Coin->Rarity)));
	RarityText->SetColorAndOpacity(FSlateColor(RC));
	PassiveText->SetText(Coin->PassiveDescription.IsEmpty() ? FText::GetEmpty() : FText::Format(FText::FromString(TEXT("Pasif: {0}")), Coin->PassiveDescription));
	SetIsFocusable(true);
}

void UFortuneGachaRevealWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;

	// Baslik buyuyerek gelir, isim asagidan kayar, ipucu yanip soner
	const float Pop = 1.f + 0.6f * FMath::Max(0.f, 1.f - Age * 4.f);
	if (HeadlineText) HeadlineText->SetRenderScale(FVector2D(Pop, Pop));
	if (NameText)
	{
		const float A = FMath::Clamp(Age * 3.f, 0.f, 1.f);
		NameText->SetRenderOpacity(A);
		NameText->SetRenderTranslation(FVector2D(0.f, (1.f - A) * 30.f));
	}
	if (HintText)
	{
		HintText->SetRenderOpacity(Age < 0.8f ? 0.f : 0.45f + 0.45f * FMath::Abs(FMath::Sin(Age * 3.f)));
	}
}

void UFortuneGachaRevealWidget::TryClose()
{
	if (bClosed || Age < 0.8f) return;
	bClosed = true;
	RemoveFromParent();
	if (Chest.IsValid())
	{
		Chest->FinishReveal();
	}
}

FReply UFortuneGachaRevealWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TryClose();
	return FReply::Handled();
}

FReply UFortuneGachaRevealWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	TryClose();
	return FReply::Handled();
}
