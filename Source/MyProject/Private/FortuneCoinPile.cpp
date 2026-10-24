#include "FortuneCoinPile.h"
#include "ProceduralMeshComponent.h"
#include "FortuneSubsystem.h"
#include "FortuneCoinData.h"
#include "FortuneCoinShape.h"
#include "Engine/GameInstance.h"
#include "Components/SceneComponent.h"

AFortuneCoinPile::AFortuneCoinPile()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void AFortuneCoinPile::BeginPlay()
{
	Super::BeginPlay();

	const UGameInstance* GI = GetGameInstance();
	UFortuneSubsystem* F = GI ? GI->GetSubsystem<UFortuneSubsystem>() : nullptr;
	if (!F) return;

	TArray<UFortuneCoinData*> Pool;
	for (UFortuneCoinData* D : F->GetAllCoins())
	{
		if (D && D->HeadsTexture && !D->CoinMesh) Pool.Add(D);
	}
	if (Pool.Num() == 0) return;

	FRandomStream R(1337);
	const float Scale = CoinDiameter / 6.f;   // BuildForCoin 6 cm cap
	CoinScale = Scale;
	const int32 NumFloat = FMath::Clamp(FloatingCount, 0, Count);
	const int32 NumRise = FMath::Clamp(RiserCount, 0, 12);

	for (int32 i = 0; i < Count + NumRise; ++i)
	{
		UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(this);
		PMC->SetupAttachment(Root);
		PMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PMC->SetCastShadow(false);
		PMC->RegisterComponent();
		// Her coin en az bir kez gorunsun, sonra rastgele
		UFortuneCoinData* D = (i < Pool.Num()) ? Pool[i] : Pool[R.RandRange(0, Pool.Num() - 1)];
		FFortuneCoinShapeBuilder::BuildForCoin(PMC, D, true, false);
		const int32 Idx = Coins.Add(PMC);

		if (i >= Count)
		{
			// Yukselen coin: dongusu Tick'te
			FRiser Ri;
			Ri.Index = Idx;
			Ri.Offset = (i - Count) * (7.f / FMath::Max(1, NumRise)) + R.FRandRange(0.f, 1.5f);
			Risers.Add(Ri);
			PMC->SetRelativeScale3D(FVector(Scale));
			continue;
		}

		if (i < NumFloat)
		{
			// Suyun ustunde dik duran, donen coinler (yuzleri gorunsun)
			const float A = i * (2.f * PI / NumFloat) + R.FRandRange(-0.3f, 0.3f);
			const float Rr = Radius * R.FRandRange(0.3f, 0.7f);
			FFloater Fl;
			Fl.Index = Idx;
			Fl.Base = FVector(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, FloatHeight + R.FRandRange(-4.f, 4.f));
			Fl.Phase = R.FRandRange(0.f, 2.f * PI);
			Fl.Speed = R.FRandRange(25.f, 55.f) * (R.FRand() < 0.5f ? -1.f : 1.f);
			Floaters.Add(Fl);
			PMC->SetRelativeTransform(FTransform(FRotator(0.f, 0.f, 90.f), Fl.Base, FVector(Scale * 1.3f)));
			continue;
		}

		// Yigin: ortaya dogru yukselen tepe, coinler rastgele egik, bazilari ters (yazi/tura)
		const float U = FMath::Sqrt(R.FRand());
		const float A = R.FRandRange(0.f, 2.f * PI);
		const float Rr = Radius * U;
		const float Z = 0.4f + MoundHeight * FMath::Square(1.f - U) + R.FRandRange(0.f, 1.5f);
		const FRotator Rot(R.FRandRange(-22.f, 22.f), R.FRandRange(0.f, 360.f), R.FRandRange(-22.f, 22.f) + (R.FRand() < 0.4f ? 180.f : 0.f));
		const FVector Loc(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, Z);
		PMC->SetRelativeTransform(FTransform(Rot, Loc, FVector(Scale)));
		FRester Re;
		Re.Index = Idx;
		Re.Loc = Loc;
		Re.Rot = Rot;
		Re.Phase = R.FRandRange(0.f, 2.f * PI);
		Resters.Add(Re);
	}
}

