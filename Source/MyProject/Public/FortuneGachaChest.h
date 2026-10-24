// Flip for Fortune - Gacha hazine sandigi. Ici renkli coin dolu, kapagi aralik.
// Tiklayinca: sallanir -> kapak acilir -> isik -> coin ekrana ucar -> "YENI!" ekrani -> tikla -> Coin Collection.
// Uzerine gelince coinlerin nadirlik ve cikma sanslari gorunur.
#pragma once

#include "CoreMinimal.h"
#include "FortuneInteractable.h"
#include "FortuneGachaChest.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UWidgetComponent;
class UFortuneCoinData;
class AFortuneCoinPreview;
class UUserWidget;
class USoundBase;
class UMaterialInterface;

UCLASS()
class MYPROJECT_API AFortuneGachaChest : public AFortuneInteractable
{
	GENERATED_BODY()

public:
	AFortuneGachaChest();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Interact() override;
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;

	/** "YENI!" ekrani kapaninca cagrilir. */
	void FinishReveal();

	/** Summon panelindeki SUMMON butonu: para harcar ve acilis animasyonunu baslatir (Count > 1: coklu acilim). false = yetersiz para. */
	bool StartSummon(int32 Count = 1);

	bool IsBusy() const { return State != EState::Idle; }

	/** Doluysa summon bu sandikta oynar (ayri SUMMON SAHNESI: kopya cesme, kendi kamerasi). */
	UPROPERTY(EditAnywhere, Category = "Gacha")
	TObjectPtr<AFortuneGachaChest> StageChest;

	/** Bu sandik bir summon sahnesiyse: animasyon boyunca bu kameraya gecilir, bitince geri donulur. */
	UPROPERTY(EditAnywhere, Category = "Gacha")
	TObjectPtr<class ACameraActor> StageCamera;

	/** Sifir degilse coinler buradan (aktore gore, dunya ekseninde) cikar: ornegin cesmenin on tarafindaki su. */
	UPROPERTY(EditAnywhere, Category = "Gacha")
	FVector EmergeOffset = FVector::ZeroVector;

	/** Coklu summonda coinlerin cikis noktalarinin yana yayilmasi (cm). */
	UPROPERTY(EditAnywhere, Category = "Gacha")
	float EmergeSpread = 40.f;

	/** Summon sirasinda parlayacak cesme. */
	UPROPERTY(EditAnywhere, Category = "Gacha")
	TObjectPtr<class AFortuneFountain> LinkedFountain;

	/** Sonuc ekrani: TAMAM (oyuna don). */
	void CloseRevealAndReturn();

	/** Sonuc ekrani: TEKRAR / x10 (sahnede kalarak yeniden summon). false = para yetmiyor. */
	bool RepeatSummon(int32 Count);

	/** Son summon kac coinlikti (TEKRAR butonu icin). */
	int32 GetLastCount() const { return LastCount; }

	/** true: sandik modeli gizlenir (dilek cesmesi). Summon, isik ve coin animasyonu aynen calisir. */
	UPROPERTY(EditAnywhere, Category = "Gacha")
	bool bHideModel = false;

	/** Sifir degilse tiklama kutusunun yari boyutu (cm), ornegin tum cesmeyi kaplasin diye. */
	UPROPERTY(EditAnywhere, Category = "Gacha")
	FVector ClickExtent = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Gacha")
	TObjectPtr<USoundBase> ShakeSound;

	UPROPERTY(EditAnywhere, Category = "Gacha")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, Category = "Gacha")
	TObjectPtr<USoundBase> RevealSound;

	/** Kapak bosta ne kadar aralik dursun (derece). */
	UPROPERTY(EditAnywhere, Category = "Gacha")
	float IdleLidAngle = 28.f;

protected:
	virtual void BeginPlay() override;

private:
	enum class EState : uint8 { Idle, Shake, Open, Fly, Reveal, Closing, Legend };

	UStaticMeshComponent* MakePart(const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm, const FRotator& Rot = FRotator::ZeroRotator);
	void SetLid(float Angle);
	void ShowRates(bool bShow);
	void BuildRatesText();

	UPROPERTY(VisibleAnywhere, Category = "Gacha")
	TObjectPtr<USceneComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Gacha")
	TObjectPtr<USceneComponent> LidPivot;

	UPROPERTY(VisibleAnywhere, Category = "Gacha")
	TObjectPtr<UPointLightComponent> InnerLight;

	UPROPERTY(VisibleAnywhere, Category = "Gacha")
	TObjectPtr<UWidgetComponent> RatesWidget;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> WoodParts;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> GoldParts;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> PileCoins;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphMesh;

	UPROPERTY()
	TObjectPtr<AFortuneCoinPreview> FlyingCoin;

	UPROPERTY()
	TObjectPtr<UUserWidget> RevealWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> MoneyChip;

	UPROPERTY()
	TObjectPtr<UFortuneCoinData> WonCoin;

	/** Coklu acilim: cikan coinler (nadirlige gore siralı, en iyisi en sonda), ekrandaki yerleri. */
	UPROPERTY()
	TArray<TObjectPtr<UFortuneCoinData>> WonCoins;

	UPROPERTY()
	TArray<TObjectPtr<AFortuneCoinPreview>> MultiCoins;

	TArray<bool> WonDups;
	TArray<FVector> MultiSlots;
	TArray<bool> MultiLanded;
	bool bMulti = false;
	int32 LastCount = 1;
	TWeakObjectPtr<AActor> ReturnViewTarget;
	void ClearRevealCoins();
	FVector EmergePoint(int32 Index, int32 Count, const FVector& Right) const;
	void SetFountainGlow(float Amount, const FLinearColor& Color);
	void TickLegend(APlayerController* PC);
	bool IsLegendWin() const;

	UPROPERTY() TObjectPtr<UStaticMeshComponent> LegendBeam;
	UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> BeamMID;
	UPROPERTY() TObjectPtr<AFortuneCoinPreview> LegendCoin;
	float NextFxTime = 0.f;
	float StageBaseFOV = 0.f;
	bool bHasFlyStart = false;
	FVector FlyStart = FVector::ZeroVector;
	int32 BestIndex = 0;
	float BurstTime = -1.f;

	void TickMultiFly(float DeltaSeconds, APlayerController* PC);
	float MultiStartTime(int32 Index) const;

	EState State = EState::Idle;
	float StateTime = 0.f;
	bool bWonDuplicate = false;
	float LidAngle = 0.f;
	FLinearColor RarityColor = FLinearColor::White;
	FVector BodyRestLoc = FVector::ZeroVector;
};
