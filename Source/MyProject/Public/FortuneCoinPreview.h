// Flip for Fortune - Coin Collection ekraninda kameranin onunde donen buyuk coin.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneCoinPreview.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UFortuneCoinData;
class UMaterialInterface;
class UStaticMesh;
class UProceduralMeshComponent;

UCLASS(NotBlueprintable)
class MYPROJECT_API AFortuneCoinPreview : public AActor
{
	GENERATED_BODY()

public:
	AFortuneCoinPreview();

	virtual void Tick(float DeltaSeconds) override;

	/** Gosterilecek coin. bOwned false ise siyah siluet. */
	void SetCoin(const UFortuneCoinData* Coin, bool bOwned, bool bEnchanted);

	/** Kameranin onune yerlestir (Distance cm). */
	void PlaceInFrontOf(const FVector& CamLoc, const FRotator& CamRot, float Distance, float SideOffset);

	/** Belirli bir noktaya koy, ViewFrom'a baksin, Scale kadar buyut (vitrin odasi icin). */
	void PlaceAt(const FVector& Location, const FVector& ViewFrom, const FVector& ViewUp, float Scale);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Spinner;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> CoinMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> HeadsFace;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> HeadsGem;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> TailsFace;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> TailsRing;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> KeyLight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProceduralMeshComponent> ShapedCoin;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UFortuneCoinAuraComponent> CoinAura;

	UPROPERTY()
	TObjectPtr<UStaticMesh> DefaultCoinMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	float Angle = 0.f;
	float PopIn = 0.f;
	FVector SpinAxis = FVector::UpVector;
	FQuat BaseRot = FQuat::Identity;
};
