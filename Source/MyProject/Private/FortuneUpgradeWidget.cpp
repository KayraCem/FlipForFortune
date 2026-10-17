#include "FortuneUpgradeWidget.h"
#include "FortuneSubsystem.h"
#include "FortuneHUDWidget.h"
#include "FortunePlayerController.h"
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

namespace FortuneUpgUI
{
	const FLinearColor UpgGold(1.f, 0.78f, 0.3f, 1.f);
	const FLinearColor UpgPanel(0.06f, 0.055f, 0.05f, 0.95f);
	const FLinearColor UpgRow(0.14f, 0.13f, 0.12f, 0.95f);

	FSlateBrush UpgRounded(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float OutlineWidth, FVector2D Size = FVector2D(32.f, 32.f))
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.ImageSize = FVector2f(Size);
		B.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	UTextBlock* UpgText(UWidgetTree* Tree, const FString& Str, int32 Size, const FLinearColor& Color, bool bBold = true)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		F.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		T->SetFont(F);
		T->SetText(FText::FromString(Str));
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetShadowOffset(FVector2D(1.f, 1.f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
		return T;
	}

	FButtonStyle UpgButtonStyle(const FLinearColor& Fill, const FLinearColor& Accent)
	{
		FButtonStyle S;
		S.SetNormal(UpgRounded(Fill, FLinearColor(1.f, 1.f, 1.f, 0.1f), 12.f, 1.f));
		S.SetHovered(UpgRounded(Fill * 1.25f, Accent, 12.f, 2.f));
		S.SetPressed(UpgRounded(Fill * 0.8f, Accent, 12.f, 2.f));
		S.SetDisabled(UpgRounded(FLinearColor(0.1f, 0.1f, 0.1f, 0.8f), FLinearColor(1.f, 1.f, 1.f, 0.05f), 12.f, 1.f));
		S.SetNormalPadding(FMargin(12.f, 6.f));
		S.SetPressedPadding(FMargin(12.f, 7.f, 12.f, 5.f));
		return S;
	}

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
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	USizeBox* PanelBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PanelBox->SetWidthOverride(820.f);
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBox);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UBorder* Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrush(UpgRounded(UpgPanel, FLinearColor(1.f, 0.72f, 0.25f, 0.8f), 22.f, 2.f));
	Panel->SetPadding(FMargin(28.f, 22.f));
	PanelBox->SetContent(Panel);

