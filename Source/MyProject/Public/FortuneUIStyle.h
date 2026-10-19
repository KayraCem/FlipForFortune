// Flip for Fortune - UI panellerinin ortak gorunum yardimcilari (yuvarlak kutular, yazilar, butonlar, cubuklar).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

namespace FortuneStyle
{
	inline FSlateBrush StyRounded(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float OutlineWidth, FVector2D Size = FVector2D(32.f, 32.f))
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.ImageSize = FVector2f(Size);
		B.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	inline FLinearColor StyWithAlpha(const FLinearColor& C, float A)
	{
		return FLinearColor(C.R, C.G, C.B, A);
	}

	inline UTextBlock* StyText(UWidgetTree* Tree, const FString& Str, int32 Size, const FLinearColor& Color, bool bBold = true, int32 LetterSpacing = 0)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		F.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		F.LetterSpacing = LetterSpacing;
		T->SetFont(F);
		T->SetText(FText::FromString(Str));
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetShadowOffset(FVector2D(1.5f, 1.5f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		return T;
	}

	inline USizeBox* StySized(UWidgetTree* Tree, UWidget* Content, float W, float H)
	{
		USizeBox* B = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (W > 0.f) B->SetWidthOverride(W);
		if (H > 0.f) B->SetHeightOverride(H);
		B->SetContent(Content);
		return B;
	}

	inline FButtonStyle StyButtonStyle(const FLinearColor& Fill, const FLinearColor& Accent, float Radius = 14.f)
	{
		FButtonStyle S;
		S.SetNormal(StyRounded(Fill, StyWithAlpha(Accent, 0.45f), Radius, 1.5f));
		S.SetHovered(StyRounded(Fill * 1.3f, Accent, Radius, 2.5f));
		S.SetPressed(StyRounded(Fill * 0.8f, Accent, Radius, 2.5f));
		S.SetDisabled(StyRounded(FLinearColor(0.09f, 0.09f, 0.1f, 0.85f), FLinearColor(1.f, 1.f, 1.f, 0.06f), Radius, 1.f));
		S.SetNormalPadding(FMargin(12.f, 6.f));
		S.SetPressedPadding(FMargin(12.f, 8.f, 12.f, 4.f));
		return S;
	}

	inline UButton* StyButton(UWidgetTree* Tree, UWidget* Content, const FLinearColor& Fill, const FLinearColor& Accent, float Radius = 14.f)
	{
		UButton* B = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		B->SetStyle(StyButtonStyle(Fill, Accent, Radius));
		if (UTextBlock* L = Cast<UTextBlock>(Content))
		{
			L->SetJustification(ETextJustify::Center);
		}
		B->SetContent(Content);
		return B;
	}

	/** Ince dolum cubugu. SetBar ile guncellenir. */
	struct FStyBar
	{
		UBorder* Root = nullptr;
		UBorder* Fill = nullptr;
		UHorizontalBoxSlot* FillSlot = nullptr;
		UHorizontalBoxSlot* RestSlot = nullptr;
	};

	inline FStyBar StyMakeBar(UWidgetTree* Tree, const FLinearColor& FillColor, float Height = 8.f, const FLinearColor& Back = FLinearColor(0.f, 0.f, 0.f, 0.55f))
	{
		FStyBar Bar;
		Bar.Root = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Bar.Root->SetBrush(StyRounded(Back, FLinearColor(1.f, 1.f, 1.f, 0.08f), Height * 0.5f, 1.f));
		Bar.Root->SetPadding(FMargin(1.f));
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Bar.Root->SetContent(StySized(Tree, Row, 0.f, Height));
		Bar.Fill = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Bar.Fill->SetBrush(StyRounded(FillColor, StyWithAlpha(FillColor * 1.4f, 0.9f), Height * 0.5f, 1.f));
		Bar.FillSlot = Row->AddChildToHorizontalBox(Bar.Fill);
		Bar.FillSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Bar.RestSlot = Row->AddChildToHorizontalBox(Tree->ConstructWidget<USpacer>(USpacer::StaticClass()));
		Bar.RestSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		return Bar;
	}

	inline void StySetBar(const FStyBar& Bar, float Pct)
	{
		if (!Bar.FillSlot || !Bar.RestSlot) return;
		const float P = FMath::Clamp(Pct, 0.f, 1.f);
		FSlateChildSize A(ESlateSizeRule::Fill);
		A.Value = P;
		FSlateChildSize B(ESlateSizeRule::Fill);
		B.Value = 1.f - P;
		Bar.FillSlot->SetSize(A);
		Bar.RestSlot->SetSize(B);
		Bar.Fill->SetVisibility(P > 0.001f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	/** Kucuk renkli etiket (chip). */
	inline UBorder* StyChip(UWidgetTree* Tree, UTextBlock* Label, const FLinearColor& Color)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		B->SetBrush(StyRounded(FLinearColor(Color.R * 0.18f, Color.G * 0.18f, Color.B * 0.18f, 0.9f), StyWithAlpha(Color, 0.75f), 10.f, 1.5f));
		B->SetPadding(FMargin(10.f, 3.f));
		B->SetContent(Label);
		return B;
	}

	/** Cift cerceveli panel: dista renkli parlak kenar, icte koyu yuzey. Icerigi koyacagin Border'i dondurur. */
	inline UBorder* StyFrame(UWidgetTree* Tree, const FLinearColor& Accent, const FLinearColor& Inner, UBorder*& OutOuter, float Radius = 24.f)
	{
		OutOuter = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		OutOuter->SetBrush(StyRounded(FLinearColor(Accent.R * 0.25f, Accent.G * 0.25f, Accent.B * 0.25f, 0.95f), Accent, Radius + 4.f, 2.5f));
		OutOuter->SetPadding(FMargin(5.f));
		UBorder* In = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		In->SetBrush(StyRounded(Inner, StyWithAlpha(Accent, 0.35f), Radius, 1.f));
		In->SetPadding(FMargin(28.f, 22.f));
		OutOuter->SetContent(In);
		return In;
	}

	/** Basliklarin altindaki ince ayirici cizgi. */
	inline UImage* StyDivider(UWidgetTree* Tree, const FLinearColor& Color)
	{
		UImage* I = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		I->SetBrush(StyRounded(StyWithAlpha(Color, 0.35f), FLinearColor::Transparent, 1.f, 0.f, FVector2D(8.f, 2.f)));
		return I;
	}

	/** Panel acilis animasyonu (0..1 arasi T): hafif buyume + belirme. */
	inline void StyOpenAnim(UWidget* W, float Age, float Dur = 0.22f)
	{
		if (!W) return;
		const float T = FMath::Clamp(Age / Dur, 0.f, 1.f);
		const float E = 1.f - FMath::Pow(1.f - T, 3.f);
		const float S = FMath::Lerp(0.92f, 1.f, E) + 0.02f * FMath::Sin(T * PI);
		W->SetRenderScale(FVector2D(S, S));
		W->SetRenderOpacity(E);
	}
}
