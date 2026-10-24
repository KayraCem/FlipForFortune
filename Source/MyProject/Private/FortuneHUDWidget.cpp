#include "FortuneHUDWidget.h"
#include "FortuneSubsystem.h"
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
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Styling/SlateTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "FortunePlayerController.h"

#define LOCTEXT_NAMESPACE "FortuneHUD"

namespace
{
	const FLinearColor HudGold(1.f, 0.76f, 0.28f, 1.f);
	const FLinearColor HudDarkGold(0.45f, 0.28f, 0.05f, 1.f);

	FSlateBrush HudRoundedBrush(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float OutlineWidth, FVector2D Size = FVector2D(32.f, 32.f))
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.ImageSize = FVector2f(Size);
		B.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	void HudStyleText(UTextBlock* T, int32 Size, const FLinearColor& Color, bool bBold)
	{
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		F.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		T->SetFont(F);
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetShadowOffset(FVector2D(1.5f, 1.5f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	}
}

FText UFortuneHUDWidget::FormatMoney(double Value)
{
	static const TCHAR* Suffix[] = { TEXT(""), TEXT("K"), TEXT("M"), TEXT("B"), TEXT("T"), TEXT("Qa"), TEXT("Qi") };
	int32 Tier = 0;
	double V = FMath::Abs(Value);
	while (V >= 1000.0 && Tier < 6)
	{
		V /= 1000.0;
		++Tier;
	}
	const double Signed = Value < 0.0 ? -V : V;
	if (Tier == 0)
	{
		// 12 -> "12", 12.5 -> "12.5"
		const bool bWhole = FMath::IsNearlyEqual(Signed, FMath::FloorToDouble(Signed + 0.5), 0.05);
		return FText::FromString(bWhole ? FString::Printf(TEXT("%.0f"), Signed) : FString::Printf(TEXT("%.1f"), Signed));
	}
	return FText::FromString(FString::Printf(TEXT("%.2f%s"), Signed, Suffix[Tier]));
}

UFortuneSubsystem* UFortuneHUDWidget::GetFortune() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
}

TSharedRef<SWidget> UFortuneHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneHUDWidget::BuildTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// Sol alt: kamera ipucu
	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CameraHint"));
	HudStyleText(Hint, 13, FLinearColor(1.f, 1.f, 1.f, 0.55f), false);
	Hint->SetText(FText::FromString(TEXT("WASD: yürü   •   Shift: koş   •   Fare: bak   •   Sol tık / E: etkileşim   •   Space: coin at   •   F: AutoFlip   •   Alt basılı: imleç")));
	Hint->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* HintSlot = Root->AddChildToCanvas(Hint);
	HintSlot->SetAnchors(FAnchors(0.f, 1.f));
	HintSlot->SetAlignment(FVector2D(0.f, 1.f));
	HintSlot->SetPosition(FVector2D(18.f, -12.f));
	HintSlot->SetAutoSize(true);

	// Orta: nisangah noktasi + baktigin esyanin adi
	{
		USizeBox* DotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Crosshair"));
		DotBox->SetWidthOverride(8.f);
		DotBox->SetHeightOverride(8.f);
		UBorder* Dot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Dot->SetBrush(HudRoundedBrush(FLinearColor(1.f, 0.95f, 0.85f, 0.85f), FLinearColor(0.f, 0.f, 0.f, 0.6f), 4.f, 1.f, FVector2D(8.f, 8.f)));
		DotBox->SetContent(Dot);
		DotBox->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* DotSlot = Root->AddChildToCanvas(DotBox);
		DotSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		DotSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		DotSlot->SetAutoSize(true);
		CrosshairDot = DotBox;

		AimText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AimText"));
		HudStyleText(AimText, 16, FLinearColor(1.f, 0.88f, 0.6f, 1.f), true);
		AimText->SetJustification(ETextJustify::Center);
		AimText->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* AimSlot = Root->AddChildToCanvas(AimText);
		AimSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		AimSlot->SetAlignment(FVector2D(0.5f, 0.f));
		AimSlot->SetPosition(FVector2D(0.f, 22.f));
		AimSlot->SetAutoSize(true);
	}

