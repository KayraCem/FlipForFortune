// Flip for Fortune - Coin'i fiskeyle atan el (kodla kurulur, model dosyasi gerekmez).
// Yumruk pozunda: coin isaret parmaginin ustunde durur, basparmak fiske vurur.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FortuneHandActor.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class USkeletalMesh;
class UPoseableMeshComponent;
class UProceduralMeshComponent;

UCLASS()
class MYPROJECT_API AFortuneHandActor : public AActor
{
	GENERATED_BODY()

public:
	AFortuneHandActor();
	virtual void Tick(float DeltaSeconds) override;

	/** Coin'in elde durdugu nokta (dunya). */
	FVector GetCoinPoint() const;

	/** Poz: Windup 0..1 (basparmak gerilir), Flick 0..1 (basparmak firlar, bilek kalkar). */
	void SetPose(float Windup, float Flick);

	UPROPERTY(EditAnywhere, Category = "Hand")
	FLinearColor SkinColor = FLinearColor(0.045f, 0.03f, 0.028f);   // koyu deri eldiven

	UPROPERTY(EditAnywhere, Category = "Hand")
	FLinearColor NailColor = FLinearColor(0.75f, 0.5f, 0.15f);    // basparmakta altin yuksuk

	UPROPERTY(EditAnywhere, Category = "Hand")
	FLinearColor SleeveColor = FLinearColor(0.06f, 0.025f, 0.05f);

	// ---- Iskeletli el (orn. First Person paketindeki mannequin kolu) ----
	/** Doluysa kodla kurulan el gizlenir; bu model parmak kemikleri bukulerek yumruk yapilir. */
	UPROPERTY(EditAnywhere, Category = "Hand|Mesh")
	TObjectPtr<USkeletalMesh> HandMesh;

	/** Modelin boyu (aktorun olcegine ek). */
	UPROPERTY(EditAnywhere, Category = "Hand|Mesh")
	float MeshScale = 0.75f;

	/** Parmaklarin bukulme ekseni (kemigin yerel ekseni). Parmaklar ters bukuluyorsa isareti cevir. */
	UPROPERTY(EditAnywhere, Category = "Hand|Mesh")
	FVector FingerCurlAxis = FVector(0.f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Hand|Mesh")
	float FingerCurlSign = 1.f;

	UPROPERTY(EditAnywhere, Category = "Hand|Mesh")
	FVector ThumbCurlAxis = FVector(0.f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Hand|Mesh")
	float ThumbCurlSign = 1.f;

	/** Gizlenecek kemikler (cocuklariyla): sol kol, bacaklar, bas... */
	UPROPERTY(EditAnywhere, Category = "Hand|Mesh")
	TArray<FName> HiddenBones = { TEXT("clavicle_l"), TEXT("thigh_l"), TEXT("thigh_r"), TEXT("neck_01") };

	/** Kol yonu duzeltmesi (derece): model yanlis acida kalirsa buradan cevir. */
	UPROPERTY(EditAnywhere, Category = "Hand|Mesh")
	FRotator MeshRotationOffset = FRotator::ZeroRotator;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UStaticMeshComponent* Part(const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm, const FRotator& Rot = FRotator::ZeroRotator);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Wrist;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ThumbPivot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ThumbTipPivot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> CoinPoint;

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SkinParts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> NailParts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SleeveParts;

	UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMaterial;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UPoseableMeshComponent> Skel;

	void SetupSkeletal();

	/** Art/Hand/*.bin dosyalarindaki puruzsuz el modelini yukler (yoksa eski el kalir). */
	bool LoadSmoothHand();
	bool LoadBinMesh(UProceduralMeshComponent* PMC, const FString& File, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> FistMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> ThumbMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> ThumbTipMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> ThumbNailMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> ArmMesh;
	void ApplySkeletalPose();
	bool bSkeletal = false;
	FVector SkelBaseLoc = FVector::ZeroVector;
	FVector CoinLocalCS = FVector::ZeroVector;   // coin noktasi (iskelet bileseni uzayinda)

	FVector WristRest = FVector::ZeroVector;
	float PoseWindup = 0.f;
	float PoseFlick = 0.f;
	float Time = 0.f;
};
