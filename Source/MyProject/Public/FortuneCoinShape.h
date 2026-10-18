// Flip for Fortune - Coin geometrisi: sekle gore (yuvarlak, sekizgen, dilimli, disli...) kalinlikli coin uretir.
// Yazi yuzu (ust), tura yuzu (alt) ve kenar ayri bolumlerdir; yuzlere doku (resim) dogru oturur.
#pragma once

#include "CoreMinimal.h"
#include "FortuneTypes.h"

class UProceduralMeshComponent;
class UFortuneCoinData;

struct MYPROJECT_API FFortuneCoinShapeBuilder
{
	/** Coin'i PMC icine kurar. Diameter/Thickness cm. Bolumler: 0 = yazi, 1 = tura, 2 = kenar. */
	static void Build(UProceduralMeshComponent* PMC, EFortuneCoinShape Shape, float Diameter, float Thickness);

	/** Coin'in seklini kurar ve yuz resimlerini/kenar rengini uygular. Coin'in yuz dokusu yoksa false doner. */
	static bool BuildForCoin(UProceduralMeshComponent* PMC, const UFortuneCoinData* Coin, bool bOwned, bool bEnchanted,
		float Diameter = 6.f, float Thickness = 0.6f);
};
