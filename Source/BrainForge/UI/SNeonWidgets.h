#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

/** A glowing outlined button in the style of the UI kit sheet. */
class SNeonButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNeonButton)
		: _Color(FLinearColor(0.2f, 0.88f, 1.f))
		, _FontSize(11)
		, _bFilled(false)
	{}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(int32, FontSize)
		SLATE_ARGUMENT(bool, bFilled)          // gold primary-style fill
		SLATE_ARGUMENT(FString, HotkeyHint)    // right-aligned small hint
		SLATE_EVENT(FSimpleDelegate, OnClicked)
		SLATE_ATTRIBUTE(bool, IsEnabledAttr)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override
	{
		return FCursorReply::Cursor(EMouseCursor::Hand);
	}

private:
	FSimpleDelegate OnClicked;
	TAttribute<bool> EnabledAttr;
	FLinearColor Color;
	bool bFilled = false;
	bool bHovered = false;
	bool bPressed = false;
};

/** Circular percent gauge like the Stability / Efficiency readouts. */
class SRadialGauge : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SRadialGauge)
		: _Color(FLinearColor(0.2f, 0.88f, 1.f))
		, _Size(44.f)
	{}
		SLATE_ATTRIBUTE(float, Percent)   // 0..1
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(float, Size)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Size, Size); }

private:
	TAttribute<float> Percent;
	FLinearColor Color;
	float Size = 44.f;
};

/** Scrolling line chart (reward history). Reads a float array by pointer each frame. */
class SLineChart : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SLineChart)
		: _Color(FLinearColor(0.45f, 1.f, 0.30f))
		, _MinHeight(70.f)
	{}
		SLATE_ARGUMENT(const TArray<float>*, Data)
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(float, MinHeight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(220.f, MinHeight); }

private:
	const TArray<float>* Data = nullptr;
	FLinearColor Color;
	float MinHeight = 70.f;
};

/** Bar chart (recent trial scores). */
class SBarChart : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SBarChart)
		: _MinHeight(60.f)
	{}
		SLATE_ARGUMENT(const TArray<float>*, Data)   // values 0..1
		SLATE_ARGUMENT(float, MinHeight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(220.f, MinHeight); }

private:
	const TArray<float>* Data = nullptr;
	float MinHeight = 60.f;
};

namespace ForgeUI
{
	/** Dark rounded panel with an optional header strip. */
	TSharedRef<SWidget> Panel(const FText& Title, TSharedRef<SWidget> Content, FLinearColor TitleColor = FLinearColor(0.80f, 0.93f, 1.0f));

	/** Small "LABEL / value" stat block used in the top bar. */
	TSharedRef<SWidget> StatBlock(const FText& Label, TAttribute<FText> Value, FLinearColor ValueColor, int32 ValueSize = 18);
}
