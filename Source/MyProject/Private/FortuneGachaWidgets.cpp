#include "FortuneGachaWidgets.h"
#include "FortuneGachaChest.h"
#include "FortuneSubsystem.h"
#include "FortuneCoinData.h"
#include "FortuneCoinIconStudio.h"
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
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "FortuneUIStyle.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Styling/SlateTypes.h"
#include "FortunePlayerController.h"
#include "Framework/Application/SlateApplication.h"
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

// ============================================================ SUMMON paneli

namespace FortuneGachaUI
{
	const FLinearColor SumGold(1.f, 0.8f, 0.3f, 1.f);
	const FLinearColor SumInner(0.075f, 0.045f, 0.035f, 0.97f);

	FString SummonOdds(float Chance)
	{
		if (Chance <= 0.f) return TEXT("-");
		const int64 OneIn = (int64)FMath::FloorToDouble(1.0 / Chance + 0.5);
		const FString Pct = Chance >= 0.01f ? FString::Printf(TEXT("%%%.1f"), Chance * 100.f) : FString::Printf(TEXT("%%%.3f"), Chance * 100.f);
		return FString::Printf(TEXT("%s   1/%s"), *Pct, *CommaNum(FMath::Max<int64>(1, OneIn)));
	}
}

TSharedRef<SWidget> UFortuneSummonWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneSummonWidget::BuildTree()
{
	using namespace FortuneStyle;
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;

	UImage* Dim = T->ConstructWidget<UImage>(UImage::StaticClass());
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	UBorder* Outer = nullptr;
	UBorder* Panel = StyFrame(T, SumGold, SumInner, Outer, 22.f);
	USizeBox* PanelBox = StySized(T, Outer, 780.f, 0.f);
	PanelBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	PanelRoot = PanelBox;
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBox);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Col = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);

	// ---- Baslik
	UHorizontalBox* Head = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(Head);
	UOverlay* Emblem = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* EmblemBg = T->ConstructWidget<UImage>(UImage::StaticClass());
	EmblemBg->SetBrush(StyRounded(FLinearColor(0.4f, 0.22f, 0.05f, 1.f), SumGold, 30.f, 2.5f, FVector2D(60.f, 60.f)));
	Emblem->AddChildToOverlay(EmblemBg);
	UOverlaySlot* EmT = Emblem->AddChildToOverlay(StyText(T, TEXT("?"), 30, FLinearColor(1.f, 0.95f, 0.7f)));
	EmT->SetHorizontalAlignment(HAlign_Center);
	EmT->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* EmSlot = Head->AddChildToHorizontalBox(StySized(T, Emblem, 60.f, 60.f));
	EmSlot->SetVerticalAlignment(VAlign_Center);
	EmSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));

	UVerticalBox* TitleCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* TitleSlot = Head->AddChildToHorizontalBox(TitleCol);
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	TitleCol->AddChildToVerticalBox(StyText(T, TEXT("DİLEK ÇEŞMESİ"), 30, SumGold, true, 100));

	UBorder* MoneyPill = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	MoneyPill->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.55f), StyWithAlpha(SumGold, 0.7f), 22.f, 1.5f));
	MoneyPill->SetPadding(FMargin(16.f, 6.f));
	MoneyText = StyText(T, TEXT("$ 0"), 22, SumGold);
	MoneyPill->SetContent(MoneyText);
	Head->AddChildToHorizontalBox(MoneyPill)->SetVerticalAlignment(VAlign_Center);

	Col->AddChildToVerticalBox(StyDivider(T, SumGold))->SetPadding(FMargin(0.f, 14.f, 0.f, 10.f));

	// ---- Koleksiyon ilerlemesi + Luck
	UHorizontalBox* InfoRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(InfoRow);
	CollectionText = StyText(T, TEXT("0 / 0"), 14, FLinearColor(0.75f, 0.95f, 1.f));
	InfoRow->AddChildToHorizontalBox(CollectionText)->SetVerticalAlignment(VAlign_Center);
	FStyBar Coll = StyMakeBar(T, FLinearColor(0.35f, 0.8f, 1.f), 8.f);
	CollFill = Coll.FillSlot;
	CollRest = Coll.RestSlot;
	CollFillBorder = Coll.Fill;
	UHorizontalBoxSlot* CollSlot = InfoRow->AddChildToHorizontalBox(StySized(T, Coll.Root, 200.f, 0.f));
	CollSlot->SetVerticalAlignment(VAlign_Center);
	CollSlot->SetPadding(FMargin(10.f, 0.f));
	InfoRow->AddChildToHorizontalBox(T->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LuckText = StyText(T, TEXT(""), 14, FLinearColor(0.55f, 1.f, 0.55f));
	InfoRow->AddChildToHorizontalBox(LuckText)->SetVerticalAlignment(VAlign_Center);

	// ---- Nadirlik dagilimi cubugu
	UBorder* DistBg = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	DistBg->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.5f), FLinearColor(1.f, 1.f, 1.f, 0.08f), 8.f, 1.f));
	DistBg->SetPadding(FMargin(2.f));
	DistRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	DistBg->SetContent(StySized(T, DistRow, 0.f, 12.f));
	Col->AddChildToVerticalBox(DistBg);
	DistBg->SetVisibility(ESlateVisibility::Collapsed);   // sade gorunum: dagilim cubugu gizli

	// ---- Liste
	UBorder* ListBg = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	ListBg->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.35f), FLinearColor(1.f, 1.f, 1.f, 0.06f), 14.f, 1.f));
	ListBg->SetPadding(FMargin(10.f, 8.f));
	USizeBox* ListBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ListBox->SetMaxDesiredHeight(400.f);
	ListBox->SetContent(ListBg);
	Col->AddChildToVerticalBox(ListBox)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	UScrollBox* Scroll = T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	ListBg->SetContent(Scroll);
	List = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Scroll->AddChild(List);

	// ---- Summon butonlari: x1 / x5 / x10 (x10 = 1 bedava)
	UHorizontalBox* Buttons = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* BS = Col->AddChildToVerticalBox(Buttons);
	BS->SetHorizontalAlignment(HAlign_Center);
	BS->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	const int32 Counts[3] = { 1, 5, 10 };
	const FLinearColor Fills[3] = { FLinearColor(0.5f, 0.3f, 0.05f, 1.f), FLinearColor(0.45f, 0.18f, 0.45f, 1.f), FLinearColor(0.6f, 0.12f, 0.1f, 1.f) };
	const FLinearColor Accents[3] = { FLinearColor(1.f, 0.88f, 0.45f), FLinearColor(1.f, 0.6f, 1.f), FLinearColor(1.f, 0.7f, 0.3f) };
	for (int32 i = 0; i < 3; ++i)
	{
		UVerticalBox* BtnCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UTextBlock* Title = StyText(T, FString::Printf(TEXT("SUMMON x%d"), Counts[i]), 21, FLinearColor(1.f, 0.95f, 0.85f), true, 60);
		Title->SetJustification(ETextJustify::Center);
		BtnCol->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* CostT = StyText(T, TEXT("$ 0"), 15, FLinearColor(1.f, 0.9f, 0.6f));
		CostT->SetJustification(ETextJustify::Center);
		BtnCol->AddChildToVerticalBox(CostT)->SetHorizontalAlignment(HAlign_Center);

		UFortuneSummonCountButton* Btn = T->ConstructWidget<UFortuneSummonCountButton>(UFortuneSummonCountButton::StaticClass());
		Btn->Setup(this, Counts[i]);
		Btn->SetStyle(StyButtonStyle(Fills[i], Accents[i], 18.f));
		Btn->SetContent(BtnCol);

		// x10 butonunun ustunde "1 BEDAVA" rozeti
		UOverlay* O = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UOverlaySlot* BtnSlot = O->AddChildToOverlay(StySized(T, Btn, 210.f, 74.f));
		BtnSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		if (Counts[i] == 10)
		{
			UOverlaySlot* Badge = O->AddChildToOverlay(StyChip(T, StyText(T, TEXT("1 BEDAVA!"), 12, FLinearColor(1.f, 0.95f, 0.4f)), FLinearColor(1.f, 0.8f, 0.2f)));
			Badge->SetHorizontalAlignment(HAlign_Right);
			Badge->SetVerticalAlignment(VAlign_Top);
		}
		O->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Buttons->AddChildToHorizontalBox(O)->SetPadding(FMargin(6.f, 0.f));

		SummonButtons.Add(Btn);
		SummonCosts.Add(CostT);
		SummonBoxes.Add(O);
	}
	Affordable.Init(false, 3);

	UButton* Close = StyButton(T, StyText(T, TEXT("KAPAT"), 18, FLinearColor(1.f, 0.6f, 0.6f)), FLinearColor(0.25f, 0.06f, 0.06f, 1.f), FLinearColor(1.f, 0.4f, 0.4f), 14.f);
	Close->OnClicked.AddDynamic(this, &UFortuneSummonWidget::OnCloseClicked);
	UVerticalBoxSlot* CS = Col->AddChildToVerticalBox(StySized(T, Close, 180.f, 42.f));
	CS->SetHorizontalAlignment(HAlign_Center);
	CS->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
}

void UFortuneSummonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
	Age = 0.f;
	FortuneStyle::StyOpenAnim(PanelRoot, 0.f);
	if (UFortuneSubsystem* F = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFortuneSubsystem>() : nullptr)
	{
		F->OnMoneyChanged.AddDynamic(this, &UFortuneSummonWidget::HandleMoneyChanged);
		F->OnCoinsChanged.AddDynamic(this, &UFortuneSummonWidget::HandleCoinsChanged);
	}
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
	{
		FPC->SetMenuOpen(true);
	}
	RebuildList();
	Refresh();
}

void UFortuneSummonWidget::NativeDestruct()
{
	if (UFortuneSubsystem* F = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFortuneSubsystem>() : nullptr)
	{
		F->OnMoneyChanged.RemoveDynamic(this, &UFortuneSummonWidget::HandleMoneyChanged);
		F->OnCoinsChanged.RemoveDynamic(this, &UFortuneSummonWidget::HandleCoinsChanged);
	}
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
	{
		FPC->SetMenuOpen(false);
	}
	Super::NativeDestruct();
}

void UFortuneSummonWidget::HandleMoneyChanged(double NewMoney)
{
	Refresh();
}

void UFortuneSummonWidget::HandleCoinsChanged()
{
	RebuildList();
	Refresh();
}

