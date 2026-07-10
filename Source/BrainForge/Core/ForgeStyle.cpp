#include "ForgeStyle.h"
#include "ForgeTypes.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Misc/CoreDelegates.h"

FForgeStyle& FForgeStyle::Get()
{
	static FForgeStyle Instance;
	return Instance;
}

FLinearColor FForgeStyle::NodeTypeColor(uint8 NodeType)
{
	switch ((ENodeType)NodeType)
	{
	case ENodeType::Input:      return Green();
	case ENodeType::Relay:      return Blue();
	case ENodeType::Threshold:  return Purple();
	case ENodeType::Memory:     return Orange();
	case ENodeType::Oscillator: return Cyan();
	case ENodeType::Inhibitor:  return Red();
	case ENodeType::Reward:     return Gold();
	case ENodeType::Output:     return Teal();
	default:                    return TextDim();
	}
}

FLinearColor FForgeStyle::ConnTypeColor(uint8 ConnType)
{
	switch ((EConnType)ConnType)
	{
	case EConnType::Excitatory: return Cyan();
	case EConnType::Inhibitory: return Purple();
	case EConnType::Modulatory: return Gold();
	case EConnType::Memory:     return Orange();
	case EConnType::Unstable:   return Red();
	case EConnType::Delayed:    return Blue();
	default:                    return TextDim();
	}
}

FSlateFontInfo FForgeStyle::Font(int32 Size, bool bBold) const
{
	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
}

FSlateFontInfo FForgeStyle::FontLight(int32 Size) const
{
	return FCoreStyle::GetDefaultFontStyle("Light", Size);
}

FSlateBrush FForgeStyle::BrushFromTexture(UTexture2D* Tex, const FVector2D& Size)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(Tex);
	Brush.ImageSize = Size;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	return Brush;
}

void FForgeStyle::Shutdown()
{
	if (!bInitialized) { return; }

	// tell Slate to drop its render proxies while the textures are still alive
	if (FSlateApplication::IsInitialized())
	{
		if (FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer())
		{
			auto Release = [Renderer](FSlateBrush& Brush)
			{
				if (Brush.GetResourceObject())
				{
					Renderer->ReleaseDynamicResource(Brush);
					Brush.SetResourceObject(nullptr);
				}
			};
			Release(GlowBrush);
			Release(DiscBrush);
			Release(RingBrush);
			Release(RingThickBrush);
			Release(HexBrush);
			Release(HexOutlineBrush);
			for (int32 i = 0; i < 9; ++i) { Release(NodeIcon[i]); }
			Release(BadgeLocked);
			Release(BadgeUnstable);
			Release(BadgeDamaged);
			Release(Logo);
		}
	}

	for (UTexture2D* Tex : OwnedTextures)
	{
		if (Tex) { Tex->RemoveFromRoot(); }
	}
	OwnedTextures.Empty();
	bInitialized = false;
}

