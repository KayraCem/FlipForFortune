// Flip for Fortune - Fare imleci acik, esyalara tiklanabilir, Space = coin at.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FortunePlayerController.generated.h"

UCLASS()
class MYPROJECT_API AFortunePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFortunePlayerController();

	/** Ekrandaki para gostergesi. Kendi WBP'ni yaparsan buraya ata. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fortune")
	TSubclassOf<class UUserWidget> HUDWidgetClass;

	/** Tam ekran menu acikken coin atma ve HUD kapanir. */
	UFUNCTION(BlueprintCallable, Category = "Fortune")
	void SetMenuOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Fortune")
	bool IsMenuOpen() const { return bMenuOpen; }

	// ---- TEST komutlari (konsol: ` tusu) ----
	/** GiveScrolls 10 */
	UFUNCTION(Exec)
	void GiveScrolls(int32 Count = 5);

	/** GiveMoney 1000 */
	UFUNCTION(Exec)
	void GiveMoney(double Amount = 1000.0);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void FlipPressed();
	void AutoFlipPressed();
	void DebugScrollsPressed() { GiveScrolls(5); }
	void DebugMoneyPressed() { GiveMoney(1000.0); }

	UPROPERTY()
	TObjectPtr<class UUserWidget> HUDWidget;

	bool bMenuOpen = false;
};