void UFortuneSummonWidget::Refresh()
{
	using namespace FortuneStyle;
	UFortuneSubsystem* F = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!F || !MoneyText) return;

	const double Money = F->GetMoney();
	MoneyText->SetText(FText::FromString(FString::Printf(TEXT("$ %s"), *UFortuneHUDWidget::FormatMoney(Money).ToString())));
	const float Luck = F->GetLuck();
	LuckText->SetText(Luck > 0.f ? FText::FromString(FString::Printf(TEXT("LUCK +%%%.0f"), Luck * 100.f)) : FText::GetEmpty());

	for (int32 i = 0; i < SummonButtons.Num(); ++i)
	{
		const double Cost = F->GetMultiSummonCost(SummonButtons[i]->Count);
		const bool bOk = Money >= Cost;
		Affordable[i] = bOk;
		SummonButtons[i]->SetIsEnabled(bOk);
		SummonCosts[i]->SetText(FText::FromString(FString::Printf(TEXT("$ %s"), *UFortuneHUDWidget::FormatMoney(Cost).ToString())));
		SummonCosts[i]->SetColorAndOpacity(FSlateColor(bOk ? FLinearColor(1.f, 0.9f, 0.6f) : FLinearColor(1.f, 0.5f, 0.45f)));
		if (!bOk) SummonBoxes[i]->SetRenderScale(FVector2D(1.f, 1.f));
	}
}