	// Sol kenar: Coin Collection + Envanter ikonlari (dikey, ortada)
	{
		UVerticalBox* Side = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SideMenu"));
		UCanvasPanelSlot* SideSlot = Root->AddChildToCanvas(Side);
		SideSlot->SetAnchors(FAnchors(0.f, 0.5f));
		SideSlot->SetAlignment(FVector2D(0.f, 0.5f));
		SideSlot->SetPosition(FVector2D(22.f, 0.f));
		SideSlot->SetAutoSize(true);
		UButton* Col = MakeSideButton(TEXT("/Game/FlipForFortune/UI/T_Icon_Collection.T_Icon_Collection"), TEXT("KOLEKSİYON"), TEXT("C"), HudGold);
		Col->OnClicked.AddDynamic(this, &UFortuneHUDWidget::HandleCollectionIcon);
		Side->AddChildToVerticalBox(Col)->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
		UButton* Inv = MakeSideButton(TEXT("/Game/FlipForFortune/UI/T_Icon_Inventory.T_Icon_Inventory"), TEXT("ENVANTER"), TEXT("I"), FLinearColor(0.8f, 0.55f, 1.f));
		Inv->OnClicked.AddDynamic(this, &UFortuneHUDWidget::HandleInventoryIcon);
		Side->AddChildToVerticalBox(Inv);
	}

