#include "FortuneCollectionWidget.h"
#include "FortuneSubsystem.h"
#include "FortuneCoinData.h"
#include "FortuneSettings.h"
#include "FortuneCoinPreview.h"
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
#include "Components/ScrollBox.h"
#include "Components/ProgressBar.h"
#include "Components/Spacer.h"
#include "Styling/SlateTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"

namespace FortuneColUI
{
	const FLinearColor ColGold(1.f, 0.78f, 0.3f, 1.f);
	const FLinearColor ColPanel(0.05f, 0.05f, 0.06f, 0.82f);
	const FLinearColor ColPanelLight(0.16f, 0.16f, 0.18f, 0.85f);

	FSlateBrush ColRounded(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float OutlineWidth, FVector2D Size = FVector2D(32.f, 32.f))
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.ImageSize = FVector2f(Size);
		B.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	UTextBlock* ColText(UWidgetTree* Tree, const FString& Str, int32 Size, const FLinearColor& Color, bool bBold = true)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		F.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		T->SetFont(F);
		T->SetText(FText::FromString(Str));
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetShadowOffset(FVector2D(1.5f, 1.5f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));
		return T;
	}

	UBorder* ColPill(UWidgetTree* Tree, const FLinearColor& Fill, float Radius, const FMargin& Padding)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		B->SetBrush(ColRounded(Fill, FLinearColor(1.f, 1.f, 1.f, 0.06f), Radius, 1.f));
		B->SetPadding(Padding);
		return B;
	}

	FButtonStyle ColButtonStyle(const FLinearColor& Accent, float Radius = 14.f)
	{
		FButtonStyle S;
		S.SetNormal(ColRounded(ColPanelLight, FLinearColor(1.f, 1.f, 1.f, 0.08f), Radius, 1.f));
		S.SetHovered(ColRounded(FLinearColor(0.24f, 0.24f, 0.27f, 0.92f), Accent, Radius, 2.f));
		S.SetPressed(ColRounded(FLinearColor(0.1f, 0.1f, 0.12f, 0.95f), Accent, Radius, 2.f));
		S.SetDisabled(ColRounded(FLinearColor(0.08f, 0.08f, 0.09f, 0.7f), FLinearColor(1.f, 1.f, 1.f, 0.04f), Radius, 1.f));
		S.SetNormalPadding(FMargin(14.f, 6.f));
		S.SetPressedPadding(FMargin(14.f, 7.f, 14.f, 5.f));
		return S;
	}

	UButton* ColButton(UWidgetTree* Tree, UTextBlock* Label, const FLinearColor& Accent)
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetStyle(ColButtonStyle(Accent));
		Btn->SetContent(Label);
		return Btn;
	}

	FLinearColor RarityColor(EFortuneRarity R)
	{
		switch (R)
		{
		case EFortuneRarity::Common:    return FLinearColor(0.82f, 0.78f, 0.72f);
		case EFortuneRarity::Uncommon:  return FLinearColor(0.4f, 0.95f, 0.45f);
		case EFortuneRarity::Rare:      return FLinearColor(0.35f, 0.65f, 1.f);
		case EFortuneRarity::Epic:      return FLinearColor(0.75f, 0.4f, 1.f);
		case EFortuneRarity::Legendary: return FLinearColor(1.f, 0.78f, 0.2f);
		default:                        return FLinearColor(1.f, 0.3f, 0.35f);
		}
	}

	FString RarityName(EFortuneRarity R)
	{
		switch (R)
		{
		case EFortuneRarity::Common:    return TEXT("COMMON");
		case EFortuneRarity::Uncommon:  return TEXT("UNCOMMON");
		case EFortuneRarity::Rare:      return TEXT("RARE");
		case EFortuneRarity::Epic:      return TEXT("EPIC");
		case EFortuneRarity::Legendary: return TEXT("LEGENDARY");
		default:                        return TEXT("MYTHIC");
		}
	}

	FString WithCommas(int64 N)
	{
		FString S = FString::Printf(TEXT("%lld"), N);
		for (int32 i = S.Len() - 3; i > 0; i -= 3)
		{
			S.InsertAt(i, TEXT(","));
		}
		return S;
	}
}

