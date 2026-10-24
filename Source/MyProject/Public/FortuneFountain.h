// Flip for Fortune - "Ruh Kadehi" dilek cesmesi: 3 katli tas cesme, akan ve dalgalanan ruh suyu, parlayan runler.
// Modeller: Art/Fountain/fountain_*.bin (Python ile uretilir). Su animasyonu materyallerde (M_Fountain_*).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneFountain.generated.h"

class UProceduralMeshComponent;
class UPointLightComponent;

UCLASS()
class MYPROJECT_API AFortuneFountain : public AActor
{
	GENERATED_BODY()

public:
	AFortuneFountain();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Fountain")
	FString ModelFolder = TEXT("Art/Fountain");

	/** Ruh isiginin rengi (kure, su). */
	UPROPERTY(EditAnywhere, Category = "Fountain")
	FLinearColor SpiritColor = FLinearColor(0.25f, 1.f, 0.85f);

	/** Summon sirasinda cesme parlar: 0 = normal, 1+ = guclu (renk = cikan coinin nadirligi). */
	void SetSummonGlow(float Amount, const FLinearColor& Color) { GlowAmount = Amount; GlowColor = Color; }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	/** Harita kaydedilirken mesh verisi dosyaya yazilmaz (yoksa .umap 100 MB'i asar); kayittan sonra yeniden yuklenir. */
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

private:
	bool LoadFountainPart(UProceduralMeshComponent* PMC, const TCHAR* File, const TCHAR* MaterialPath, bool bShadow);
	void LoadAll();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> StoneMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> GlowMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> WaterMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> FallsMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> OrbLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> PoolLight;

	float Time = 0.f;
	float GlowAmount = 0.f;
	float GlowShown = 0.f;
	FLinearColor GlowColor = FLinearColor::White;
	bool bLoaded = false;
	bool bReloadAfterSave = false;
};