	// Alt orta: [popup] + [para kapsulu]
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	UCanvasPanelSlot* ColumnSlot = Root->AddChildToCanvas(Column);
	ColumnSlot->SetAnchors(FAnchors(0.5f, 1.f));
	ColumnSlot->SetAlignment(FVector2D(0.5f, 1.f));
	ColumnSlot->SetPosition(FVector2D(0.f, -28.f));
	ColumnSlot->SetAutoSize(true);
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	LevelUpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelUp"));
	HudStyleText(LevelUpText, 26, FLinearColor(0.35f, 0.9f, 1.f, 1.f), true);
	LevelUpText->SetJustification(ETextJustify::Center);
	LevelUpText->SetRenderOpacity(0.f);
	LevelUpText->SetVisibility(ESlateVisibility::HitTestInvisible);
	UVerticalBoxSlot* LevelSlot = Column->AddChildToVerticalBox(LevelUpText);
	LevelSlot->SetHorizontalAlignment(HAlign_Center);
	LevelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));

	PopupText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Popup"));
	HudStyleText(PopupText, 28, HudGold, true);
	PopupText->SetJustification(ETextJustify::Center);
	PopupText->SetRenderOpacity(0.f);
	PopupText->SetVisibility(ESlateVisibility::HitTestInvisible);
	UVerticalBoxSlot* PopupSlot = Column->AddChildToVerticalBox(PopupText);
	PopupSlot->SetHorizontalAlignment(HAlign_Center);
	PopupSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	// Kapsul
	UBorder* Pill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MoneyPill"));
	Pill->SetBrush(HudRoundedBrush(FLinearColor(0.03f, 0.02f, 0.01f, 0.78f), FLinearColor(1.f, 0.72f, 0.25f, 0.85f), 26.f, 2.f));
	Pill->SetPadding(FMargin(12.f, 8.f, 12.f, 8.f));
	Pill->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UVerticalBoxSlot* PillSlot = Column->AddChildToVerticalBox(Pill);
	PillSlot->SetHorizontalAlignment(HAlign_Center);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	Pill->SetContent(Row);
	Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// Coin simgesi: altin daire + "$"
	USizeBox* CoinBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CoinBox"));
	CoinBox->SetWidthOverride(40.f);
	CoinBox->SetHeightOverride(40.f);
	UOverlay* CoinOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CoinOverlay"));
	CoinBox->SetContent(CoinOverlay);
	UImage* CoinImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CoinIcon"));
	CoinImg->SetBrush(HudRoundedBrush(HudGold, HudDarkGold, 20.f, 3.f, FVector2D(40.f, 40.f)));
	UOverlaySlot* ImgSlot = CoinOverlay->AddChildToOverlay(CoinImg);
	ImgSlot->SetHorizontalAlignment(HAlign_Fill);
	ImgSlot->SetVerticalAlignment(VAlign_Fill);
	UTextBlock* CoinSign = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CoinSign"));
	CoinSign->SetText(FText::FromString(TEXT("$")));
	HudStyleText(CoinSign, 20, HudDarkGold, true);
	CoinSign->SetShadowOffset(FVector2D::ZeroVector);
	UOverlaySlot* SignSlot = CoinOverlay->AddChildToOverlay(CoinSign);
	SignSlot->SetHorizontalAlignment(HAlign_Center);
	SignSlot->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* CoinSlot = Row->AddChildToHorizontalBox(CoinBox);
	CoinSlot->SetVerticalAlignment(VAlign_Center);
	CoinSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

	// Para miktari
	MoneyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Money"));
	HudStyleText(MoneyText, 30, HudGold, true);
	MoneyText->SetText(FText::FromString(TEXT("0")));
	MoneyText->SetVisibility(ESlateVisibility::HitTestInvisible);
	UHorizontalBoxSlot* MoneySlot = Row->AddChildToHorizontalBox(MoneyText);
	MoneySlot->SetVerticalAlignment(VAlign_Center);

	// Ayirici + Enchant Scroll sayisi
	UImage* Divider = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Divider"));
	Divider->SetBrush(HudRoundedBrush(FLinearColor(1.f, 0.72f, 0.25f, 0.35f), FLinearColor::Transparent, 1.f, 0.f, FVector2D(2.f, 28.f)));
	UHorizontalBoxSlot* DivSlot = Row->AddChildToHorizontalBox(Divider);
	DivSlot->SetVerticalAlignment(VAlign_Center);
	DivSlot->SetPadding(FMargin(20.f, 0.f, 16.f, 0.f));

	UImage* ScrollIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ScrollIcon"));
	ScrollIcon->SetBrush(HudRoundedBrush(FLinearColor(0.55f, 0.3f, 0.95f, 1.f), FLinearColor(0.85f, 0.7f, 1.f, 1.f), 4.f, 2.f, FVector2D(18.f, 24.f)));
	UHorizontalBoxSlot* ScrollIconSlot = Row->AddChildToHorizontalBox(ScrollIcon);
	ScrollIconSlot->SetVerticalAlignment(VAlign_Center);
	ScrollIconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	ScrollText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Scrolls"));
	HudStyleText(ScrollText, 20, FLinearColor(0.85f, 0.75f, 1.f, 1.f), true);
	ScrollText->SetText(FText::FromString(TEXT("x0")));
	UHorizontalBoxSlot* ScrollSlot = Row->AddChildToHorizontalBox(ScrollText);
	ScrollSlot->SetVerticalAlignment(VAlign_Center);

	// Ayirici + AUTO FLIP butonu
	UImage* Divider2 = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Divider2"));
	Divider2->SetBrush(HudRoundedBrush(FLinearColor(1.f, 0.72f, 0.25f, 0.35f), FLinearColor::Transparent, 1.f, 0.f, FVector2D(2.f, 28.f)));
	UHorizontalBoxSlot* Div2Slot = Row->AddChildToHorizontalBox(Divider2);
	Div2Slot->SetVerticalAlignment(VAlign_Center);
	Div2Slot->SetPadding(FMargin(16.f, 0.f, 12.f, 0.f));

	AutoButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AutoFlipButton"));
	FButtonStyle Style;
	Style.SetNormal(HudRoundedBrush(FLinearColor(1.f, 1.f, 1.f, 0.06f), FLinearColor(1.f, 0.72f, 0.25f, 0.5f), 14.f, 1.5f));
	Style.SetHovered(HudRoundedBrush(FLinearColor(1.f, 0.85f, 0.4f, 0.18f), FLinearColor(1.f, 0.8f, 0.35f, 1.f), 14.f, 1.5f));
	Style.SetPressed(HudRoundedBrush(FLinearColor(1.f, 0.85f, 0.4f, 0.3f), FLinearColor(1.f, 0.8f, 0.35f, 1.f), 14.f, 2.f));
	Style.SetDisabled(HudRoundedBrush(FLinearColor(0.f, 0.f, 0.f, 0.2f), FLinearColor(0.5f, 0.5f, 0.5f, 0.4f), 14.f, 1.f));
	Style.SetNormalPadding(FMargin(10.f, 4.f));
	Style.SetPressedPadding(FMargin(10.f, 5.f, 10.f, 3.f));
	AutoButton->SetStyle(Style);
	AutoButton->SetVisibility(ESlateVisibility::Visible);
	AutoButton->OnClicked.AddDynamic(this, &UFortuneHUDWidget::HandleAutoClicked);
	UHorizontalBoxSlot* AutoSlot = Row->AddChildToHorizontalBox(AutoButton);
	AutoSlot->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* AutoRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AutoRow"));
	AutoButton->SetContent(AutoRow);

	// Durum lambasi (yesil = acik, kirmizi = kapali, gri = kilitli)
	AutoLight = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AutoLight"));
	USizeBox* LightBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AutoLightBox"));
	LightBox->SetWidthOverride(12.f);
	LightBox->SetHeightOverride(12.f);
	LightBox->SetContent(AutoLight);
	UHorizontalBoxSlot* LightSlot = AutoRow->AddChildToHorizontalBox(LightBox);
	LightSlot->SetVerticalAlignment(VAlign_Center);
	LightSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	AutoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AutoText"));
	HudStyleText(AutoText, 16, FLinearColor::White, true);
	AutoText->SetText(FText::FromString(TEXT("AUTO")));
	UHorizontalBoxSlot* AutoTextSlot = AutoRow->AddChildToHorizontalBox(AutoText);
	AutoTextSlot->SetVerticalAlignment(VAlign_Center);
}

