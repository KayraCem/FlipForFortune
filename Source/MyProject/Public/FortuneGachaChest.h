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
	enum class EState : uint8 { Idle, Shake, Open, Fly, Reveal, Closing };

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
