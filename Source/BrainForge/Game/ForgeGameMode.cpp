#include "ForgeGameMode.h"
#include "ForgePlayerController.h"
#include "ForgeHUD.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/SpectatorPawn.h"

AForgeGameMode::AForgeGameMode()
{
	PlayerControllerClass = AForgePlayerController::StaticClass();
	HUDClass = AForgeHUD::StaticClass();
	DefaultPawnClass = ASpectatorPawn::StaticClass();
}
