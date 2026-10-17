// Flip for Fortune - Sahnedeki coin + (istege bagli) el. Flip() cagrilinca coini atar.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneTypes.h"
#include "FortuneCoinFlipper.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UNiagaraComponent;
class UFortuneSubsystem;
class UMaterialInterface;

UCLASS(Blueprintable)
class MYPROJECT_API AFortuneCoinFlipper : public AActor
{
	GENERATED_BODY()

public:
	AFortuneCoinFlipper();

	virtual void Tick(float DeltaSeconds) override;

	/** Coini at. Zaten havadaysa false doner. UI butonuna veya bir tusa bagla. */
	UFUNCTION(BlueprintCallable, Category = "Fortune")
	bool Flip(bool bManual = true);

	UFUNCTION(BlueprintPure, Category = "Fortune")
	bool IsFlipping() const { return bIsFlipping; }

	/** Kusanili coinin mesh/materyal/enchant efektini gunceller. */
	UFUNCTION(BlueprintCallable, Category = "Fortune")
	void RefreshCoinVisuals();

	// ---- Bilesenler ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Coinin masadaki dinlenme noktasi; coin bunun icinde hareket eder. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> CoinPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CoinMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> EnchantFX;

	/** Yazi yuzu (ust): parlak amblem. Coin kendi mesh'ini kullanirsa gizlenir. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HeadsFace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HeadsGem;

	/** Tura yuzu (alt): koyu amblem. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TailsFace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TailsRing;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	FLinearColor HeadsColor = FLinearColor(1.f, 0.93f, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	FLinearColor TailsColor = FLinearColor(0.12f, 0.06f, 0.02f);

	/** Insan eli: Skeletal Mesh ve Anim Blueprint'ini buradan ata (montage icin "DefaultSlot" gerekir). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> HandMesh;

	// ---- Ayarlar ----
	/** El animasyonu oynarsa coinin havaya kalkmasi icin beklenecek sure (montage'daki fiske ani). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	float HandReleaseDelay = 0.15f;

	/** true ise coin her atista ayni noktaya, false ise biraz yana duser. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fortune")
	bool bLandInPlace = true;

	// ---- Blueprint olaylari (efekt, UI, kamera sarsintisi vb. icin) ----
	UFUNCTION(BlueprintImplementableEvent, Category = "Fortune")
	void OnFlipStarted(const FFortuneFlipResult& Result);

	UFUNCTION(BlueprintImplementableEvent, Category = "Fortune")
	void OnFlipLanded(const FFortuneFlipResult& Result);

	UFUNCTION(BlueprintImplementableEvent, Category = "Fortune")
	void OnCriticalHit(const FFortuneFlipResult& Result);

	UFUNCTION(BlueprintImplementableEvent, Category = "Fortune")
	void OnJackpot(const FFortuneFlipResult& Result);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UFUNCTION()
	void HandleCoinsChanged();

	void Land();
	void PlayCriticalEffects();
	UFortuneSubsystem* GetFortune() const;

	bool bIsFlipping = false;
	float Elapsed = 0.f;
	float Delay = 0.f;
	FFortuneFlipResult Current;
	FTransform RestTransform;
	FVector LandOffset = FVector::ZeroVector;
	bool bRestIsHeads = true;
	float AutoTimer = 0.f;
};
