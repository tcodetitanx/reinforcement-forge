#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ForgePlayerController.generated.h"

UCLASS()
class BRAINFORGE_API AForgePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AForgePlayerController();

	virtual void BeginPlay() override;
};
