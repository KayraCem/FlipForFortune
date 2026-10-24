// Flip for Fortune - Birinci sahis: WASD yuru, fare ile bak, nisangahla etkilesim, Space = coin at.
// Menu/panel acikken (veya Alt basiliyken) fare imleci gorunur ve HUD ikonlarina tiklanabilir.
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

	/** Coin Collection ekranini ac/kapat (HUD'daki sol ikon veya C tusu). */
	UFUNCTION(BlueprintCallable, Category = "Fortune")
	void ToggleCollection();

	/** Envanteri ac/kapat (HUD'daki sol ikon veya I tusu). */
	UFUNCTION(BlueprintCallable, Category = "Fortune")
	void ToggleInventory();

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

	/** Fare ile bakma hassasiyeti (derece / fare birimi). */
	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float MouseSensitivity = 0.35f;

	/** Karakter yoksa: sag tik basili + fare ile bakma hassasiyeti. */
	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float LookSensitivity = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float MaxLookYaw = 80.f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float MaxLookUp = 30.f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float MaxLookDown = 25.f;

	/** Birinci sahis: baslangic bakis acisi ve sinirlar (derece). */
	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float StartViewPitch = -10.f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float MinViewPitch = -75.f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float MaxViewPitch = 70.f;

	/** Kollarin tasarlandigi bakis acisi: bu acida el ekranin altinda durur, kollar bakisi takip eder. */
	UPROPERTY(EditAnywhere, Category = "Fortune|Camera")
	float ArmBasePitch = -34.f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Move")
	float WalkSpeed = 230.f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Move")
	float RunSpeed = 400.f;

	UPROPERTY(EditAnywhere, Category = "Fortune|Move")
	float CapsuleRadius = 28.f;

	/** Nisangahla etkilesim mesafesi (cm). */
	UPROPERTY(EditAnywhere, Category = "Fortune|Move")
	float InteractDistance = 450.f;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void FlipPressed();
	void AutoFlipPressed();
	void LookPressed() { bLooking = true; }
	void LookReleased() { bLooking = false; }
	void ZoomIn() { TargetFOV = FMath::Clamp(TargetFOV - 4.f, 50.f, 100.f); }
	void ZoomOut() { TargetFOV = FMath::Clamp(TargetFOV + 4.f, 50.f, 100.f); }
	void ResetLook() { TargetYaw = 0.f; TargetPitch = 0.f; TargetFOV = BaseFOV; }

	// ---- birinci sahis ----
	void ClickPressed();     // sol tik: nisangahtaki esya, yoksa coin at
	void UsePressed();       // E: nisangahtaki esya
	bool WantsUI() const;
	void ApplyInputMode(bool bUI);
	void TickFirstPerson(float DeltaTime);
	void MoveHero(const FVector& Delta);
	class AFortuneInteractable* TraceInteractable() const;

	TWeakObjectPtr<class AFortuneHeroActor> Hero;
	TWeakObjectPtr<class AFortuneInteractable> Hovered;
	FVector CamRelLoc = FVector::ZeroVector;
	float HeroYaw = 0.f, ViewPitch = 0.f;
	float BobTime = 0.f, BobAmount = 0.f;
	bool bUIMode = true;

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
private:
	void TogglePanel(TSubclassOf<UUserWidget> Cls, TWeakObjectPtr<UUserWidget>& Panel);
	TWeakObjectPtr<UUserWidget> CollectionPanel;
	TWeakObjectPtr<UUserWidget> InventoryPanel;
};
