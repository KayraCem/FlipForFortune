#include "FortuneMenuWidgets.h"
#include "FortuneSubsystem.h"
#include "FortuneSettings.h"
#include "FortuneCoinData.h"
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
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Styling/SlateTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"

namespace FortuneMenuUI
{
	FSlateBrush MenuRounded(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float OutlineWidth, FVector2D Size = FVector2D(32.f, 32.f))
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.ImageSize = FVector2f(Size);
		B.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	UTextBlock* MenuText(UWidgetTree* Tree, const FString& Str, int32 Size, const FLinearColor& Color, bool bBold = true)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		F.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		T->SetFont(F);
		T->SetText(FText::FromString(Str));
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetShadowOffset(FVector2D(2.f, 2.f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		return T;
	}

	UButton* MenuButton(UWidgetTree* Tree, UTextBlock* Label, const FLinearColor& Fill, const FLinearColor& Accent, float Radius = 16.f)
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		FButtonStyle S;
		S.SetNormal(MenuRounded(Fill, FLinearColor(1.f, 0.85f, 0.4f, 0.5f), Radius, 2.f));
		S.SetHovered(MenuRounded(Fill * 1.3f, Accent, Radius, 3.f));
		S.SetPressed(MenuRounded(Fill * 0.8f, Accent, Radius, 3.f));
		S.SetDisabled(MenuRounded(FLinearColor(0.1f, 0.1f, 0.1f, 0.8f), FLinearColor(1.f, 1.f, 1.f, 0.06f), Radius, 1.f));
		S.SetNormalPadding(FMargin(16.f, 8.f));
		S.SetPressedPadding(FMargin(16.f, 10.f, 16.f, 6.f));
		Btn->SetStyle(S);
		Label->SetJustification(ETextJustify::Center);
		Btn->SetContent(Label);
		return Btn;
	}

	USizeBox* Sized(UWidgetTree* Tree, UWidget* Content, float W, float H)
	{
		USizeBox* B = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (W > 0.f) B->SetWidthOverride(W);
		if (H > 0.f) B->SetHeightOverride(H);
		B->SetContent(Content);
		return B;
	}
}

using namespace FortuneMenuUI;

// ============================================================ ANA MENU

TSharedRef<SWidget> UFortuneMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneMainMenuWidget::BuildTree()
{
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;

	// Arkada oda gorunur ama karartilir
	// Sag tarafta koyu panel (solda yagan coinler gorunsun)
	UImage* Side = T->ConstructWidget<UImage>(UImage::StaticClass());
	Side->SetColorAndOpacity(FLinearColor(0.02f, 0.01f, 0.01f, 0.55f));
	UCanvasPanelSlot* SideSlot = Root->AddChildToCanvas(Side);
	SideSlot->SetAnchors(FAnchors(0.52f, 0.f, 1.f, 1.f));
	SideSlot->SetOffsets(FMargin(0.f));

	UVerticalBox* Col = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UCanvasPanelSlot* ColSlot = Root->AddChildToCanvas(Col);
	ColSlot->SetAnchors(FAnchors(0.76f, 0.5f));
	ColSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	ColSlot->SetAutoSize(true);

	// Logo (Python betigi T_FFF_Logo olarak ice aktarir); yoksa yazi
	Logo = T->ConstructWidget<UImage>(UImage::StaticClass());
	UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/FlipForFortune/UI/T_FFF_Logo.T_FFF_Logo"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Tex)
	{
		FSlateBrush B;
		B.SetResourceObject(Tex);
		B.ImageSize = FVector2f(600.f, 450.f);
		Logo->SetBrush(B);
		Col->AddChildToVerticalBox(Sized(T, Logo, 600.f, 450.f))->SetHorizontalAlignment(HAlign_Center);
	}
	else
	{
		FallbackTitle = MenuText(T, TEXT("FLIP FOR FORTUNE"), 72, FLinearColor(1.f, 0.8f, 0.3f));
		Col->AddChildToVerticalBox(FallbackTitle)->SetHorizontalAlignment(HAlign_Center);
	}

	MoneyText = MenuText(T, TEXT(""), 20, FLinearColor(1.f, 1.f, 1.f, 0.7f), false);
	UVerticalBoxSlot* MoneySlot = Col->AddChildToVerticalBox(MoneyText);
	MoneySlot->SetHorizontalAlignment(HAlign_Center);
	MoneySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));

	UButton* Play = MenuButton(T, MenuText(T, TEXT("OYNA"), 40, FLinearColor(1.f, 0.95f, 0.8f)), FLinearColor(0.55f, 0.32f, 0.05f, 1.f), FLinearColor(1.f, 0.85f, 0.4f), 22.f);
	Play->OnClicked.AddDynamic(this, &UFortuneMainMenuWidget::OnPlayClicked);
	Col->AddChildToVerticalBox(Sized(T, Play, 360.f, 84.f))->SetHorizontalAlignment(HAlign_Center);

	UButton* Quit = MenuButton(T, MenuText(T, TEXT("ÇIKIŞ"), 24, FLinearColor(1.f, 0.6f, 0.6f)), FLinearColor(0.2f, 0.05f, 0.05f, 0.9f), FLinearColor(1.f, 0.4f, 0.4f));
	Quit->OnClicked.AddDynamic(this, &UFortuneMainMenuWidget::OnQuitClicked);
	UVerticalBoxSlot* QuitSlot = Col->AddChildToVerticalBox(Sized(T, Quit, 240.f, 54.f));
	QuitSlot->SetHorizontalAlignment(HAlign_Center);
	QuitSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	UTextBlock* Version = MenuText(T, TEXT("v0.1"), 14, FLinearColor(1.f, 1.f, 1.f, 0.4f), false);
	UCanvasPanelSlot* VerSlot = Root->AddChildToCanvas(Version);
	VerSlot->SetAnchors(FAnchors(1.f, 1.f));
	VerSlot->SetAlignment(FVector2D(1.f, 1.f));
	VerSlot->SetPosition(FVector2D(-24.f, -18.f));
	VerSlot->SetAutoSize(true);
}

void UFortuneMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	APlayerController* PC = GetOwningPlayer();
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(PC))
	{
		FPC->SetMenuOpen(true);
	}
	// Menu sahnesi (Python betigi kurar): kamera oraya gecer, masa gorunmez
	if (PC && GetWorld())
	{
		for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("MenuCamera")))
			{
				MenuCamera = *It;
				GameViewTarget = PC->GetViewTarget();
				PC->SetViewTarget(*It);
				break;
			}
		}
	}
	const UGameInstance* GI = GetGameInstance();
	if (const UFortuneSubsystem* F = GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr)
	{
		MoneyText->SetText(FText::FromString(F->GetMoney() > 0.0
			? FString::Printf(TEXT("Paran: $ %s"), *UFortuneHUDWidget::FormatMoney(F->GetMoney()).ToString())
			: FString(TEXT("Yazı mı, tura mı? Şansını dene!"))));
	}
}

void UFortuneMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;

	// Logo hafif suzulur ve nefes alir
	UWidget* Title = Logo && Logo->GetParent() ? Cast<UWidget>(Logo) : Cast<UWidget>(FallbackTitle);
	if (Title)
	{
		const float In = FMath::Clamp(Age * 2.f, 0.f, 1.f);
		const float S = (0.85f + 0.15f * (1.f - FMath::Pow(1.f - In, 3.f))) * (1.f + 0.015f * FMath::Sin(Age * 2.f));
		Title->SetRenderScale(FVector2D(S, S));
		Title->SetRenderTranslation(FVector2D(0.f, FMath::Sin(Age * 1.4f) * 6.f));
	}

	// Baska bir kamera (oyun kamerasi) kendini aktif ettiyse menu kamerasina geri don
	if (FadeOut < 0.f && MenuCamera.IsValid())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			AActor* Current = PC->GetViewTarget();
			if (Current != MenuCamera.Get())
			{
				if (Current && Current != PC && Current != PC->GetPawn()) GameViewTarget = Current;
				PC->SetViewTarget(MenuCamera.Get());
			}
		}
	}

	if (FadeOut >= 0.f)
	{
		FadeOut += InDeltaTime;
		SetRenderOpacity(FMath::Clamp(1.f - FadeOut / 0.35f, 0.f, 1.f));
		if (FadeOut >= 0.35f)
		{
			if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
			{
				FPC->SetMenuOpen(false);
			}
			RemoveFromParent();
		}
	}
	else
	{
		SetRenderOpacity(FMath::Clamp(Age * 3.f, 0.f, 1.f));
	}
}

