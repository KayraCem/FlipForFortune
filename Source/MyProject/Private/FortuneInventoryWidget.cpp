#include "FortuneInventoryWidget.h"
#include "FortuneSubsystem.h"
#include "FortuneSettings.h"
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
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Styling/SlateTypes.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"

namespace FortuneInvUI
{
	FSlateBrush InvRounded(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float OutlineWidth, FVector2D Size = FVector2D(32.f, 32.f))
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.ImageSize = FVector2f(Size);
		B.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	UTextBlock* InvText(UWidgetTree* Tree, const FString& Str, int32 Size, const FLinearColor& Color, bool bBold = true)
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

	const FLinearColor InvWood(0.12f, 0.07f, 0.04f, 0.97f);
	const FLinearColor InvTrim(1.f, 0.72f, 0.25f, 0.9f);
}

using namespace FortuneInvUI;

// ============================================================ Slot

void UFortuneInventorySlot::Setup(UFortuneInventoryWidget* InOwner, int32 InIndex)
{
	Owner = InOwner;
	Index = InIndex;
	OnClicked.AddUniqueDynamic(this, &UFortuneInventorySlot::HandleClicked);
}

void UFortuneInventorySlot::HandleClicked()
{
	if (Owner.IsValid()) Owner->SelectSlot(Index);
}

// ============================================================ Esya bilgileri

FString UFortuneInventoryWidget::ItemName(EFortuneItemType T)
{
	switch (T)
	{
	case EFortuneItemType::EnchantScroll: return TEXT("Enchant Scroll");
	default:                              return TEXT("Eşya");
	}
}

FString UFortuneInventoryWidget::ItemDesc(EFortuneItemType T)
{
	switch (T)
	{
	case EFortuneItemType::EnchantScroll:
	{
		const float Chance = GetDefault<UFortuneSettings>()->EnchantScrollDropChance;
		return FString::Printf(TEXT("Coinine rastgele bir güç (enchant) verir.\n\nNasıl kullanılır: masadaki büyü kitabına tıkla.\nNasıl bulunur: her atışta 1/%.0f şansla düşer (Luck artırır)."),
			Chance > 0.f ? 1.f / Chance : 0.f);
	}
	default: return TEXT("");
	}
}

FString UFortuneInventoryWidget::ItemRarity(EFortuneItemType T)
{
	switch (T)
	{
	case EFortuneItemType::EnchantScroll: return TEXT("EPIC");
	default:                              return TEXT("COMMON");
	}
}

FLinearColor UFortuneInventoryWidget::ItemColor(EFortuneItemType T)
{
	switch (T)
	{
	case EFortuneItemType::EnchantScroll: return FLinearColor(0.7f, 0.42f, 1.f);
	default:                              return FLinearColor(0.8f, 0.8f, 0.8f);
	}
}

// ============================================================ Panel

