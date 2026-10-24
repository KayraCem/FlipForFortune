// Flip for Fortune - Coinin etrafinda surekli oynayan tema efekti (alev, zincir, hale, yorunge, isilti).
// Coinin pivotuna (yuz normali = yerel Z) baglanir; coin ile birlikte doner/ucar.
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FortuneTypes.h"
#include "FortuneCoinAura.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS(ClassGroup = (Fortune), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UFortuneCoinAuraComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFortuneCoinAuraComponent();

	/** Efekti kur. Radius/HalfThickness: coinin yerel olculeri (cm). None = temizle. */
	void SetAura(EFortuneCoinAura InAura, const FLinearColor& InColor, float InRadius = 3.f, float InHalfThickness = 0.3f);

	EFortuneCoinAura GetAura() const { return Aura; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void OnUnregister() override;
	virtual void OnVisibilityChanged() override;

private:
	struct FFlame
	{
		FVector Pos = FVector::ZeroVector;
		FVector Vel = FVector::ZeroVector;
		float Age = 0.f;
		float Life = 1.f;
		float Size = 1.f;
		float Phase = 0.f;
	};

	void Clear();
	UInstancedStaticMeshComponent* MakeISM(UStaticMesh* Mesh, const FLinearColor& Color, bool bGlow, float Glow, bool bWorldSpace, bool bPlain = false);
	void AddLink(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& U, const FVector& V, float Len, float Wid, float Bar);
	void BuildChains();
	void BuildPadlock(const FVector& Center, const FVector& U, const FVector& V, const FVector& N, float Scale = 1.f);
	void BuildThorns();
	void BuildTape();
	void BuildBolts();
	void BuildStem();
	void BuildBones();
	void TickWings();
	void TickClock();
	void TickCompass();
	void TickVault();
	void TickSparkle();
	void TickPages();
	void TickRose(float Dt);
	void BuildRose();
	void BuildVines(bool bLeaves);
	void TickKelp();
	void TickZone();
	void RespawnFlame(FFlame& F, int32 Group, bool bRandomAge);
	void TickFire(float Dt);
	void TickLocal(float Dt);

	EFortuneCoinAura Aura = EFortuneCoinAura::None;
	FLinearColor Color = FLinearColor::White;
	float Radius = 3.f;
	float Half = 0.3f;
	float Time = 0.f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Groups;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MetalMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PlainMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ConeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylMesh;

	TArray<FFlame> Flames[3];       // 0 dis alev, 1 ic cekirdek, 2 kor
	TArray<FVector> SparkSeeds;      // isilti / hale icin rastgele degerler
	TArray<FFlame> Petals;           // dokulen gul yapraklari (dunya)
	int32 PetalGroup = INDEX_NONE;
	float LightBase = 0.f;
};