void UFortuneSummonWidget::RebuildList()
{
	using namespace FortuneStyle;
	UFortuneSubsystem* F = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!F || !List || !DistRow) return;
	UWidgetTree* T = WidgetTree;

	// Koleksiyon
	int32 OwnedCount = 0;
	int32 Total = 0;
	TMap<FName, int32> OwnedLevels;
	for (const FFortuneCollectionEntry& E : F->GetCollection())
	{
		if (!E.Coin) continue;
		++Total;
		if (E.bOwned)
		{
			++OwnedCount;
			OwnedLevels.Add(E.Coin->GetCoinId(), E.Level);
		}
	}
	CollectionText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), OwnedCount, Total)));
	FStyBar CB;
	CB.FillSlot = CollFill;
	CB.RestSlot = CollRest;
	CB.Fill = CollFillBorder;
	StySetBar(CB, Total > 0 ? float(OwnedCount) / float(Total) : 0.f);

	TArray<UFortuneCoinData*> Coins = F->GetAllCoins();
	Coins.RemoveAll([](const UFortuneCoinData* C) { return C == nullptr; });
	Coins.Sort([](const UFortuneCoinData& A, const UFortuneCoinData& B) { return (uint8)A.Rarity < (uint8)B.Rarity; });

	// Nadirlik basina toplam sans
	TMap<EFortuneRarity, float> RarityChance;
	for (const UFortuneCoinData* C : Coins)
	{
		RarityChance.FindOrAdd(C->Rarity) += F->GetSummonChance(C);
	}

	// Dagilim cubugu (cok kucuk sanslar da gorunsun diye en az %1.5)
	DistRow->ClearChildren();
	for (const TPair<EFortuneRarity, float>& P : RarityChance)
	{
		if (P.Value <= 0.f) continue;
		const FLinearColor RC = UFortuneSubsystem::GetRarityColor(P.Key);
		UImage* Seg = T->ConstructWidget<UImage>(UImage::StaticClass());
		Seg->SetBrush(StyRounded(RC, StyWithAlpha(RC * 1.3f, 0.8f), 4.f, 1.f, FVector2D(8.f, 12.f)));
		Seg->SetToolTipText(FText::FromString(FString::Printf(TEXT("%s  %%%.2f"), *UFortuneSubsystem::GetRarityName(P.Key), P.Value * 100.f)));
		UHorizontalBoxSlot* S = DistRow->AddChildToHorizontalBox(Seg);
		FSlateChildSize Sz(ESlateSizeRule::Fill);
		Sz.Value = FMath::Max(P.Value, 0.015f);
		S->SetSize(Sz);
		S->SetPadding(FMargin(1.f, 0.f));
	}

	// Nadirlik gruplari: baslik (isim + toplam sans) + o nadirlikteki coinlerin kendisi
	List->ClearChildren();
	int32 LastRarity = -1;
	UWrapBox* Grid = nullptr;
	for (const UFortuneCoinData* C : Coins)
	{
		const FLinearColor RC = UFortuneSubsystem::GetRarityColor(C->Rarity);
		const float Chance = F->GetSummonChance(C);
		const int32* Lv = OwnedLevels.Find(C->GetCoinId());
		const bool bOwned = Lv != nullptr;

		if ((int32)C->Rarity != LastRarity)
		{
			LastRarity = (int32)C->Rarity;
			UHorizontalBox* GroupRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			List->AddChildToVerticalBox(GroupRow)->SetPadding(FMargin(0.f, List->GetChildrenCount() > 0 ? 10.f : 0.f, 0.f, 4.f));
			GroupRow->AddChildToHorizontalBox(StyText(T, UFortuneSubsystem::GetRarityName(C->Rarity), 14, RC, true, 80))->SetVerticalAlignment(VAlign_Center);
			UImage* Line = T->ConstructWidget<UImage>(UImage::StaticClass());
			Line->SetBrush(StyRounded(StyWithAlpha(RC, 0.25f), FLinearColor::Transparent, 1.f, 0.f, FVector2D(8.f, 2.f)));
			UHorizontalBoxSlot* LS = GroupRow->AddChildToHorizontalBox(Line);
			LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LS->SetVerticalAlignment(VAlign_Center);
			LS->SetPadding(FMargin(10.f, 0.f));
			const float* GC = RarityChance.Find(C->Rarity);
			const float GV = GC ? *GC : 0.f;
			const FString GS = GV >= 0.01f ? FString::Printf(TEXT("%%%.1f"), GV * 100.f) : FString::Printf(TEXT("%%%.2f"), GV * 100.f);
			GroupRow->AddChildToHorizontalBox(StyText(T, GS, 14, StyWithAlpha(RC, 0.9f)))->SetVerticalAlignment(VAlign_Center);

			Grid = T->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
			Grid->SetInnerSlotPadding(FVector2D(8.f, 8.f));
			List->AddChildToVerticalBox(Grid);
		}

		// Coin karti: coinin kendi yuzu (sahip degilsen karanlik siluet + ?)
		UOverlay* Card = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Ring = T->ConstructWidget<UImage>(UImage::StaticClass());
		Ring->SetBrush(StyRounded(FLinearColor(RC.R * 0.12f, RC.G * 0.12f, RC.B * 0.12f, 0.9f), StyWithAlpha(RC, bOwned ? 0.9f : 0.35f), 34.f, 2.f, FVector2D(68.f, 68.f)));
		Card->AddChildToOverlay(Ring);
		UImage* Face = T->ConstructWidget<UImage>(UImage::StaticClass());
		if (C->HeadsTexture)
		{
			AFortuneCoinIconStudio::ApplyIcon(Face, this, C, bOwned, 66.f);   // coinin 3D gorunusu
		}
		else
		{
			Face->SetBrush(StyRounded(bOwned ? C->IconColor : FLinearColor(0.05f, 0.05f, 0.06f), RC, 29.f, 0.f, FVector2D(58.f, 58.f)));
		}
		UOverlaySlot* FS = Card->AddChildToOverlay(Face);
		FS->SetHorizontalAlignment(HAlign_Center);
		FS->SetVerticalAlignment(VAlign_Center);
		if (!bOwned)
		{
			UOverlaySlot* Q = Card->AddChildToOverlay(StyText(T, TEXT("?"), 22, StyWithAlpha(RC, 0.85f)));
			Q->SetHorizontalAlignment(HAlign_Center);
			Q->SetVerticalAlignment(VAlign_Center);
		}
		else
		{
			// sag altta kucuk seviye rozeti
			UOverlaySlot* LvS = Card->AddChildToOverlay(StyChip(T, StyText(T, FString::Printf(TEXT("%d"), *Lv), 11, FLinearColor(0.55f, 1.f, 0.6f)), FLinearColor(0.45f, 0.95f, 0.5f)));
			LvS->SetHorizontalAlignment(HAlign_Right);
			LvS->SetVerticalAlignment(VAlign_Bottom);
		}
		const FString Tip = FString::Printf(TEXT("%s   %%%.2f"), bOwned ? *C->DisplayName.ToString() : TEXT("???"), Chance * 100.f);
		Card->SetToolTipText(FText::FromString(Tip));
		if (Grid) Grid->AddChildToWrapBox(StySized(T, Card, 68.f, 68.f));
	}
}

void UFortuneSummonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	FortuneStyle::StyOpenAnim(PanelRoot, Age);
	// Parasi yeten SUMMON butonlari nabiz gibi atar (sirayla)
	for (int32 i = 0; i < SummonBoxes.Num(); ++i)
	{
		if (Affordable.IsValidIndex(i) && Affordable[i])
		{
			const float S = 1.f + 0.03f * (0.5f + 0.5f * FMath::Sin(Age * 4.f - i * 0.8f));
			SummonBoxes[i]->SetRenderScale(FVector2D(S, S));
		}
	}
}

void UFortuneSummonCountButton::Setup(UFortuneSummonWidget* InOwner, int32 InCount)
{
	Owner = InOwner;
	Count = InCount;
	OnClicked.AddUniqueDynamic(this, &UFortuneSummonCountButton::HandleClicked);
}

