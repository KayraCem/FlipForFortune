#include "FortuneUpgradeWidget.h"
#include "FortuneSubsystem.h"
#include "FortuneHUDWidget.h"
#include "FortunePlayerController.h"
#include "FortuneUIStyle.h"
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
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Styling/SlateTypes.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"

using namespace FortuneStyle;

namespace FortuneUpgUI
{
	const FLinearColor UpgGold(1.f, 0.78f, 0.3f, 1.f);
	const FLinearColor UpgInner(0.055f, 0.045f, 0.04f, 0.97f);
	const FLinearColor UpgRow(0.12f, 0.105f, 0.095f, 0.95f);

	struct FUpgInfo { EFortuneUpgradeType Type; const TCHAR* Letter; FLinearColor Color; };
	const FUpgInfo Infos[] = {
		{ EFortuneUpgradeType::MoneyEarn,        TEXT("$"),  FLinearColor(0.35f, 0.9f, 0.4f) },
		{ EFortuneUpgradeType::HeadChance,       TEXT("Y"),  FLinearColor(1.f, 0.85f, 0.3f) },
		{ EFortuneUpgradeType::CoinFlipSpeed,    TEXT(">>"), FLinearColor(0.35f, 0.7f, 1.f) },
		{ EFortuneUpgradeType::CriticalChance,   TEXT("K"),  FLinearColor(1.f, 0.5f, 0.2f) },
		{ EFortuneUpgradeType::CriticalMultiply, TEXT("x"),  FLinearColor(1.f, 0.3f, 0.35f) },
	};

	FString FormatBonus(EFortuneUpgradeType Type, float Bonus)
	{
		switch (Type)
		{
		case EFortuneUpgradeType::MoneyEarn:        return FString::Printf(TEXT("+%s"), *UFortuneHUDWidget::FormatMoney(Bonus).ToString());
		case EFortuneUpgradeType::CriticalMultiply: return FString::Printf(TEXT("+%.1fx"), Bonus);
		default:                                    return FString::Printf(TEXT("+%%%.0f"), Bonus * 100.f);
		}
	}

	const TCHAR* ModeNames[3] = { TEXT("x1"), TEXT("x10"), TEXT("MAKS") };
	const int32 ModeCounts[3] = { 1, 10, 0 };
}

using namespace FortuneUpgUI;

// ============================================================ Buton

void UFortuneUpgradeBuyButton::Setup(UFortuneUpgradeWidget* InOwner, EFortuneUpgradeType InType)
{
	Owner = InOwner;
	Type = InType;
	OnClicked.AddUniqueDynamic(this, &UFortuneUpgradeBuyButton::HandleClicked);
}

void UFortuneUpgradeBuyButton::HandleClicked()
{
	if (Owner.IsValid())
	{
		Owner->Buy(Type);
	}
}

// ============================================================ Panel

UFortuneSubsystem* UFortuneUpgradeWidget::GetFortune() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
}

