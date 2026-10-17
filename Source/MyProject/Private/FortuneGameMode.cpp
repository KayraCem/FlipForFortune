#include "FortuneGameMode.h"
#include "FortunePlayerController.h"

AFortuneGameMode::AFortuneGameMode()
{
	PlayerControllerClass = AFortunePlayerController::StaticClass();
	DefaultPawnClass = nullptr;   // Yurunecek karakter yok, sabit kamera
}