void UFortuneHUDWidget::RefreshAutoFlip()
{
	const UFortuneSubsystem* F = GetFortune();
	if (!F || !AutoText || !AutoLight || !AutoButton) return;

	FLinearColor Light;
	if (!F->IsAutoFlipUnlocked())
	{
		int32 Cur = 0, Req = 0;
		F->GetAutoFlipProgress(Cur, Req);
		AutoText->SetText(FText::FromString(FString::Printf(TEXT("AUTO  %d/%d"), Cur, Req)));
		AutoText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.f)));
		Light = FLinearColor(0.35f, 0.35f, 0.35f, 1.f);
		AutoButton->SetIsEnabled(false);
	}
	else if (F->IsAutoFlipEnabled())
	{
		AutoText->SetText(FText::FromString(TEXT("AUTO  ACIK")));
		AutoText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 1.f, 0.55f, 1.f)));
		Light = FLinearColor(0.2f, 1.f, 0.3f, 1.f);
		AutoButton->SetIsEnabled(true);
	}
	else
	{
		AutoText->SetText(FText::FromString(TEXT("AUTO  KAPALI")));
		AutoText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.9f, 0.75f, 1.f)));
		Light = FLinearColor(0.9f, 0.25f, 0.2f, 1.f);
		AutoButton->SetIsEnabled(true);
	}
	AutoLight->SetBrush(HudRoundedBrush(Light, FLinearColor(0.f, 0.f, 0.f, 0.4f), 6.f, 1.f, FVector2D(12.f, 12.f)));
}

void UFortuneHUDWidget::HandleAutoClicked()
{
	if (UFortuneSubsystem* F = GetFortune())
	{
		F->ToggleAutoFlip();
	}
	// Klavye odagini oyuna geri ver (yoksa Space butonu tekrar tiklar)
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void UFortuneHUDWidget::HandleAutoChanged()
{
	RefreshAutoFlip();
}

void UFortuneHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);   // bos alana tiklama dunyaya gecsin, buton tiklanir

	if (UFortuneSubsystem* F = GetFortune())
	{
		F->OnMoneyChanged.AddDynamic(this, &UFortuneHUDWidget::HandleMoneyChanged);
		F->OnFlipResolved.AddDynamic(this, &UFortuneHUDWidget::HandleFlip);
		F->OnInventoryChanged.AddDynamic(this, &UFortuneHUDWidget::HandleInventoryChanged);
		F->OnAutoFlipChanged.AddDynamic(this, &UFortuneHUDWidget::HandleAutoChanged);
		DisplayedMoney = TargetMoney = F->GetMoney();
	}
	if (MoneyText)
	{
		MoneyText->SetText(FormatMoney(DisplayedMoney));
	}
	RefreshScrolls();
	RefreshAutoFlip();
}

void UFortuneHUDWidget::NativeDestruct()
{
	if (UFortuneSubsystem* F = GetFortune())
	{
		F->OnMoneyChanged.RemoveDynamic(this, &UFortuneHUDWidget::HandleMoneyChanged);
		F->OnFlipResolved.RemoveDynamic(this, &UFortuneHUDWidget::HandleFlip);
		F->OnInventoryChanged.RemoveDynamic(this, &UFortuneHUDWidget::HandleInventoryChanged);
		F->OnAutoFlipChanged.RemoveDynamic(this, &UFortuneHUDWidget::HandleAutoChanged);
	}
	Super::NativeDestruct();
}

