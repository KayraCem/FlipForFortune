#include "FortunePlayerController.h"
#include "FortuneCoinFlipper.h"
#include "FortuneHUDWidget.h"
#include "FortuneMenuWidgets.h"
#include "FortuneCollectionWidget.h"
#include "FortuneInventoryWidget.h"
#include "FortuneSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Components/InputComponent.h"
#include "FortuneHeroActor.h"
#include "FortuneCoinIconStudio.h"
#include "FortuneInteractable.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

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

	// UI coin ikonlari: her coin gorunmez stüdyoda 3D cekilir
	if (IsLocalController())
	{
		AFortuneCoinIconStudio::Prewarm(this);
	}

	// Birinci sahis: karakter varsa WASD + fare ile bakis
	for (TActorIterator<AFortuneHeroActor> It(GetWorld()); It; ++It)
	{
		Hero = *It;
		break;
	}
	if (AFortuneHeroActor* H = Hero.Get())
	{
		if (Cam)
		{
			if (Cam->GetAttachParentActor() != H)
			{
				Cam->AttachToActor(H, FAttachmentTransformRules::KeepWorldTransform);
			}
			CamRelLoc = Cam->GetRootComponent()->GetRelativeLocation();
		}
		HeroYaw = H->GetActorRotation().Yaw;
		ViewPitch = StartViewPitch;
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
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AFortunePlayerController::AutoFlipPressed);
	// Birinci sahis: sol tik = nisangahtaki esya (yoksa coin at), E = etkilesim. WASD/Shift/fare PlayerTick'te okunur.
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AFortunePlayerController::ClickPressed);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AFortunePlayerController::UsePressed);
	// Karakter yoksa: sag tik basili tut + fare = etrafa bak, tekerlek = yakinlas, R = sifirla
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AFortunePlayerController::LookPressed);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AFortunePlayerController::LookReleased);
	InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AFortunePlayerController::ZoomIn);
	InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AFortunePlayerController::ZoomOut);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AFortunePlayerController::ResetLook);
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AFortunePlayerController::ToggleCollection);
	InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AFortunePlayerController::ToggleInventory);
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

	if (Hero.IsValid())
	{
		TickFirstPerson(DeltaTime);
		return;
	}

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

void AFortunePlayerController::TogglePanel(TSubclassOf<UUserWidget> Cls, TWeakObjectPtr<UUserWidget>& Panel)
{
	if (Panel.IsValid() && Panel->IsInViewport())
	{
		Panel->RemoveFromParent();
		Panel = nullptr;
		return;
	}
	if (bMenuOpen) return;   // baska bir menu acikken acma
	if (UUserWidget* W = CreateWidget<UUserWidget>(this, Cls))
	{
		W->AddToViewport(10);
		Panel = W;
	}
}

void AFortunePlayerController::ToggleCollection()
{
	TogglePanel(UFortuneCollectionWidget::StaticClass(), CollectionPanel);
}

void AFortunePlayerController::ToggleInventory()
{
	TogglePanel(UFortuneInventoryWidget::StaticClass(), InventoryPanel);
}

// ============================================================ birinci sahis

bool AFortunePlayerController::WantsUI() const
{
	if (bMenuOpen || IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt)) return true;
	const ACameraActor* Cam = MainCamera.Get();
	if (Cam && GetViewTarget() != Cam) return true;   // vitrin / menu kamerasi
	// HUD disinda ekranda acik bir panel var mi (Yukseltme, Summon, Koleksiyon...)
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(const_cast<AFortunePlayerController*>(this), Found, UUserWidget::StaticClass(), true);
	for (const UUserWidget* W : Found)
	{
		if (!W || W == HUDWidget.Get() || !W->IsInViewport()) continue;
		const ESlateVisibility V = W->GetVisibility();
		if (V == ESlateVisibility::Collapsed || V == ESlateVisibility::Hidden) continue;
		return true;
	}
	return false;
}

void AFortunePlayerController::ApplyInputMode(bool bUI)
{
	bUIMode = bUI;
	bShowMouseCursor = bUI;
	bEnableClickEvents = bUI;
	bEnableMouseOverEvents = bUI;
	if (bUI)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		if (Hovered.IsValid()) Hovered->NotifyActorEndCursorOver();
		Hovered = nullptr;
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
	if (UFortuneHUDWidget* HW = Cast<UFortuneHUDWidget>(HUDWidget))
	{
		HW->SetCrosshair(!bUI, FText::GetEmpty());
	}
}

AFortuneInteractable* AFortunePlayerController::TraceInteractable() const
{
	const ACameraActor* Cam = MainCamera.Get();
	if (!Cam) return nullptr;
	const FVector Start = Cam->GetCameraComponent()->GetComponentLocation();
	const FVector End = Start + Cam->GetActorForwardVector() * InteractDistance;
	FCollisionQueryParams P(SCENE_QUERY_STAT(FortuneAim), false, Hero.Get());
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, P))
	{
		return Cast<AFortuneInteractable>(Hit.GetActor());
	}
	return nullptr;
}