TSharedRef<SWidget> UFortuneUpgradeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneUpgradeWidget::BuildTree()
{
	UWidgetTree* T = WidgetTree;
	const UFortuneSubsystem* F = GetFortune();

	UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;

	UImage* Dim = T->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Dim"));
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	UBorder* Outer = nullptr;
	UBorder* Panel = StyFrame(T, UpgGold, UpgInner, Outer, 22.f);
	USizeBox* PanelBox = StySized(T, Outer, 900.f, 0.f);
	PanelBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	PanelRoot = PanelBox;
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBox);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Col = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);

	// ---- Baslik: amblem + yazi + para
	UHorizontalBox* Header = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(Header);

	UOverlay* Emblem = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* EmblemBg = T->ConstructWidget<UImage>(UImage::StaticClass());
	EmblemBg->SetBrush(StyRounded(FLinearColor(0.32f, 0.2f, 0.06f, 1.f), UpgGold, 16.f, 2.5f, FVector2D(60.f, 60.f)));
	Emblem->AddChildToOverlay(EmblemBg);
	UOverlaySlot* EmblemT = Emblem->AddChildToOverlay(StyText(T, TEXT("UP"), 24, UpgGold));
	EmblemT->SetHorizontalAlignment(HAlign_Center);
	EmblemT->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* EmblemSlot = Header->AddChildToHorizontalBox(StySized(T, Emblem, 60.f, 60.f));
	EmblemSlot->SetVerticalAlignment(VAlign_Center);
	EmblemSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));

	UVerticalBox* TitleCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(TitleCol);
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	TitleCol->AddChildToVerticalBox(StyText(T, TEXT("YÜKSELTMELER"), 32, UpgGold, true, 120));
	TitleCol->AddChildToVerticalBox(StyText(T, TEXT("Örste paranı güce dönüştür"), 14, FLinearColor(1.f, 1.f, 1.f, 0.55f), false));

	UBorder* MoneyPill = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	MoneyPill->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.55f), StyWithAlpha(UpgGold, 0.7f), 22.f, 1.5f));
	MoneyPill->SetPadding(FMargin(14.f, 6.f, 20.f, 6.f));
	UHorizontalBox* MoneyRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	MoneyPill->SetContent(MoneyRow);
	UImage* CoinIcon = T->ConstructWidget<UImage>(UImage::StaticClass());
	CoinIcon->SetBrush(StyRounded(UpgGold, FLinearColor(0.45f, 0.28f, 0.05f), 14.f, 2.5f, FVector2D(28.f, 28.f)));
	MoneyRow->AddChildToHorizontalBox(CoinIcon)->SetVerticalAlignment(VAlign_Center);
	MoneyText = StyText(T, TEXT("0"), 26, UpgGold);
	MoneyText->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	UHorizontalBoxSlot* MoneySlot = MoneyRow->AddChildToHorizontalBox(MoneyText);
	MoneySlot->SetVerticalAlignment(VAlign_Center);
	MoneySlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	Header->AddChildToHorizontalBox(MoneyPill)->SetVerticalAlignment(VAlign_Center);

	Col->AddChildToVerticalBox(StyDivider(T, UpgGold))->SetPadding(FMargin(0.f, 14.f, 0.f, 10.f));

	// ---- Guncel degerler (chip) + satin alma miktari
	UHorizontalBox* InfoRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(InfoRow);
	for (int32 i = 0; i < 4; ++i)
	{
		const FLinearColor C = i == 0 ? Infos[1].Color : (i == 1 ? Infos[3].Color : (i == 2 ? Infos[4].Color : Infos[2].Color));
		UTextBlock* L = StyText(T, TEXT(""), 14, C);
		StatChips.Add(L);
		InfoRow->AddChildToHorizontalBox(StyChip(T, L, C))->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	InfoRow->AddChildToHorizontalBox(T->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UHorizontalBoxSlot* ModeLabel = InfoRow->AddChildToHorizontalBox(StyText(T, TEXT("AL:"), 14, FLinearColor(1.f, 1.f, 1.f, 0.55f)));
	ModeLabel->SetVerticalAlignment(VAlign_Center);
	ModeLabel->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	for (int32 m = 0; m < 3; ++m)
	{
		UButton* MB = StyButton(T, StyText(T, ModeNames[m], 15, FLinearColor::White), FLinearColor(0.16f, 0.14f, 0.12f, 1.f), UpgGold, 10.f);
		if (m == 0) MB->OnClicked.AddDynamic(this, &UFortuneUpgradeWidget::OnModeX1);
		if (m == 1) MB->OnClicked.AddDynamic(this, &UFortuneUpgradeWidget::OnModeX10);
		if (m == 2) MB->OnClicked.AddDynamic(this, &UFortuneUpgradeWidget::OnModeMax);
		ModeButtons.Add(MB);
		InfoRow->AddChildToHorizontalBox(StySized(T, MB, 64.f, 32.f))->SetPadding(FMargin(3.f, 0.f));
	}

	Col->AddChildToVerticalBox(T->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetPadding(FMargin(0.f, 6.f));

	// ---- Satirlar
	for (const FUpgInfo& Info : Infos)
	{
		const FFortuneUpgradeConfig Cfg = F ? F->GetUpgradeConfig(Info.Type) : FFortuneUpgradeConfig();

		UBorder* RowBorder = T->ConstructWidget<UBorder>(UBorder::StaticClass());
		RowBorder->SetBrush(StyRounded(UpgRow, StyWithAlpha(Info.Color, 0.25f), 16.f, 1.5f));
		RowBorder->SetPadding(FMargin(14.f, 10.f));
		Col->AddChildToVerticalBox(RowBorder)->SetPadding(FMargin(0.f, 4.f));

		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		RowBorder->SetContent(Row);

		// Simge
		UOverlay* IconO = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* IconBg = T->ConstructWidget<UImage>(UImage::StaticClass());
		IconBg->SetBrush(StyRounded(Info.Color * 0.3f + FLinearColor(0.f, 0.f, 0.f, 0.7f), Info.Color, 14.f, 2.f, FVector2D(56.f, 56.f)));
		UOverlaySlot* BgSlot = IconO->AddChildToOverlay(IconBg);
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
		UOverlaySlot* LetterSlot = IconO->AddChildToOverlay(StyText(T, Info.Letter, 24, Info.Color));
		LetterSlot->SetHorizontalAlignment(HAlign_Center);
		LetterSlot->SetVerticalAlignment(VAlign_Center);
		Row->AddChildToHorizontalBox(StySized(T, IconO, 56.f, 56.f))->SetVerticalAlignment(VAlign_Center);

		// Isim + aciklama + seviye cubugu
		UVerticalBox* NameCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameCol);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(14.f, 0.f));
		UHorizontalBox* NameLine = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		NameCol->AddChildToVerticalBox(NameLine);
		NameLine->AddChildToHorizontalBox(StyText(T, Cfg.DisplayName.ToString(), 21, FLinearColor::White))->SetVerticalAlignment(VAlign_Center);
		UTextBlock* LevelT = StyText(T, TEXT("Lv 0"), 15, Info.Color);
		LevelT->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		UHorizontalBoxSlot* LvSlot = NameLine->AddChildToHorizontalBox(LevelT);
		LvSlot->SetVerticalAlignment(VAlign_Center);
		LvSlot->SetPadding(FMargin(12.f, 2.f, 0.f, 0.f));
		UTextBlock* Desc = StyText(T, Cfg.Description.ToString(), 14, FLinearColor(1.f, 1.f, 1.f, 0.6f), false);
		Desc->SetAutoWrapText(true);
		NameCol->AddChildToVerticalBox(Desc);
		FStyBar Bar = StyMakeBar(T, Info.Color, 7.f);
		NameCol->AddChildToVerticalBox(Bar.Root)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

		// Deger
		UTextBlock* ValueT = StyText(T, TEXT(""), 16, FLinearColor(1.f, 1.f, 1.f, 0.9f));
		ValueT->SetJustification(ETextJustify::Right);
		UHorizontalBoxSlot* ValSlot = Row->AddChildToHorizontalBox(StySized(T, ValueT, 170.f, 0.f));
		ValSlot->SetVerticalAlignment(VAlign_Center);

		// AL butonu: ust satir fiyat, alt satir kac seviye
		UVerticalBox* BtnCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UTextBlock* CostT = StyText(T, TEXT("0"), 19, FLinearColor::White);
		CostT->SetJustification(ETextJustify::Center);
		BtnCol->AddChildToVerticalBox(CostT)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* SubT = StyText(T, TEXT("+1 Lv"), 12, FLinearColor(0.7f, 1.f, 0.7f, 0.85f));
		SubT->SetJustification(ETextJustify::Center);
		BtnCol->AddChildToVerticalBox(SubT)->SetHorizontalAlignment(HAlign_Center);
		UFortuneUpgradeBuyButton* Btn = T->ConstructWidget<UFortuneUpgradeBuyButton>(UFortuneUpgradeBuyButton::StaticClass());
		Btn->Setup(this, Info.Type);
		Btn->SetStyle(StyButtonStyle(FLinearColor(0.16f, 0.34f, 0.12f, 1.f), FLinearColor(0.5f, 1.f, 0.45f), 12.f));
		Btn->SetContent(BtnCol);
		UHorizontalBoxSlot* BtnSlot = Row->AddChildToHorizontalBox(StySized(T, Btn, 156.f, 58.f));
		BtnSlot->SetVerticalAlignment(VAlign_Center);
		BtnSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));

		FRow R;
		R.Type = Info.Type;
		R.Color = Info.Color;
		R.RowBorder = RowBorder;
		R.Level = LevelT;
		R.Value = ValueT;
		R.Cost = CostT;
		R.CostSub = SubT;
		R.Button = Btn;
		R.BarFill = Bar.FillSlot;
		R.BarRest = Bar.RestSlot;
		R.BarFillBorder = Bar.Fill;
		Rows.Add(R);
		KeepAlive.Add(RowBorder);
		KeepAlive.Add(LevelT);
		KeepAlive.Add(ValueT);
		KeepAlive.Add(CostT);
		KeepAlive.Add(SubT);
		KeepAlive.Add(Btn);
		KeepAlive.Add(Bar.FillSlot);
		KeepAlive.Add(Bar.RestSlot);
		KeepAlive.Add(Bar.Fill);
	}

	// ---- Kapat
	UButton* Close = StyButton(T, StyText(T, TEXT("KAPAT"), 20, FLinearColor(1.f, 0.6f, 0.6f)), FLinearColor(0.28f, 0.07f, 0.07f, 1.f), FLinearColor(1.f, 0.4f, 0.4f));
	Close->OnClicked.AddDynamic(this, &UFortuneUpgradeWidget::OnCloseClicked);
	UVerticalBoxSlot* CloseSlot = Col->AddChildToVerticalBox(StySized(T, Close, 220.f, 48.f));
	CloseSlot->SetHorizontalAlignment(HAlign_Center);
	CloseSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	BuyMode = 0;
	for (int32 m = 0; m < ModeButtons.Num(); ++m)
	{
		ModeButtons[m]->SetStyle(StyButtonStyle(m == 0 ? FLinearColor(0.45f, 0.3f, 0.06f, 1.f) : FLinearColor(0.16f, 0.14f, 0.12f, 1.f),
			m == 0 ? FLinearColor(1.f, 0.9f, 0.5f) : UpgGold, 10.f));
	}
}

void UFortuneUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
	Age = 0.f;
	StyOpenAnim(PanelRoot, 0.f);
	if (UFortuneSubsystem* F = GetFortune())
	{
		F->OnUpgradesChanged.AddDynamic(this, &UFortuneUpgradeWidget::HandleChanged);
		F->OnMoneyChanged.AddDynamic(this, &UFortuneUpgradeWidget::HandleMoneyChanged);
	}
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
	{
		FPC->SetMenuOpen(true);
	}
	Refresh();
}

void UFortuneUpgradeWidget::NativeDestruct()
{
	if (UFortuneSubsystem* F = GetFortune())
	{
		F->OnUpgradesChanged.RemoveDynamic(this, &UFortuneUpgradeWidget::HandleChanged);
		F->OnMoneyChanged.RemoveDynamic(this, &UFortuneUpgradeWidget::HandleMoneyChanged);
	}
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
	{
		FPC->SetMenuOpen(false);
	}
	Super::NativeDestruct();
}

void UFortuneUpgradeWidget::HandleChanged()
{
	Refresh();
}

void UFortuneUpgradeWidget::HandleMoneyChanged(double NewMoney)
{
	Refresh();
}

void UFortuneUpgradeWidget::OnModeX1()  { SetBuyMode(0); }
void UFortuneUpgradeWidget::OnModeX10() { SetBuyMode(1); }
void UFortuneUpgradeWidget::OnModeMax() { SetBuyMode(2); }