void UFortuneSummonCountButton::HandleClicked()
{
	if (Owner.IsValid())
	{
		Owner->Summon(Count);
	}
}

void UFortuneSummonWidget::Summon(int32 Count)
{
	AFortuneGachaChest* C = Chest.Get();
	if (!C || C->IsBusy()) return;
	RemoveFromParent();             // once paneli kapat (menu modu kapanir)
	if (!C->StartSummon(Count))     // sonra sandik animasyonu baslar
	{
		// para yetmediyse (araya harcama girdiyse) paneli geri ac
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (UFortuneSummonWidget* W = CreateWidget<UFortuneSummonWidget>(PC, UFortuneSummonWidget::StaticClass()))
			{
				W->Chest = C;
				W->AddToViewport(15);
			}
		}
	}
}

void UFortuneSummonWidget::OnCloseClicked()
{
	RemoveFromParent();
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
	BottomSlot->SetPosition(FVector2D(0.f, -130.f));
	BottomSlot->SetAutoSize(true);

	RarityText = GachaText(T, TEXT("COMMON"), 22, FLinearColor::White);
	Bottom->AddChildToVerticalBox(RarityText)->SetHorizontalAlignment(HAlign_Center);
	NameText = GachaText(T, TEXT("Coin"), 52, FLinearColor::White);
	Bottom->AddChildToVerticalBox(NameText)->SetHorizontalAlignment(HAlign_Center);
	PassiveText = GachaText(T, TEXT(""), 18, FLinearColor(1.f, 1.f, 1.f, 0.8f), false);
	PassiveText->SetJustification(ETextJustify::Center);
	Bottom->AddChildToVerticalBox(PassiveText)->SetHorizontalAlignment(HAlign_Center);
	HintText = GachaText(T, TEXT("Enter: Tamam"), 16, FLinearColor(1.f, 1.f, 1.f, 0.6f));
	UVerticalBoxSlot* HintSlot = Bottom->AddChildToVerticalBox(HintText);
	HintSlot->SetHorizontalAlignment(HAlign_Center);
	HintSlot->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
	HintText->SetRenderOpacity(0.f);
	HintText->SetVisibility(ESlateVisibility::Collapsed);
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
	if (UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget))
	{
		if (UFortuneSummonAgainBar* Bar = CreateWidget<UFortuneSummonAgainBar>(GetOwningPlayer(), UFortuneSummonAgainBar::StaticClass()))
		{
			Bar->Chest = InChest;
			UCanvasPanelSlot* S = RootCanvas->AddChildToCanvas(Bar);
			S->SetAnchors(FAnchors(0.5f, 1.f));
			S->SetAlignment(FVector2D(0.5f, 1.f));
			S->SetPosition(FVector2D(0.f, -30.f));
			S->SetAutoSize(true);
		}
	}
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
	return FReply::Handled();   // bos yere tik kapatmaz: alttaki butonlar
}

FReply UFortuneGachaRevealWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Enter || K == EKeys::Escape || K == EKeys::SpaceBar) TryClose();
	return FReply::Handled();
}

// ============================================================ COKLU SUMMON sonuc ekrani

TSharedRef<SWidget> UFortuneGachaMultiRevealWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneGachaMultiRevealWidget::BuildTree()
{
	using namespace FortuneStyle;
	UWidgetTree* T = WidgetTree;
	Canvas = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Canvas;

	UImage* Dim = T->ConstructWidget<UImage>(UImage::StaticClass());
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.3f));
	UCanvasPanelSlot* DimSlot = Canvas->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	HeadlineText = StyText(T, TEXT("SUMMON!"), 46, FLinearColor(1.f, 0.85f, 0.3f), true, 150);
	HeadlineText->SetJustification(ETextJustify::Center);
	HeadlineText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	UCanvasPanelSlot* HS = Canvas->AddChildToCanvas(HeadlineText);
	HS->SetAnchors(FAnchors(0.5f, 0.f));
	HS->SetAlignment(FVector2D(0.5f, 0.f));
	HS->SetPosition(FVector2D(0.f, 40.f));
	HS->SetAutoSize(true);

	UVerticalBox* Bottom = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UCanvasPanelSlot* BSlot = Canvas->AddChildToCanvas(Bottom);
	BSlot->SetAnchors(FAnchors(0.5f, 1.f));
	BSlot->SetAlignment(FVector2D(0.5f, 1.f));
	BSlot->SetPosition(FVector2D(0.f, -112.f));
	BSlot->SetAutoSize(true);
	SummaryText = StyText(T, TEXT(""), 20, FLinearColor::White);
	SummaryText->SetJustification(ETextJustify::Center);
	Bottom->AddChildToVerticalBox(SummaryText)->SetHorizontalAlignment(HAlign_Center);
	HintText = StyText(T, TEXT("Enter: Tamam"), 14, FLinearColor(1.f, 1.f, 1.f, 0.6f));
	UVerticalBoxSlot* HintSlot = Bottom->AddChildToVerticalBox(HintText);
	HintSlot->SetHorizontalAlignment(HAlign_Center);
	HintSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	HintText->SetRenderOpacity(0.f);
	Bottom->SetVisibility(ESlateVisibility::Collapsed);   // ozet ve ipucu yazilari gizli (altta butonlar var)
}

