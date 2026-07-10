#include "SNeonWidgets.h"
#include "Core/ForgeStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"

// ------------------------------------------------------------- SNeonButton

void SNeonButton::Construct(const FArguments& InArgs)
{
	OnClicked = InArgs._OnClicked;
	EnabledAttr = InArgs._IsEnabledAttr.IsSet() ? InArgs._IsEnabledAttr : true;
	Color = InArgs._Color;
	bFilled = InArgs._bFilled;

	const FForgeStyle& Style = FForgeStyle::Get();

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(InArgs._Text)
			.Font(Style.Font(InArgs._FontSize, true))
			.ColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this]()
			{
				const bool bOn = !EnabledAttr.IsSet() || EnabledAttr.Get();
				if (!bOn) { return FSlateColor(FForgeStyle::TextDim() * 0.7f); }
				return FSlateColor(bFilled ? FLinearColor(0.05f, 0.04f, 0.f) : (bHovered ? FLinearColor::White : Color));
			}))
		];

	if (!InArgs._HotkeyHint.IsEmpty())
	{
		Row->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(6, 0, 2, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(InArgs._HotkeyHint))
			.Font(Style.Font(8))
			.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
		];
	}

	ChildSlot
	.Padding(FMargin(10.f, 6.f))
	[
		Row
	];
}

FReply SNeonButton::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && (!EnabledAttr.IsSet() || EnabledAttr.Get()))
	{
		bPressed = true;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	return FReply::Unhandled();
}

FReply SNeonButton::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bPressed && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bPressed = false;
		if (MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition()) && (!EnabledAttr.IsSet() || EnabledAttr.Get()))
		{
			OnClicked.ExecuteIfBound();
		}
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

void SNeonButton::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	bHovered = true;
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
}

void SNeonButton::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	bHovered = false;
	bPressed = false;
	SCompoundWidget::OnMouseLeave(MouseEvent);
}

int32 SNeonButton::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const bool bOn = !EnabledAttr.IsSet() || EnabledAttr.Get();
	const float Bright = !bOn ? 0.35f : (bPressed ? 1.3f : (bHovered ? 1.15f : 1.f));

	FSlateBrush Bg;
	Bg.DrawAs = ESlateBrushDrawType::RoundedBox;
	Bg.OutlineSettings = FSlateBrushOutlineSettings(5.f, Color.CopyWithNewOpacity(bOn ? 0.9f : 0.25f) * Bright, 1.4f);
	if (bFilled && bOn)
	{
		Bg.TintColor = Color * Bright;
	}
	else
	{
		Bg.TintColor = FLinearColor(Color.R * 0.10f, Color.G * 0.10f, Color.B * 0.10f, 0.85f);
	}

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &Bg, ESlateDrawEffect::None, Bg.TintColor.GetSpecifiedColor());

	// hover glow
	if (bHovered && bOn)
	{
		const FForgeStyle& Style = FForgeStyle::Get();
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(AllottedGeometry.GetLocalSize() * 1.15f,
				FSlateLayoutTransform(-AllottedGeometry.GetLocalSize() * 0.075f)),
			&Style.GlowBrush, ESlateDrawEffect::None, Color.CopyWithNewOpacity(0.18f));
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 1, InWidgetStyle, bParentEnabled);
}

// ------------------------------------------------------------- SRadialGauge

void SRadialGauge::Construct(const FArguments& InArgs)
{
	Percent = InArgs._Percent;
	Color = InArgs._Color;
	Size = InArgs._Size;
	ForceVolatile(true);
}

int32 SRadialGauge::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FForgeStyle& Style = FForgeStyle::Get();
	const FVector2D Local = AllottedGeometry.GetLocalSize();
	const FVector2D Center = Local * 0.5f;
	const float R = FMath::Min(Local.X, Local.Y) * 0.5f - 3.f;
	const float P = FMath::Clamp(Percent.Get(0.f), 0.f, 1.f);

	// background ring
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(R * 2.f, R * 2.f), FSlateLayoutTransform(Center - FVector2D(R, R))),
		&Style.RingThickBrush, ESlateDrawEffect::None, FForgeStyle::PanelBorder().CopyWithNewOpacity(0.6f));

	// arc: polyline segments from -90 degrees clockwise
	if (P > 0.01f)
	{
		const int32 Segs = FMath::Max(3, (int32)(P * 40.f));
		TArray<FVector2D> Pts;
		Pts.Reserve(Segs + 1);
		for (int32 s = 0; s <= Segs; ++s)
		{
			const float Ang = -1.5707963f + P * 6.2831853f * s / Segs;
			Pts.Add(Center + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * (R - 1.f));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
			Pts, ESlateDrawEffect::None, Color, true, 3.4f);
	}

	return LayerId + 2;
}

