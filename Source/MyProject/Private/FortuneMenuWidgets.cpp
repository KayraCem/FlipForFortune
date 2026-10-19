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
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "FortuneUIStyle.h"
#include "Styling/SlateTypes.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
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

	using namespace FortuneStyle;
	const FLinearColor Gold(1.f, 0.8f, 0.32f);

	// Sag tarafa dogru yumusakca koyulasan gecis: tek parca degrade doku (serit/cizgi yok)
	{
		const int32 W = 256;
		UTexture2D* Grad = UTexture2D::CreateTransient(W, 1, PF_B8G8R8A8);
		if (Grad)
		{
			Grad->Filter = TF_Bilinear;
			Grad->AddressX = TA_Clamp;
			Grad->AddressY = TA_Clamp;
			Grad->SRGB = false;
			FTexture2DMipMap& Mip = Grad->GetPlatformData()->Mips[0];
			FColor* Px = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
			for (int32 x = 0; x < W; ++x)
			{
				const float T01 = FMath::Clamp((float)x / (W - 1) / 0.5f, 0.f, 1.f);   // ilk yarida yukselir
				Px[x] = FColor(5, 2, 3, (uint8)FMath::RoundToInt32(158.f * FMath::SmoothStep(0.f, 1.f, T01)));
			}
			Mip.BulkData.Unlock();
			Grad->UpdateResource();
			GradientTexture = Grad;

			UImage* Side = T->ConstructWidget<UImage>(UImage::StaticClass());
			FSlateBrush GB;
			GB.SetResourceObject(Grad);
			GB.ImageSize = FVector2f(256.f, 1.f);
			Side->SetBrush(GB);
			UCanvasPanelSlot* SideSlot = Root->AddChildToCanvas(Side);
			SideSlot->SetAnchors(FAnchors(0.32f, 0.f, 1.f, 1.f));
			SideSlot->SetOffsets(FMargin(0.f));
		}
	}

	UVerticalBox* Col = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UCanvasPanelSlot* ColSlot = Root->AddChildToCanvas(Col);
	ColSlot->SetAnchors(FAnchors(0.77f, 0.5f));
	ColSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	ColSlot->SetAutoSize(true);

	// Logo (Python betigi T_FFF_Logo olarak ice aktarir); yoksa yazi
	Logo = T->ConstructWidget<UImage>(UImage::StaticClass());
	UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/FlipForFortune/UI/T_FFF_Logo.T_FFF_Logo"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Tex)
	{
		FSlateBrush B;
		B.SetResourceObject(Tex);
		B.ImageSize = FVector2f(620.f, 465.f);
		Logo->SetBrush(B);
		Logo->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Col->AddChildToVerticalBox(Sized(T, Logo, 620.f, 465.f))->SetHorizontalAlignment(HAlign_Center);
	}
	else
	{
		FallbackTitle = MenuText(T, TEXT("FLIP FOR FORTUNE"), 72, Gold);
		Col->AddChildToVerticalBox(FallbackTitle)->SetHorizontalAlignment(HAlign_Center);
	}

	// Para + koleksiyon bilgisi: tek bir hap icinde
	UBorder* Info = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	Info->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.45f), StyWithAlpha(Gold, 0.5f), 20.f, 1.5f));
	Info->SetPadding(FMargin(22.f, 8.f));
	MoneyText = MenuText(T, TEXT(""), 20, FLinearColor(1.f, 0.92f, 0.75f), true);
	MoneyText->SetJustification(ETextJustify::Center);
	Info->SetContent(MoneyText);
	UVerticalBoxSlot* MoneySlot = Col->AddChildToVerticalBox(Info);
	MoneySlot->SetHorizontalAlignment(HAlign_Center);
	MoneySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 22.f));

	UButton* Play = StyButton(T, StyText(T, TEXT("OYNA"), 40, FLinearColor(1.f, 0.96f, 0.85f), true, 220),
		FLinearColor(0.62f, 0.36f, 0.05f, 1.f), FLinearColor(1.f, 0.88f, 0.45f), 26.f);
	Play->OnClicked.AddDynamic(this, &UFortuneMainMenuWidget::OnPlayClicked);
	USizeBox* PlayBox = Sized(T, Play, 380.f, 88.f);
	PlayBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	PlayButtonBox = PlayBox;
	Col->AddChildToVerticalBox(PlayBox)->SetHorizontalAlignment(HAlign_Center);

	UButton* Quit = StyButton(T, StyText(T, TEXT("ÇIKIŞ"), 22, FLinearColor(1.f, 0.7f, 0.7f), true, 120),
		FLinearColor(0.16f, 0.04f, 0.05f, 0.9f), FLinearColor(1.f, 0.45f, 0.45f), 18.f);
	Quit->OnClicked.AddDynamic(this, &UFortuneMainMenuWidget::OnQuitClicked);
	UVerticalBoxSlot* QuitSlot = Col->AddChildToVerticalBox(Sized(T, Quit, 240.f, 54.f));
	QuitSlot->SetHorizontalAlignment(HAlign_Center);
	QuitSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

	UTextBlock* Version = MenuText(T, TEXT("v0.2"), 14, FLinearColor(1.f, 1.f, 1.f, 0.4f), false);
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
		int32 Owned = 0, Total = 0;
		for (const FFortuneCollectionEntry& E : F->GetCollection())
		{
			if (!E.Coin) continue;
			++Total;
			if (E.bOwned) ++Owned;
		}
		MoneyText->SetText(FText::FromString(F->GetMoney() > 0.0
			? FString::Printf(TEXT("$ %s     •     Koleksiyon %d/%d"), *UFortuneHUDWidget::FormatMoney(F->GetMoney()).ToString(), Owned, Total)
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

	// OYNA butonu hafifce nabiz gibi atar
	if (PlayButtonBox)
	{
		const float P = 1.f + 0.03f * (0.5f + 0.5f * FMath::Sin(Age * 3.f));
		PlayButtonBox->SetRenderScale(FVector2D(P, P));
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
	using namespace FortuneStyle;
	UWidgetTree* T = WidgetTree;
	const FLinearColor Purple(0.78f, 0.5f, 1.f);
	const FLinearColor Label(1.f, 1.f, 1.f, 0.5f);

	UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;

	UImage* Dim = T->ConstructWidget<UImage>(UImage::StaticClass());
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.62f));
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	UBorder* Outer = nullptr;
	UBorder* Panel = StyFrame(T, Purple, FLinearColor(0.05f, 0.035f, 0.075f, 0.97f), Outer, 22.f);
	USizeBox* PanelBox = FortuneStyle::StySized(T, Outer, 1040.f, 0.f);
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
	UImage* EmBg = T->ConstructWidget<UImage>(UImage::StaticClass());
	EmBg->SetBrush(StyRounded(FLinearColor(0.25f, 0.1f, 0.4f, 1.f), Purple, 30.f, 2.5f, FVector2D(60.f, 60.f)));
	Emblem->AddChildToOverlay(EmBg);
	UOverlaySlot* EmT = Emblem->AddChildToOverlay(FortuneStyle::StyText(T, TEXT("E"), 30, FLinearColor(0.95f, 0.85f, 1.f)));
	EmT->SetHorizontalAlignment(HAlign_Center);
	EmT->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* EmSlot = Head->AddChildToHorizontalBox(FortuneStyle::StySized(T, Emblem, 60.f, 60.f));
	EmSlot->SetVerticalAlignment(VAlign_Center);
	EmSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	UVerticalBox* TitleCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* TitleSlot = Head->AddChildToHorizontalBox(TitleCol);
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	TitleCol->AddChildToVerticalBox(FortuneStyle::StyText(T, TEXT("ENCHANT"), 32, Purple, true, 160));
	TitleCol->AddChildToVerticalBox(FortuneStyle::StyText(T, TEXT("Büyü kitabıyla coinine güç kat"), 14, Label, false));
	UBorder* ScrollPill = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	ScrollPill->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.5f), StyWithAlpha(Purple, 0.7f), 22.f, 1.5f));
	ScrollPill->SetPadding(FMargin(16.f, 6.f));
	ScrollText = FortuneStyle::StyText(T, TEXT("Scroll x0"), 22, FLinearColor(0.88f, 0.78f, 1.f));
	ScrollPill->SetContent(ScrollText);
	Head->AddChildToHorizontalBox(ScrollPill)->SetVerticalAlignment(VAlign_Center);

	Col->AddChildToVerticalBox(StyDivider(T, Purple))->SetPadding(FMargin(0.f, 14.f, 0.f, 14.f));

	UHorizontalBox* Main = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(Main);

	// ================= Sol: coin karti + mevcut enchant
	UVerticalBox* Left = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Main->AddChildToHorizontalBox(FortuneStyle::StySized(T, Left, 330.f, 0.f));

	UBorder* Card = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.35f), FLinearColor(1.f, 1.f, 1.f, 0.08f), 18.f, 1.f));
	Card->SetPadding(FMargin(16.f, 14.f));
	Left->AddChildToVerticalBox(Card);
	UVerticalBox* CardV = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Card->SetContent(CardV);
	UOverlay* IconO = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	CoinIcon = T->ConstructWidget<UImage>(UImage::StaticClass());
	IconO->AddChildToOverlay(CoinIcon);
	CoinLetter = FortuneStyle::StyText(T, TEXT("?"), 46, FLinearColor(0.f, 0.f, 0.f, 0.45f));
	UOverlaySlot* LS = IconO->AddChildToOverlay(CoinLetter);
	LS->SetHorizontalAlignment(HAlign_Center);
	LS->SetVerticalAlignment(VAlign_Center);
	CardV->AddChildToVerticalBox(FortuneStyle::StySized(T, IconO, 110.f, 110.f))->SetHorizontalAlignment(HAlign_Center);
	CoinNameText = FortuneStyle::StyText(T, TEXT("Coin"), 22, FLinearColor::White);
	CoinNameText->SetJustification(ETextJustify::Center);
	CardV->AddChildToVerticalBox(CoinNameText)->SetHorizontalAlignment(HAlign_Center);
	CoinInfoText = FortuneStyle::StyText(T, TEXT(""), 14, Label);
	CoinInfoText->SetJustification(ETextJustify::Center);
	CardV->AddChildToVerticalBox(CoinInfoText)->SetHorizontalAlignment(HAlign_Center);
	UHorizontalBox* Pick = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* PickSlot = CardV->AddChildToVerticalBox(Pick);
	PickSlot->SetHorizontalAlignment(HAlign_Center);
	PickSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	UButton* PrevCoin = FortuneStyle::StyButton(T, FortuneStyle::StyText(T, TEXT("< ÖNCEKİ"), 14, FLinearColor::White), FLinearColor(0.2f, 0.12f, 0.3f, 1.f), Purple, 10.f);
	PrevCoin->OnClicked.AddDynamic(this, &UFortuneEnchantWidget::OnPrevCoin);
	Pick->AddChildToHorizontalBox(FortuneStyle::StySized(T, PrevCoin, 130.f, 36.f))->SetPadding(FMargin(4.f, 0.f));
	UButton* NextCoin = FortuneStyle::StyButton(T, FortuneStyle::StyText(T, TEXT("SONRAKİ >"), 14, FLinearColor::White), FLinearColor(0.2f, 0.12f, 0.3f, 1.f), Purple, 10.f);
	NextCoin->OnClicked.AddDynamic(this, &UFortuneEnchantWidget::OnNextCoin);
	Pick->AddChildToHorizontalBox(FortuneStyle::StySized(T, NextCoin, 130.f, 36.f))->SetPadding(FMargin(4.f, 0.f));

	Left->AddChildToVerticalBox(FortuneStyle::StyText(T, TEXT("MEVCUT ENCHANT"), 13, Label, true, 60))->SetPadding(FMargin(0.f, 16.f, 0.f, 6.f));
	CurrentBox = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	CurrentBox->SetPadding(FMargin(14.f, 10.f));
	UVerticalBox* CurV = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CurrentBox->SetContent(CurV);
	CurrentTier = FortuneStyle::StyText(T, TEXT(""), 14, FLinearColor::White);
	CurV->AddChildToVerticalBox(CurrentTier);
	CurrentStat = FortuneStyle::StyText(T, TEXT(""), 20, FLinearColor::White);
	CurV->AddChildToVerticalBox(CurrentStat);
	CurrentDesc = FortuneStyle::StyText(T, TEXT(""), 13, FLinearColor(1.f, 1.f, 1.f, 0.6f), false);
	CurrentDesc->SetAutoWrapText(true);
	CurV->AddChildToVerticalBox(CurrentDesc);
	Left->AddChildToVerticalBox(CurrentBox);

	// ================= Sag: sanslar, sonuc, olasi enchantlar
	UVerticalBox* Right = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* RightSlot = Main->AddChildToHorizontalBox(Right);
	RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RightSlot->SetPadding(FMargin(22.f, 0.f, 0.f, 0.f));

	Right->AddChildToVerticalBox(FortuneStyle::StyText(T, TEXT("TIER ŞANSLARI"), 13, Label, true, 60))->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	for (int32 Tier = 1; Tier <= 3; ++Tier)
	{
		const FLinearColor C = TierColor(Tier);
		UHorizontalBox* TR = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Right->AddChildToVerticalBox(TR)->SetPadding(FMargin(0.f, 3.f));
		TR->AddChildToHorizontalBox(FortuneStyle::StySized(T, FortuneStyle::StyText(T, TierName(Tier), 14, C), 210.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		FStyBar Bar = StyMakeBar(T, C, 12.f);
		UHorizontalBoxSlot* BarSlot = TR->AddChildToHorizontalBox(Bar.Root);
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		TierFill.Add(Bar.FillSlot);
		TierRest.Add(Bar.RestSlot);
		TierFillBorder.Add(Bar.Fill);
		UTextBlock* Pct = FortuneStyle::StyText(T, TEXT("%0"), 18, FLinearColor::White);
		Pct->SetJustification(ETextJustify::Right);
		TR->AddChildToHorizontalBox(FortuneStyle::StySized(T, Pct, 74.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		ChanceTexts.Add(Pct);
	}

	Right->AddChildToVerticalBox(FortuneStyle::StyText(T, TEXT("SONUÇ"), 13, Label, true, 60))->SetPadding(FMargin(0.f, 14.f, 0.f, 6.f));
	ResultBox = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	ResultBox->SetPadding(FMargin(16.f, 10.f));
	ResultBox->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.4f), FLinearColor(1.f, 1.f, 1.f, 0.1f), 16.f, 2.f));
	ResultBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	UVerticalBox* ResV = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ResultBox->SetContent(FortuneStyle::StySized(T, ResV, 0.f, 88.f));
	ResultTier = FortuneStyle::StyText(T, TEXT(""), 16, FLinearColor::White);
	ResV->AddChildToVerticalBox(ResultTier)->SetHorizontalAlignment(HAlign_Center);
	ResultStat = FortuneStyle::StyText(T, TEXT("?"), 30, FLinearColor::White);
	ResV->AddChildToVerticalBox(ResultStat)->SetHorizontalAlignment(HAlign_Center);
	ResultHint = FortuneStyle::StyText(T, TEXT("Enchant'a bas ve şansını dene!"), 13, FLinearColor(1.f, 1.f, 1.f, 0.55f), false);
	ResV->AddChildToVerticalBox(ResultHint)->SetHorizontalAlignment(HAlign_Center);
	Right->AddChildToVerticalBox(ResultBox);

	// Olasi enchantlar tablosu
	Right->AddChildToVerticalBox(FortuneStyle::StyText(T, TEXT("OLASI ENCHANTLAR"), 13, Label, true, 60))->SetPadding(FMargin(0.f, 14.f, 0.f, 4.f));
	UHorizontalBox* TH = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Right->AddChildToVerticalBox(TH)->SetPadding(FMargin(10.f, 0.f, 22.f, 2.f));
	TH->AddChildToHorizontalBox(FortuneStyle::StyText(T, TEXT("BOOST"), 12, Label))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	for (int32 Tier = 1; Tier <= 3; ++Tier)
	{
		UTextBlock* H = FortuneStyle::StyText(T, FString::Printf(TEXT("T%d"), Tier), 12, TierColor(Tier));
		H->SetJustification(ETextJustify::Right);
		TH->AddChildToHorizontalBox(FortuneStyle::StySized(T, H, 70.f, 0.f));
	}
	UBorder* TableBg = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	TableBg->SetBrush(StyRounded(FLinearColor(0.f, 0.f, 0.f, 0.35f), FLinearColor(1.f, 1.f, 1.f, 0.06f), 12.f, 1.f));
	TableBg->SetPadding(FMargin(6.f));
	USizeBox* TableBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	TableBox->SetMaxDesiredHeight(200.f);
	TableBox->SetContent(TableBg);
	Right->AddChildToVerticalBox(TableBox);
	UScrollBox* TableScroll = T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	TableBg->SetContent(TableScroll);
	{
		const UFortuneSettings* S = GetDefault<UFortuneSettings>();
		for (const TPair<EFortuneEnchantStat, FFortuneEnchantStatConfig>& P : S->EnchantStats)
		{
			if (P.Value.RollWeight <= 0.f) continue;
			UBorder* RowB = T->ConstructWidget<UBorder>(UBorder::StaticClass());
			RowB->SetPadding(FMargin(8.f, 3.f));
			RowB->SetBrush(StyRounded(FLinearColor(1.f, 1.f, 1.f, 0.03f), FLinearColor::Transparent, 8.f, 0.f));
			RowB->SetToolTipText(FText::FromString(UFortuneSubsystem::GetEnchantDescription(P.Key)));
			TableScroll->AddChild(RowB);
			UHorizontalBox* R = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			RowB->SetContent(R);
			R->AddChildToHorizontalBox(FortuneStyle::StyText(T, P.Value.DisplayName.ToString(), 14, FLinearColor(1.f, 1.f, 1.f, 0.9f), false))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			for (int32 Tier = 1; Tier <= 3; ++Tier)
			{
				const float V = P.Value.GetValue(Tier);
				const FString VS = P.Value.bPercent ? FString::Printf(TEXT("+%%%.0f"), V * 100.f) : FString::Printf(TEXT("+%.1f"), V);
				UTextBlock* VT = FortuneStyle::StyText(T, VS, 14, TierColor(Tier));
				VT->SetJustification(ETextJustify::Right);
				R->AddChildToHorizontalBox(FortuneStyle::StySized(T, VT, 70.f, 0.f));
			}
			StatRows.Add(RowB);
			StatRowStats.Add(P.Key);
		}
	}

	Col->AddChildToVerticalBox(FortuneStyle::StyText(T, TEXT("Yeni enchant mevcut enchant'ın yerine geçer."), 13, FLinearColor(1.f, 0.7f, 0.5f, 0.8f), false))
		->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	// ---- Butonlar
	UHorizontalBox* Buttons = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* BtnSlot = Col->AddChildToVerticalBox(Buttons);
	BtnSlot->SetHorizontalAlignment(HAlign_Center);
	BtnSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	RollText = FortuneStyle::StyText(T, TEXT("ENCHANT"), 26, FLinearColor(0.95f, 0.85f, 1.f), true, 120);
	RollButton = FortuneStyle::StyButton(T, RollText, FLinearColor(0.34f, 0.12f, 0.52f, 1.f), FLinearColor(0.9f, 0.65f, 1.f), 18.f);
	RollButton->OnClicked.AddDynamic(this, &UFortuneEnchantWidget::OnRollClicked);
	USizeBox* RB = FortuneStyle::StySized(T, RollButton, 300.f, 62.f);
	RB->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	RollBox = RB;
	Buttons->AddChildToHorizontalBox(RB)->SetPadding(FMargin(6.f, 0.f));
	UButton* Close = FortuneStyle::StyButton(T, FortuneStyle::StyText(T, TEXT("KAPAT"), 22, FLinearColor(1.f, 0.6f, 0.6f)), FLinearColor(0.25f, 0.06f, 0.06f, 1.f), FLinearColor(1.f, 0.4f, 0.4f), 18.f);
	Close->OnClicked.AddDynamic(this, &UFortuneEnchantWidget::OnCloseClicked);
	Buttons->AddChildToHorizontalBox(FortuneStyle::StySized(T, Close, 180.f, 62.f))->SetPadding(FMargin(6.f, 0.f));
}

void UFortuneEnchantWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Age = 0.f;
	FortuneStyle::StyOpenAnim(PanelRoot, 0.f);
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
		const FLinearColor RC = UFortuneSubsystem::GetRarityColor(Coin->Rarity);
		CoinNameText->SetText(Coin->DisplayName);
		CoinNameText->SetColorAndOpacity(FSlateColor(RC));
		CoinInfoText->SetText(FText::FromString(bOwned
			? FString::Printf(TEXT("%s  •  Lv %d"), *UFortuneSubsystem::GetRarityName(Coin->Rarity), Owned.Level)
			: UFortuneSubsystem::GetRarityName(Coin->Rarity)));
		// Coin simgesi: rengi + nadirlik halkasi (enchantliysa tier renginde)
		const FLinearColor Ring = (bOwned && Owned.Enchant.IsActive()) ? TierColor(Owned.Enchant.Tier) : RC;
		CoinIcon->SetBrush(FortuneStyle::StyRounded(bOwned ? Coin->IconColor : FLinearColor(0.05f, 0.05f, 0.06f), Ring, 55.f, 4.f, FVector2D(110.f, 110.f)));
		const FString Name = Coin->DisplayName.ToString();
		CoinLetter->SetText(FText::FromString(Name.Len() > 0 ? Name.Left(1).ToUpper() : FString(TEXT("?"))));
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
		if (TierFill.IsValidIndex(i))
		{
			FortuneStyle::FStyBar Bar;
			Bar.FillSlot = TierFill[i];
			Bar.RestSlot = TierRest[i];
			Bar.Fill = TierFillBorder[i];
			FortuneStyle::StySetBar(Bar, Ch[i]);
		}
	}

	ShowEnchant(CurrentBox, CurrentTier, CurrentStat, bOwned ? Owned.Enchant : FFortuneEnchant(), false);
	if (CurrentDesc)
	{
		CurrentDesc->SetText(FText::FromString((bOwned && Owned.Enchant.IsActive())
			? UFortuneSubsystem::GetEnchantDescription(Owned.Enchant.Stat)
			: FString(TEXT("Bu coinde henüz enchant yok."))));
	}

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
				HighlightRow = StatRowStats.IndexOfByKey(Fake.Stat);
				HighlightTier = Fake.Tier;
				HighlightTime = 0.35f;
			}
		}
		if (RollTime >= Dur)
		{
			bRolling = false;
			ShowEnchant(ResultBox, ResultTier, ResultStat, RollResult, true);
			ResultHint->SetText(FText::FromString(RollResult.Tier >= 3 ? TEXT("EFSANEVİ!") : (RollResult.Tier == 2 ? TEXT("Harika!") : TEXT("Enchant uygulandı."))));
			ResultHint->SetColorAndOpacity(FSlateColor(TierColor(RollResult.Tier)));
			ResultPop = 1.f;
			HighlightRow = StatRowStats.IndexOfByKey(RollResult.Stat);
			HighlightTier = RollResult.Tier;
			HighlightTime = 2.5f;
			Refresh();
		}
	}

	Age += InDeltaTime;
	FortuneStyle::StyOpenAnim(PanelRoot, Age);

	// Olasi enchant tablosu: carkin durdugu / cikan satir parlar
	HighlightTime = FMath::Max(0.f, HighlightTime - InDeltaTime);
	for (int32 i = 0; i < StatRows.Num(); ++i)
	{
		const float H = (i == HighlightRow) ? FMath::Clamp(HighlightTime * 3.f, 0.f, 1.f) : 0.f;
		const FLinearColor C = TierColor(HighlightTier);
		StatRows[i]->SetBrush(FortuneStyle::StyRounded(FMath::Lerp(FLinearColor(1.f, 1.f, 1.f, 0.03f), FLinearColor(C.R * 0.35f, C.G * 0.35f, C.B * 0.35f, 0.9f), H),
			FortuneStyle::StyWithAlpha(C, H), 8.f, 1.5f * H));
	}

	// Enchant yapilabiliyorsa buton nabiz gibi atar
	if (RollBox)
	{
		const bool bReady = RollButton && RollButton->GetIsEnabled() && !bRolling;
		const float S = bReady ? 1.f + 0.035f * (0.5f + 0.5f * FMath::Sin(Age * 4.f)) : 1.f;
		RollBox->SetRenderScale(FVector2D(S, S));
	}

	// Sonuc gelince kutu buyuyup oturur
	if (ResultPop > 0.f && ResultBox)
	{
		ResultPop = FMath::Max(0.f, ResultPop - InDeltaTime * 3.f);
		const float S = 1.f + 0.12f * ResultPop;
		ResultBox->SetRenderScale(FVector2D(S, S));
	}
}