void FForgeStyle::Initialize()
{
	if (bInitialized) { return; }
	bInitialized = true;

	FCoreDelegates::OnPreExit.AddRaw(this, &FForgeStyle::Shutdown);

	// ---- simple brushes
	WhiteBrush = FSlateBrush();
	WhiteBrush.DrawAs = ESlateBrushDrawType::Image;
	WhiteBrush.TintColor = FLinearColor::White;

	PanelBrush = FSlateBrush();
	PanelBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	PanelBrush.OutlineSettings = FSlateBrushOutlineSettings(6.f, PanelBorder(), 1.2f);
	PanelBrush.TintColor = PanelBg();

	PanelBrushDark = PanelBrush;
	PanelBrushDark.TintColor = PanelBgDark();

	ThinBorderBrush = FSlateBrush();
	ThinBorderBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	ThinBorderBrush.TintColor = FLinearColor(0, 0, 0, 0);
	ThinBorderBrush.OutlineSettings = FSlateBrushOutlineSettings(6.f, PanelBorder(), 1.2f);

	// ---- procedural textures
	UTexture2D* GlowTex = MakeGlowTexture(128, 2.4f);
	UTexture2D* DiscTex = MakeDiscTexture(64);
	UTexture2D* RingTex = MakeRingTexture(96, 0.06f);
	UTexture2D* RingThickTex = MakeRingTexture(96, 0.14f);
	UTexture2D* HexTex = MakeHexTexture(96, false);
	UTexture2D* HexOutTex = MakeHexTexture(96, true);

	GlowBrush = BrushFromTexture(GlowTex, FVector2D(128, 128));
	DiscBrush = BrushFromTexture(DiscTex, FVector2D(64, 64));
	RingBrush = BrushFromTexture(RingTex, FVector2D(96, 96));
	RingThickBrush = BrushFromTexture(RingThickTex, FVector2D(96, 96));
	HexBrush = BrushFromTexture(HexTex, FVector2D(96, 96));
	HexOutlineBrush = BrushFromTexture(HexOutTex, FVector2D(96, 96));

	// ---- sprite crops (present in packaged builds via RawAssets staging)
	const FString Dir = FPaths::ProjectContentDir() / TEXT("RawAssets/Sprites");
	auto LoadIcon = [this, &Dir](const TCHAR* Name) -> UTexture2D*
	{
		return LoadPng(Dir / Name);
	};

	const TCHAR* IconFiles[9] =
	{
		TEXT("node_input.png"), TEXT("node_relay.png"), TEXT("node_threshold.png"),
		TEXT("node_memory.png"), TEXT("node_oscillator.png"), TEXT("node_inhibitor.png"),
		TEXT("node_reward.png"), TEXT("node_output.png"), TEXT("node_mutate.png")
	};

	bHasSprites = true;
	for (int32 i = 0; i < 9; ++i)
	{
		if (UTexture2D* Tex = LoadIcon(IconFiles[i]))
		{
			NodeIcon[i] = BrushFromTexture(Tex, FVector2D(64, 64));
		}
		else
		{
			NodeIcon[i] = DiscBrush;
			bHasSprites = false;
		}
	}

	if (UTexture2D* T = LoadIcon(TEXT("badge_locked.png")))   { BadgeLocked = BrushFromTexture(T, FVector2D(48, 48)); }
	if (UTexture2D* T = LoadIcon(TEXT("badge_unstable.png"))) { BadgeUnstable = BrushFromTexture(T, FVector2D(48, 48)); }
	if (UTexture2D* T = LoadIcon(TEXT("badge_damaged.png")))  { BadgeDamaged = BrushFromTexture(T, FVector2D(48, 48)); }
	if (UTexture2D* T = LoadIcon(TEXT("logo_plaque.png")))
	{
		Logo = BrushFromTexture(T, LogoSize);
	}
	else
	{
		Logo = HexOutlineBrush;
	}
}

namespace
{
	UTexture2D* CreateFromPixels(int32 W, int32 H, const TArray<FColor>& Pixels)
	{
		UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
		if (!Tex) { return nullptr; }
		void* Data = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, Pixels.GetData(), W * H * sizeof(FColor));
		Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
		Tex->SRGB = true;
		Tex->UpdateResource();
		Tex->AddToRoot(); // style outlives GC scopes
		return Tex;
	}
}

UTexture2D* FForgeStyle::MakeGlowTexture(int32 Size, float Exponent)
{
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Size * Size);
	const float Half = Size * 0.5f;
	for (int32 y = 0; y < Size; ++y)
	{
		for (int32 x = 0; x < Size; ++x)
		{
			const float D = FMath::Sqrt(FMath::Square(x - Half + 0.5f) + FMath::Square(y - Half + 0.5f)) / Half;
			const float A = FMath::Pow(FMath::Clamp(1.f - D, 0.f, 1.f), Exponent);
			Pixels[y * Size + x] = FColor(255, 255, 255, (uint8)(A * 255));
		}
	}
	UTexture2D* Tex = CreateFromPixels(Size, Size, Pixels);
	OwnedTextures.Add(Tex);
	return Tex;
}