void UFortuneHUDWidget::RefreshScrolls()
{
	if (ScrollText)
	{
		const UFortuneSubsystem* F = GetFortune();
		const int32 N = F ? F->GetItemCount(EFortuneItemType::EnchantScroll) : 0;
		ScrollText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), N)));
	}
}

void UFortuneHUDWidget::HandleInventoryChanged()
{
	RefreshScrolls();
}

void UFortuneHUDWidget::HandleMoneyChanged(double NewMoney)
{
	TargetMoney = NewMoney;
}

void UFortuneHUDWidget::HandleFlip(const FFortuneFlipResult& Result)
{
	if (!Result.bSuccess) return;
	// Kazanci coin inene kadar gosterme
	Pending = Result;
	HeldBack += Result.MoneyEarned;
	PendingTime = Result.FlipDuration;
}

void UFortuneHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Coin indi: popup goster, kazanci sayaca birak
	if (PendingTime >= 0.f)
	{
		PendingTime -= InDeltaTime;
		if (PendingTime < 0.f)
		{
			HeldBack = 0.0;
			PopupAge = 0.f;
			const FText Amount = FormatMoney(Pending.MoneyEarned);
			FLinearColor Color = Pending.bHeads ? HudGold : FLinearColor(0.75f, 0.75f, 0.75f, 1.f);
			const FText Side = Pending.bHeads ? LOCTEXT("Heads", "YAZI") : LOCTEXT("Tails", "TURA");
			FText Msg = FText::Format(LOCTEXT("Plus", "{0}!  +{1}"), Side, Amount);
			if (Pending.bJackpot)
			{
				Msg = FText::Format(LOCTEXT("Jackpot", "{0} - JACKPOT!  +{1}"), Side, Amount);
				Color = FLinearColor(1.f, 0.3f, 0.9f, 1.f);
			}
			else if (Pending.bCritical)
			{
				Msg = FText::Format(LOCTEXT("Crit", "{0} - KRITIK!  +{1}"), Side, Amount);
				Color = FLinearColor(1.f, 0.45f, 0.1f, 1.f);
			}
			else if (Pending.bDoubleFlip)
			{
				Msg = FText::Format(LOCTEXT("Double", "{0} - x2!  +{1}"), Side, Amount);
				Color = FLinearColor(0.4f, 0.9f, 1.f, 1.f);
			}
			if (Pending.bLeveledUp && LevelUpText)
			{
				LevelUpText->SetText(FText::Format(LOCTEXT("LvUp", "LEVEL {0}!"), FText::AsNumber(Pending.NewLevel)));
				LevelUpAge = 0.f;
			}
			if (PopupText)
			{
				PopupText->SetText(Msg);
				PopupText->SetColorAndOpacity(FSlateColor(Color));
			}
		}
	}

	// Sayac animasyonu
	const double Goal = TargetMoney - HeldBack;
	if (!FMath::IsNearlyEqual(DisplayedMoney, Goal, 0.01))
	{
		DisplayedMoney = FMath::FInterpTo(DisplayedMoney, Goal, (double)InDeltaTime, 10.0);
		if (FMath::Abs(DisplayedMoney - Goal) < 0.05)
		{
			DisplayedMoney = Goal;
		}
		if (MoneyText)
		{
			MoneyText->SetText(FormatMoney(DisplayedMoney));
		}
	}

	// LEVEL UP: buyuyerek cikar, biraz daha uzun kalir
	if (LevelUpText && LevelUpAge < 2.6f)
	{
		LevelUpAge += InDeltaTime;
		LevelUpText->SetRenderOpacity(FMath::Clamp(1.f - (LevelUpAge - 1.6f), 0.f, 1.f));
		LevelUpText->SetRenderTranslation(FVector2D(0.f, -LevelUpAge * 20.f));
		const float LvPop = 1.f + 0.4f * FMath::Max(0.f, 1.f - LevelUpAge * 4.f);
		LevelUpText->SetRenderScale(FVector2D(LvPop, LvPop));
	}

	// Popup: yukari suzulup kaybolur
	if (PopupText && PopupAge < 2.f)
	{
		PopupAge += InDeltaTime;
		const float A = FMath::Clamp(1.f - (PopupAge - 0.8f) / 1.2f, 0.f, 1.f);
		PopupText->SetRenderOpacity(A);
		PopupText->SetRenderTranslation(FVector2D(0.f, -PopupAge * 25.f));
		const float Pop = 1.f + 0.25f * FMath::Max(0.f, 1.f - PopupAge * 5.f);
		PopupText->SetRenderScale(FVector2D(Pop, Pop));
	}
}