void UFortuneMainMenuWidget::OnPlayClicked()
{
	if (FadeOut < 0.f)
	{
		FadeOut = 0.f;
		// Kamera menu sahnesinden masaya kayarak gecer
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (GameViewTarget.IsValid())
			{
				PC->SetViewTargetWithBlend(GameViewTarget.Get(), 1.0f, VTBlend_EaseInOut, 2.f);
			}
		}
		SetVisibility(ESlateVisibility::HitTestInvisible);
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
}

void UFortuneMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

// ============================================================ ENCHANT

FString UFortuneEnchantWidget::TierName(int32 Tier)
{
	switch (Tier)
	{
	case 1:  return TEXT("TIER 1  •  RARE");
	case 2:  return TEXT("TIER 2  •  EPIC");
	case 3:  return TEXT("TIER 3  •  LEGENDARY");
	default: return TEXT("ENCHANT YOK");
	}
}

FLinearColor UFortuneEnchantWidget::TierColor(int32 Tier)
{
	switch (Tier)
	{
	case 1:  return FLinearColor(0.35f, 0.65f, 1.f);
	case 2:  return FLinearColor(0.75f, 0.4f, 1.f);
	case 3:  return FLinearColor(1.f, 0.78f, 0.2f);
	default: return FLinearColor(0.6f, 0.6f, 0.6f);
	}
}

UFortuneSubsystem* UFortuneEnchantWidget::GetFortune() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
}