void UFortuneGachaMultiRevealWidget::Setup(AFortuneGachaChest* InChest, const TArray<UFortuneCoinData*>& Coins, const TArray<bool>& Duplicates,
	const TArray<FVector>& WorldPositions, int32 BestIndex, int32 DuplicateXP)
{
	using namespace FortuneStyle;
	Chest = InChest;
	Positions = WorldPositions;
	if (!Canvas) return;
	UWidgetTree* T = WidgetTree;

	int32 NewCount = 0;
	for (int32 i = 0; i < Coins.Num(); ++i)
	{
		const UFortuneCoinData* C = Coins[i];
		if (!C) continue;
		const bool bDup = Duplicates.IsValidIndex(i) && Duplicates[i];
		if (!bDup) ++NewCount;
		const bool bBest = i == BestIndex;
		const FLinearColor RC = UFortuneSubsystem::GetRarityColor(C->Rarity);

		UBorder* Card = T->ConstructWidget<UBorder>(UBorder::StaticClass());
		Card->SetBrush(StyRounded(FLinearColor(RC.R * 0.15f, RC.G * 0.15f, RC.B * 0.15f, 0.88f), RC, 10.f, bBest ? 3.f : 1.5f));
		Card->SetPadding(FMargin(8.f, 4.f));
		UVerticalBox* V = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Card->SetContent(V);
		if (bBest)
		{
			V->AddChildToVerticalBox(StyText(T, TEXT("EN İYİ"), 12, FLinearColor(1.f, 0.9f, 0.4f)))->SetHorizontalAlignment(HAlign_Center);
		}
		V->AddChildToVerticalBox(StyText(T, C->DisplayName.ToString(), bBest ? 16 : 13, FLinearColor::White))->SetHorizontalAlignment(HAlign_Center);
		V->AddChildToVerticalBox(StyText(T, UFortuneSubsystem::GetRarityName(C->Rarity), 11, RC))->SetHorizontalAlignment(HAlign_Center);
		const FString Tag = bDup ? FString::Printf(TEXT("+%d XP"), DuplicateXP) : FString(TEXT("YENİ!"));
		V->AddChildToVerticalBox(StyText(T, Tag, 11, bDup ? FLinearColor(0.55f, 0.85f, 1.f) : FLinearColor(0.5f, 1.f, 0.5f)))->SetHorizontalAlignment(HAlign_Center);

		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Card);
		S->SetAutoSize(true);
		S->SetAlignment(FVector2D(0.5f, 0.f));
		Card->SetRenderTransformPivot(FVector2D(0.5f, 0.f));
		Card->SetRenderScale(FVector2D(0.f, 0.f));
		Labels.Add(Card);
	}

	HeadlineText->SetText(FText::FromString(FString::Printf(TEXT("%dx SUMMON!"), Coins.Num())));
	if (Coins.IsValidIndex(BestIndex) && Coins[BestIndex])
	{
		const UFortuneCoinData* Best = Coins[BestIndex];
		HeadlineText->SetColorAndOpacity(FSlateColor(UFortuneSubsystem::GetRarityColor(Best->Rarity)));
		SummaryText->SetText(FText::FromString(FString::Printf(TEXT("Yeni coin: %d     En iyi: %s (%s)"),
			NewCount, *Best->DisplayName.ToString(), *UFortuneSubsystem::GetRarityName(Best->Rarity))));
	}
	SetIsFocusable(true);
	if (UFortuneSummonAgainBar* Bar = CreateWidget<UFortuneSummonAgainBar>(GetOwningPlayer(), UFortuneSummonAgainBar::StaticClass()))
	{
		Bar->Chest = InChest;
		Bar->Delay = 1.f;
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Bar);
		S->SetAnchors(FAnchors(0.5f, 1.f));
		S->SetAlignment(FVector2D(0.5f, 1.f));
		S->SetPosition(FVector2D(0.f, -26.f));
		S->SetAutoSize(true);
	}
}

void UFortuneGachaMultiRevealWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;

	// Etiketleri coinlerin altina yerlestir (ekrana izdusum)
	APlayerController* PC = GetOwningPlayer();
	for (int32 i = 0; i < Labels.Num() && i < Positions.Num(); ++i)
	{
		FVector2D P;
		if (PC && UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Positions[i], P, false))
		{
			if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Labels[i]->Slot))
			{
				S->SetPosition(P);
			}
		}
		// Sirayla patlayarak belirir
		const float T = FMath::Clamp((Age - 0.08f * i) / 0.25f, 0.f, 1.f);
		const float Back = 1.f + 0.25f * FMath::Sin(T * PI);
		Labels[i]->SetRenderScale(FVector2D(T * Back, T * Back));
	}

	const float Pop = 1.f + 0.5f * FMath::Max(0.f, 1.f - Age * 4.f);
	if (HeadlineText) HeadlineText->SetRenderScale(FVector2D(Pop, Pop));
	if (HintText)
	{
		HintText->SetRenderOpacity(Age < 1.f ? 0.f : 0.45f + 0.45f * FMath::Abs(FMath::Sin(Age * 3.f)));
	}
}