TSharedRef<SWidget> UFortuneInventoryWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UFortuneInventoryWidget::BuildTree()
{
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;

	UImage* Dim = T->ConstructWidget<UImage>(UImage::StaticClass());
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));

	UBorder* Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrush(InvRounded(InvWood, InvTrim, 22.f, 3.f));
	Panel->SetPadding(FMargin(26.f, 20.f));
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Col = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Col);

	// Baslik satiri
	UHorizontalBox* Head = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(Head)->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
	UVerticalBox* TitleCol = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Head->AddChildToHorizontalBox(TitleCol)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleCol->AddChildToVerticalBox(InvText(T, TEXT("ENVANTER"), 34, FLinearColor(1.f, 0.8f, 0.35f)));
	CapacityText = InvText(T, TEXT("0 / 24"), 15, FLinearColor(1.f, 1.f, 1.f, 0.6f), false);
	TitleCol->AddChildToVerticalBox(CapacityText);
	UBorder* MoneyPill = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	MoneyPill->SetBrush(InvRounded(FLinearColor(0.f, 0.f, 0.f, 0.45f), InvTrim, 18.f, 1.5f));
	MoneyPill->SetPadding(FMargin(16.f, 6.f));
	MoneyText = InvText(T, TEXT("$ 0"), 22, FLinearColor(1.f, 0.8f, 0.35f));
	MoneyPill->SetContent(MoneyText);
	Head->AddChildToHorizontalBox(MoneyPill)->SetVerticalAlignment(VAlign_Center);

	// Izgara + detay paneli
	UHorizontalBox* Body = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col->AddChildToVerticalBox(Body);

	UBorder* GridBg = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	GridBg->SetBrush(InvRounded(FLinearColor(0.f, 0.f, 0.f, 0.35f), FLinearColor(1.f, 1.f, 1.f, 0.06f), 16.f, 1.f));
	GridBg->SetPadding(FMargin(10.f));
	Body->AddChildToHorizontalBox(GridBg);
	UUniformGridPanel* Grid = T->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	Grid->SetSlotPadding(FMargin(5.f));
	GridBg->SetContent(Grid);

	for (int32 i = 0; i < Columns * Rows; ++i)
	{
		UFortuneInventorySlot* S = T->ConstructWidget<UFortuneInventorySlot>(UFortuneInventorySlot::StaticClass());
		S->Setup(this, i);
		USizeBox* Box = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(84.f);
		Box->SetHeightOverride(84.f);
		Box->SetContent(S);
		UUniformGridSlot* GS = Grid->AddChildToUniformGrid(Box, i / Columns, i % Columns);
		GS->SetHorizontalAlignment(HAlign_Fill);
		GS->SetVerticalAlignment(VAlign_Fill);

		UOverlay* O = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		S->SetContent(O);
		S->Icon = T->ConstructWidget<UImage>(UImage::StaticClass());
		UOverlaySlot* IS = O->AddChildToOverlay(S->Icon);
		IS->SetHorizontalAlignment(HAlign_Center);
		IS->SetVerticalAlignment(VAlign_Center);
		S->IconLines = T->ConstructWidget<UImage>(UImage::StaticClass());
		UOverlaySlot* LS = O->AddChildToOverlay(S->IconLines);
		LS->SetHorizontalAlignment(HAlign_Center);
		LS->SetVerticalAlignment(VAlign_Center);
		S->CountText = InvText(T, TEXT(""), 16, FLinearColor::White);
		UOverlaySlot* CS = O->AddChildToOverlay(S->CountText);
		CS->SetHorizontalAlignment(HAlign_Right);
		CS->SetVerticalAlignment(VAlign_Bottom);
		Slots.Add(S);
	}

	// Detay
	UBorder* Detail = T->ConstructWidget<UBorder>(UBorder::StaticClass());
	Detail->SetBrush(InvRounded(FLinearColor(0.f, 0.f, 0.f, 0.35f), FLinearColor(1.f, 1.f, 1.f, 0.06f), 16.f, 1.f));
	Detail->SetPadding(FMargin(18.f, 14.f));
	USizeBox* DetailBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	DetailBox->SetWidthOverride(330.f);
	DetailBox->SetContent(Detail);
	UHorizontalBoxSlot* DS = Body->AddChildToHorizontalBox(DetailBox);
	DS->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
	DS->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* DV = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Detail->SetContent(DV);
	DetailIcon = T->ConstructWidget<UImage>(UImage::StaticClass());
	DV->AddChildToVerticalBox(DetailIcon)->SetHorizontalAlignment(HAlign_Center);
	DetailName = InvText(T, TEXT(""), 26, FLinearColor::White);
	UVerticalBoxSlot* NS = DV->AddChildToVerticalBox(DetailName);
	NS->SetHorizontalAlignment(HAlign_Center);
	NS->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	DetailRarity = InvText(T, TEXT(""), 16, FLinearColor::White);
	DV->AddChildToVerticalBox(DetailRarity)->SetHorizontalAlignment(HAlign_Center);
	DetailCount = InvText(T, TEXT(""), 20, FLinearColor(1.f, 0.85f, 0.45f));
	UVerticalBoxSlot* CntS = DV->AddChildToVerticalBox(DetailCount);
	CntS->SetHorizontalAlignment(HAlign_Center);
	CntS->SetPadding(FMargin(0.f, 6.f, 0.f, 10.f));
	DetailDesc = InvText(T, TEXT(""), 15, FLinearColor(1.f, 1.f, 1.f, 0.85f), false);
	DetailDesc->SetAutoWrapText(true);
	DV->AddChildToVerticalBox(DetailDesc);

	// Kapat
	UButton* Close = T->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle St;
	St.SetNormal(InvRounded(FLinearColor(0.3f, 0.08f, 0.08f, 1.f), FLinearColor(1.f, 1.f, 1.f, 0.1f), 14.f, 1.f));
	St.SetHovered(InvRounded(FLinearColor(0.42f, 0.1f, 0.1f, 1.f), FLinearColor(1.f, 0.4f, 0.4f), 14.f, 2.f));
	St.SetPressed(InvRounded(FLinearColor(0.22f, 0.06f, 0.06f, 1.f), FLinearColor(1.f, 0.4f, 0.4f), 14.f, 2.f));
	Close->SetStyle(St);
	UTextBlock* CloseT = InvText(T, TEXT("KAPAT"), 22, FLinearColor(1.f, 0.6f, 0.6f));
	CloseT->SetJustification(ETextJustify::Center);
	Close->SetContent(CloseT);
	Close->OnClicked.AddDynamic(this, &UFortuneInventoryWidget::OnCloseClicked);
	USizeBox* CloseBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CloseBox->SetWidthOverride(220.f);
	CloseBox->SetHeightOverride(50.f);
	CloseBox->SetContent(Close);
	UVerticalBoxSlot* CloseSlot = Col->AddChildToVerticalBox(CloseBox);
	CloseSlot->SetHorizontalAlignment(HAlign_Center);
	CloseSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
}

void UFortuneInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
	if (UFortuneSubsystem* F = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFortuneSubsystem>() : nullptr)
	{
		F->OnInventoryChanged.AddDynamic(this, &UFortuneInventoryWidget::HandleChanged);
	}
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
	{
		FPC->SetMenuOpen(true);
	}
	Refresh();
}