TSharedRef<SWidget> UFortuneEnchantWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneEnchantWidget::BuildTree()
{
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;

	UImage* Dim = T->ConstructWidget<UImage>(UImage::StaticClass());
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	UBorder* Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrush(MenuRounded(FLinearColor(0.06f, 0.04f, 0.08f, 0.96f), FLinearColor(0.75f, 0.45f, 1.f, 0.85f), 24.f, 2.f));
	Panel->SetPadding(FMargin(30.f, 24.f));
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Sized(T, Panel, 720.f, 0.f));
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Col = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);

	// Baslik
	UHorizontalBox* Head = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(Head);
	UVerticalBox* TitleCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Head->AddChildToHorizontalBox(TitleCol)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleCol->AddChildToVerticalBox(MenuText(T, TEXT("ENCHANT"), 36, FLinearColor(0.85f, 0.6f, 1.f)));
	UHorizontalBox* CoinRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TitleCol->AddChildToVerticalBox(CoinRow)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	UButton* PrevCoin = MenuButton(T, MenuText(T, TEXT("<"), 18, FLinearColor::White), FLinearColor(0.2f, 0.12f, 0.28f, 1.f), FLinearColor(0.85f, 0.6f, 1.f), 10.f);
	PrevCoin->OnClicked.AddDynamic(this, &UFortuneEnchantWidget::OnPrevCoin);
	CoinRow->AddChildToHorizontalBox(Sized(T, PrevCoin, 44.f, 36.f))->SetVerticalAlignment(VAlign_Center);
	CoinNameText = MenuText(T, TEXT("Coin"), 20, FLinearColor::White);
	UHorizontalBoxSlot* NameSlot = CoinRow->AddChildToHorizontalBox(CoinNameText);
	NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetPadding(FMargin(12.f, 0.f));
	UButton* NextCoin = MenuButton(T, MenuText(T, TEXT(">"), 18, FLinearColor::White), FLinearColor(0.2f, 0.12f, 0.28f, 1.f), FLinearColor(0.85f, 0.6f, 1.f), 10.f);
	NextCoin->OnClicked.AddDynamic(this, &UFortuneEnchantWidget::OnNextCoin);
	CoinRow->AddChildToHorizontalBox(Sized(T, NextCoin, 44.f, 36.f))->SetVerticalAlignment(VAlign_Center);
	UBorder* ScrollPill = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	ScrollPill->SetBrush(MenuRounded(FLinearColor(0.f, 0.f, 0.f, 0.45f), FLinearColor(0.75f, 0.45f, 1.f, 0.6f), 18.f, 1.5f));
	ScrollPill->SetPadding(FMargin(16.f, 6.f));
	ScrollText = MenuText(T, TEXT("Scroll x0"), 22, FLinearColor(0.85f, 0.75f, 1.f));
	ScrollPill->SetContent(ScrollText);
	Head->AddChildToHorizontalBox(ScrollPill)->SetVerticalAlignment(VAlign_Center);

	// Tier sanslari: 3 renkli kutu
	Col->AddChildToVerticalBox(MenuText(T, TEXT("TIER ŞANSLARI"), 15, FLinearColor(1.f, 1.f, 1.f, 0.55f)))->SetPadding(FMargin(0.f, 16.f, 0.f, 6.f));
	UHorizontalBox* Chances = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(Chances);
	for (int32 Tier = 1; Tier <= 3; ++Tier)
	{
		const FLinearColor C = TierColor(Tier);
		UBorder* Box = T->ConstructWidget<UBorder>(UBorder::StaticClass());
		Box->SetBrush(MenuRounded(C * 0.18f + FLinearColor(0.f, 0.f, 0.f, 0.82f), C, 14.f, 2.f));
		Box->SetPadding(FMargin(10.f, 8.f));
		UVerticalBox* V = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Box->SetContent(V);
		V->AddChildToVerticalBox(MenuText(T, TierName(Tier), 15, C))->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* Pct = MenuText(T, TEXT("%0"), 26, FLinearColor::White);
		V->AddChildToVerticalBox(Pct)->SetHorizontalAlignment(HAlign_Center);
		ChanceTexts.Add(Pct);
		UHorizontalBoxSlot* S = Chances->AddChildToHorizontalBox(Box);
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(Tier == 1 ? 0.f : 6.f, 0.f, Tier == 3 ? 0.f : 6.f, 0.f));
	}

	// Mevcut enchant
	Col->AddChildToVerticalBox(MenuText(T, TEXT("MEVCUT"), 15, FLinearColor(1.f, 1.f, 1.f, 0.55f)))->SetPadding(FMargin(0.f, 16.f, 0.f, 6.f));
	CurrentBox = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	CurrentBox->SetPadding(FMargin(16.f, 10.f));
	UVerticalBox* CurV = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CurrentBox->SetContent(CurV);
	CurrentTier = MenuText(T, TEXT(""), 15, FLinearColor::White);
	CurV->AddChildToVerticalBox(CurrentTier);
	CurrentStat = MenuText(T, TEXT(""), 22, FLinearColor::White);
	CurV->AddChildToVerticalBox(CurrentStat);
	Col->AddChildToVerticalBox(CurrentBox);

	// Sonuc / cark
	Col->AddChildToVerticalBox(MenuText(T, TEXT("SONUÇ"), 15, FLinearColor(1.f, 1.f, 1.f, 0.55f)))->SetPadding(FMargin(0.f, 16.f, 0.f, 6.f));
	ResultBox = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	ResultBox->SetPadding(FMargin(16.f, 14.f));
	ResultBox->SetBrush(MenuRounded(FLinearColor(0.f, 0.f, 0.f, 0.4f), FLinearColor(1.f, 1.f, 1.f, 0.1f), 16.f, 2.f));
	UVerticalBox* ResV = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ResultBox->SetContent(Sized(T, ResV, 0.f, 92.f));
	ResultTier = MenuText(T, TEXT(""), 18, FLinearColor::White);
	ResV->AddChildToVerticalBox(ResultTier)->SetHorizontalAlignment(HAlign_Center);
	ResultStat = MenuText(T, TEXT("?"), 32, FLinearColor::White);
	ResV->AddChildToVerticalBox(ResultStat)->SetHorizontalAlignment(HAlign_Center);
	ResultHint = MenuText(T, TEXT("Enchant'a bas ve şansını dene!"), 14, FLinearColor(1.f, 1.f, 1.f, 0.55f), false);
	ResV->AddChildToVerticalBox(ResultHint)->SetHorizontalAlignment(HAlign_Center);
	Col->AddChildToVerticalBox(ResultBox);

	Col->AddChildToVerticalBox(MenuText(T, TEXT("Yeni enchant mevcut enchant'ın yerine geçer."), 14, FLinearColor(1.f, 0.7f, 0.5f, 0.8f), false))
		->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	// Butonlar
	UHorizontalBox* Buttons = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* BtnSlot = Col->AddChildToVerticalBox(Buttons);
	BtnSlot->SetHorizontalAlignment(HAlign_Center);
	BtnSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	RollText = MenuText(T, TEXT("ENCHANT"), 26, FLinearColor(0.95f, 0.85f, 1.f));
	RollButton = MenuButton(T, RollText, FLinearColor(0.32f, 0.12f, 0.5f, 1.f), FLinearColor(0.85f, 0.6f, 1.f));
	RollButton->OnClicked.AddDynamic(this, &UFortuneEnchantWidget::OnRollClicked);
	Buttons->AddChildToHorizontalBox(Sized(T, RollButton, 280.f, 60.f))->SetPadding(FMargin(6.f, 0.f));
	UButton* Close = MenuButton(T, MenuText(T, TEXT("KAPAT"), 22, FLinearColor(1.f, 0.6f, 0.6f)), FLinearColor(0.25f, 0.06f, 0.06f, 1.f), FLinearColor(1.f, 0.4f, 0.4f));
	Close->OnClicked.AddDynamic(this, &UFortuneEnchantWidget::OnCloseClicked);
	Buttons->AddChildToHorizontalBox(Sized(T, Close, 180.f, 60.f))->SetPadding(FMargin(6.f, 0.f));
}

void UFortuneEnchantWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
	if (bStandalone)
	{
		if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
		{
			FPC->SetMenuOpen(true);
		}
	}
	if (UFortuneSubsystem* F = GetFortune())
	{
		F->OnInventoryChanged.AddDynamic(this, &UFortuneEnchantWidget::HandleChanged);
		F->OnCoinsChanged.AddDynamic(this, &UFortuneEnchantWidget::HandleChanged);
	}
	Refresh();
}

void UFortuneEnchantWidget::NativeDestruct()
{
	if (UFortuneSubsystem* F = GetFortune())
	{
		F->OnInventoryChanged.RemoveDynamic(this, &UFortuneEnchantWidget::HandleChanged);
		F->OnCoinsChanged.RemoveDynamic(this, &UFortuneEnchantWidget::HandleChanged);
	}
	if (bStandalone)
	{
		if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
		{
			FPC->SetMenuOpen(false);
		}
	}
	Super::NativeDestruct();
}

void UFortuneEnchantWidget::HandleChanged()
{
	if (!bRolling) Refresh();
}

void UFortuneEnchantWidget::ShowEnchant(UBorder* Box, UTextBlock* TierT, UTextBlock* StatT, const FFortuneEnchant& E, bool bBig)
{
	const FLinearColor C = TierColor(E.IsActive() ? E.Tier : 0);
	Box->SetBrush(MenuRounded(C * 0.2f + FLinearColor(0.f, 0.f, 0.f, 0.8f), C, 16.f, bBig ? 3.f : 2.f));
	TierT->SetText(FText::FromString(TierName(E.IsActive() ? E.Tier : 0)));
	TierT->SetColorAndOpacity(FSlateColor(C));

	FString Stat = TEXT("-");
	if (E.IsActive())
	{
		if (const UFortuneSubsystem* F = GetFortune())
		{
			// "Tier 3 - Money Earn +%60" -> sadece "Money Earn +%60"
			Stat = F->FormatEnchant(E).ToString();
			int32 Dash = INDEX_NONE;
			if (Stat.FindChar(TEXT('-'), Dash))
			{
				Stat = Stat.Mid(Dash + 1).TrimStartAndEnd();
			}
		}
	}
	StatT->SetText(FText::FromString(Stat));
	StatT->SetColorAndOpacity(FSlateColor(E.IsActive() ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.5f)));
}

void UFortuneEnchantWidget::Refresh()
{
	UFortuneSubsystem* F = GetFortune();
	if (!F) return;
	const UFortuneCoinData* Coin = F->FindCoinData(CoinId);
	FFortuneOwnedCoin Owned;
	const bool bOwned = F->GetOwnedCoin(CoinId, Owned);

	if (Coin)
	{
		CoinNameText->SetText(FText::FromString(FString::Printf(TEXT("%s  •  %s"), *Coin->DisplayName.ToString(), *UFortuneSubsystem::GetRarityName(Coin->Rarity))));
		CoinNameText->SetColorAndOpacity(FSlateColor(UFortuneSubsystem::GetRarityColor(Coin->Rarity)));
	}

	const int32 Scrolls = F->GetItemCount(EFortuneItemType::EnchantScroll);
	ScrollText->SetText(FText::FromString(FString::Printf(TEXT("Scroll  x%d"), Scrolls)));

	// Tier sanslari kusanili coinin Luck'ina gore hesaplanir
	float T1 = 0.f, T2 = 0.f, T3 = 0.f;
	F->GetEnchantTierChances(T1, T2, T3);
	const float Ch[3] = { T1, T2, T3 };
	for (int32 i = 0; i < ChanceTexts.Num() && i < 3; ++i)
	{
		const FString Pct = (i == 2) ? FString::Printf(TEXT("%%%.1f"), Ch[i] * 100.f) : FString::Printf(TEXT("%%%.0f"), Ch[i] * 100.f);
		ChanceTexts[i]->SetText(FText::FromString(Pct));
	}

	ShowEnchant(CurrentBox, CurrentTier, CurrentStat, bOwned ? Owned.Enchant : FFortuneEnchant(), false);

	const bool bCan = bOwned && Scrolls > 0 && !bRolling;
	RollButton->SetIsEnabled(bCan);
	RollText->SetText(FText::FromString(bOwned ? (Scrolls > 0 ? FString(TEXT("ENCHANT")) : FString(TEXT("SCROLL YOK"))) : FString(TEXT("KİLİTLİ"))));
}

