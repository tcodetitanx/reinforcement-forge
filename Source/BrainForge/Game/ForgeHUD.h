#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ForgeGameInstance.h"
#include "ForgeHUD.generated.h"

class SBrainEditor;
class SOverlay;

UCLASS()
class BRAINFORGE_API AForgeHUD : public AHUD
{
	GENERATED_BODY()

public:
	AForgeHUD();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	TSharedPtr<SOverlay> Root;
	TSharedPtr<SWidget> CurrentWidget;
	TSharedPtr<SBrainEditor> EditorWidget;
	EForgeScreen BuiltScreen = EForgeScreen::MainMenu;
	float AutosaveTimer = 0.f;
	int32 LastSavedGeneration = 1;
	int32 NextHintIndex = 0;

	UForgeGameInstance* GI() const;
	void RebuildScreen();
	void HandleNavigate(int32 Screen);
};