UTexture2D* FForgeStyle::MakeDiscTexture(int32 Size)
{
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Size * Size);
	const float Half = Size * 0.5f;
	for (int32 y = 0; y < Size; ++y)
	{
		for (int32 x = 0; x < Size; ++x)
		{
			const float D = FMath::Sqrt(FMath::Square(x - Half + 0.5f) + FMath::Square(y - Half + 0.5f));
			const float A = FMath::Clamp(Half - D - 0.5f, 0.f, 1.f); // 1px AA edge
			Pixels[y * Size + x] = FColor(255, 255, 255, (uint8)(A * 255));
		}
	}
	UTexture2D* Tex = CreateFromPixels(Size, Size, Pixels);
	OwnedTextures.Add(Tex);
	return Tex;
}

UTexture2D* FForgeStyle::MakeRingTexture(int32 Size, float Thickness01)
{
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Size * Size);
	const float Half = Size * 0.5f;
	const float Outer = Half - 1.f;
	const float Inner = Outer * (1.f - Thickness01 * 2.f);
	for (int32 y = 0; y < Size; ++y)
	{
		for (int32 x = 0; x < Size; ++x)
		{
			const float D = FMath::Sqrt(FMath::Square(x - Half + 0.5f) + FMath::Square(y - Half + 0.5f));
			float A = FMath::Clamp(Outer - D, 0.f, 1.f) * FMath::Clamp(D - Inner, 0.f, 1.f);
			Pixels[y * Size + x] = FColor(255, 255, 255, (uint8)(FMath::Clamp(A, 0.f, 1.f) * 255));
		}
	}
	UTexture2D* Tex = CreateFromPixels(Size, Size, Pixels);
	OwnedTextures.Add(Tex);
	return Tex;
}

UTexture2D* FForgeStyle::MakeHexTexture(int32 Size, bool bOutline)
{
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Size * Size);
	const float Half = Size * 0.5f;
	const float R = Half - 2.f;

	// point-in-hexagon via max of three axis projections (flat-top hex)
	auto HexDist = [R](float PX, float PY) -> float
	{
		const float AX = FMath::Abs(PX);
		const float AY = FMath::Abs(PY);
		return FMath::Max(AY * 0.8660254f + AX * 0.5f, AX) - R * 0.8660254f;
	};

	for (int32 y = 0; y < Size; ++y)
	{
		for (int32 x = 0; x < Size; ++x)
		{
			const float PX = x - Half + 0.5f;
			const float PY = y - Half + 0.5f;
			const float D = HexDist(PY, PX); // rotated: pointy-top
			float A;
			if (bOutline)
			{
				A = FMath::Clamp(1.5f - FMath::Abs(D + 1.5f) / 2.2f, 0.f, 1.f);
			}
			else
			{
				A = FMath::Clamp(-D, 0.f, 1.f);
			}
			Pixels[y * Size + x] = FColor(255, 255, 255, (uint8)(A * 255));
		}
	}
	UTexture2D* Tex = CreateFromPixels(Size, Size, Pixels);
	OwnedTextures.Add(Tex);
	return Tex;
}

UTexture2D* FForgeStyle::LoadPng(const FString& Path)
{
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("ForgeStyle: missing sprite %s"), *Path);
		return nullptr;
	}

	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(FileData.GetData(), FileData.Num()))
	{
		return nullptr;
	}

	TArray<uint8> Raw;
	if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
	{
		return nullptr;
	}

	const int32 W = Wrapper->GetWidth();
	const int32 H = Wrapper->GetHeight();
	UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	if (!Tex) { return nullptr; }

	void* Data = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Raw.GetData(), Raw.Num());
	Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
	Tex->SRGB = true;
	Tex->UpdateResource();
	Tex->AddToRoot();
	OwnedTextures.Add(Tex);
	return Tex;
}