using namespace FortuneColUI;

// ============================================================ Slot

void UFortuneCollectionSlot::Setup(UFortuneCollectionWidget* InOwner, int32 InIndex)
{
	Owner = InOwner;
	Index = InIndex;
	OnClicked.AddUniqueDynamic(this, &UFortuneCollectionSlot::HandleClicked);
}

void UFortuneCollectionSlot::HandleClicked()
{
	if (Owner.IsValid())
	{
		Owner->SelectIndex(Index);
	}
}

// ============================================================ Widget

UFortuneSubsystem* UFortuneCollectionWidget::GetFortune() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
}

TSharedRef<SWidget> UFortuneCollectionWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneCollectionWidget::BuildTree()
{
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;

	// Hafif karartma (dunyaya tiklamayi da engeller)
	UImage* Dim = T->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Dim"));
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.08f));
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	// Alt koyu bant
	UImage* Band = T->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Band"));
	Band->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	UCanvasPanelSlot* BandSlot = Root->AddChildToCanvas(Band);
	BandSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
	BandSlot->SetAlignment(FVector2D(0.f, 1.f));
	BandSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 360.f));

	// Sol ust baslik
	UVerticalBox* TitleBox = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TitleBox"));
	UCanvasPanelSlot* TitleSlot = Root->AddChildToCanvas(TitleBox);
	TitleSlot->SetAnchors(FAnchors(0.f, 0.f));
	TitleSlot->SetPosition(FVector2D(48.f, 36.f));
	TitleSlot->SetAutoSize(true);
	TitleBox->AddChildToVerticalBox(ColText(T, TEXT("COIN COLLECTION"), 34, ColGold));
	CountText = ColText(T, TEXT("0/0"), 18, FLinearColor(1.f, 1.f, 1.f, 0.75f), false);
	TitleBox->AddChildToVerticalBox(CountText);

	// Toast (enchant sonucu vb.)
	ToastText = ColText(T, TEXT(""), 26, ColGold);
	ToastText->SetJustification(ETextJustify::Center);
	ToastText->SetRenderOpacity(0.f);
	UCanvasPanelSlot* ToastSlot = Root->AddChildToCanvas(ToastText);
	ToastSlot->SetAnchors(FAnchors(0.5f, 1.f));
	ToastSlot->SetAlignment(FVector2D(0.5f, 1.f));
	ToastSlot->SetPosition(FVector2D(0.f, -380.f));
	ToastSlot->SetAutoSize(true);

	// ---------------- Alt alan ----------------
	UHorizontalBox* Bottom = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Bottom"));
	UCanvasPanelSlot* BottomSlot = Root->AddChildToCanvas(Bottom);
	BottomSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
	BottomSlot->SetAlignment(FVector2D(0.f, 1.f));
	BottomSlot->SetOffsets(FMargin(40.f, -20.f, 40.f, 320.f));

	// ===== Sol kolon
	UVerticalBox* Left = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Left"));
	UHorizontalBoxSlot* LeftSlot = Bottom->AddChildToHorizontalBox(Left);
	LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LeftSlot->SetHorizontalAlignment(HAlign_Fill);

	// Ust satir: [DEGER ^ 12.5 | 1/250]  ......  [Coin Adi]
	UHorizontalBox* TopRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopRow"));
	UVerticalBoxSlot* TopRowSlot = Left->AddChildToVerticalBox(TopRow);
	TopRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	UBorder* EarnPill = ColPill(T, ColPanel, 18.f, FMargin(20.f, 8.f));
	UHorizontalBox* EarnRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	EarnPill->SetContent(EarnRow);
	EarnRow->AddChildToHorizontalBox(ColText(T, TEXT("DEĞER ↑"), 22, FLinearColor(0.4f, 1.f, 0.45f)))->SetVerticalAlignment(VAlign_Center);
	EarnText = ColText(T, TEXT("0"), 22, FLinearColor::White);
	UHorizontalBoxSlot* EarnTextSlot = EarnRow->AddChildToHorizontalBox(EarnText);
	EarnTextSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	EarnTextSlot->SetVerticalAlignment(VAlign_Center);
	EarnRow->AddChildToHorizontalBox(T->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetPadding(FMargin(18.f, 0.f, 0.f, 0.f));
	OddsText = ColText(T, TEXT("1/1"), 22, FLinearColor(1.f, 0.95f, 0.45f));
	EarnRow->AddChildToHorizontalBox(OddsText)->SetVerticalAlignment(VAlign_Center);
	TopRow->AddChildToHorizontalBox(EarnPill);

	TopRow->AddChildToHorizontalBox(T->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UBorder* NamePill = ColPill(T, ColPanel, 18.f, FMargin(28.f, 8.f));
	NameText = ColText(T, TEXT("Coin"), 24, ColGold);
	NamePill->SetContent(NameText);
	TopRow->AddChildToHorizontalBox(NamePill);

	// Coin seridi
	UBorder* StripPanel = ColPill(T, ColPanel, 16.f, FMargin(10.f));
	Left->AddChildToVerticalBox(StripPanel);
	UScrollBox* Scroll = T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Strip"));
	Scroll->SetOrientation(Orient_Horizontal);
	Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	StripPanel->SetContent(Scroll);
	SlotRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotRow"));
	Scroll->AddChild(SlotRow);

	// Butonlar: TAK | GERI | ENCHANT
	UHorizontalBox* Buttons = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	UVerticalBoxSlot* ButtonsSlot = Left->AddChildToVerticalBox(Buttons);
	ButtonsSlot->SetHorizontalAlignment(HAlign_Center);
	ButtonsSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	auto AddButton = [&](UButton* Btn)
	{
		USizeBox* Box = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(210.f);
		Box->SetHeightOverride(50.f);
		Box->SetContent(Btn);
		Buttons->AddChildToHorizontalBox(Box)->SetPadding(FMargin(6.f, 0.f));
	};
	EquipText = ColText(T, TEXT("TAK"), 24, FLinearColor::White);
	EquipButton = ColButton(T, EquipText, FLinearColor(0.4f, 1.f, 0.45f));
	EquipButton->OnClicked.AddDynamic(this, &UFortuneCollectionWidget::OnEquipClicked);
	AddButton(EquipButton);

	UButton* Back = ColButton(T, ColText(T, TEXT("GERİ"), 24, FLinearColor(1.f, 0.3f, 0.3f)), FLinearColor(1.f, 0.3f, 0.3f));
	Back->OnClicked.AddDynamic(this, &UFortuneCollectionWidget::OnBackClicked);
	AddButton(Back);

	EnchantText = ColText(T, TEXT("ENCHANT"), 22, FLinearColor(0.85f, 0.7f, 1.f));
	EnchantButton = ColButton(T, EnchantText, FLinearColor(0.7f, 0.45f, 1.f));
	EnchantButton->OnClicked.AddDynamic(this, &UFortuneCollectionWidget::OnEnchantClicked);
	AddButton(EnchantButton);

	// ===== Sag kolon: BILGILER
	UVerticalBox* Right = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Right"));
	USizeBox* RightBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RightBox->SetWidthOverride(520.f);
	RightBox->SetContent(Right);
	UHorizontalBoxSlot* RightSlot = Bottom->AddChildToHorizontalBox(RightBox);
	RightSlot->SetPadding(FMargin(18.f, 0.f, 0.f, 0.f));
	RightSlot->SetVerticalAlignment(VAlign_Fill);

	UBorder* InfoHeader = ColPill(T, ColPanelLight, 18.f, FMargin(24.f, 8.f));
	UHorizontalBox* InfoHeaderRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	InfoHeader->SetContent(InfoHeaderRow);
	InfoHeaderRow->AddChildToHorizontalBox(ColText(T, TEXT("BİLGİLER"), 24, FLinearColor::White));
	InfoHeaderRow->AddChildToHorizontalBox(T->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RarityText = ColText(T, TEXT("COMMON"), 20, FLinearColor::White);
	InfoHeaderRow->AddChildToHorizontalBox(RarityText)->SetVerticalAlignment(VAlign_Center);
	Right->AddChildToVerticalBox(InfoHeader)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	UBorder* InfoPanel = ColPill(T, ColPanel, 16.f, FMargin(20.f, 12.f));
	UVerticalBoxSlot* InfoPanelSlot = Right->AddChildToVerticalBox(InfoPanel);
	InfoPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UVerticalBox* Info = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	InfoPanel->SetContent(Info);

	// Sayfa basligi: <  ISTATISTIK  >
	UHorizontalBox* PageRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Info->AddChildToVerticalBox(PageRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	UButton* Prev = ColButton(T, ColText(T, TEXT("<"), 20, FLinearColor::White), ColGold);
	Prev->OnClicked.AddDynamic(this, &UFortuneCollectionWidget::OnPrevPage);
	PageRow->AddChildToHorizontalBox(Prev);
	PageTitle = ColText(T, TEXT("AÇIKLAMA"), 22, FLinearColor::White);
	PageTitle->SetJustification(ETextJustify::Center);
	UHorizontalBoxSlot* PageTitleSlot = PageRow->AddChildToHorizontalBox(PageTitle);
	PageTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	PageTitleSlot->SetVerticalAlignment(VAlign_Center);
	UButton* Next = ColButton(T, ColText(T, TEXT(">"), 20, FLinearColor::White), ColGold);
	Next->OnClicked.AddDynamic(this, &UFortuneCollectionWidget::OnNextPage);
	PageRow->AddChildToHorizontalBox(Next);

	// Govde: tasarsa kaydirilabilir alan (yazilar XP cubuguyla cakismasin)
	UScrollBox* BodyScroll = T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BodyScroll"));
	BodyScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	BodyScroll->SetClipping(EWidgetClipping::ClipToBounds);
	UVerticalBoxSlot* BodySlot = Info->AddChildToVerticalBox(BodyScroll);
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	PageBody = ColText(T, TEXT(""), 15, FLinearColor(1.f, 1.f, 1.f, 0.9f), false);
	PageBody->SetAutoWrapText(true);
	BodyScroll->AddChild(PageBody);

	// Seviye + XP cubugu
	UHorizontalBox* XPRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Info->AddChildToVerticalBox(XPRow)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	LevelText = ColText(T, TEXT("LV 1"), 20, ColGold);
	XPRow->AddChildToHorizontalBox(LevelText)->SetVerticalAlignment(VAlign_Center);
	XPBar = T->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	XPBar->SetFillColorAndOpacity(ColGold);
	XPBar->SetPercent(0.f);
	USizeBox* BarBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarBox->SetHeightOverride(14.f);
	BarBox->SetContent(XPBar);
	UHorizontalBoxSlot* BarSlot = XPRow->AddChildToHorizontalBox(BarBox);
	BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BarSlot->SetVerticalAlignment(VAlign_Center);
	BarSlot->SetPadding(FMargin(12.f, 0.f));
	XPText = ColText(T, TEXT("0/10 XP"), 16, FLinearColor(1.f, 1.f, 1.f, 0.8f), false);
	XPRow->AddChildToHorizontalBox(XPText)->SetVerticalAlignment(VAlign_Center);
}

void UFortuneCollectionWidget::BuildSlots()
{
	SlotRow->ClearChildren();
	Slots.Reset();
	UWidgetTree* T = WidgetTree;

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		UFortuneCollectionSlot* S = T->ConstructWidget<UFortuneCollectionSlot>(UFortuneCollectionSlot::StaticClass());
		S->Setup(this, i);

		USizeBox* Box = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(96.f);
		Box->SetHeightOverride(96.f);
		Box->SetContent(S);
		SlotRow->AddChildToHorizontalBox(Box)->SetPadding(FMargin(5.f, 0.f));

		UOverlay* O = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		S->SetContent(O);

		S->Icon = T->ConstructWidget<UImage>(UImage::StaticClass());
		UOverlaySlot* IconSlot = O->AddChildToOverlay(S->Icon);
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));

		S->LevelText = ColText(T, TEXT(""), 18, FLinearColor::White);
		UOverlaySlot* LvSlot = O->AddChildToOverlay(S->LevelText);
		LvSlot->SetHorizontalAlignment(HAlign_Center);
		LvSlot->SetVerticalAlignment(VAlign_Bottom);

		S->EquippedBadge = T->ConstructWidget<UImage>(UImage::StaticClass());
		S->EquippedBadge->SetBrush(ColRounded(FLinearColor(0.3f, 1.f, 0.4f), FLinearColor(0.f, 0.f, 0.f, 0.5f), 6.f, 1.5f, FVector2D(12.f, 12.f)));
		UOverlaySlot* EqSlot = O->AddChildToOverlay(S->EquippedBadge);
		EqSlot->SetHorizontalAlignment(HAlign_Right);
		EqSlot->SetVerticalAlignment(VAlign_Top);

		S->EnchantBadge = T->ConstructWidget<UImage>(UImage::StaticClass());
		S->EnchantBadge->SetBrush(ColRounded(FLinearColor(0.7f, 0.4f, 1.f), FLinearColor(0.f, 0.f, 0.f, 0.5f), 3.f, 1.5f, FVector2D(10.f, 14.f)));
		UOverlaySlot* EnSlot = O->AddChildToOverlay(S->EnchantBadge);
		EnSlot->SetHorizontalAlignment(HAlign_Left);
		EnSlot->SetVerticalAlignment(VAlign_Top);

		Slots.Add(S);
	}
}

void UFortuneCollectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);

	UFortuneSubsystem* F = GetFortune();
	if (F)
	{
		F->OnCoinsChanged.AddDynamic(this, &UFortuneCollectionWidget::HandleCoinsChanged);
		F->OnInventoryChanged.AddDynamic(this, &UFortuneCollectionWidget::HandleCoinsChanged);
		Entries = F->GetCollection();
		Selected = 0;
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			if (Entries[i].bEquipped) Selected = i;
		}
	}
	BuildSlots();

	APlayerController* PC = GetOwningPlayer();
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(PC))
	{
		FPC->SetMenuOpen(true);
	}

	// 3D coin onizlemesi: kameranin onunde, biraz yukarida
	if (PC && PC->PlayerCameraManager && GetWorld())
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Preview = GetWorld()->SpawnActor<AFortuneCoinPreview>(AFortuneCoinPreview::StaticClass(), FTransform::Identity, P);
		// Vitrin odasi varsa kamerayi oraya gecir, coini kaidenin ustune koy
		ACameraActor* ShowCam = nullptr;
		AActor* ShowPoint = nullptr;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("CollectionCamera"))) ShowCam = Cast<ACameraActor>(*It);
			if (It->ActorHasTag(TEXT("CollectionPreviewPoint"))) ShowPoint = *It;
		}
		if (Preview && ShowCam && ShowPoint)
		{
			PrevViewTarget = PC->GetViewTarget();
			PC->SetViewTargetWithBlend(ShowCam, 0.35f, VTBlend_EaseInOut, 2.f);
			Preview->PlaceAt(ShowPoint->GetActorLocation(), ShowCam->GetActorLocation(), FVector::UpVector, 4.f);
		}
		else if (Preview)
		{
			FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
			const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
			// Onizleme ekranin ust-ortasinda dursun (kameranin biraz yukarisina bak)
			FRotator AimRot = CamRot;
			AimRot.Pitch += 3.f;
			Preview->PlaceInFrontOf(CamLoc, AimRot, 30.f, 0.f);
		}
	}

	RefreshAll();
}

