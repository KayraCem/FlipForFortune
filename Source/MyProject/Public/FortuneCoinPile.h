// Flip for Fortune - Dilek cesmesinin dibindeki coin yigini: oyundaki gercek coinler (ana menudeki gibi).
// Bir kismi suyun ustunde yavasca donerek suzulur.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneCoinPile.generated.h"

class UProceduralMeshComponent;

UCLASS()
class MYPROJECT_API AFortuneCoinPile : public AActor
{
	GENERATED_BODY()

public:
	AFortuneCoinPile();
	virtual void Tick(float DeltaSeconds) override;

	/** Yiginin yaricapi (cm). */
	UPROPERTY(EditAnywhere, Category = "Pile")
	float Radius = 55.f;

	UPROPERTY(EditAnywhere, Category = "Pile", meta = (ClampMin = "1", ClampMax = "200"))
	int32 Count = 80;

	UPROPERTY(EditAnywhere, Category = "Pile")
	float CoinDiameter = 7.f;

	/** Ortadaki tepenin yuksekligi (cm). */
	UPROPERTY(EditAnywhere, Category = "Pile")
	float MoundHeight = 9.f;

	/** Suyun ustunde donerek suzulen coin sayisi. */
	UPROPERTY(EditAnywhere, Category = "Pile")
	int32 FloatingCount = 6;

	/** Suzulen coinlerin yuksekligi (yiginin tabanindan, cm). */
	UPROPERTY(EditAnywhere, Category = "Pile")
	float FloatHeight = 36.f;

	/** Dipten yukselip suyun ustunde donen, sonra geri batan coin sayisi. */
	UPROPERTY(EditAnywhere, Category = "Pile")
	int32 RiserCount = 3;

	/** Yukselen coinlerin cikacagi yukseklik (cm). */
	UPROPERTY(EditAnywhere, Category = "Pile")
	float RiseHeight = 38.f;

	/** Dipteki coinlerin su akintisiyla sallanma miktari (derece). */
	UPROPERTY(EditAnywhere, Category = "Pile")
	float SwayDegrees = 4.f;

protected:
	virtual void BeginPlay() override;

private:
	struct FFloater
	{
		int32 Index = 0;
		FVector Base = FVector::ZeroVector;
		float Phase = 0.f;
		float Speed = 30.f;
	};

	struct FRester
	{
		int32 Index = 0;
		FVector Loc = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		float Phase = 0.f;
	};

	struct FRiser
	{
		int32 Index = 0;
		float Offset = 0.f;
		int32 Cycle = -1;
		FVector From = FVector::ZeroVector;
		FRotator RestRot = FRotator::ZeroRotator;
	};

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TArray<TObjectPtr<UProceduralMeshComponent>> Coins;

	TArray<FFloater> Floaters;
	TArray<FRester> Resters;
	TArray<FRiser> Risers;
	float CoinScale = 1.f;
	float Time = 0.f;
};