void UFortuneInventoryWidget::NativeDestruct()
{
	if (UFortuneSubsystem* F = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFortuneSubsystem>() : nullptr)
	{
		F->OnInventoryChanged.RemoveDynamic(this, &UFortuneInventoryWidget::HandleChanged);
	}
	if (AFortunePlayerController* FPC = Cast<AFortunePlayerController>(GetOwningPlayer()))
	{
		FPC->SetMenuOpen(false);
	}
	Super::NativeDestruct();
}

void UFortuneInventoryWidget::HandleChanged()
{
	Refresh();
}

void UFortuneInventoryWidget::SelectSlot(int32 Index)
{
	Selected = Index;
	Refresh();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void UFortuneInventoryWidget::Refresh()
{
	UFortuneSubsystem* F = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!F) return;

	// Sahip olunan esyalari sirala (adet > 0)
	Items.Reset();
	const UEnum* Enum = StaticEnum<EFortuneItemType>();
	for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
	{
		const EFortuneItemType Type = (EFortuneItemType)Enum->GetValueByIndex(i);
		const int32 N = F->GetItemCount(Type);
		if (N > 0) Items.Add({ Type, N });
	}

	MoneyText->SetText(FText::FromString(FString::Printf(TEXT("$ %s"), *UFortuneHUDWidget::FormatMoney(F->GetMoney()).ToString())));
	CapacityText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d yuva dolu"), Items.Num(), Columns * Rows)));

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		UFortuneInventorySlot* S = Slots[i];
		const bool bHas = Items.IsValidIndex(i);
		const bool bSel = (i == Selected) && bHas;
		const FLinearColor C = bHas ? ItemColor(Items[i].Type) : FLinearColor(1.f, 1.f, 1.f, 0.1f);

		FButtonStyle St;
		const FSlateBrush Normal = InvRounded(bSel ? FLinearColor(0.25f, 0.2f, 0.3f, 0.95f) : FLinearColor(0.08f, 0.06f, 0.05f, 0.9f),
			bSel ? C : FLinearColor(C.R, C.G, C.B, bHas ? 0.45f : 0.1f), 12.f, bSel ? 3.f : 1.5f);
		St.SetNormal(Normal);
		St.SetHovered(InvRounded(FLinearColor(0.18f, 0.14f, 0.12f, 0.95f), FLinearColor(C.R, C.G, C.B, 0.9f), 12.f, 2.f));
		St.SetPressed(Normal);
		St.SetNormalPadding(FMargin(4.f));
		St.SetPressedPadding(FMargin(4.f));
		S->SetStyle(St);

		if (bHas)
		{
			// Scroll simgesi: rulo + uzerinde cizgiler
			S->Icon->SetBrush(InvRounded(FLinearColor(0.95f, 0.88f, 0.7f), C, 6.f, 3.f, FVector2D(44.f, 54.f)));
			S->IconLines->SetBrush(InvRounded(FLinearColor(C.R, C.G, C.B, 0.85f), FLinearColor::Transparent, 2.f, 0.f, FVector2D(26.f, 4.f)));
			S->Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
			S->IconLines->SetVisibility(ESlateVisibility::HitTestInvisible);
			S->CountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Items[i].Count)));
		}
		else
		{
			S->Icon->SetVisibility(ESlateVisibility::Collapsed);
			S->IconLines->SetVisibility(ESlateVisibility::Collapsed);
			S->CountText->SetText(FText::GetEmpty());
		}
	}

	if (Items.IsValidIndex(Selected))
	{
		const FItemView& It = Items[Selected];
		const FLinearColor C = ItemColor(It.Type);
		DetailIcon->SetBrush(InvRounded(FLinearColor(0.95f, 0.88f, 0.7f), C, 10.f, 4.f, FVector2D(80.f, 100.f)));
		DetailIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		DetailName->SetText(FText::FromString(ItemName(It.Type)));
		DetailName->SetColorAndOpacity(FSlateColor(C));
		DetailRarity->SetText(FText::FromString(ItemRarity(It.Type)));
		DetailRarity->SetColorAndOpacity(FSlateColor(C));
		DetailCount->SetText(FText::FromString(FString::Printf(TEXT("Adet: %d"), It.Count)));
		DetailDesc->SetText(FText::FromString(ItemDesc(It.Type)));
	}
	else
	{
		DetailIcon->SetVisibility(ESlateVisibility::Collapsed);
		DetailName->SetText(FText::FromString(Items.Num() == 0 ? TEXT("Envanter boş") : TEXT("Bir eşya seç")));
		DetailName->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		DetailRarity->SetText(FText::GetEmpty());
		DetailCount->SetText(FText::GetEmpty());
		DetailDesc->SetText(FText::FromString(Items.Num() == 0
			? TEXT("Coin attıkça nadiren Enchant Scroll gibi eşyalar düşer. Burada birikirler.")
			: TEXT("")));
	}
}

void UFortuneInventoryWidget::OnCloseClicked()
{
	RemoveFromParent();
}