void UFortuneCollectionWidget::NativeDestruct()
{
	if (UFortuneSubsystem* F = GetFortune())
	{
		F->OnCoinsChanged.RemoveDynamic(this, &UFortuneCollectionWidget::HandleCoinsChanged);
		F->OnInventoryChanged.RemoveDynamic(this, &UFortuneCollectionWidget::HandleCoinsChanged);
	}
	if (Preview)
	{
		Preview->Destroy();
		Preview = nullptr;
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (PrevViewTarget.IsValid())
		{
			PC->SetViewTargetWithBlend(PrevViewTarget.Get(), 0.35f, VTBlend_EaseInOut, 2.f);
		}
	}
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
	{
		FPC->SetMenuOpen(false);
	}
	Super::NativeDestruct();
}

void UFortuneCollectionWidget::HandleCoinsChanged()
{
	if (UFortuneSubsystem* F = GetFortune())
	{
		const int32 OldNum = Entries.Num();
		Entries = F->GetCollection();
		if (Entries.Num() != OldNum)
		{
			BuildSlots();
		}
	}
	RefreshAll();
}

void UFortuneCollectionWidget::SelectIndex(int32 NewIndex)
{
	if (!Entries.IsValidIndex(NewIndex)) return;
	Selected = NewIndex;
	RefreshAll();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void UFortuneCollectionWidget::RefreshAll()
{
	Selected = Entries.Num() > 0 ? FMath::Clamp(Selected, 0, Entries.Num() - 1) : 0;
	RefreshSlots();
	RefreshDetails();
}

void UFortuneCollectionWidget::RefreshSlots()
{
	int32 OwnedCount = 0;
	for (int32 i = 0; i < Slots.Num() && i < Entries.Num(); ++i)
	{
		const FFortuneCollectionEntry& E = Entries[i];
		UFortuneCollectionSlot* S = Slots[i];
		if (!S || !E.Coin) continue;
		OwnedCount += E.bOwned ? 1 : 0;

		const FLinearColor RC = RarityColor(E.Coin->Rarity);
		const bool bSel = (i == Selected);

		FButtonStyle Style = ColButtonStyle(RC, 12.f);
		const FSlateBrush Normal = ColRounded(bSel ? FLinearColor(0.25f, 0.25f, 0.28f, 0.95f) : ColPanelLight,
			bSel ? RC : FLinearColor(RC.R, RC.G, RC.B, 0.35f), 12.f, bSel ? 3.f : 1.5f);
		Style.SetNormal(Normal);
		Style.SetNormalPadding(FMargin(4.f));
		Style.SetPressedPadding(FMargin(4.f, 5.f, 4.f, 3.f));
		S->SetStyle(Style);

		if (E.bOwned)
		{
			if (E.Coin->Icon)
			{
				FSlateBrush IconBrush;
				IconBrush.SetResourceObject(E.Coin->Icon);
				IconBrush.ImageSize = FVector2f(58.f, 58.f);
				S->Icon->SetBrush(IconBrush);
			}
			else
			{
				const FLinearColor C = E.Coin->IconColor;
				S->Icon->SetBrush(ColRounded(C, C * 0.55f, 29.f, 4.f, FVector2D(58.f, 58.f)));
			}
			S->LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv%d"), E.Level)));
			S->LevelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
		else
		{
			S->Icon->SetBrush(ColRounded(FLinearColor(0.03f, 0.03f, 0.035f), FLinearColor(1.f, 1.f, 1.f, 0.08f), 29.f, 2.f, FVector2D(58.f, 58.f)));
			S->LevelText->SetText(FText::FromString(TEXT("?")));
			S->LevelText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4f)));
		}
		S->EquippedBadge->SetVisibility(E.bEquipped ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		S->EnchantBadge->SetVisibility(E.bEnchanted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (CountText)
	{
		CountText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d topladın"), OwnedCount, Entries.Num())));
	}
}

void UFortuneCollectionWidget::RefreshDetails()
{
	UFortuneSubsystem* F = GetFortune();
	if (!F || !Entries.IsValidIndex(Selected) || !Entries[Selected].Coin) return;

	const FFortuneCollectionEntry& E = Entries[Selected];
	const UFortuneCoinData* C = E.Coin;
	const UFortuneSettings* S = GetDefault<UFortuneSettings>();
	const FLinearColor RC = RarityColor(C->Rarity);

	// 3D onizleme (sadece coin/durum degisince yenile)
	const FName Key(*FString::Printf(TEXT("%s_%d_%d"), *C->GetCoinId().ToString(), E.bOwned ? 1 : 0, E.bEnchanted ? 1 : 0));
	if (Preview && Key != LastPreviewKey)
	{
		Preview->SetCoin(C, E.bOwned, E.bEnchanted);
		LastPreviewKey = Key;
	}

	NameText->SetText(E.bOwned ? C->DisplayName : FText::FromString(TEXT("???")));
	NameText->SetColorAndOpacity(FSlateColor(RC));
	RarityText->SetText(FText::FromString(RarityName(C->Rarity)));
	RarityText->SetColorAndOpacity(FSlateColor(RC));

	EarnText->SetText(E.bOwned ? UFortuneHUDWidget::FormatMoney(E.CurrentValue) : FText::FromString(TEXT("?")));
	const float Chance = F->GetSummonChance(C);
	const int64 OneIn = Chance > 0.f ? (int64)FMath::FloorToDouble(1.0 / Chance + 0.5) : 0;
	OddsText->SetText(FText::FromString(Chance > 0.f ? FString::Printf(TEXT("1/%s"), *WithCommas(FMath::Max<int64>(1, OneIn))) : TEXT("-")));

	// Seviye animasyonlari
	const TArray<FFortuneFlipAnimTier>& Tiers = C->FlipAnimTiers.Num() > 0 ? C->FlipAnimTiers : S->DefaultFlipAnimTiers;
	const FFortuneFlipAnimTier* Cur = nullptr;
	const FFortuneFlipAnimTier* Nxt = nullptr;
	for (const FFortuneFlipAnimTier& Tier : Tiers)
	{
		if (Tier.MinLevel <= FMath::Max(1, E.Level) && (!Cur || Tier.MinLevel > Cur->MinLevel)) Cur = &Tier;
		if (Tier.MinLevel > E.Level && (!Nxt || Tier.MinLevel < Nxt->MinLevel)) Nxt = &Tier;
	}

	// Sayfa icerigi
	FString Title, Body;
	if (Page == 0)
	{
		Title = TEXT("AÇIKLAMA");
		if (E.bOwned)
		{
			Body = C->Description.ToString();
			Body += TEXT("\n\nPasif: ");
			Body += C->PassiveDescription.IsEmpty() ? TEXT("Yok") : C->PassiveDescription.ToString();
		}
		else
		{
			Body = TEXT("Bu coini henüz bulmadın.\nGacha'dan çıkabilir.");
		}
	}
	else if (Page == 1)
	{
		Title = TEXT("İSTATİSTİK");
		const bool bTailsFull = C->PassiveType == EFortunePassiveType::TailsFullValue;
		const double TailsVal = E.CurrentValue * (bTailsFull ? 1.0 : FMath::Min(1.0, (double)S->TailsValueMultiplier));
		Body = FString::Printf(TEXT("Nadirlik:  %s     Gacha:  %%%.2f\n"), *RarityName(C->Rarity), Chance * 100.f);
		if (E.bOwned)
		{
			Body += FString::Printf(TEXT("Yazı:  +%s     Tura:  +%s\n"),
				*UFortuneHUDWidget::FormatMoney(E.CurrentValue).ToString(), *UFortuneHUDWidget::FormatMoney(TailsVal).ToString());
			Body += FString::Printf(TEXT("Pasif:  %s\n"), C->PassiveDescription.IsEmpty() ? TEXT("Yok") : *C->PassiveDescription.ToString());
			Body += FString::Printf(TEXT("Enchant:  %s"), *E.EnchantText.ToString());
		}
	}
	else
	{
		Title = TEXT("SEVİYE");
		if (E.bOwned)
		{
			Body = FString::Printf(TEXT("Seviye:  %d / %d\n"), E.Level, C->MaxLevel);
			Body += FString::Printf(TEXT("Flip animasyonu:  %s\n"), Cur ? *Cur->DisplayName.ToString() : TEXT("-"));
			Body += Nxt ? FString::Printf(TEXT("Sonraki:  %s  (Lv %d)\n"), *Nxt->DisplayName.ToString(), Nxt->MinLevel)
			            : FString(TEXT("Sonraki:  Tüm animasyonlar açık\n"));
			Body += TEXT("Coini kullandıkça seviyesi ve değeri artar.");
		}
		else
		{
			Body = TEXT("Bu coini bulunca seviyesi burada görünecek.");
		}
	}
	PageTitle->SetText(FText::FromString(Title));
	PageBody->SetText(FText::FromString(Body));

	// XP cubugu
	LevelText->SetText(FText::FromString(E.bOwned ? FString::Printf(TEXT("LV %d"), E.Level) : FString(TEXT("LV -"))));
	const float Pct = (E.bOwned && E.XPForNextLevel > 0) ? (float)E.XP / (float)E.XPForNextLevel : (E.bOwned ? 1.f : 0.f);
	XPBar->SetPercent(Pct);
	XPText->SetText(FText::FromString(E.bOwned
		? (E.XPForNextLevel > 0 ? FString::Printf(TEXT("%d/%d XP"), E.XP, E.XPForNextLevel) : FString(TEXT("MAKS")))
		: FString(TEXT(""))));

	// Butonlar
	if (!E.bOwned)
	{
		EquipText->SetText(FText::FromString(TEXT("KİLİTLİ")));
		EquipButton->SetIsEnabled(false);
	}
	else if (E.bEquipped)
	{
		EquipText->SetText(FText::FromString(TEXT("TAKILI")));
		EquipText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 1.f, 0.45f)));
		EquipButton->SetIsEnabled(false);
	}
	else
	{
		EquipText->SetText(FText::FromString(TEXT("TAK")));
		EquipText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		EquipButton->SetIsEnabled(true);
	}

	const int32 Scrolls = F->GetItemCount(EFortuneItemType::EnchantScroll);
	EnchantText->SetText(FText::FromString(FString::Printf(TEXT("ENCHANT  x%d"), Scrolls)));
	EnchantButton->SetIsEnabled(E.bOwned && Scrolls > 0);
}

