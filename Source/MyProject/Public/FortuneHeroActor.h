// Flip for Fortune - Ana karakter (sokak genci). Parcali model: her parca kendi ekleminde doner, kodla animasyon.
// Coin'i avucundan fiskeyle atar ve yine avucuna yakalar (masa olmadan da calisir).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneHeroActor.generated.h"

class UProceduralMeshComponent;

UCLASS()
class MYPROJECT_API AFortuneHeroActor : public AActor
{
	GENERATED_BODY()

public:
	AFortuneHeroActor();
	virtual void Tick(float DeltaSeconds) override;

	/** Coin'in avuctaki yeri (dunya). */
	FVector GetCoinPoint() const;

	/** Windup 0..1 (el asagi cekilir), Flick 0..1 (el yukari firlatir). */
	void SetPose(float Windup, float Flick);

	/** Yakalama: 0..1 (el coinle birlikte asagi esner). */
	void SetCatch(float Catch);

	/** Karakter bu noktaya bakar (coin havadayken). Bos (ZeroVector) = avucuna bakar. */
	void SetLookTarget(const FVector& WorldPoint);

	/** Coin hangi kolda: true = +Y tarafindaki kol. */
	UPROPERTY(EditAnywhere, Category = "Hero")
	bool bCoinArmPlusY = true;

	/** Birinci sahis: bas, govde ve bacaklar gizlenir (kamera goz hizasinda); sadece kollar ve eller gorunur. */
	UPROPERTY(EditAnywhere, Category = "Hero")
	bool bFirstPerson = true;

	/** Birinci sahis: kollarin bakisla birlikte donecegi aci (derece, + = yukari). */
	void SetViewPitch(float Pitch) { ViewPitch = Pitch; }

	/** Govdenin dunya donusu (yaw + kollarin bakis egimi): avuctaki coin bununla birlikte doner. */
	FQuat GetBodyFrame() const;

	/** Goz noktasi (dunya) - birinci sahis kamerasi buraya konur. */
	FVector GetEyePoint() const;

	/** Modeller: Art/Hero/hero_*.bin (renkli, Python ile uretilir). */
	UPROPERTY(EditAnywhere, Category = "Hero")
	FString PartsFolder = TEXT("Art/Hero");

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	/** Harita kaydedilirken mesh verisi dosyaya yazilmaz (yoksa .umap 100 MB'i asar); kayittan sonra yeniden yuklenir. */
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

private:
	struct FJointDef { const TCHAR* Name; int32 Parent; FVector Loc; const TCHAR* Part; };
	static const TArray<FJointDef>& Defs();

	bool LoadParts();
	bool LoadPart(UProceduralMeshComponent* PMC, const FString& File);
	void ApplyPose();
	int32 JointIndex(const TCHAR* Name) const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<USceneComponent>> Joints;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UProceduralMeshComponent>> Meshes;

	float Windup = 0.f;
	float Flick = 0.f;
	float Catch = 0.f;
	float Time = 0.f;
	float ViewPitch = 0.f;
	FVector LookTarget = FVector::ZeroVector;
	float LookBlend = 0.f;
	bool bLoaded = false;
	bool bReloadAfterSave = false;
};
