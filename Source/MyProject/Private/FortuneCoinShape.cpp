#include "FortuneCoinShape.h"
#include "ProceduralMeshComponent.h"
#include "FortuneCoinData.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** Sekle gore dis hat noktalari (merkezden yaricap). bFlat: kenar yuzeyleri keskin mi. */
	void Outline(EFortuneCoinShape Shape, float R, TArray<FVector2D>& Out, bool& bFlat)
	{
		Out.Reset();
		bFlat = false;
		auto Poly = [&](int32 Sides, float Rot)
		{
			bFlat = true;
			for (int32 i = 0; i < Sides; ++i)
			{
				const float A = Rot + 2.f * PI * i / Sides;
				Out.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
			}
		};
		switch (Shape)
		{
		case EFortuneCoinShape::Octagon:  Poly(8, PI / 8.f); break;
		case EFortuneCoinShape::Hexagon:  Poly(6, 0.f); break;
		case EFortuneCoinShape::Decagon:  Poly(10, 0.f); break;
		case EFortuneCoinShape::Scalloped:
		{
			const int32 N = 120;
			for (int32 i = 0; i < N; ++i)
			{
				const float A = 2.f * PI * i / N;
				const float Rr = R * (0.93f + 0.07f * FMath::Abs(FMath::Cos(A * 6.f)));
				Out.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rr);
			}
			break;
		}
		case EFortuneCoinShape::Gear:
		{
			bFlat = true;
			const int32 Teeth = 14;
			for (int32 t = 0; t < Teeth; ++t)
			{
				const float Base = 2.f * PI * t / Teeth;
				const float Step = 2.f * PI / Teeth;
				const float Pts[4][2] = { {0.f, 0.86f}, {0.18f, 1.f}, {0.5f, 1.f}, {0.68f, 0.86f} };
				for (const auto& P : Pts)
				{
					const float A = Base + P[0] * Step;
					Out.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)) * R * P[1]);
				}
			}
			break;
		}
		default:
		{
			const int32 N = 72;
			for (int32 i = 0; i < N; ++i)
			{
				const float A = 2.f * PI * i / N;
				Out.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
			}
			break;
		}
		}
	}
}