void UFortuneCollectionWidget::ShowToast(const FText& Msg, const FLinearColor& Color)
{
	if (!ToastText) return;
	ToastText->SetText(Msg);
	ToastText->SetColorAndOpacity(FSlateColor(Color));
	ToastAge = 0.f;
}

void UFortuneCollectionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (ToastText && ToastAge < 3.f)
	{
		ToastAge += InDeltaTime;
		ToastText->SetRenderOpacity(FMath::Clamp(1.f - (ToastAge - 2.f), 0.f, 1.f));
		const float Pop = 1.f + 0.3f * FMath::Max(0.f, 1.f - ToastAge * 5.f);
		ToastText->SetRenderScale(FVector2D(Pop, Pop));
	}
}

void UFortuneCollectionWidget::OnEquipClicked()
{
	UFortuneSubsystem* F = GetFortune();
	if (F && Entries.IsValidIndex(Selected) && Entries[Selected].Coin)
	{
		if (F->EquipCoin(Entries[Selected].Coin->GetCoinId()))
		{
			ShowToast(FText::Format(FText::FromString(TEXT("{0} takıldı!")), Entries[Selected].Coin->DisplayName), FLinearColor(0.4f, 1.f, 0.45f));
		}
	}
}

void UFortuneCollectionWidget::OnEnchantClicked()
{
	UFortuneSubsystem* F = GetFortune();
	if (!F || !Entries.IsValidIndex(Selected) || !Entries[Selected].Coin) return;

	FFortuneEnchant Result;
	if (F->UseEnchantScroll(Entries[Selected].Coin->GetCoinId(), Result))
	{
		const FLinearColor TierColor = Result.Tier >= 3 ? FLinearColor(1.f, 0.78f, 0.2f)
			: (Result.Tier == 2 ? FLinearColor(0.75f, 0.4f, 1.f) : FLinearColor(0.4f, 0.75f, 1.f));
		ShowToast(F->FormatEnchant(Result), TierColor);
		Page = 1;   // istatistik sayfasinda enchant'i goster
		RefreshDetails();
	}
}

void UFortuneCollectionWidget::OnBackClicked()
{
	RemoveFromParent();
}

void UFortuneCollectionWidget::OnPrevPage()
{
	Page = (Page + 2) % 3;
	RefreshDetails();
}

void UFortuneCollectionWidget::OnNextPage()
{
	Page = (Page + 1) % 3;
	RefreshDetails();
}
