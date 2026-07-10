#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Fonts/SlateFontInfo.h"

class UTexture2D;

/**
 * Central palette + brush/texture cache for the neon UI.
 * All textures are either loaded from keyed sprite crops (Content/RawAssets/Sprites)
 * or generated procedurally at startup (glows, rings, discs).
 */
struct BRAINFORGE_API FForgeStyle
{
	static FForgeStyle& Get();
	void Initialize();
	bool IsInitialized() const { return bInitialized; }

	// ---------- palette (sampled from the target screenshot) ----------
	static FLinearColor Background()      { return FLinearColor(0.012f, 0.022f, 0.045f); }
	static FLinearColor PanelBg()         { return FLinearColor(0.020f, 0.042f, 0.075f, 0.94f); }
	static FLinearColor PanelBgDark()     { return FLinearColor(0.010f, 0.024f, 0.048f, 0.97f); }
	static FLinearColor PanelBorder()     { return FLinearColor(0.10f, 0.24f, 0.35f); }
	static FLinearColor GridLine()        { return FLinearColor(0.05f, 0.10f, 0.16f, 0.5f); }
	static FLinearColor TextBright()      { return FLinearColor(0.80f, 0.93f, 1.0f); }
	static FLinearColor TextDim()         { return FLinearColor(0.36f, 0.50f, 0.62f); }
	static FLinearColor Cyan()            { return FLinearColor(0.20f, 0.88f, 1.0f); }
	static FLinearColor Green()           { return FLinearColor(0.45f, 1.0f, 0.30f); }
	static FLinearColor SoftGreen()       { return FLinearColor(0.25f, 1.0f, 0.55f); }
	static FLinearColor Gold()            { return FLinearColor(1.0f, 0.78f, 0.22f); }
	static FLinearColor Purple()          { return FLinearColor(0.70f, 0.40f, 1.0f); }
	static FLinearColor Red()             { return FLinearColor(1.0f, 0.28f, 0.34f); }
	static FLinearColor Teal()            { return FLinearColor(0.16f, 1.0f, 0.83f); }
	static FLinearColor Blue()            { return FLinearColor(0.24f, 0.55f, 1.0f); }
	static FLinearColor Orange()          { return FLinearColor(1.0f, 0.55f, 0.15f); }

	static FLinearColor NodeTypeColor(uint8 NodeType);
	static FLinearColor ConnTypeColor(uint8 ConnType);

	// ---------- fonts ----------
	FSlateFontInfo Font(int32 Size, bool bBold = false) const;
	FSlateFontInfo FontLight(int32 Size) const;

	// ---------- brushes ----------
	FSlateBrush PanelBrush;          // rounded dark panel
	FSlateBrush PanelBrushDark;
	FSlateBrush ThinBorderBrush;     // 1px border only
	FSlateBrush WhiteBrush;          // solid white (tint at draw time)

	// procedural textures
	FSlateBrush GlowBrush;           // soft radial glow
	FSlateBrush DiscBrush;           // solid antialiased disc
	FSlateBrush RingBrush;           // thin ring
	FSlateBrush RingThickBrush;
	FSlateBrush HexBrush;            // filled hexagon
	FSlateBrush HexOutlineBrush;

	// sprite crops
	FSlateBrush NodeIcon[9];         // indexed by ENodeType
	FSlateBrush BadgeLocked;
	FSlateBrush BadgeUnstable;
	FSlateBrush BadgeDamaged;
	FSlateBrush Logo;
	FVector2D LogoSize = FVector2D(370, 120);

	bool bHasSprites = false;

private:
	bool bInitialized = false;
	TArray<UTexture2D*> OwnedTextures;

	UTexture2D* MakeGlowTexture(int32 Size, float Exponent);
	UTexture2D* MakeDiscTexture(int32 Size);
	UTexture2D* MakeRingTexture(int32 Size, float Thickness01);
	UTexture2D* MakeHexTexture(int32 Size, bool bOutline);
	UTexture2D* LoadPng(const FString& Path);
	static FSlateBrush BrushFromTexture(UTexture2D* Tex, const FVector2D& Size);
};
