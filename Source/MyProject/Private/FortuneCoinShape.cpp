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

	// Kenar disa dogru normaller (duz kenarli sekillerde kenar basina, yuvarlakta nokta basina)
	auto EdgeNormal = [&](int32 i, int32 j) -> FVector2D
	{
		const FVector2D E = (Pts[j] - Pts[i]).GetSafeNormal();
		FVector2D Nn(E.Y, -E.X);
		if (FVector2D::DotProduct(Nn, Pts[i]) < 0.f) Nn = -Nn;
		return Nn;
	};

	// Yuz profili (disaridan iceri): pah -> kalkik dudak -> basamak -> cukur ic alan
	const float Bevel = Thickness * 0.12f;
	const float Dip = Thickness * 0.14f;
	struct FRing { float F; float Z; };
	const FRing Prof[] = { {1.f, -Bevel}, {0.965f, 0.f}, {0.885f, 0.f}, {0.86f, -Dip} };
	const int32 NR = UE_ARRAY_COUNT(Prof);

	auto UVof = [&](const FVector& P, bool bTop)
	{
		const float U = bTop ? 0.5f + P.Y / (2.f * R) : 0.5f - P.Y / (2.f * R);
		return FVector2D(U, 0.5f - P.X / (2.f * R));
	};

	for (int32 Side = 0; Side < 2; ++Side)
	{
		const bool bTop = (Side == 0);
		const float Sgn = bTop ? 1.f : -1.f;
		TArray<FVector> V; TArray<int32> Tri; TArray<FVector> Nrm; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
		const FProcMeshTangent T(0.f, bTop ? 1.f : -1.f, 0.f);

		// Halkalar arasi seritler
		for (int32 k = 0; k + 1 < NR; ++k)
		{
			const FRing& O = Prof[k];
			const FRing& I = Prof[k + 1];
			for (int32 i = 0; i < N; ++i)
			{
				const int32 j = (i + 1) % N;
				const FVector2D Ri = bFlat ? EdgeNormal(i, j) : Pts[i].GetSafeNormal();
				const FVector2D Rj = bFlat ? EdgeNormal(i, j) : Pts[j].GetSafeNormal();
				const float DR = (O.F - I.F) * R;
				const float DZ = (I.Z - O.Z);
				const FVector2D SL = FVector2D(DZ, DR).GetSafeNormal();   // (radyal, dikey) egim
				const FVector Ni = FVector(Ri * SL.X, Sgn * SL.Y).GetSafeNormal();
				const FVector Nj = FVector(Rj * SL.X, Sgn * SL.Y).GetSafeNormal();

				const FVector Oi(Pts[i] * O.F, Sgn * (H + O.Z));
				const FVector Oj(Pts[j] * O.F, Sgn * (H + O.Z));
				const FVector Ii(Pts[i] * I.F, Sgn * (H + I.Z));
				const FVector Ij(Pts[j] * I.F, Sgn * (H + I.Z));
				const int32 B0 = V.Num();
				V.Add(Oi); V.Add(Oj); V.Add(Ii); V.Add(Ij);
				Nrm.Add(Ni); Nrm.Add(Nj); Nrm.Add(Ni); Nrm.Add(Nj);
				UV.Add(UVof(Oi, bTop)); UV.Add(UVof(Oj, bTop)); UV.Add(UVof(Ii, bTop)); UV.Add(UVof(Ij, bTop));
				for (int32 q = 0; q < 4; ++q) Tan.Add(T);
				// (ic, sonraki dis, dis) ust yuzde on yuz
				if (bTop)
				{
					Tri.Add(B0 + 2); Tri.Add(B0 + 1); Tri.Add(B0 + 0);
					Tri.Add(B0 + 2); Tri.Add(B0 + 3); Tri.Add(B0 + 1);
				}
				else
				{
					Tri.Add(B0 + 2); Tri.Add(B0 + 0); Tri.Add(B0 + 1);
					Tri.Add(B0 + 2); Tri.Add(B0 + 1); Tri.Add(B0 + 3);
				}
			}
		}

		// Cukur ic alan (merkezden yelpaze)
		{
			const FRing& L = Prof[NR - 1];
			const int32 C0 = V.Num();
			const FVector Cen(0.f, 0.f, Sgn * (H + L.Z));
			V.Add(Cen); Nrm.Add(FVector(0.f, 0.f, Sgn)); UV.Add(UVof(Cen, bTop)); Tan.Add(T);
			for (int32 i = 0; i < N; ++i)
			{
				const FVector P(Pts[i] * L.F, Sgn * (H + L.Z));
				V.Add(P); Nrm.Add(FVector(0.f, 0.f, Sgn)); UV.Add(UVof(P, bTop)); Tan.Add(T);
			}
			for (int32 i = 0; i < N; ++i)
			{
				const int32 A = C0 + 1 + i, B = C0 + 1 + (i + 1) % N;
				if (bTop) { Tri.Add(C0); Tri.Add(B); Tri.Add(A); }
				else      { Tri.Add(C0); Tri.Add(A); Tri.Add(B); }
			}
		}
		PMC->CreateMeshSection(Side, V, Tri, Nrm, UV, TArray<FColor>(), Tan, false);
	}

	// --- Kenar (pahlar arasinda)
	{
		const float HZ = H - Bevel;
		TArray<FVector> V; TArray<int32> Tri; TArray<FVector> Nrm; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
		for (int32 i = 0; i < N; ++i)
		{
			const int32 j = (i + 1) % N;
			const FVector2D& P0 = Pts[i];
			const FVector2D& P1 = Pts[j];
			const FVector2D A0 = bFlat ? EdgeNormal(i, j) : P0.GetSafeNormal();
			const FVector2D A1 = bFlat ? EdgeNormal(i, j) : P1.GetSafeNormal();
			const FVector N0(A0, 0.f), N1(A1, 0.f);

			const int32 Base = V.Num();
			V.Add(FVector(P0.X, P0.Y, HZ));  V.Add(FVector(P0.X, P0.Y, -HZ));
			V.Add(FVector(P1.X, P1.Y, HZ));  V.Add(FVector(P1.X, P1.Y, -HZ));
			Nrm.Add(N0); Nrm.Add(N0); Nrm.Add(N1); Nrm.Add(N1);
			const float U0 = (float)i / N, U1 = (float)(i + 1) / N;
			UV.Add(FVector2D(U0 * 8.f, 0.f)); UV.Add(FVector2D(U0 * 8.f, 1.f));
			UV.Add(FVector2D(U1 * 8.f, 0.f)); UV.Add(FVector2D(U1 * 8.f, 1.f));
			const FVector Tg = FVector(P1 - P0, 0.f).GetSafeNormal();
			for (int32 k = 0; k < 4; ++k) Tan.Add(FProcMeshTangent(Tg, false));
			Tri.Add(Base + 0); Tri.Add(Base + 2); Tri.Add(Base + 1);
			Tri.Add(Base + 2); Tri.Add(Base + 3); Tri.Add(Base + 1);
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
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_CoinFace2.M_CoinFace2"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_CoinFace.M_CoinFace"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	UTexture2D* RimN = LoadObject<UTexture2D>(nullptr, TEXT("/Game/FlipForFortune/Coins/Textures/T_Coin_RimNormal.T_Coin_RimNormal"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	UTexture2D* White = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"), nullptr, LOAD_NoWarn | LOAD_Quiet);

	const FLinearColor Tint = bOwned ? FLinearColor::White : FLinearColor(0.015f, 0.015f, 0.02f);
	const float Glow = bOwned ? (bEnchanted ? 0.35f : 0.f) : 0.f;

	auto Apply = [&](int32 Section, UTexture2D* Tex, const FLinearColor& Color, UTexture2D* NormalTex = nullptr)
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, PMC))
		{
			if (Tex) MID->SetTextureParameterValue(TEXT("Face"), Tex);
			if (NormalTex) MID->SetTextureParameterValue(TEXT("FaceNormal"), NormalTex);
			MID->SetVectorParameterValue(TEXT("Tint"), Color);
			MID->SetVectorParameterValue(TEXT("Color"), Color);   // yedek materyal icin
			MID->SetScalarParameterValue(TEXT("Glow"), Glow);
			PMC->SetMaterial(Section, MID);
		}
	};
	Apply(0, Coin->HeadsTexture, Tint, Coin->HeadsNormal);
	Apply(1, Coin->TailsTexture ? Coin->TailsTexture.Get() : Coin->HeadsTexture.Get(), Tint,
		Coin->TailsNormal ? Coin->TailsNormal.Get() : Coin->HeadsNormal.Get());
	Apply(2, White, bOwned ? Coin->RimColor : Tint, RimN);
	return true;
}