void AFortuneCoinPile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	for (const FFloater& Fl : Floaters)
	{
		if (!Coins.IsValidIndex(Fl.Index) || !Coins[Fl.Index]) continue;
		const FVector P = Fl.Base + FVector(0.f, 0.f, FMath::Sin(Time * 1.3f + Fl.Phase) * 3.f);
		const FRotator Rot(FMath::Sin(Time * 0.8f + Fl.Phase) * 10.f, Time * Fl.Speed + FMath::RadiansToDegrees(Fl.Phase), 90.f);
		Coins[Fl.Index]->SetRelativeLocationAndRotation(P, Rot);
	}

	// Dipteki coinler: su akintisiyla hafifce sallanir ve kayar
	for (const FRester& Re : Resters)
	{
		if (!Coins.IsValidIndex(Re.Index) || !Coins[Re.Index]) continue;
		const float T = Time + Re.Phase;
		const FVector P = Re.Loc + FVector(FMath::Sin(T * 0.5f) * 0.5f, FMath::Cos(T * 0.43f) * 0.5f, 0.f);
		const FRotator Rot = Re.Rot + FRotator(FMath::Sin(T * 0.9f) * SwayDegrees, FMath::Sin(T * 0.3f) * 3.f, FMath::Cos(T * 0.75f) * SwayDegrees);
		Coins[Re.Index]->SetRelativeLocationAndRotation(P, Rot);
	}

	// Yukselen coinler: dipten kalkar, suyun ustunde donup parlar, geri batar (7 sn dongu)
	const float Period = 7.f;
	for (FRiser& Ri : Risers)
	{
		if (!Coins.IsValidIndex(Ri.Index) || !Coins[Ri.Index]) continue;
		const float T = Time + Ri.Offset;
		const int32 Cycle = FMath::FloorToInt(T / Period);
		if (Cycle != Ri.Cycle)
		{
			Ri.Cycle = Cycle;
			FRandomStream S(Ri.Index * 7919 + Cycle * 104729);
			const float A = S.FRandRange(0.f, 2.f * PI);
			const float Rr = Radius * FMath::Sqrt(S.FRandRange(0.05f, 0.8f));
			Ri.From = FVector(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, 1.f + MoundHeight * FMath::Square(1.f - Rr / Radius));
			Ri.RestRot = FRotator(S.FRandRange(-15.f, 15.f), S.FRandRange(0.f, 360.f), S.FRandRange(-15.f, 15.f));
		}
		const float C = FMath::Fmod(T, Period);
		const FVector Top = Ri.From + FVector(0.f, 0.f, RiseHeight);
		FVector P = Ri.From;
		FRotator Rot = Ri.RestRot;
		if (C < 2.f)            // yuksel
		{
			const float E = FMath::InterpEaseInOut(0.f, 1.f, C / 2.f, 2.f);
			P = FMath::Lerp(Ri.From, Top, E);
			Rot = FRotator(Ri.RestRot.Pitch * (1.f - E), Ri.RestRot.Yaw + E * 540.f, FMath::Lerp(Ri.RestRot.Roll, 90.f, E));
		}
		else if (C < 3.2f)      // havada don
		{
			const float E = (C - 2.f) / 1.2f;
			P = Top + FVector(0.f, 0.f, FMath::Sin(E * PI) * 3.f);
			Rot = FRotator(0.f, Ri.RestRot.Yaw + 540.f + E * 360.f, 90.f);
		}
		else if (C < 5.2f)      // geri bat
		{
			const float E = FMath::InterpEaseInOut(0.f, 1.f, (C - 3.2f) / 2.f, 2.f);
			P = FMath::Lerp(Top, Ri.From, E);
			Rot = FRotator(Ri.RestRot.Pitch * E, Ri.RestRot.Yaw + 900.f + E * 360.f, FMath::Lerp(90.f, Ri.RestRot.Roll, E));
		}
		Coins[Ri.Index]->SetRelativeLocationAndRotation(P, Rot);
	}
}
