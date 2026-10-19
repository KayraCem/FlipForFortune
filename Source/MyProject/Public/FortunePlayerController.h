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

	/** Oyun acilinca ana menu gosterilsin mi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fortune")
	bool bShowMainMenu = true;

	/** Tam ekran menu acikken coin atma ve HUD kapanir. */
	UFUNCTION(BlueprintCallable, Category = "Fortune")
	void SetMenuOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Fortune")
	bool IsMenuOpen() const { return bMenuOpen; }

	// ---- TEST komutlari (konsol: ` tusu) ----
	/** GiveScrolls 10 */
	UFUNCTION(Exec)
	void GiveScrolls(int32 Count = 5);

	/** GiveCoin Skull  (isim parcasi; bos = tum coinler) */
	UFUNCTION(Exec)
	void GiveCoin(const FString& Name);

	/** GiveAllCoins: tum coinleri ver */
	UFUNCTION(Exec)
	void GiveAllCoins();

	/** GiveMoney 1000 */
	UFUNCTION(Exec)
	void GiveMoney(double Amount = 1000.0);

	/** Sag tik basili + fare: etrafa bakma hassasiyeti. */
	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float LookSensitivity = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float MaxLookYaw = 80.f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float MaxLookUp = 30.f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float MaxLookDown = 25.f;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void FlipPressed();
	void AutoFlipPressed();
	void LookPressed() { bLooking = true; }
	void LookReleased() { bLooking = false; }
	void ZoomIn() { TargetFOV = FMath::Clamp(TargetFOV - 4.f, 35.f, 85.f); }
	void ZoomOut() { TargetFOV = FMath::Clamp(TargetFOV + 4.f, 35.f, 85.f); }
	void ResetLook() { TargetYaw = 0.f; TargetPitch = 0.f; TargetFOV = BaseFOV; }

	TWeakObjectPtr<class ACameraActor> MainCamera;
	FRotator BaseRotation = FRotator::ZeroRotator;
	float BaseFOV = 60.f;
	float TargetYaw = 0.f, TargetPitch = 0.f, TargetFOV = 60.f;
	float CurYaw = 0.f, CurPitch = 0.f;
	bool bLooking = false;
	void DebugScrollsPressed() { GiveScrolls(5); }
	void DebugMoneyPressed() { GiveMoney(1000.0); }

	UPROPERTY()
	TObjectPtr<class UUserWidget> HUDWidget;

	bool bMenuOpen = false;
};