void UFortuneGachaMultiRevealWidget::TryClose()
{
	if (bClosed || Age < 1.f) return;
	bClosed = true;
	RemoveFromParent();
	if (Chest.IsValid())
	{
		Chest->FinishReveal();
	}
}

FReply UFortuneGachaMultiRevealWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return FReply::Handled();   // bos yere tik kapatmaz: alttaki butonlar
}

FReply UFortuneGachaMultiRevealWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Enter || K == EKeys::Escape || K == EKeys::SpaceBar) TryClose();
	return FReply::Handled();
}

// ============================================================ Legendary+ afisi

TSharedRef<SWidget> UFortuneLegendBannerWidget::RebuildWidget()
{
	using namespace FortuneStyle;
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		Flash = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		UCanvasPanelSlot* FS = Root->AddChildToCanvas(Flash);
		FS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		FS->SetOffsets(FMargin(0.f));
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UCanvasPanelSlot* CS = Root->AddChildToCanvas(Col);
		CS->SetAnchors(FAnchors(0.5f, 0.22f));
		CS->SetAlignment(FVector2D(0.5f, 0.5f));
		CS->SetAutoSize(true);
		BigText = StyText(WidgetTree, TEXT("LEGENDARY!"), 86, FLinearColor::White, true, 250);
		BigText->SetJustification(ETextJustify::Center);
		BigText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Col->AddChildToVerticalBox(BigText)->SetHorizontalAlignment(HAlign_Center);
		SubText = StyText(WidgetTree, TEXT(""), 30, FLinearColor::White, true, 80);
		SubText->SetJustification(ETextJustify::Center);
		Col->AddChildToVerticalBox(SubText)->SetHorizontalAlignment(HAlign_Center);
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return Super::RebuildWidget();
}

void UFortuneLegendBannerWidget::Setup(const FString& RarityName, const FLinearColor& Color, const FString& CoinName)
{
	Tint = Color;
	if (BigText)
	{
		BigText->SetText(FText::FromString(RarityName.ToUpper() + TEXT("!")));
		BigText->SetColorAndOpacity(FSlateColor(Color));
	}
	if (SubText) SubText->SetText(FText::FromString(CoinName));
}

void UFortuneLegendBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	// Beyaz patlama -> nadirlik rengine soner
	if (Flash)
	{
		const float A = Age < 0.08f ? Age / 0.08f : FMath::Max(0.f, 1.f - (Age - 0.08f) / 0.7f);
		const FLinearColor C = FMath::Lerp(FLinearColor::White, Tint, FMath::Clamp(Age * 3.f, 0.f, 1.f));
		Flash->SetColorAndOpacity(FLinearColor(C.R, C.G, C.B, 0.85f * A));
	}
	// Yazi: buyuk gelir, sallanarak durur, sonda kaybolur
	const float In = FMath::Clamp((Age - 0.25f) / 0.35f, 0.f, 1.f);
	const float Out = FMath::Clamp((Age - 2.5f) / 0.4f, 0.f, 1.f);
	if (BigText)
	{
		const float S = FMath::Lerp(2.4f, 1.f, FMath::InterpEaseOut(0.f, 1.f, In, 3.f)) + 0.04f * FMath::Sin(Age * 8.f);
		BigText->SetRenderScale(FVector2D(S, S));
		BigText->SetRenderOpacity(In * (1.f - Out));
	}
	if (SubText) SubText->SetRenderOpacity(FMath::Clamp((Age - 0.7f) / 0.4f, 0.f, 1.f) * (1.f - Out));
	if (Age > 3.f) RemoveFromParent();
}

// ============================================================ Summon sahnesi para gostergesi

TSharedRef<SWidget> UFortuneMoneyChipWidget::RebuildWidget()
{
	using namespace FortuneStyle;
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		const FLinearColor Gold(1.f, 0.8f, 0.3f, 1.f);
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		UBorder* Pill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Pill->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.6f), StyWithAlpha(Gold, 0.75f), 22.f, 1.5f));
		Pill->SetPadding(FMargin(18.f, 6.f));
		MoneyText = StyText(WidgetTree, TEXT("$ 0"), 24, Gold);
		Pill->SetContent(MoneyText);
		UCanvasPanelSlot* S = Root->AddChildToCanvas(Pill);
		S->SetAnchors(FAnchors(1.f, 0.f));
		S->SetAlignment(FVector2D(1.f, 0.f));
		S->SetPosition(FVector2D(-28.f, 24.f));
		S->SetAutoSize(true);
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return Super::RebuildWidget();
}

void UFortuneMoneyChipWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const UGameInstance* GI = GetGameInstance();
	const UFortuneSubsystem* F = GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!F || !MoneyText) return;
	const double M = F->GetMoney();
	if (M != Shown)
	{
		Shown = M;
		MoneyText->SetText(FText::FromString(FString::Printf(TEXT("$ %s"), *UFortuneHUDWidget::FormatMoney(M).ToString())));
	}
}