UButton* UFortuneHUDWidget::MakeSideButton(const TCHAR* IconPath, const FString& Label, const FString& Key, const FLinearColor& Accent)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle St;
	St.SetNormal(HudRoundedBrush(FLinearColor(0.03f, 0.025f, 0.03f, 0.82f), FLinearColor(Accent.R, Accent.G, Accent.B, 0.55f), 18.f, 2.f));
	St.SetHovered(HudRoundedBrush(FLinearColor(0.12f, 0.08f, 0.04f, 0.92f), Accent, 18.f, 3.f));
	St.SetPressed(HudRoundedBrush(FLinearColor(0.02f, 0.02f, 0.02f, 0.95f), Accent, 18.f, 3.f));
	St.SetNormalPadding(FMargin(8.f, 8.f, 8.f, 6.f));
	St.SetPressedPadding(FMargin(8.f, 10.f, 8.f, 4.f));
	B->SetStyle(St);
	B->SetVisibility(ESlateVisibility::Visible);

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UOverlay* O = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	IconBox->SetWidthOverride(76.f);
	IconBox->SetHeightOverride(76.f);
	UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, IconPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		FSlateBrush Br;
		Br.SetResourceObject(Tex);
		Br.ImageSize = FVector2f(76.f, 76.f);
		Icon->SetBrush(Br);
	}
	else
	{
		Icon->SetBrush(HudRoundedBrush(Accent * 0.4f, Accent, 38.f, 3.f, FVector2D(76.f, 76.f)));
	}
	IconBox->SetContent(Icon);
	O->AddChildToOverlay(IconBox);

	// Kisayol tusu rozeti (sag alt kose): hangi tusla acildigi
	UBorder* KeyB = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	KeyB->SetBrush(HudRoundedBrush(FLinearColor(0.f, 0.f, 0.f, 0.75f), FLinearColor(Accent.R, Accent.G, Accent.B, 0.8f), 8.f, 1.5f));
	KeyB->SetPadding(FMargin(7.f, 1.f));
	UTextBlock* KeyT = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HudStyleText(KeyT, 14, Accent, true);
	KeyT->SetText(FText::FromString(Key));
	KeyB->SetContent(KeyT);
	UOverlaySlot* KS = O->AddChildToOverlay(KeyB);
	KS->SetHorizontalAlignment(HAlign_Right);
	KS->SetVerticalAlignment(VAlign_Bottom);
	V->AddChildToVerticalBox(O)->SetHorizontalAlignment(HAlign_Center);

	UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HudStyleText(L, 11, FLinearColor(1.f, 0.92f, 0.8f, 0.9f), true);
	L->SetText(FText::FromString(Label));
	L->SetJustification(ETextJustify::Center);
	V->AddChildToVerticalBox(L)->SetHorizontalAlignment(HAlign_Center);

	B->SetContent(V);
	return B;
}

void UFortuneHUDWidget::SetCrosshair(bool bShow, const FText& Target)
{
	if (bShow != bCrosshairShown && CrosshairDot)
	{
		bCrosshairShown = bShow;
		CrosshairDot->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	const FString L = bShow ? Target.ToString() : FString();
	if (L != AimLabel && AimText)
	{
		AimLabel = L;
		AimText->SetText(FText::FromString(L.IsEmpty() ? FString() : FString::Printf(TEXT("[E]  %s"), *L)));
		AimText->SetVisibility(L.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UFortuneHUDWidget::HandleCollectionIcon()
{
	if (AFortunePlayerController* PC = Cast<AFortunePlayerController>(GetOwningPlayer())) PC->ToggleCollection();
	if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

void UFortuneHUDWidget::HandleInventoryIcon()
{
	if (AFortunePlayerController* PC = Cast<AFortunePlayerController>(GetOwningPlayer())) PC->ToggleInventory();
	if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

#undef LOCTEXT_NAMESPACE