void UFortuneEnchantWidget::OnRollClicked()
{
	UFortuneSubsystem* F = GetFortune();
	if (!F || bRolling) return;
	FFortuneEnchant Result;
	if (!F->UseEnchantScroll(CoinId, Result)) return;

	RollResult = Result;
	bRolling = true;
	RollTime = 0.f;
	NextTick = 0.f;
	RollButton->SetIsEnabled(false);
	ResultHint->SetText(FText::FromString(TEXT("...")));
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void UFortuneEnchantWidget::OnPrevCoin() { CycleCoin(-1); }
void UFortuneEnchantWidget::OnNextCoin() { CycleCoin(1); }

void UFortuneEnchantWidget::CycleCoin(int32 Dir)
{
	UFortuneSubsystem* F = GetFortune();
	if (!F || bRolling) return;
	TArray<FName> Owned;
	for (const FFortuneCollectionEntry& E : F->GetCollection())
	{
		if (E.bOwned && E.Coin) Owned.Add(E.Coin->GetCoinId());
	}
	if (Owned.Num() == 0) return;
	const int32 Cur = Owned.IndexOfByKey(CoinId);
	const int32 Next = (Cur == INDEX_NONE) ? 0 : (Cur + Dir + Owned.Num()) % Owned.Num();
	CoinId = Owned[Next];
	ResultTier->SetText(FText::GetEmpty());
	ResultStat->SetText(FText::FromString(TEXT("?")));
	ResultBox->SetBrush(MenuRounded(FLinearColor(0.f, 0.f, 0.f, 0.4f), FLinearColor(1.f, 1.f, 1.f, 0.1f), 16.f, 2.f));
	Refresh();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void UFortuneEnchantWidget::OnCloseClicked()
{
	if (!bRolling) RemoveFromParent();
}

void UFortuneEnchantWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bRolling)
	{
		// Cark: once hizli, sonra yavaslayarak rastgele boostlar arasinda doner
		RollTime += InDeltaTime;
		const float Dur = 1.6f;
		if (RollTime >= NextTick && RollTime < Dur)
		{
			const float T = RollTime / Dur;
			NextTick = RollTime + FMath::Lerp(0.04f, 0.22f, T * T);
			const UFortuneSettings* S = GetDefault<UFortuneSettings>();
			TArray<EFortuneEnchantStat> Keys;
			S->EnchantStats.GetKeys(Keys);
			if (Keys.Num() > 0)
			{
				FFortuneEnchant Fake;
				Fake.Stat = Keys[FMath::RandRange(0, Keys.Num() - 1)];
				Fake.Tier = FMath::RandRange(1, 3);
				const FFortuneEnchantStatConfig* Cfg = S->EnchantStats.Find(Fake.Stat);
				Fake.Value = Cfg ? Cfg->GetValue(Fake.Tier) : 0.f;
				ShowEnchant(ResultBox, ResultTier, ResultStat, Fake, true);
			}
		}
		if (RollTime >= Dur)
		{
			bRolling = false;
			ShowEnchant(ResultBox, ResultTier, ResultStat, RollResult, true);
			ResultHint->SetText(FText::FromString(RollResult.Tier >= 3 ? TEXT("EFSANEVİ!") : (RollResult.Tier == 2 ? TEXT("Harika!") : TEXT("Enchant uygulandı."))));
			ResultHint->SetColorAndOpacity(FSlateColor(TierColor(RollResult.Tier)));
			ResultPop = 1.f;
			Refresh();
		}
	}

	// Sonuc gelince kutu buyuyup oturur
	if (ResultPop > 0.f && ResultBox)
	{
		ResultPop = FMath::Max(0.f, ResultPop - InDeltaTime * 3.f);
		const float S = 1.f + 0.12f * ResultPop;
		ResultBox->SetRenderScale(FVector2D(S, S));
	}
}
