#include "FortunePlayerController.h"
#include "FortuneCoinFlipper.h"
#include "FortuneHUDWidget.h"
#include "FortuneMenuWidgets.h"
#include "FortuneSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
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
		MainCamera = Cam;
		BaseRotation = Cam->GetActorRotation();
		BaseFOV = TargetFOV = Cam->GetCameraComponent()->FieldOfView;
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
	// Kamera: sag tik basili tut + fare = etrafa bak, tekerlek = yakinlas, R = sifirla
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AFortunePlayerController::LookPressed);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AFortunePlayerController::LookReleased);
	InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AFortunePlayerController::ZoomIn);
	InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AFortunePlayerController::ZoomOut);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AFortunePlayerController::ResetLook);
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

void AFortunePlayerController::GiveCoin(const FString& Name)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFortuneSubsystem* F = GI->GetSubsystem<UFortuneSubsystem>())
		{
			const int32 N = F->DebugGiveCoins(Name);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(3, 2.5f, FColor(120, 220, 255), FString::Printf(TEXT("TEST: %d coin verildi (%s)"), N, *Name));
			}
		}
	}
}

void AFortunePlayerController::GiveAllCoins()
{
	GiveCoin(FString());
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

void AFortunePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ACameraActor* Cam = MainCamera.Get();
	if (!Cam || GetViewTarget() != Cam) return;   // menu / vitrin kamerasindayken dokunma

	if (bLooking && !bMenuOpen)
	{
		float DX = 0.f, DY = 0.f;
		GetInputMouseDelta(DX, DY);
		TargetYaw = FMath::Clamp(TargetYaw + DX * LookSensitivity, -MaxLookYaw, MaxLookYaw);
		TargetPitch = FMath::Clamp(TargetPitch + DY * LookSensitivity, -MaxLookDown, MaxLookUp);
	}

	CurYaw = FMath::FInterpTo(CurYaw, TargetYaw, DeltaTime, 10.f);
	CurPitch = FMath::FInterpTo(CurPitch, TargetPitch, DeltaTime, 10.f);
	Cam->SetActorRotation(FRotator(BaseRotation.Pitch + CurPitch, BaseRotation.Yaw + CurYaw, 0.f));

	UCameraComponent* CC = Cam->GetCameraComponent();
	CC->SetFieldOfView(FMath::FInterpTo(CC->FieldOfView, TargetFOV, DeltaTime, 8.f));
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
