// Flip for Fortune - Masadaki tiklanabilir esya (sandik = envanter, kitap = Coin Collection ...)
// Gorsel parcalar (StaticMeshActor'lar) bu aktore attach edilir; tiklamayi ClickBox yakalar.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneInteractable.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UUserWidget;
class USoundBase;

UENUM(BlueprintType)
enum class EFortuneInteraction : uint8
{
	Inventory,
	CoinCollection,
	Upgrades,
	Summon,
	FlipCoin,
	Custom
};

UCLASS(Blueprintable)
class MYPROJECT_API AFortuneInteractable : public AActor
{
	GENERATED_BODY()

public:
	AFortuneInteractable();

	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;
	virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

	UFUNCTION(BlueprintCallable, Category = "Fortune")
	void Interact();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> ClickBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> HoverLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	EFortuneInteraction Action = EFortuneInteraction::Custom;

	/** Uzerine gelince gosterilen isim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	FText Label;

	/** Tiklayinca acilacak UI (WBP_Inventory vb.). Bos ise gecici olarak ekrana bilgi yazar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	TSubclassOf<UUserWidget> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	TObjectPtr<USoundBase> ClickSound;

	/** Uzerine gelince ne kadar yukari kalkar (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	float HoverLift = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	float HoverLightIntensity = 600.f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Fortune")
	void OnInteracted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Fortune")
	void OnHoverChanged(bool bIsHovered);

protected:
	virtual void BeginPlay() override;

private:
	void ShowDebugInfo() const;

	bool bHovered = false;
	float HoverAlpha = 0.f;
	FVector BaseLocation;

	UPROPERTY()
	TObjectPtr<UUserWidget> OpenWidget;
};