void FFortuneCoinShapeBuilder::Build(UProceduralMeshComponent* PMC, EFortuneCoinShape Shape, float Diameter, float Thickness)
{
	if (!PMC) return;
	PMC->ClearAllMeshSections();

	const float R = Diameter * 0.5f;
	const float H = Thickness * 0.5f;
	TArray<FVector2D> Pts;
	bool bFlat = false;
	Outline(Shape, R, Pts, bFlat);
	const int32 N = Pts.Num();

	// --- Yuzler (merkezden yelpaze). UV: duzlemsel, coin capi = 0..1
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const bool bTop = (Side == 0);
		const float Z = bTop ? H : -H;
		TArray<FVector> V; TArray<int32> Tri; TArray<FVector> Nrm; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
		V.Add(FVector(0.f, 0.f, Z));
		UV.Add(FVector2D(0.5f, 0.5f));
		for (const FVector2D& P : Pts)
		{
			V.Add(FVector(P.X, P.Y, Z));
			// Tura yuzunde yatay ayna: coin cevrilince resim duz okunsun
			const float U = bTop ? 0.5f + P.Y / (2.f * R) : 0.5f - P.Y / (2.f * R);
			UV.Add(FVector2D(U, 0.5f - P.X / (2.f * R)));
		}
		for (int32 i = 0; i < V.Num(); ++i)
		{
			Nrm.Add(FVector(0.f, 0.f, bTop ? 1.f : -1.f));
			Tan.Add(FProcMeshTangent(0.f, bTop ? 1.f : -1.f, 0.f));
		}
		for (int32 i = 0; i < N; ++i)
		{
			const int32 A = 1 + i, B = 1 + (i + 1) % N;
			// UE: bakan kisiye gore saat yonu = on yuz
			if (bTop) { Tri.Add(0); Tri.Add(A); Tri.Add(B); }
			else      { Tri.Add(0); Tri.Add(B); Tri.Add(A); }
		}
		PMC->CreateMeshSection(Side, V, Tri, Nrm, UV, TArray<FColor>(), Tan, false);
	}

	// --- Kenar
	{
		TArray<FVector> V; TArray<int32> Tri; TArray<FVector> Nrm; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D& P0 = Pts[i];
			const FVector2D& P1 = Pts[(i + 1) % N];
			FVector N0, N1;
			if (bFlat)
			{
				const FVector2D E = (P1 - P0).GetSafeNormal();
				N0 = N1 = FVector(E.Y, -E.X, 0.f);
			}
			else
			{
				N0 = FVector(P0.GetSafeNormal(), 0.f);
				N1 = FVector(P1.GetSafeNormal(), 0.f);
			}
			// Disa bakan normal kontrolu
			if (FVector::DotProduct(N0, FVector(P0, 0.f)) < 0.f) { N0 = -N0; }
			if (FVector::DotProduct(N1, FVector(P1, 0.f)) < 0.f) { N1 = -N1; }

			const int32 Base = V.Num();
			V.Add(FVector(P0.X, P0.Y, H));  V.Add(FVector(P0.X, P0.Y, -H));
			V.Add(FVector(P1.X, P1.Y, H));  V.Add(FVector(P1.X, P1.Y, -H));
			Nrm.Add(N0); Nrm.Add(N0); Nrm.Add(N1); Nrm.Add(N1);
			const float U0 = (float)i / N, U1 = (float)(i + 1) / N;
			UV.Add(FVector2D(U0 * 8.f, 0.f)); UV.Add(FVector2D(U0 * 8.f, 1.f));
			UV.Add(FVector2D(U1 * 8.f, 0.f)); UV.Add(FVector2D(U1 * 8.f, 1.f));
			for (int32 k = 0; k < 4; ++k) Tan.Add(FProcMeshTangent(0.f, 0.f, 1.f));
			Tri.Add(Base + 0); Tri.Add(Base + 1); Tri.Add(Base + 2);
			Tri.Add(Base + 2); Tri.Add(Base + 1); Tri.Add(Base + 3);
		}
		PMC->CreateMeshSection(2, V, Tri, Nrm, UV, TArray<FColor>(), Tan, false);
	}
}

bool FFortuneCoinShapeBuilder::BuildForCoin(UProceduralMeshComponent* PMC, const UFortuneCoinData* Coin, bool bOwned, bool bEnchanted,
	float Diameter, float Thickness)
{
	if (!PMC || !Coin || !Coin->HeadsTexture) return false;

	Build(PMC, Coin->Shape, Diameter, Thickness);

	// M_CoinFace: Face (doku) x Tint -> renk, Glow -> parlama (Python betigi olusturur)
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_CoinFace.M_CoinFace"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	UTexture2D* White = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"), nullptr, LOAD_NoWarn | LOAD_Quiet);

	const FLinearColor Tint = bOwned ? FLinearColor::White : FLinearColor(0.015f, 0.015f, 0.02f);
	const float Glow = bOwned ? (bEnchanted ? 0.35f : 0.f) : 0.f;

	auto Apply = [&](int32 Section, UTexture2D* Tex, const FLinearColor& Color)
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, PMC))
		{
			if (Tex) MID->SetTextureParameterValue(TEXT("Face"), Tex);
			MID->SetVectorParameterValue(TEXT("Tint"), Color);
			MID->SetVectorParameterValue(TEXT("Color"), Color);   // yedek materyal icin
			MID->SetScalarParameterValue(TEXT("Glow"), Glow);
			PMC->SetMaterial(Section, MID);
		}
	};
	Apply(0, Coin->HeadsTexture, Tint);
	Apply(1, Coin->TailsTexture ? Coin->TailsTexture.Get() : Coin->HeadsTexture.Get(), Tint);
	Apply(2, White, bOwned ? Coin->RimColor : Tint);
	return true;
}
