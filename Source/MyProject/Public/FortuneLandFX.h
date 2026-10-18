// Flip for Fortune - Coin masaya inince cikan, kodla yapilmis parcacik efekti.
// Niagara dosyasi gerektirmez; instanced mesh ile toz/kivilcim/halka/yaprak/alev.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneTypes.h"
#include "FortuneLandFX.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS(NotBlueprintable)
class MYPROJECT_API AFortuneLandFXActor : public AActor
{
	GENERATED_BODY()

public:
	AFortuneLandFXActor();

	virtual void Tick(float DeltaSeconds) override;

	/** Efekti baslat. Power: 1 = normal, kritikte ~1.6. bHeads false ise biraz soluk. */
	void Play(EFortuneLandFX Type, float Power, bool bHeads);

	/** Kolay kullanim: dunyada bir noktada efekt olustur. */
	static void SpawnAt(UWorld* World, const FVector& Location, EFortuneLandFX Type, float Power, bool bHeads);

private:
	struct FParticle
	{
		int32 Group = 0;
		int32 Instance = 0;
		FVector Pos = FVector::ZeroVector;
		FVector Vel = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		FRotator RotVel = FRotator::ZeroRotator;
		FVector Size = FVector(1.f);
		float Life = 1.f;
		float Age = 0.f;
		float Gravity = 0.f;
		float Drag = 0.f;
		bool bGrowFirst = false;
		bool bFlicker = false;
	};

	int32 AddGroup(UStaticMesh* Mesh, const FLinearColor& Color, bool bGlow, float Glow = 6.f);
	void Emit(int32 Group, const FVector& Vel, const FVector& Size, float Life, float Gravity, float Drag,
		const FVector& Offset = FVector::ZeroVector, bool bGrowFirst = false, bool bSpin = false, bool bFlicker = false);
	void Flash(const FLinearColor& Color, float Intensity, float Radius);

	FVector RandRing(float MinSpeed, float MaxSpeed, float Up) const;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Groups;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> LitMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowMaterial;

	TArray<FParticle> Particles;
	float LightAge = 0.f;
	float LightIntensity = 0.f;
	float TotalAge = 0.f;
};
