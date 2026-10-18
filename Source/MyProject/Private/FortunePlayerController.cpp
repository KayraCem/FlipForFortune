#include "FortunePlayerController.h"
#include "FortuneCoinFlipper.h"
#include "FortuneHUDWidget.h"
#include "FortuneMenuWidgets.h"
#include "FortuneSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "Components/InputComponent.h"

AFortunePlayerController::AFortunePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultClickTraceChannel = ECC_Visibility;
	HUDWidgetClass = UFortuneHUDWidget::StaticClass();
}

void AFortunePlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);

	// "FortuneCamera" etiketli kamerayi (yoksa ilk kamerayi) kullan
	ACameraActor* Cam = nullptr;
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (!Cam || It->ActorHasTag(TEXT("FortuneCamera")))
		{
			Cam = *It;
		}
	}
	if (Cam)
	{
		SetViewTarget(Cam);
	}

	if (IsLocalController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
		}
		if (bShowMainMenu)
		{
			if (UUserWidget* Menu = CreateWidget<UUserWidget>(this, UFortuneMainMenuWidget::StaticClass()))
			{
				Menu->AddToViewport(50);
			}
		}
	}
}

void AFortunePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AFortunePlayerController::FlipPressed);
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AFortunePlayerController::AutoFlipPressed);
#if !UE_BUILD_SHIPPING
	// TEST: F5 = +5 Enchant Scroll, F6 = +1000 para
	InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AFortunePlayerController::DebugScrollsPressed);
	InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &AFortunePlayerController::DebugMoneyPressed);
#endif
}

void AFortunePlayerController::GiveScrolls(int32 Count)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFortuneSubsystem* F = GI->GetSubsystem<UFortuneSubsystem>())
		{
			F->AddItem(EFortuneItemType::EnchantScroll, Count);
			F->SaveProgress();
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(2, 2.f, FColor(190, 140, 255), FString::Printf(TEXT("TEST: +%d Enchant Scroll (toplam %d)"),
					Count, F->GetItemCount(EFortuneItemType::EnchantScroll)));
			}
		}
	}
}

void AFortunePlayerController::GiveMoney(double Amount)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFortuneSubsystem* F = GI->GetSubsystem<UFortuneSubsystem>())
		{
			F->AddMoney(Amount);
			F->SaveProgress();
		}
	}
}

void AFortunePlayerController::SetMenuOpen(bool bOpen)
{
	bMenuOpen = bOpen;
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(bOpen ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

void AFortunePlayerController::AutoFlipPressed()
{
	if (bMenuOpen) return;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFortuneSubsystem* F = GI->GetSubsystem<UFortuneSubsystem>())
		{
			F->ToggleAutoFlip();
		}
	}
}

void AFortunePlayerController::FlipPressed()
{
	if (bMenuOpen) return;
	for (TActorIterator<AFortuneCoinFlipper> It(GetWorld()); It; ++It)
	{
		It->Flip();
		break;
	}
}
