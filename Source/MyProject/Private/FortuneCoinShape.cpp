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
		case EFortuneCoinShape::Dodecagon: Poly(12, PI / 12.f); break;
		case EFortuneCoinShape::Pumpkin:
		case EFortuneCoinShape::Skull:
		{
			const int32 N = 200;
			for (int32 i = 0; i < N; ++i)
			{
				const float A = 2.f * PI * i / N;   // 0 = ust (+X)
				const float Ca = FMath::Cos(A), Sa = FMath::Sin(A);
				float Rr;
				if (Shape == EFortuneCoinShape::Pumpkin)
				{
					// genis elips + dilim tumsekleri + ustte sap cukuru
					Rr = 1.f / FMath::Sqrt(FMath::Square(Ca / 0.84f) + FMath::Square(Sa / 1.f));
					Rr *= 1.f - 0.05f * (1.f - FMath::Abs(FMath::Cos(5.f * A)));
					const float D0 = A / 0.42f, D1 = (A - 2.f * PI) / 0.42f;
					Rr *= 1.f - 0.11f * FMath::Exp(-D0 * D0) - 0.11f * FMath::Exp(-D1 * D1);
				}
				else
				{
					// kafatasi: dikey elips kafa, elmacik altinda iceri kivrim, duz cene
					Rr = 1.f / FMath::Sqrt(FMath::Square(Ca / 1.f) + FMath::Square(Sa / 0.86f));
					const float Ang = FMath::Abs(FMath::Atan2(Sa, Ca));   // 0..PI (ustten)
					const float Notch = (Ang - 2.3f) / 0.2f;
					Rr *= 1.f - 0.16f * FMath::Exp(-Notch * Notch);
					if (Ca < -0.05f)
					{
						Rr = FMath::Min(Rr, 0.9f / -Ca);   // duz alt cene
						if (Ang > 2.45f) Rr = FMath::Min(Rr, 0.62f / FMath::Max(FMath::Abs(Sa), 0.05f));   // dar cene
					}
				}
				Out.Add(FVector2D(Ca, Sa) * R * Rr);
			}
			break;
		}
		case EFortuneCoinShape::Star:
		case EFortuneCoinShape::Clover:
		case EFortuneCoinShape::Ghost:
		{
			const int32 N = 160;
			for (int32 i = 0; i < N; ++i)
			{
				const float A = 2.f * PI * i / N;
				float Rr = R;
				if (Shape == EFortuneCoinShape::Star)
				{
					// yuvarlak uclu 5 koseli yildiz, bir ucu yukari (+X)
					Rr = R * (0.6f + 0.4f * FMath::Pow(FMath::Abs(FMath::Cos(2.5f * A)), 2.2f));
				}
				else if (Shape == EFortuneCoinShape::Clover)
				{
					Rr = R * (0.66f + 0.34f * FMath::Pow(FMath::Abs(FMath::Cos(2.f * (A - PI / 4.f))), 0.55f));
				}
				else
				{
					// ust (+X) yuvarlak, alt (-X) dalgali etek
					const float Down = FMath::Max(0.f, -FMath::Cos(A) - 0.15f) / 0.85f;
					Rr = R * (1.f - 0.13f * Down * (0.5f + 0.5f * FMath::Sin(A * 9.f)));
				}
				Out.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rr);
			}
			break;
		}
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
		case EFortuneCoinShape::GearRing:
		{
			bFlat = true;
			const int32 Teeth = Shape == EFortuneCoinShape::GearRing ? 8 : 14;
			for (int32 t = 0; t < Teeth; ++t)
			{
				const float Base = 2.f * PI * t / Teeth;
				const float Step = 2.f * PI / Teeth;
				const float Lo = Shape == EFortuneCoinShape::GearRing ? 0.8f : 0.86f;
				const float Pts[4][2] = { {0.f, Lo}, {0.14f, 1.f}, {0.42f, 1.f}, {0.56f, Lo} };
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

// Saturn: kure (ust yari = yazi, alt yari = tura, doku ustten izdusum) + egik halka (bolum 2 = kenar materyali)
static void FortuneShape_BuildSaturn(UProceduralMeshComponent* PMC, float R)
{
	const float Rp = R * 0.66f;
	const int32 Slices = 48, Stacks = 14;
	auto AddTri = [](TArray<int32>& Tri, const TArray<FVector>& V, const TArray<FVector>& N, int32 A, int32 B, int32 C)
	{
		// UE on yuz kurali: cross(B-A, C-A) normalin tersine baksin
		const FVector Cr = FVector::CrossProduct(V[B] - V[A], V[C] - V[A]);
		if (FVector::DotProduct(Cr, N[A] + N[B] + N[C]) > 0.f) Swap(B, C);
		Tri.Add(A); Tri.Add(B); Tri.Add(C);
	};
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sgn = Side == 0 ? 1.f : -1.f;
		TArray<FVector> V; TArray<int32> Tri; TArray<FVector> Nrm; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
		for (int32 i = 0; i <= Stacks; ++i)
		{
			const float Phi = (PI * 0.5f) * i / Stacks;          // kutuptan ekvatora
			for (int32 j = 0; j <= Slices; ++j)
			{
				const float Th = 2.f * PI * j / Slices;
				const FVector D(FMath::Sin(Phi) * FMath::Cos(Th), FMath::Sin(Phi) * FMath::Sin(Th), Sgn * FMath::Cos(Phi));
				const FVector P = D * Rp;
				V.Add(P); Nrm.Add(D);
				const float U = Side == 0 ? 0.5f + P.Y / (2.f * Rp) : 0.5f - P.Y / (2.f * Rp);
				UV.Add(FVector2D(U, 0.5f - P.X / (2.f * Rp)));
				Tan.Add(FProcMeshTangent(0.f, Sgn, 0.f));
			}
		}
		for (int32 i = 0; i < Stacks; ++i)
		{
			for (int32 j = 0; j < Slices; ++j)
			{
				const int32 A = i * (Slices + 1) + j, B = A + 1, C = A + Slices + 1, D = C + 1;
				AddTri(Tri, V, Nrm, A, C, B);
				AddTri(Tri, V, Nrm, B, C, D);
			}
		}
		PMC->CreateMeshSection(Side, V, Tri, Nrm, UV, TArray<FColor>(), Tan, false);
	}
	// Halka: coinin ustu (+X) ekseni etrafinda 72 derece egik, iki yuzlu
	{
		TArray<FVector> V; TArray<int32> Tri; TArray<FVector> Nrm; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tan;
		const FQuat Tilt(FVector::ForwardVector, FMath::DegreesToRadians(72.f));
		const float R0 = R * 0.86f, R1 = R * 1.22f;
		const int32 Seg = 72;
		for (int32 Face = 0; Face < 2; ++Face)
		{
			const FVector N = Tilt.RotateVector(FVector(0.f, 0.f, Face == 0 ? 1.f : -1.f));
			const int32 B0 = V.Num();
			for (int32 j = 0; j <= Seg; ++j)
			{
				const float Th = 2.f * PI * j / Seg;
				const FVector Dir(FMath::Cos(Th), FMath::Sin(Th), 0.f);
				for (int32 k = 0; k < 2; ++k)
				{
					const float Rr = k == 0 ? R0 : R1;
					V.Add(Tilt.RotateVector(Dir * Rr + FVector(0.f, 0.f, Face == 0 ? 0.03f : -0.03f)));
					Nrm.Add(N);
					UV.Add(FVector2D(k * 6.f, j / float(Seg)));   // kenar normal dokusu: es merkezli halka cizgileri
					Tan.Add(FProcMeshTangent(Tilt.RotateVector(Dir), false));
				}
			}
			for (int32 j = 0; j < Seg; ++j)
			{
				const int32 A = B0 + j * 2, B = A + 1, C = A + 2, D = A + 3;
				AddTri(Tri, V, Nrm, A, C, B);
				AddTri(Tri, V, Nrm, B, C, D);
			}
		}
		PMC->CreateMeshSection(2, V, Tri, Nrm, UV, TArray<FColor>(), Tan, false);
	}
}

void FFortuneCoinShapeBuilder::Build(UProceduralMeshComponent* PMC, EFortuneCoinShape Shape, float Diameter, float Thickness)
{
	if (!PMC) return;
	PMC->ClearAllMeshSections();
	if (Shape == EFortuneCoinShape::Saturn)
	{
		FortuneShape_BuildSaturn(PMC, Diameter * 0.5f);
		return;
	}

	const float R = Diameter * 0.5f;
	const float H = Thickness * 0.5f;
	TArray<FVector2D> Pts;
	bool bFlat = false;
	Outline(Shape, R, Pts, bFlat);
	const int32 N = Pts.Num();
	const float HoleR = (Shape == EFortuneCoinShape::GearRing) ? R * 0.3f : 0.f;

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

		// Delikli sekil: ic alan halka seridi (delige kadar)
		if (HoleR > 0.f)
		{
			const FRing& L = Prof[NR - 1];
			const int32 C0 = V.Num();
			for (int32 i = 0; i < N; ++i)
			{
				const FVector Po(Pts[i] * L.F, Sgn * (H + L.Z));
				const FVector Pi(Pts[i].GetSafeNormal() * HoleR, Sgn * (H + L.Z));
				V.Add(Po); Nrm.Add(FVector(0.f, 0.f, Sgn)); UV.Add(UVof(Po, bTop)); Tan.Add(T);
				V.Add(Pi); Nrm.Add(FVector(0.f, 0.f, Sgn)); UV.Add(UVof(Pi, bTop)); Tan.Add(T);
			}
			for (int32 i = 0; i < N; ++i)
			{
				const int32 j = (i + 1) % N;
				const int32 O0 = C0 + 2 * i, I0 = O0 + 1, O1 = C0 + 2 * j, I1 = O1 + 1;
				if (bTop) { Tri.Add(I0); Tri.Add(O1); Tri.Add(O0); Tri.Add(I0); Tri.Add(I1); Tri.Add(O1); }
				else      { Tri.Add(I0); Tri.Add(O0); Tri.Add(O1); Tri.Add(I0); Tri.Add(O1); Tri.Add(I1); }
			}
			PMC->CreateMeshSection(Side, V, Tri, Nrm, UV, TArray<FColor>(), Tan, false);
			continue;
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
		// Delik ic duvari (iceri bakan)
		if (HoleR > 0.f)
		{
			const float HD = H - Thickness * 0.14f;
			const int32 M = 48;
			for (int32 i = 0; i < M; ++i)
			{
				const float A0 = 2.f * PI * i / M, A1 = 2.f * PI * (i + 1) / M;
				const FVector2D D0(FMath::Cos(A0), FMath::Sin(A0)), D1(FMath::Cos(A1), FMath::Sin(A1));
				const int32 Base = V.Num();
				V.Add(FVector(D0 * HoleR, HD)); V.Add(FVector(D0 * HoleR, -HD));
				V.Add(FVector(D1 * HoleR, HD)); V.Add(FVector(D1 * HoleR, -HD));
				Nrm.Add(FVector(-D0, 0.f)); Nrm.Add(FVector(-D0, 0.f)); Nrm.Add(FVector(-D1, 0.f)); Nrm.Add(FVector(-D1, 0.f));
				UV.Add(FVector2D(i * 0.1f, 0.f)); UV.Add(FVector2D(i * 0.1f, 1.f)); UV.Add(FVector2D((i + 1) * 0.1f, 0.f)); UV.Add(FVector2D((i + 1) * 0.1f, 1.f));
				for (int32 k = 0; k < 4; ++k) Tan.Add(FProcMeshTangent(FVector(-D0.Y, D0.X, 0.f), false));
				// ic duvar: ters sarim
				Tri.Add(Base + 0); Tri.Add(Base + 1); Tri.Add(Base + 2);
				Tri.Add(Base + 2); Tri.Add(Base + 1); Tri.Add(Base + 3);
			}
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
	// M_CoinFace3 = M_CoinFace2 + neon parlama dokusu (FaceGlow x GlowColor x GlowStrength, nabiz gibi atar)
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_CoinFace3.M_CoinFace3"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FlipForFortune/Materials/M_CoinFace2.M_CoinFace2"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
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

	UTexture2D* Black = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/Black.Black"), nullptr, LOAD_NoWarn | LOAD_Quiet);

	auto Apply = [&](int32 Section, UTexture2D* Tex, const FLinearColor& Color, UTexture2D* NormalTex = nullptr, UTexture2D* GlowTex = nullptr)
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, PMC))
		{
			if (Tex) MID->SetTextureParameterValue(TEXT("Face"), Tex);
			if (NormalTex) MID->SetTextureParameterValue(TEXT("FaceNormal"), NormalTex);
			const bool bGlow = GlowTex && bOwned && Coin->GlowStrength > 0.f;
			if (bGlow) MID->SetTextureParameterValue(TEXT("FaceGlow"), GlowTex);
			else if (Black) MID->SetTextureParameterValue(TEXT("FaceGlow"), Black);
			MID->SetScalarParameterValue(TEXT("GlowStrength"), bGlow ? Coin->GlowStrength : 0.f);
			MID->SetVectorParameterValue(TEXT("GlowColor"), Coin->GlowColor);
			MID->SetVectorParameterValue(TEXT("Tint"), Color);
			MID->SetVectorParameterValue(TEXT("Color"), Color);   // yedek materyal icin
			MID->SetScalarParameterValue(TEXT("Glow"), Glow);
			PMC->SetMaterial(Section, MID);
		}
	};
	Apply(0, Coin->HeadsTexture, Tint, Coin->HeadsNormal, Coin->HeadsGlow);
	Apply(1, Coin->TailsTexture ? Coin->TailsTexture.Get() : Coin->HeadsTexture.Get(), Tint,
		Coin->TailsNormal ? Coin->TailsNormal.Get() : Coin->HeadsNormal.Get(),
		Coin->TailsGlow ? Coin->TailsGlow.Get() : Coin->HeadsGlow.Get());
	Apply(2, White, bOwned ? Coin->RimColor : Tint, RimN);
	return true;
}