void UFortuneUpgradeWidget::SetBuyMode(int32 Mode)
{
	BuyMode = FMath::Clamp(Mode, 0, 2);
	for (int32 m = 0; m < ModeButtons.Num(); ++m)
	{
		const bool bSel = m == BuyMode;
		ModeButtons[m]->SetStyle(StyButtonStyle(bSel ? FLinearColor(0.45f, 0.3f, 0.06f, 1.f) : FLinearColor(0.16f, 0.14f, 0.12f, 1.f),
			bSel ? FLinearColor(1.f, 0.9f, 0.5f) : UpgGold, 10.f));
	}
	Refresh();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void UFortuneUpgradeWidget::Refresh()
{
	UFortuneSubsystem* F = GetFortune();
	if (!F) return;
	const double Money = F->GetMoney();
	if (MoneyText)
	{
		MoneyText->SetText(UFortuneHUDWidget::FormatMoney(Money));
	}
	if (StatChips.Num() >= 4)
	{
		StatChips[0]->SetText(FText::FromString(FString::Printf(TEXT("YAZI %%%.0f"), F->GetHeadChance() * 100.f)));
		StatChips[1]->SetText(FText::FromString(FString::Printf(TEXT("KRİTİK %%%.0f"), F->GetCritChance() * 100.f)));
		StatChips[2]->SetText(FText::FromString(FString::Printf(TEXT("ÇARPAN x%.1f"), F->GetCritMultiplier())));
		StatChips[3]->SetText(FText::FromString(FString::Printf(TEXT("ATIŞ %.2f sn"), F->GetBaseFlipDuration())));
	}

	for (FRow& R : Rows)
	{
		const FFortuneUpgradeConfig Cfg = F->GetUpgradeConfig(R.Type);
		const int32 Lv = F->GetUpgradeLevel(R.Type);
		int32 Levels = 0;
		const double Cost = F->GetUpgradeCostMulti(R.Type, ModeCounts[BuyMode], Levels);
		const float Now = Cfg.ValuePerLevel * Lv;

		R.Level->SetText(FText::FromString(FString::Printf(TEXT("Lv %d / %d"), Lv, Cfg.MaxLevel)));
		const float Pct = Cfg.MaxLevel > 0 ? float(Lv) / float(Cfg.MaxLevel) : 0.f;
		FortuneStyle::FStyBar Bar;
		Bar.FillSlot = R.BarFill;
		Bar.RestSlot = R.BarRest;
		Bar.Fill = R.BarFillBorder;
		StySetBar(Bar, Pct);

		if (Cost < 0.0)
		{
			R.Value->SetText(FText::FromString(FString::Printf(TEXT("%s\nMAKS"), *FormatBonus(R.Type, Now))));
			R.Cost->SetText(FText::FromString(TEXT("MAKS")));
			R.Cost->SetColorAndOpacity(FSlateColor(UpgGold));
			R.CostSub->SetText(FText::GetEmpty());
			R.Button->SetIsEnabled(false);
		}
		else
		{
			const float Next = Cfg.ValuePerLevel * (Lv + Levels);
			R.Value->SetText(FText::FromString(FString::Printf(TEXT("%s\n> %s"), *FormatBonus(R.Type, Now), *FormatBonus(R.Type, Next))));
			R.Cost->SetText(FText::FromString(FString::Printf(TEXT("$ %s"), *UFortuneHUDWidget::FormatMoney(Cost).ToString())));
			R.CostSub->SetText(FText::FromString(FString::Printf(TEXT("+%d Lv"), Levels)));
			const bool bAfford = Money >= Cost;
			R.Button->SetIsEnabled(bAfford);
			R.Cost->SetColorAndOpacity(FSlateColor(bAfford ? FLinearColor::White : FLinearColor(1.f, 0.45f, 0.45f)));
		}
		// Alinabilen satirlar renkli kenarla one cikar
		if (R.Flash <= 0.f)
		{
			const bool bReady = Cost >= 0.0 && Money >= Cost;
			R.RowBorder->SetBrush(StyRounded(UpgRow, StyWithAlpha(R.Color, bReady ? 0.6f : 0.2f), 16.f, bReady ? 2.f : 1.5f));
		}
	}
}

void UFortuneUpgradeWidget::Buy(EFortuneUpgradeType Type)
{
	if (UFortuneSubsystem* F = GetFortune())
	{
		if (F->BuyUpgradeMulti(Type, ModeCounts[BuyMode]) > 0)
		{
			for (FRow& R : Rows)
			{
				if (R.Type == Type) R.Flash = 1.f;
			}
			MoneyPop = 1.f;
		}
	}
	Refresh();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void UFortuneUpgradeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	StyOpenAnim(PanelRoot, Age);

	// Satin alinan satir parlar, seviye yazisi buyur
	for (FRow& R : Rows)
	{
		if (R.Flash > 0.f && R.Level)
		{
			R.Flash = FMath::Max(0.f, R.Flash - InDeltaTime * 2.2f);
			const float S = 1.f + 0.4f * R.Flash;
			R.Level->SetRenderScale(FVector2D(S, S));
			const FLinearColor Fill = FMath::Lerp(UpgRow, R.Color * 0.45f + FLinearColor(0.f, 0.f, 0.f, 0.95f), R.Flash);
			R.RowBorder->SetBrush(StyRounded(Fill, R.Color, 16.f, 1.5f + 2.f * R.Flash));
			if (R.Flash <= 0.f) Refresh();
		}
	}
	if (MoneyPop > 0.f && MoneyText)
	{
		MoneyPop = FMath::Max(0.f, MoneyPop - InDeltaTime * 3.f);
		const float S = 1.f - 0.12f * FMath::Sin(MoneyPop * PI);
		MoneyText->SetRenderScale(FVector2D(S, S));
		MoneyText->SetColorAndOpacity(FSlateColor(FMath::Lerp(UpgGold, FLinearColor(1.f, 0.4f, 0.35f), MoneyPop)));
	}
}

void UFortuneUpgradeWidget::OnCloseClicked()
{
	RemoveFromParent();
}
