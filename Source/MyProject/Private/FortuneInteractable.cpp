#include "FortuneInteractable.h"
#include "FortuneSubsystem.h"
#include "FortuneCoinData.h"
#include "FortuneCoinFlipper.h"
#include "FortuneCollectionWidget.h"
#include "FortuneUpgradeWidget.h"
#include "FortuneMenuWidgets.h"
#include "FortunePlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"

AFortuneInteractable::AFortuneInteractable()
{
	PrimaryActorTick.bCanEverTick = true;

	ClickBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBox"));
	SetRootComponent(ClickBox);
	ClickBox->SetBoxExtent(FVector(10.f, 10.f, 8.f));
	ClickBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ClickBox->SetMobility(EComponentMobility::Movable);

	HoverLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("HoverLight"));
	HoverLight->SetupAttachment(ClickBox);
	HoverLight->SetRelativeLocation(FVector(-15.f, 0.f, 20.f));
	HoverLight->SetIntensity(0.f);
	HoverLight->SetAttenuationRadius(60.f);
	HoverLight->SetLightColor(FLinearColor(1.f, 0.85f, 0.5f));
	HoverLight->SetCastShadows(false);
}

void AFortuneInteractable::BeginPlay()
{
	Super::BeginPlay();
	BaseLocation = GetActorLocation();
}

void AFortuneInteractable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Target = bHovered ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(HoverAlpha, Target, 0.001f)) return;

	HoverAlpha = FMath::FInterpTo(HoverAlpha, Target, DeltaSeconds, 12.f);
	SetActorLocation(BaseLocation + FVector(0.f, 0.f, HoverLift * HoverAlpha));
	HoverLight->SetIntensity(HoverLightIntensity * HoverAlpha);
}

void AFortuneInteractable::NotifyActorBeginCursorOver()
{
	Super::NotifyActorBeginCursorOver();
	bHovered = true;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->CurrentMouseCursor = EMouseCursor::Hand;
	}
	OnHoverChanged(true);
}

void AFortuneInteractable::NotifyActorEndCursorOver()
{
	Super::NotifyActorEndCursorOver();
	bHovered = false;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->CurrentMouseCursor = EMouseCursor::Default;
	}
	OnHoverChanged(false);
}

void AFortuneInteractable::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);
	Interact();
}

void AFortuneInteractable::Interact()
{
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}

	if (Action == EFortuneInteraction::Enchant && !WidgetClass)
	{
		// Buyu kitabi: kusanili coin secili Enchant ekrani
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		const AFortunePlayerController* FPC = Cast<AFortunePlayerController>(PC);
		const UGameInstance* GI = GetGameInstance();
		const UFortuneSubsystem* F = GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
		if (PC && F && !(FPC && FPC->IsMenuOpen()))
		{
			if (UFortuneEnchantWidget* W = CreateWidget<UFortuneEnchantWidget>(PC, UFortuneEnchantWidget::StaticClass()))
			{
				W->CoinId = F->GetEquippedCoinId();
				W->bStandalone = true;
				W->AddToViewport(15);
			}
		}
	}
	else if (Action == EFortuneInteraction::FlipCoin)
	{
		for (TActorIterator<AFortuneCoinFlipper> It(GetWorld()); It; ++It)
		{
			It->Flip();
			break;
		}
	}
	else if (WidgetClass || Action == EFortuneInteraction::CoinCollection || Action == EFortuneInteraction::Upgrades)
	{
		TSubclassOf<UUserWidget> Cls = WidgetClass;
		if (!Cls)
		{
			Cls = (Action == EFortuneInteraction::Upgrades)
				? TSubclassOf<UUserWidget>(UFortuneUpgradeWidget::StaticClass())
				: TSubclassOf<UUserWidget>(UFortuneCollectionWidget::StaticClass());
		}
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (OpenWidget && OpenWidget->IsInViewport())
		{
			OpenWidget->RemoveFromParent();
			OpenWidget = nullptr;
		}
		else if (PC)
		{
			if (const AFortunePlayerController* FPC = Cast<AFortunePlayerController>(PC))
			{
				if (FPC->IsMenuOpen()) return;
			}
			OpenWidget = CreateWidget<UUserWidget>(PC, Cls);
			if (OpenWidget)
			{
				OpenWidget->AddToViewport(10);
			}
		}
	}
	else
	{
		ShowDebugInfo();
	}

	OnInteracted();
}

void AFortuneInteractable::ShowDebugInfo() const
{
	// UI hazir olana kadar gecici bilgi: ekranin sol ustune yazar
	const UGameInstance* GI = GetGameInstance();
	UFortuneSubsystem* F = GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!F || !GEngine) return;

	const FColor Gold(255, 200, 80);
	GEngine->ClearOnScreenDebugMessages();   // ust uste binmesin
	int32 Key = 1000;
	switch (Action)
	{
	case EFortuneInteraction::Inventory:
		GEngine->AddOnScreenDebugMessage(Key++, 4.f, Gold, FString::Printf(TEXT("ENVANTER  |  Enchant Scroll: %d"),
			F->GetItemCount(EFortuneItemType::EnchantScroll)));
		break;

	case EFortuneInteraction::CoinCollection:
		for (const FFortuneCollectionEntry& E : F->GetCollection())
		{
			if (!E.Coin) continue;
			GEngine->AddOnScreenDebugMessage(Key++, 6.f, E.bOwned ? Gold : FColor::Silver,
				E.bOwned
				? FString::Printf(TEXT("%s  Lv%d  (%d/%d XP)  Deger %.1f  %s"), *E.Coin->DisplayName.ToString(), E.Level, E.XP,
					E.XPForNextLevel, E.CurrentValue, *E.EnchantText.ToString())
				: FString::Printf(TEXT("???  (%s)"), *E.Coin->DisplayName.ToString()));
		}
		GEngine->AddOnScreenDebugMessage(Key++, 6.f, FColor::White, TEXT("COIN COLLECTION"));
		break;

	case EFortuneInteraction::Upgrades:
	{
		const UEnum* Enum = StaticEnum<EFortuneUpgradeType>();
		for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
		{
			const EFortuneUpgradeType U = (EFortuneUpgradeType)Enum->GetValueByIndex(i);
			GEngine->AddOnScreenDebugMessage(Key++, 5.f, Gold, FString::Printf(TEXT("%s  Lv%d  ->  %.0f"),
				*Enum->GetDisplayNameTextByIndex(i).ToString(), F->GetUpgradeLevel(U), F->GetUpgradeCost(U)));
		}
		GEngine->AddOnScreenDebugMessage(Key++, 5.f, FColor::White, FString::Printf(TEXT("YUKSELTMELER  |  Para: %.0f"), F->GetMoney()));
		break;
	}

	case EFortuneInteraction::Summon:
	{
		UFortuneCoinData* Coin = nullptr;
		bool bDup = false;
		if (F->SummonCoin(Coin, bDup) && Coin)
		{
			GEngine->AddOnScreenDebugMessage(Key++, 4.f, Gold, FString::Printf(TEXT("GACHA: %s %s"),
				*Coin->DisplayName.ToString(), bDup ? TEXT("(tekrar, +XP)") : TEXT("(YENI!)")));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(Key++, 3.f, FColor::Red, FString::Printf(TEXT("GACHA: yetersiz para (%.0f gerekli)"), F->GetSummonCost()));
		}
		break;
	}

	default:
		break;
	}
}