// ============================================================ Sonuc ekrani butonlari

TSharedRef<SWidget> UFortuneSummonAgainBar::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

UButton* UFortuneSummonAgainBar::MakeBarButton(const FString& Label, const FLinearColor& Fill, const FLinearColor& Accent, UTextBlock*& OutText, float Width)
{
	using namespace FortuneStyle;
	OutText = StyText(WidgetTree, Label, 17, FLinearColor::White);
	OutText->SetJustification(ETextJustify::Center);
	UButton* B = StyButton(WidgetTree, OutText, Fill, Accent, 14.f);
	return B;
}

void UFortuneSummonAgainBar::BuildTree()
{
	using namespace FortuneStyle;
	if (const AFortuneGachaChest* C = Chest.Get())
	{
		AgainCount = C->GetLastCount();
	}
	OtherCount = (AgainCount == 10) ? 1 : 10;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AgainRow"));
	WidgetTree->RootWidget = Row;

	UTextBlock* T1 = nullptr; UTextBlock* T2 = nullptr; UTextBlock* T3 = nullptr;
	AgainBtn = MakeBarButton(TEXT("TEKRAR"), FLinearColor(0.07f, 0.2f, 0.18f, 0.95f), FLinearColor(0.35f, 1.f, 0.85f), T1, 200.f);
	OtherBtn = MakeBarButton(TEXT("x10"), FLinearColor(0.2f, 0.12f, 0.03f, 0.95f), FLinearColor(1.f, 0.78f, 0.3f), T2, 200.f);
	OkBtn = MakeBarButton(TEXT("TAMAM"), FLinearColor(0.08f, 0.08f, 0.09f, 0.95f), FLinearColor(1.f, 1.f, 1.f, 0.8f), T3, 160.f);
	AgainText = T1; OtherText = T2; OkText = T3;
	AgainBtn->OnClicked.AddDynamic(this, &UFortuneSummonAgainBar::OnAgain);
	OtherBtn->OnClicked.AddDynamic(this, &UFortuneSummonAgainBar::OnOther);
	OkBtn->OnClicked.AddDynamic(this, &UFortuneSummonAgainBar::OnOk);
	// Sira: TAMAM (sol) - diger - x10 (en sag)
	const bool bAgainIsTen = AgainCount == 10;
	UButton* Btns[] = { OkBtn, bAgainIsTen ? OtherBtn.Get() : AgainBtn.Get(), bAgainIsTen ? AgainBtn.Get() : OtherBtn.Get() };
	const float Widths[] = { 160.f, 210.f, 210.f };
	for (int32 i = 0; i < 3; ++i)
	{
		UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(StySized(WidgetTree, Btns[i], Widths[i], 62.f));
		S->SetPadding(FMargin(8.f, 0.f));
		S->SetVerticalAlignment(VAlign_Center);
	}
	Row->SetRenderOpacity(0.f);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UFortuneSummonAgainBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	const float A = FMath::Clamp((Age - Delay) / 0.3f, 0.f, 1.f);
	if (UWidget* Root = WidgetTree ? WidgetTree->RootWidget : nullptr)
	{
		Root->SetRenderOpacity(A);
		Root->SetRenderTranslation(FVector2D(0.f, (1.f - A) * 20.f));
	}

	// Fiyat ve para yetiyor mu (her karede, ucuz)
	const UGameInstance* GI = GetGameInstance();
	const UFortuneSubsystem* F = GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!F) return;
	const bool bReady = A >= 1.f && !bUsed;
	auto Setup = [&](UButton* B, UTextBlock* T, int32 Count, const TCHAR* Title)
	{
		if (!B || !T) return;
		const double Cost = F->GetMultiSummonCost(Count);
		const bool bCan = F->GetMoney() >= Cost;
		T->SetText(FText::FromString(FString::Printf(TEXT("%s x%d\n%s"), Title, Count, *UFortuneHUDWidget::FormatMoney(Cost).ToString())));
		B->SetIsEnabled(bReady && bCan);
	};
	Setup(AgainBtn, AgainText, AgainCount, TEXT("TEKRAR"));
	Setup(OtherBtn, OtherText, OtherCount, TEXT("SUMMON"));
	if (OkBtn) OkBtn->SetIsEnabled(A >= 1.f && !bUsed);
}

void UFortuneSummonAgainBar::OnAgain()
{
	if (bUsed || !Chest.IsValid()) return;
	bUsed = true;
	if (!Chest->RepeatSummon(AgainCount)) bUsed = false;
}

void UFortuneSummonAgainBar::OnOther()
{
	if (bUsed || !Chest.IsValid()) return;
	bUsed = true;
	if (!Chest->RepeatSummon(OtherCount)) bUsed = false;
}

void UFortuneSummonAgainBar::OnOk()
{
	if (bUsed || !Chest.IsValid()) return;
	bUsed = true;
	Chest->CloseRevealAndReturn();
}
