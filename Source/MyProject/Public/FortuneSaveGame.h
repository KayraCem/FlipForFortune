#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FortuneTypes.h"
#include "FortuneSaveGame.generated.h"

UCLASS()
class MYPROJECT_API UFortuneSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	double Money = 0.0;

	UPROPERTY(SaveGame)
	TArray<FFortuneOwnedCoin> OwnedCoins;

	UPROPERTY(SaveGame)
	FName EquippedCoin;

	UPROPERTY(SaveGame)
	TMap<EFortuneUpgradeType, int32> UpgradeLevels;

	UPROPERTY(SaveGame)
	TMap<EFortuneItemType, int32> Items;

	UPROPERTY(SaveGame)
	int64 TotalFlips = 0;

	/** Elle yapilan atislar (AutoFlip kilidi icin). */
	UPROPERTY(SaveGame)
	int64 ManualFlips = 0;

	UPROPERTY(SaveGame)
	bool bAutoFlipUnlocked = false;

	UPROPERTY(SaveGame)
	bool bAutoFlipEnabled = false;
};