	UVerticalBox* Col = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);

	// Baslik + para
	UHorizontalBox* Header = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(Header);
	UVerticalBox* TitleCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Header->AddChildToHorizontalBox(TitleCol)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleCol->AddChildToVerticalBox(UpgText(T, TEXT("YÜKSELTMELER"), 34, UpgGold));
	StatsText = UpgText(T, TEXT(""), 15, FLinearColor(1.f, 1.f, 1.f, 0.65f), false);
	TitleCol->AddChildToVerticalBox(StatsText);

	UBorder* MoneyPill = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	MoneyPill->SetBrush(UpgRounded(FLinearColor(0.f, 0.f, 0.f, 0.5f), FLinearColor(1.f, 0.72f, 0.25f, 0.6f), 20.f, 1.5f));
	MoneyPill->SetPadding(FMargin(16.f, 6.f, 20.f, 6.f));
	UHorizontalBox* MoneyRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	MoneyPill->SetContent(MoneyRow);
	UImage* CoinIcon = T->ConstructWidget<UImage>(UImage::StaticClass());
	CoinIcon->SetBrush(UpgRounded(UpgGold, FLinearColor(0.45f, 0.28f, 0.05f), 14.f, 2.5f, FVector2D(28.f, 28.f)));
	MoneyRow->AddChildToHorizontalBox(CoinIcon)->SetVerticalAlignment(VAlign_Center);
	MoneyText = UpgText(T, TEXT("0"), 26, UpgGold);
	UHorizontalBoxSlot* MoneySlot = MoneyRow->AddChildToHorizontalBox(MoneyText);
	MoneySlot->SetVerticalAlignment(VAlign_Center);
	MoneySlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	UHorizontalBoxSlot* PillSlot = Header->AddChildToHorizontalBox(MoneyPill);
	PillSlot->SetVerticalAlignment(VAlign_Center);

	Col->AddChildToVerticalBox(T->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetPadding(FMargin(0.f, 8.f));

	// Satirlar
	for (const FUpgInfo& Info : Infos)
	{
		const FFortuneUpgradeConfig Cfg = F ? F->GetUpgradeConfig(Info.Type) : FFortuneUpgradeConfig();

		UBorder* RowBorder = T->ConstructWidget<UBorder>(UBorder::StaticClass());
		RowBorder->SetBrush(UpgRounded(UpgRow, FLinearColor(Info.Color.R, Info.Color.G, Info.Color.B, 0.25f), 16.f, 1.5f));
		RowBorder->SetPadding(FMargin(14.f, 10.f));
		Col->AddChildToVerticalBox(RowBorder)->SetPadding(FMargin(0.f, 5.f));

		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		RowBorder->SetContent(Row);

		// Simge
		USizeBox* IconBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconBox->SetWidthOverride(52.f);
		IconBox->SetHeightOverride(52.f);
		UOverlay* IconO = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		IconBox->SetContent(IconO);
		UImage* IconBg = T->ConstructWidget<UImage>(UImage::StaticClass());
		IconBg->SetBrush(UpgRounded(Info.Color * 0.35f, Info.Color, 14.f, 2.f, FVector2D(52.f, 52.f)));
		UOverlaySlot* BgSlot = IconO->AddChildToOverlay(IconBg);
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
		UOverlaySlot* LetterSlot = IconO->AddChildToOverlay(UpgText(T, Info.Letter, 22, Info.Color));
		LetterSlot->SetHorizontalAlignment(HAlign_Center);
		LetterSlot->SetVerticalAlignment(VAlign_Center);
		Row->AddChildToHorizontalBox(IconBox)->SetVerticalAlignment(VAlign_Center);

		// Isim + aciklama
		UVerticalBox* NameCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameCol);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(14.f, 0.f));
		NameCol->AddChildToVerticalBox(UpgText(T, Cfg.DisplayName.ToString(), 22, FLinearColor::White));
		UTextBlock* Desc = UpgText(T, Cfg.Description.ToString(), 15, FLinearColor(1.f, 1.f, 1.f, 0.65f), false);
		Desc->SetAutoWrapText(true);
		NameCol->AddChildToVerticalBox(Desc);

		// Seviye + deger
		UVerticalBox* StatCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		USizeBox* StatBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		StatBox->SetWidthOverride(190.f);
		StatBox->SetContent(StatCol);
		UHorizontalBoxSlot* StatSlot = Row->AddChildToHorizontalBox(StatBox);
		StatSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* LevelT = UpgText(T, TEXT("Lv 0"), 18, Info.Color);
		LevelT->SetJustification(ETextJustify::Right);
		StatCol->AddChildToVerticalBox(LevelT)->SetHorizontalAlignment(HAlign_Right);
		UTextBlock* ValueT = UpgText(T, TEXT(""), 16, FLinearColor(1.f, 1.f, 1.f, 0.85f), false);
		ValueT->SetJustification(ETextJustify::Right);
		StatCol->AddChildToVerticalBox(ValueT)->SetHorizontalAlignment(HAlign_Right);

		// AL butonu
		UFortuneUpgradeBuyButton* Btn = T->ConstructWidget<UFortuneUpgradeBuyButton>(UFortuneUpgradeBuyButton::StaticClass());
		Btn->Setup(this, Info.Type);
		Btn->SetStyle(UpgButtonStyle(FLinearColor(0.18f, 0.32f, 0.12f, 1.f), FLinearColor(0.5f, 1.f, 0.45f)));
		UTextBlock* CostT = UpgText(T, TEXT("0"), 20, FLinearColor::White);
		CostT->SetJustification(ETextJustify::Center);
		Btn->SetContent(CostT);
		USizeBox* BtnBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BtnBox->SetWidthOverride(150.f);
		BtnBox->SetHeightOverride(52.f);
		BtnBox->SetContent(Btn);
		UHorizontalBoxSlot* BtnSlot = Row->AddChildToHorizontalBox(BtnBox);
		BtnSlot->SetVerticalAlignment(VAlign_Center);
		BtnSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));

		FRow R;
		R.Type = Info.Type;
		R.Level = LevelT;
		R.Value = ValueT;
		R.Cost = CostT;
		R.Button = Btn;
		Rows.Add(R);
		KeepAlive.Add(LevelT);
		KeepAlive.Add(ValueT);
		KeepAlive.Add(CostT);
		KeepAlive.Add(Btn);
	}

	// Kapat
	UButton* Close = T->ConstructWidget<UButton>(UButton::StaticClass());
	Close->SetStyle(UpgButtonStyle(FLinearColor(0.3f, 0.08f, 0.08f, 1.f), FLinearColor(1.f, 0.35f, 0.35f)));
	Close->SetContent(UpgText(T, TEXT("KAPAT"), 22, FLinearColor(1.f, 0.55f, 0.55f)));
	Close->OnClicked.AddDynamic(this, &UFortuneUpgradeWidget::OnCloseClicked);
	USizeBox* CloseBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CloseBox->SetWidthOverride(220.f);
	CloseBox->SetHeightOverride(50.f);
	CloseBox->SetContent(Close);
	UVerticalBoxSlot* CloseSlot = Col->AddChildToVerticalBox(CloseBox);
	CloseSlot->SetHorizontalAlignment(HAlign_Center);
	CloseSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
}

void UFortuneUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
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

void UFortuneUpgradeWidget::Refresh()
{
	UFortuneSubsystem* F = GetFortune();
	if (!F) return;
	const double Money = F->GetMoney();
	if (MoneyText)
	{
		MoneyText->SetText(UFortuneHUDWidget::FormatMoney(Money));
	}
	if (StatsText)
	{
		StatsText->SetText(FText::FromString(FString::Printf(
			TEXT("Yazı şansı %%%.0f   |   Kritik %%%.0f  x%.1f   |   Atış %.2f sn"),
			F->GetHeadChance() * 100.f, F->GetCritChance() * 100.f, F->GetCritMultiplier(), F->GetBaseFlipDuration())));
	}

	for (FRow& R : Rows)
	{
		const FFortuneUpgradeConfig Cfg = F->GetUpgradeConfig(R.Type);
		const int32 Lv = F->GetUpgradeLevel(R.Type);
		const double Cost = F->GetUpgradeCost(R.Type);
		const float Now = Cfg.ValuePerLevel * Lv;
		const float Next = Cfg.ValuePerLevel * (Lv + 1);

		R.Level->SetText(FText::FromString(FString::Printf(TEXT("Lv %d / %d"), Lv, Cfg.MaxLevel)));
		if (Cost < 0.0)
		{
			R.Value->SetText(FText::FromString(FString::Printf(TEXT("%s  (MAKS)"), *FormatBonus(R.Type, Now))));
			R.Cost->SetText(FText::FromString(TEXT("MAKS")));
			R.Button->SetIsEnabled(false);
		}
		else
		{
			R.Value->SetText(FText::FromString(FString::Printf(TEXT("%s  >  %s"), *FormatBonus(R.Type, Now), *FormatBonus(R.Type, Next))));
			R.Cost->SetText(FText::FromString(FString::Printf(TEXT("$ %s"), *UFortuneHUDWidget::FormatMoney(Cost).ToString())));
			const bool bAfford = Money >= Cost;
			R.Button->SetIsEnabled(bAfford);
			R.Cost->SetColorAndOpacity(FSlateColor(bAfford ? FLinearColor::White : FLinearColor(1.f, 0.45f, 0.45f)));
		}
	}
}

void UFortuneUpgradeWidget::Buy(EFortuneUpgradeType Type)
{
	if (UFortuneSubsystem* F = GetFortune())
	{
		if (F->BuyUpgrade(Type))
		{
			for (FRow& R : Rows)
			{
				if (R.Type == Type) R.Flash = 1.f;
			}
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
	// Satin alinan satirin seviye yazisi kisa sure buyuyup parlar
	for (FRow& R : Rows)
	{
		if (R.Flash > 0.f && R.Level)
		{
			R.Flash = FMath::Max(0.f, R.Flash - InDeltaTime * 2.5f);
			const float S = 1.f + 0.35f * R.Flash;
			R.Level->SetRenderScale(FVector2D(S, S));
		}
	}
}

void UFortuneUpgradeWidget::OnCloseClicked()
{
	RemoveFromParent();
}
