// Flip for Fortune - Ana menude yagan, donen renkli coinler (sonsuz dongu).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneCoinRain.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS()
class MYPROJECT_API AFortuneCoinRain : public AActor
{
	GENERATED_BODY()

public:
	AFortuneCoinRain();
	virtual void Tick(float DeltaSeconds) override;

	/** Coinlerin yagdigi kutunun yari boyutu (cm). */
	UPROPERTY(EditAnywhere, Category = "Rain")
	FVector BoxExtent = FVector(35.f, 55.f, 90.f);

	UPROPERTY(EditAnywhere, Category = "Rain", meta = (ClampMin = "1", ClampMax = "300"))
	int32 Count = 70;

	UPROPERTY(EditAnywhere, Category = "Rain")
	float CoinDiameter = 10.f;

	UPROPERTY(EditAnywhere, Category = "Rain")
	FVector2D FallSpeed = FVector2D(35.f, 80.f);

protected:
	virtual void BeginPlay() override;

private:
	struct FCoin
	{
		int32 Group = 0;
		int32 Instance = 0;
		FVector Pos = FVector::ZeroVector;   // aktore gore
		float Speed = 50.f;
		FRotator Rot = FRotator::ZeroRotator;
		FRotator Spin = FRotator::ZeroRotator;
		float Sway = 0.f;
		float Phase = 0.f;
	};

	void Respawn(FCoin& C, bool bAnywhere);

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Groups;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CoinMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	TArray<FCoin> Coins;
	float Time = 0.f;
};