// ------------------------------------------------------------- SLineChart

void SLineChart::Construct(const FArguments& InArgs)
{
	Data = InArgs._Data;
	Color = InArgs._Color;
	MinHeight = InArgs._MinHeight;
	ForceVolatile(true);
}

int32 SLineChart::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FForgeStyle& Style = FForgeStyle::Get();
	const FVector2D Size2 = AllottedGeometry.GetLocalSize();

	// frame
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		&Style.WhiteBrush, ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.35f));

	if (!Data || Data->Num() < 2)
	{
		return LayerId + 1;
	}

	float MaxV = 1.f;
	for (float V : *Data) { MaxV = FMath::Max(MaxV, V); }

	const int32 N = Data->Num();
	TArray<FVector2D> Pts;
	Pts.Reserve(N);
	for (int32 i = 0; i < N; ++i)
	{
		const float X = Size2.X * i / (N - 1);
		const float Y = Size2.Y - 4.f - ((*Data)[i] / MaxV) * (Size2.Y - 10.f);
		Pts.Add(FVector2D(X, Y));
	}

	// soft glow underlay then core line
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
		Pts, ESlateDrawEffect::None, Color.CopyWithNewOpacity(0.25f), true, 4.f);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
		Pts, ESlateDrawEffect::None, Color, true, 1.6f);

	// latest-value dot
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
		AllottedGeometry.ToPaintGeometry(FVector2D(8, 8), FSlateLayoutTransform(Pts.Last() - FVector2D(4, 4))),
		&Style.GlowBrush, ESlateDrawEffect::None, Color);

	return LayerId + 4;
}

// ------------------------------------------------------------- SBarChart

void SBarChart::Construct(const FArguments& InArgs)
{
	Data = InArgs._Data;
	MinHeight = InArgs._MinHeight;
	ForceVolatile(true);
}

int32 SBarChart::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FForgeStyle& Style = FForgeStyle::Get();
	const FVector2D Size2 = AllottedGeometry.GetLocalSize();

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		&Style.WhiteBrush, ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.35f));

	if (!Data || Data->Num() == 0)
	{
		return LayerId + 1;
	}

	const int32 N = Data->Num();
	const float BarW = FMath::Max(2.f, Size2.X / N - 3.f);

	for (int32 i = 0; i < N; ++i)
	{
		const float V = FMath::Clamp((*Data)[i], 0.02f, 1.f);
		const float H = V * (Size2.Y - 8.f);
		const FVector2D Pos(Size2.X * i / N + 1.5f, Size2.Y - 3.f - H);

		// newest bars glow green, older fade to blue
		const float Age = (float)i / N;
		const FLinearColor C = FMath::Lerp(FForgeStyle::Blue() * 0.8f, FForgeStyle::Green(), Age * Age);

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(BarW, H), FSlateLayoutTransform(Pos)),
			&Style.WhiteBrush, ESlateDrawEffect::None, C.CopyWithNewOpacity(0.85f));
	}

	return LayerId + 2;
}

// ------------------------------------------------------------- helpers

namespace ForgeUI
{
	TSharedRef<SWidget> Panel(const FText& Title, TSharedRef<SWidget> Content, FLinearColor TitleColor)
	{
		const FForgeStyle& Style = FForgeStyle::Get();

		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
		if (!Title.IsEmpty())
		{
			Box->AddSlot()
			.AutoHeight()
			.Padding(12, 9, 12, 4)
			[
				SNew(STextBlock)
				.Text(Title)
				.Font(Style.Font(10, true))
				.ColorAndOpacity(FSlateColor(TitleColor))
			];
		}
		Box->AddSlot()
		.AutoHeight()
		.Padding(12, 2, 12, 10)
		[
			Content
		];

		return SNew(SBorder)
			.BorderImage(&Style.PanelBrush)
			.Padding(0)
			[
				Box
			];
	}

	TSharedRef<SWidget> StatBlock(const FText& Label, TAttribute<FText> Value, FLinearColor ValueColor, int32 ValueSize)
	{
		const FForgeStyle& Style = FForgeStyle::Get();
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(Style.Font(8, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 1, 0, 0)
			[
				SNew(STextBlock)
				.Text(Value)
				.Font(Style.Font(ValueSize, true))
				.ColorAndOpacity(FSlateColor(ValueColor))
			];
	}
}