void AFortunePlayerController::MoveHero(const FVector& Delta)
{
	AFortuneHeroActor* H = Hero.Get();
	if (!H) return;
	// Kapsul (dizden basa): duvar, kolon, cesme, bank vb. icinden gecmez, kenara surtunerek kayar
	const float HalfH = 55.f;
	const FVector Off(0.f, 0.f, HalfH + 10.f);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(CapsuleRadius, HalfH);
	FCollisionQueryParams P(SCENE_QUERY_STAT(FortuneHeroMove), false, H);
	if (APawn* Pw = GetPawn()) P.AddIgnoredActor(Pw);

	FVector Pos = H->GetActorLocation();
	FVector Remaining = FVector(Delta.X, Delta.Y, 0.f);
	for (int32 Iter = 0; Iter < 3 && !Remaining.IsNearlyZero(0.01f); ++Iter)
	{
		const FVector Start = Pos + Off;
		FHitResult Hit;
		if (!GetWorld()->SweepSingleByChannel(Hit, Start, Start + Remaining, FQuat::Identity, ECC_Pawn, Shape, P))
		{
			Pos += Remaining;
			break;
		}
		FVector N = Hit.Normal;
		N.Z = 0.f;
		N = N.GetSafeNormal();
		if (Hit.bStartPenetrating)
		{
			Pos += N * (Hit.PenetrationDepth + 0.5f);   // icine sikistiysa disari it
			continue;
		}
		Pos = Hit.Location - Off + N * 0.1f;
		const FVector Left = Remaining * (1.f - Hit.Time);
		Remaining = Left - N * FVector::DotProduct(Left, N);   // duvar boyunca kay
	}
	Pos.Z = H->GetActorLocation().Z;
	H->SetActorLocation(Pos);
}

void AFortunePlayerController::TickFirstPerson(float DeltaTime)
{
	AFortuneHeroActor* H = Hero.Get();
	ACameraActor* Cam = MainCamera.Get();
	if (!H || !Cam) return;

	const bool bUI = WantsUI();
	if (bUI != bUIMode) ApplyInputMode(bUI);
	if (GetViewTarget() != Cam) return;   // vitrin / menu kamerasindayken dokunma

	FVector Move = FVector::ZeroVector;
	float Speed = WalkSpeed;
	if (!bUIMode)
	{
		float DX = 0.f, DY = 0.f;
		GetInputMouseDelta(DX, DY);
		HeroYaw = FRotator::NormalizeAxis(HeroYaw + DX * MouseSensitivity);
		ViewPitch = FMath::Clamp(ViewPitch + DY * MouseSensitivity, MinViewPitch, MaxViewPitch);

		const float Fwd = (IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
		const float Right = (IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
		Move = FRotator(0.f, HeroYaw, 0.f).RotateVector(FVector(Fwd, Right, 0.f)).GetClampedToMaxSize(1.f);
		if (IsInputKeyDown(EKeys::LeftShift)) Speed = RunSpeed;
	}

	H->SetActorRotation(FRotator(0.f, HeroYaw, 0.f));
	const bool bMoving = !Move.IsNearlyZero();
	if (bMoving) MoveHero(Move * Speed * DeltaTime);

	// Yururken hafif bas sallanmasi
	const bool bRun = bMoving && Speed > WalkSpeed;
	BobAmount = FMath::FInterpTo(BobAmount, bMoving ? (bRun ? 1.4f : 1.f) : 0.f, DeltaTime, 6.f);
	if (BobAmount > 0.01f) BobTime += DeltaTime * (bRun ? 13.f : 9.f);
	const FVector Bob(0.f, FMath::Sin(BobTime * 0.5f) * 1.2f * BobAmount, FMath::Abs(FMath::Sin(BobTime)) * 2.2f * BobAmount);
	Cam->GetRootComponent()->SetRelativeLocation(CamRelLoc + Bob);
	Cam->SetActorRotation(FRotator(ViewPitch, HeroYaw, 0.f));
	H->SetViewPitch(ViewPitch - ArmBasePitch);   // kollar bakisi takip eder, el hep ekranda

	UCameraComponent* CC = Cam->GetCameraComponent();
	CC->SetFieldOfView(FMath::FInterpTo(CC->FieldOfView, TargetFOV, DeltaTime, 8.f));

	// Nisangah: baktigin esya parlar, adi ekranda yazar
	AFortuneInteractable* Aim = bUIMode ? nullptr : TraceInteractable();
	if (Aim != Hovered.Get())
	{
		if (Hovered.IsValid()) Hovered->NotifyActorEndCursorOver();
		Hovered = Aim;
		if (Aim) Aim->NotifyActorBeginCursorOver();
	}
	if (UFortuneHUDWidget* HW = Cast<UFortuneHUDWidget>(HUDWidget))
	{
		HW->SetCrosshair(!bUIMode, Aim ? Aim->Label : FText::GetEmpty());
	}
}

void AFortunePlayerController::ClickPressed()
{
	if (!Hero.IsValid() || bUIMode || bMenuOpen) return;   // imlec aciksa tiklama olaylari kullanilir
	if (AFortuneInteractable* I = Hovered.Get())
	{
		I->Interact();
		return;
	}
	FlipPressed();   // bos yere sol tik = coin at
}

void AFortunePlayerController::UsePressed()
{
	if (!Hero.IsValid() || bUIMode || bMenuOpen) return;
	if (AFortuneInteractable* I = Hovered.Get())
	{
		I->Interact();
	}
}
