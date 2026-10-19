#include "FortuneCoinRain.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "FortuneSubsystem.h"
#include "FortuneCoinData.h"
#include "FortuneCoinShape.h"
#include "ProceduralMeshComponent.h"
#include "Engine/GameInstance.h"

AFortuneCoinRain::AFortuneCoinRain()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shape(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CoinMesh = Cyl.Object;
	ShapeMaterial = Shape.Object;
}

void AFortuneCoinRain::BeginPlay()
{
	Super::BeginPlay();

	// Oyundaki coinler (yuz dokulu, sekilli): her yagan coin gercek bir coin modeli olur
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (UFortuneSubsystem* F = GI->GetSubsystem<UFortuneSubsystem>())
		{
			TArray<UFortuneCoinData*> Pool;
			for (UFortuneCoinData* D : F->GetAllCoins())
			{
				if (D && D->HeadsTexture && !D->CoinMesh) Pool.Add(D);
			}
			if (Pool.Num() > 0)
			{
				const int32 N = FMath::Min(Count, 45);
				Coins.SetNum(N);
				for (int32 i = 0; i < N; ++i)
				{
					UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(this);
					PMC->SetupAttachment(GetRootComponent());
					PMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					PMC->SetCastShadow(false);
					PMC->RegisterComponent();
					// Her coin en az bir kez gorunsun, sonra rastgele
					UFortuneCoinData* D = Pool[i < Pool.Num() ? i : FMath::RandRange(0, Pool.Num() - 1)];
					FFortuneCoinShapeBuilder::BuildForCoin(PMC, D, true, false);
					FCoin& C = Coins[i];
					C.Shaped = ShapedCoins.Add(PMC);
					Respawn(C, true);
				}
				return;
			}
		}
	}

	// Projedeki metal coin materyalleri (Python betigi olusturur); yoksa renkli duz materyal
	struct FLook { const TCHAR* Path; FLinearColor Fallback; int32 Weight; };
	const FLook Looks[] = {
		{ TEXT("/Game/FlipForFortune/Materials/M_CoinGold.M_CoinGold"),       FLinearColor(1.f, 0.72f, 0.25f), 5 },
		{ TEXT("/Game/FlipForFortune/Materials/M_CoinBronze.M_CoinBronze"),   FLinearColor(0.62f, 0.36f, 0.18f), 3 },
		{ TEXT("/Game/FlipForFortune/Materials/M_CoinSilver.M_CoinSilver"),   FLinearColor(0.85f, 0.86f, 0.9f), 3 },
		{ TEXT("/Game/FlipForFortune/Materials/M_CoinClover.M_CoinClover"),   FLinearColor(0.2f, 0.75f, 0.35f), 1 },
		{ TEXT("/Game/FlipForFortune/Materials/M_CoinPhoenix.M_CoinPhoenix"), FLinearColor(1.f, 0.3f, 0.08f), 1 },
	};
	TArray<int32> Bag;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Looks); ++i)
	{
		UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
		ISM->SetMobility(EComponentMobility::Movable);
		ISM->SetupAttachment(GetRootComponent());
		ISM->SetStaticMesh(CoinMesh);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCastShadow(false);
		ISM->RegisterComponent();
		if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, Looks[i].Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			ISM->SetMaterial(0, Mat);
		}
		else if (UMaterialInstanceDynamic* MID = ISM->CreateDynamicMaterialInstance(0, ShapeMaterial))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Looks[i].Fallback);
		}
		Groups.Add(ISM);
		for (int32 w = 0; w < Looks[i].Weight; ++w) Bag.Add(i);
	}

	Coins.SetNum(Count);
	for (FCoin& C : Coins)
	{
		C.Group = Bag[FMath::RandRange(0, Bag.Num() - 1)];
		C.Instance = Groups[C.Group]->AddInstance(FTransform::Identity, false);
		Respawn(C, true);
	}
}

void AFortuneCoinRain::Respawn(FCoin& C, bool bAnywhere)
{
	C.Pos = FVector(
		FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
		FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y),
		bAnywhere ? FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z) : BoxExtent.Z + FMath::FRandRange(0.f, 20.f));
	C.Speed = FMath::FRandRange(FallSpeed.X, FallSpeed.Y);
	C.Rot = FRotator(FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f));
	C.Spin = FRotator(FMath::FRandRange(-220.f, 220.f), FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-260.f, 260.f));
	C.Sway = FMath::FRandRange(2.f, 8.f);
	C.Phase = FMath::FRandRange(0.f, 2.f * PI);
}

void AFortuneCoinRain::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	const FVector Scale(CoinDiameter / 100.f, CoinDiameter / 100.f, CoinDiameter * 0.12f / 100.f);

	const float ShapedScale = CoinDiameter / 6.f;   // BuildForCoin 6 cm cap
	for (FCoin& C : Coins)
	{
		C.Pos.Z -= C.Speed * DeltaSeconds;
		C.Rot += C.Spin * DeltaSeconds;
		if (C.Pos.Z < -BoxExtent.Z)
		{
			Respawn(C, false);
		}
		const FVector P = C.Pos + FVector(0.f, FMath::Sin(Time * 1.3f + C.Phase) * C.Sway, 0.f);
		if (ShapedCoins.IsValidIndex(C.Shaped))
		{
			ShapedCoins[C.Shaped]->SetRelativeTransform(FTransform(C.Rot, P, FVector(ShapedScale)));
			continue;
		}
		Groups[C.Group]->UpdateInstanceTransform(C.Instance, FTransform(C.Rot, P, Scale), false, false, true);
	}
	for (UInstancedStaticMeshComponent* G : Groups)
	{
		if (G) G->MarkRenderStateDirty();
	}
}
