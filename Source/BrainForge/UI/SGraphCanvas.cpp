#include "SGraphCanvas.h"
#include "Core/ForgeStyle.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
	FLinearColor RegionColor(ERegionRole Role)
	{
		switch (Role)
		{
		case ERegionRole::Vision:
		case ERegionRole::Hearing:
		case ERegionRole::TouchPain:
		case ERegionRole::Balance:
		case ERegionRole::HungerThirst:     return FForgeStyle::Green();
		case ERegionRole::Motor:            return FForgeStyle::Teal();
		case ERegionRole::ThreatDetection:
		case ERegionRole::SelfPreservation: return FForgeStyle::Red();
		case ERegionRole::Memory:
		case ERegionRole::Attention:        return FForgeStyle::Orange();
		case ERegionRole::Language:         return FForgeStyle::Blue();
		case ERegionRole::SocialCognition:
		case ERegionRole::Emotion:          return FForgeStyle::Purple();
		case ERegionRole::RewardMotivation: return FForgeStyle::Gold();
		case ERegionRole::Planning:         return FForgeStyle::Cyan();
		default:                            return FForgeStyle::TextDim();
		}
	}

	constexpr float NodeRadius = 26.f;
	constexpr float RegionRadius = 48.f;

	int32 NodeIconIndex(ENodeType Type)
	{
		// FForgeStyle::NodeIcon order: input, relay, threshold, memory, oscillator, inhibitor, reward, output, mutate
		switch (Type)
		{
		case ENodeType::Input:      return 0;
		case ENodeType::Relay:      return 1;
		case ENodeType::Threshold:  return 2;
		case ENodeType::Memory:     return 3;
		case ENodeType::Oscillator: return 4;
		case ENodeType::Inhibitor:  return 5;
		case ENodeType::Reward:     return 6;
		case ENodeType::Output:     return 7;
		default:                    return 1;
		}
	}
}

void SGraphCanvas::Construct(const FArguments& InArgs, FForgeSession* InSession)
{
	Session = InSession;
	SetCanTick(true);
	ForceVolatile(true);
}

void SGraphCanvas::ClearSelection()
{
	SelectedNodeIds.Reset();
	SelectedConnId = INDEX_NONE;
	SelectedRegionId = INDEX_NONE;
	SelectedLinkId = INDEX_NONE;
}

void SGraphCanvas::FocusContent()
{
	if (!Session) { return; }

	FVector2D Min(FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX);
	auto Extend = [&Min, &Max](const FVector2D& P)
	{
		Min.X = FMath::Min(Min.X, P.X); Min.Y = FMath::Min(Min.Y, P.Y);
		Max.X = FMath::Max(Max.X, P.X); Max.Y = FMath::Max(Max.Y, P.Y);
	};

	if (InNetworkView())
	{
		if (FNeuralNetwork* Net = Session->OpenNetwork())
		{
			for (const FNeuralNode& N : Net->Nodes) { Extend(N.Pos); }
		}
	}
	else
	{
		for (const FBrainRegion& R : Session->Brain.Regions) { Extend(R.Pos); }
	}

	if (Min.X > Max.X) { return; }
	const FVector2D Size = GetTickSpaceGeometry().GetLocalSize();
	if (Size.X < 10.f) { ViewOffset = Min - FVector2D(120, 80); Zoom = 0.75f; return; }

	// generous padding keeps content clear of the side panels and top bar
	const FVector2D Content = Max - Min + FVector2D(660, 380);
	Zoom = FMath::Clamp((float)FMath::Min(Size.X / Content.X, Size.Y / Content.Y), 0.35f, 1.4f);
	ViewOffset = (Min + Max) * 0.5f - Size * 0.5f / Zoom + FVector2D(0, -30.f / Zoom);
}

void SGraphCanvas::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	Now = InCurrentTime;
	if (!bViewInitialized && AllottedGeometry.GetLocalSize().X > 10.f)
	{
		bViewInitialized = true;
		FocusContent();
	}
}

// ---------------------------------------------------------------- hit tests

int32 SGraphCanvas::HitTestNode(const FVector2D& World) const
{
	if (!Session) { return INDEX_NONE; }
	if (FNeuralNetwork* Net = const_cast<FForgeSession*>(Session)->OpenNetwork())
	{
		for (int32 i = Net->Nodes.Num() - 1; i >= 0; --i)
		{
			if (FVector2D::Distance(Net->Nodes[i].Pos, World) <= NodeRadius + 6.f)
			{
				return Net->Nodes[i].Id;
			}
		}
	}
	return INDEX_NONE;
}

int32 SGraphCanvas::HitTestRegion(const FVector2D& World) const
{
	if (!Session) { return INDEX_NONE; }
	for (int32 i = Session->Brain.Regions.Num() - 1; i >= 0; --i)
	{
		if (FVector2D::Distance(Session->Brain.Regions[i].Pos, World) <= RegionRadius + 8.f)
		{
			return Session->Brain.Regions[i].Id;
		}
	}
	return INDEX_NONE;
}

int32 SGraphCanvas::HitTestConnection(const FVector2D& World) const
{
	if (!Session) { return INDEX_NONE; }
	FNeuralNetwork* Net = const_cast<FForgeSession*>(Session)->OpenNetwork();
	if (!Net) { return INDEX_NONE; }

	for (const FNeuralConnection& C : Net->Connections)
	{
		const FNeuralNode* A = Net->FindNode(C.Source);
		const FNeuralNode* B = Net->FindNode(C.Target);
		if (!A || !B) { continue; }
		FVector2D P1, P2;
		BezierPoints(A->Pos, B->Pos, P1, P2);
		for (int32 s = 1; s < 24; ++s)
		{
			const FVector2D P = EvalBezier(A->Pos, P1, P2, B->Pos, s / 24.f);
			if (FVector2D::Distance(P, World) < 9.f) { return C.Id; }
		}
	}
	return INDEX_NONE;
}

int32 SGraphCanvas::HitTestRegionLink(const FVector2D& World) const
{
	if (!Session) { return INDEX_NONE; }
	for (const FRegionLink& L : Session->Brain.Links)
	{
		const FBrainRegion* A = Session->Brain.FindRegion(L.Source);
		const FBrainRegion* B = Session->Brain.FindRegion(L.Target);
		if (!A || !B) { continue; }
		FVector2D P1, P2;
		BezierPoints(A->Pos, B->Pos, P1, P2);
		for (int32 s = 1; s < 24; ++s)
		{
			const FVector2D P = EvalBezier(A->Pos, P1, P2, B->Pos, s / 24.f);
			if (FVector2D::Distance(P, World) < 12.f) { return L.Id; }
		}
	}
	return INDEX_NONE;
}

// ---------------------------------------------------------------- input

FReply SGraphCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D World = ScreenToWorld(Local);
	LastMouseScreen = Local;
	MouseDownScreen = Local;
	CurrentMouseWorld = World;

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton || MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bPanning = true;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Session)
	{
		return FReply::Unhandled();
	}

	// pending stamp / add-node actions consume the click
	if (InNetworkView() && PendingPatternId != INDEX_NONE)
	{
		Session->ActionInsertPattern(PendingPatternId, World);
		PendingPatternId = INDEX_NONE;
		return FReply::Handled();
	}
	if (InNetworkView() && PendingAddNodeType != INDEX_NONE)
	{
		Session->ActionAddNode((ENodeType)PendingAddNodeType, World);
		PendingAddNodeType = INDEX_NONE;
		return FReply::Handled();
	}

	if (InNetworkView())
	{
		const int32 NodeId = HitTestNode(World);
		if (NodeId != INDEX_NONE)
		{
			const bool bConnectDrag = bConnectArmed || MouseEvent.IsControlDown();
			if (bConnectDrag)
			{
				bDraggingConnect = true;
				ConnectFromId = NodeId;
			}
			else
			{
				bDraggingNode = true;
				DragNodeId = NodeId;
				if (MouseEvent.IsShiftDown())
				{
					SelectedNodeIds.AddUnique(NodeId);
				}
				else if (!SelectedNodeIds.Contains(NodeId))
				{
					SelectedNodeIds.Reset();
					SelectedNodeIds.Add(NodeId);
				}
				SelectedConnId = INDEX_NONE;
			}
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		const int32 ConnId = HitTestConnection(World);
		if (ConnId != INDEX_NONE)
		{
			SelectedConnId = ConnId;
			SelectedNodeIds.Reset();
			return FReply::Handled();
		}

		if (!MouseEvent.IsShiftDown()) { ClearSelection(); }
	}
	else
	{
		const int32 RegionId = HitTestRegion(World);
		if (RegionId != INDEX_NONE)
		{
			const bool bConnectDrag = bConnectArmed || MouseEvent.IsControlDown();
			if (bConnectDrag)
			{
				bDraggingConnect = true;
				ConnectFromId = RegionId;
			}
			else
			{
				bDraggingNode = true;
				DragNodeId = RegionId;
				SelectedRegionId = RegionId;
				SelectedLinkId = INDEX_NONE;
			}
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		const int32 LinkId = HitTestRegionLink(World);
		if (LinkId != INDEX_NONE)
		{
			SelectedLinkId = LinkId;
			SelectedRegionId = INDEX_NONE;
			return FReply::Handled();
		}

		ClearSelection();
	}

	// empty space: pan with left button too (calm, Mini Motorways-style)
	bPanning = true;
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SGraphCanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D Delta = Local - LastMouseScreen;
	LastMouseScreen = Local;
	CurrentMouseWorld = ScreenToWorld(Local);

	if (bPanning)
	{
		ViewOffset -= Delta / Zoom;
		return FReply::Handled();
	}

	if (bDraggingNode && Session)
	{
		const FVector2D WorldDelta = Delta / Zoom;
		if (InNetworkView())
		{
			if (FNeuralNetwork* Net = Session->OpenNetwork())
			{
				for (int32 Id : SelectedNodeIds)
				{
					if (FNeuralNode* N = Net->FindNode(Id)) { N->Pos += WorldDelta; }
				}
			}
		}
		else if (FBrainRegion* R = Session->Brain.FindRegion(DragNodeId))
		{
			R->Pos += WorldDelta;
		}
		return FReply::Handled();
	}

	if (bDraggingConnect)
	{
		return FReply::Handled();
	}

	// hover tracking
	if (Session)
	{
		if (InNetworkView())
		{
			HoveredNodeId = HitTestNode(CurrentMouseWorld);
			HoveredRegionId = INDEX_NONE;
		}
		else
		{
			HoveredRegionId = HitTestRegion(CurrentMouseWorld);
			HoveredNodeId = INDEX_NONE;
		}
	}
	return FReply::Unhandled();
}

FReply SGraphCanvas::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D World = ScreenToWorld(Local);

	if (bDraggingConnect && Session)
	{
		if (InNetworkView())
		{
			const int32 TargetId = HitTestNode(World);
			if (TargetId != INDEX_NONE && TargetId != ConnectFromId)
			{
				Session->ActionConnect(ConnectFromId, TargetId, PendingConnType);
			}
		}
		else
		{
			const int32 TargetId = HitTestRegion(World);
			if (TargetId != INDEX_NONE && TargetId != ConnectFromId)
			{
				Session->ActionConnectRegions(ConnectFromId, TargetId);
			}
		}
		bDraggingConnect = false;
		ConnectFromId = INDEX_NONE;
		bConnectArmed = false;
	}

	bPanning = false;
	bDraggingNode = false;
	DragNodeId = INDEX_NONE;

	if (HasMouseCapture())
	{
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Handled();
}

FReply SGraphCanvas::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D WorldBefore = ScreenToWorld(Local);

	Zoom = FMath::Clamp(Zoom * (MouseEvent.GetWheelDelta() > 0 ? 1.12f : 0.89f), 0.25f, 2.6f);

	// keep the point under the cursor fixed
	ViewOffset += WorldBefore - ScreenToWorld(Local);
	return FReply::Handled();
}

FReply SGraphCanvas::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!Session || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	const FVector2D World = ScreenToWorld(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));

	if (!InNetworkView())
	{
		const int32 RegionId = HitTestRegion(World);
		if (RegionId != INDEX_NONE)
		{
			if (const FBrainRegion* R = Session->Brain.FindRegion(RegionId))
			{
				if (R->bAwake)
				{
					Session->EnterRegion(RegionId);
					ClearSelection();
					bViewInitialized = false; // refit for the subnetwork
				}
				else
				{
					Session->ActionAwakenRegion(RegionId);
				}
			}
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

// ---------------------------------------------------------------- painting

void SGraphCanvas::BezierPoints(const FVector2D& A, const FVector2D& B, FVector2D& OutP1, FVector2D& OutP2)
{
	const float Dist = FVector2D::Distance(A, B);
	const float Tangent = FMath::Clamp(Dist * 0.45f, 30.f, 220.f);
	OutP1 = A + FVector2D(Tangent, 0);
	OutP2 = B - FVector2D(Tangent, 0);
}

FVector2D SGraphCanvas::EvalBezier(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float T)
{
	const float U = 1.f - T;
	return P0 * (U * U * U) + P1 * (3.f * U * U * T) + P2 * (3.f * U * T * T) + P3 * (T * T * T);
}

void SGraphCanvas::PaintGrid(const FGeometry& Geo, FSlateWindowElementList& Out, int32 Layer) const
{
	const FForgeStyle& Style = FForgeStyle::Get();
	const FVector2D Size = Geo.GetLocalSize();

	// background fill
	FSlateDrawElement::MakeBox(Out, Layer, Geo.ToPaintGeometry(),
		&Style.WhiteBrush, ESlateDrawEffect::None, FForgeStyle::Background());

	const float GridWorld = 120.f;
	const float Step = GridWorld * Zoom;
	if (Step < 14.f) { return; }

	const FLinearColor LineColor = FForgeStyle::GridLine();
	const float OffsetX = FMath::Fmod(-ViewOffset.X * Zoom, Step);
	const float OffsetY = FMath::Fmod(-ViewOffset.Y * Zoom, Step);

	TArray<FVector2D> Points;
	Points.SetNum(2);
	for (float X = OffsetX; X < Size.X; X += Step)
	{
		Points[0] = FVector2D(X, 0);
		Points[1] = FVector2D(X, Size.Y);
		FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(), Points, ESlateDrawEffect::None, LineColor, false, 1.f);
	}
	for (float Y = OffsetY; Y < Size.Y; Y += Step)
	{
		Points[0] = FVector2D(0, Y);
		Points[1] = FVector2D(Size.X, Y);
		FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(), Points, ESlateDrawEffect::None, LineColor, false, 1.f);
	}
}

void SGraphCanvas::PaintConnectionCurve(const FGeometry& Geo, FSlateWindowElementList& Out, int32 Layer,
	const FVector2D& SrcW, const FVector2D& DstW, const FLinearColor& Color, float CoreThickness,
	float SignalStrength, float Traffic, bool bUnstableJitter, bool bSelected, uint32 JitterSeed, bool bDashed) const
{
	const FVector2D A = WorldToScreen(SrcW);
	const FVector2D B = WorldToScreen(DstW);
	FVector2D P1W, P2W;
	BezierPoints(SrcW, DstW, P1W, P2W);
	const FVector2D P1 = WorldToScreen(P1W);
	const FVector2D P2 = WorldToScreen(P2W);

	const float T = CoreThickness * Zoom;
	const float Glow = FMath::Clamp(Traffic * 2.2f + SignalStrength, 0.f, 1.f);

	if (bUnstableJitter)
	{
		// jagged polyline recomputed every few frames (guide section 20)
		FRandomStream JRng(JitterSeed * 131 + (int32)(Now * 9.0));
		const int32 Segs = 14;
		TArray<FVector2D> Pts;
		Pts.Reserve(Segs + 1);
		for (int32 s = 0; s <= Segs; ++s)
		{
			const float F = (float)s / Segs;
			FVector2D P = EvalBezier(A, P1, P2, B, F);
			const float Amp = 7.f * Zoom * FMath::Sin(F * 3.14159f);
			P += FVector2D(JRng.FRandRange(-Amp, Amp), JRng.FRandRange(-Amp, Amp));
			Pts.Add(P);
		}
		FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(), Pts, ESlateDrawEffect::None,
			Color.CopyWithNewOpacity(0.16f + 0.3f * Glow), true, T * 3.f);
		FSlateDrawElement::MakeLines(Out, Layer + 1, Geo.ToPaintGeometry(), Pts, ESlateDrawEffect::None,
			Color.CopyWithNewOpacity(0.75f), true, T);
		return;
	}

	if (bDashed)
	{
		// dashed curve: alternating sampled segments
		const int32 Segs = 22;
		for (int32 s = 0; s < Segs; s += 2)
		{
			TArray<FVector2D> Pts;
			Pts.Add(EvalBezier(A, P1, P2, B, (float)s / Segs));
			Pts.Add(EvalBezier(A, P1, P2, B, (float)(s + 1) / Segs));
			FSlateDrawElement::MakeLines(Out, Layer + 1, Geo.ToPaintGeometry(), Pts, ESlateDrawEffect::None,
				Color.CopyWithNewOpacity(0.55f), true, T);
		}
		return;
	}

	// layered glow (guide section 19)
	FSlateDrawElement::MakeCubicBezierSpline(Out, Layer, Geo.ToPaintGeometry(), A, P1, P2, B,
		T * 4.5f, ESlateDrawEffect::None, Color.CopyWithNewOpacity(0.05f + 0.14f * Glow));
	FSlateDrawElement::MakeCubicBezierSpline(Out, Layer + 1, Geo.ToPaintGeometry(), A, P1, P2, B,
		T * 2.f, ESlateDrawEffect::None, Color.CopyWithNewOpacity(0.16f + 0.3f * Glow));
	FSlateDrawElement::MakeCubicBezierSpline(Out, Layer + 2, Geo.ToPaintGeometry(), A, P1, P2, B,
		FMath::Max(1.2f, T), ESlateDrawEffect::None, Color.CopyWithNewOpacity(0.55f + 0.45f * Glow));

	if (SignalStrength > 0.35f)
	{
		// white-hot active center
		FSlateDrawElement::MakeCubicBezierSpline(Out, Layer + 3, Geo.ToPaintGeometry(), A, P1, P2, B,
			FMath::Max(1.f, T * 0.5f), ESlateDrawEffect::None,
			FLinearColor(1.f, 1.f, 1.f, FMath::Min(0.85f, SignalStrength)));
	}

	if (bSelected)
	{
		FSlateDrawElement::MakeCubicBezierSpline(Out, Layer + 4, Geo.ToPaintGeometry(), A, P1, P2, B,
			T * 6.f, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.10f));
	}
}

void SGraphCanvas::PaintPulses(const FGeometry& Geo, FSlateWindowElementList& Out, int32 Layer,
	const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3,
	const FLinearColor& Color, float Traffic, float PhaseSeed) const
{
	if (Traffic < 0.03f) { return; }

	const FForgeStyle& Style = FForgeStyle::Get();
	const int32 PulseCount = Traffic > 0.4f ? 3 : (Traffic > 0.15f ? 2 : 1);
	const float Speed = 0.35f + Traffic * 0.5f;

	for (int32 p = 0; p < PulseCount; ++p)
	{
		const float T = FMath::Fmod((float)Now * Speed + PhaseSeed + (float)p / PulseCount, 1.f);
		const FVector2D Pos = EvalBezier(WorldToScreen(P0), WorldToScreen(P1), WorldToScreen(P2), WorldToScreen(P3), T);
		const float R = (4.f + Traffic * 5.f) * Zoom;

		FSlateDrawElement::MakeBox(Out, Layer,
			Geo.ToPaintGeometry(FVector2D(R * 4.f, R * 4.f), FSlateLayoutTransform(Pos - FVector2D(R * 2.f, R * 2.f))),
			&Style.GlowBrush, ESlateDrawEffect::None, Color.CopyWithNewOpacity(0.35f));
		FSlateDrawElement::MakeBox(Out, Layer + 1,
			Geo.ToPaintGeometry(FVector2D(R, R), FSlateLayoutTransform(Pos - FVector2D(R * 0.5f, R * 0.5f))),
			&Style.DiscBrush, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.9f));
	}
}

void SGraphCanvas::PaintNodeBody(const FGeometry& Geo, FSlateWindowElementList& Out, int32& Layer,
	const FVector2D& WorldPos, float WorldRadius, const FLinearColor& Color, float Activation,
	int32 IconIndex, bool bSelected, bool bHovered, bool bLocked, bool bDim, bool bHexFrame) const
{
	const FForgeStyle& Style = FForgeStyle::Get();
	const FVector2D Center = WorldToScreen(WorldPos);
	const float R = WorldRadius * Zoom;

	auto Box = [&](const FSlateBrush* Brush, float Radius, const FLinearColor& Tint, int32 LayerOffset)
	{
		FSlateDrawElement::MakeBox(Out, Layer + LayerOffset,
			Geo.ToPaintGeometry(FVector2D(Radius * 2.f, Radius * 2.f), FSlateLayoutTransform(Center - FVector2D(Radius, Radius))),
			Brush, ESlateDrawEffect::None, Tint);
	};

	const float Act = FMath::Clamp(Activation, 0.f, 1.f);
	const FLinearColor Main = bDim ? Color * 0.35f : Color;

	// breathing outer glow scales with activation (guide section 21)
	const float Breathe = 1.f + 0.06f * FMath::Sin((float)Now * 2.0f + WorldPos.X * 0.01f);
	Box(&Style.GlowBrush, R * (1.7f + Act * 0.9f) * Breathe, Main.CopyWithNewOpacity(bDim ? 0.06f : 0.10f + Act * 0.4f), 0);

	// frame
	if (bHexFrame)
	{
		Box(&Style.HexBrush, R * 1.06f, FForgeStyle::Background().CopyWithNewOpacity(0.92f), 1);
		Box(&Style.HexOutlineBrush, R * 1.12f, Main.CopyWithNewOpacity(bDim ? 0.4f : 0.9f), 2);
	}
	else
	{
		Box(&Style.DiscBrush, R * 1.02f, FForgeStyle::Background().CopyWithNewOpacity(0.92f), 1);
		Box(&Style.RingBrush, R * 1.12f, Main.CopyWithNewOpacity(bDim ? 0.4f : 0.9f), 2);
	}

	// inner activation ring brightens with activation
	Box(&Style.RingThickBrush, R * 0.82f, Main.CopyWithNewOpacity(0.15f + Act * 0.8f), 3);

	// icon
	if (IconIndex >= 0 && IconIndex < 9)
	{
		const float IconR = R * 0.62f;
		FSlateDrawElement::MakeBox(Out, Layer + 4,
			Geo.ToPaintGeometry(FVector2D(IconR * 2.f, IconR * 2.f), FSlateLayoutTransform(Center - FVector2D(IconR, IconR))),
			&Style.NodeIcon[IconIndex], ESlateDrawEffect::None,
			FLinearColor(1.f, 1.f, 1.f, bDim ? 0.35f : 0.75f + Act * 0.25f));
	}

	// core hot spot pulses at high activation
	if (Act > 0.55f)
	{
		const float PulseR = R * (0.25f + 0.1f * FMath::Sin((float)Now * 9.f));
		Box(&Style.GlowBrush, PulseR * 2.f, FLinearColor(1.f, 1.f, 1.f, (Act - 0.55f) * 1.4f), 5);
	}

	if (bHovered)
	{
		Box(&Style.RingBrush, R * 1.3f, FLinearColor(1.f, 1.f, 1.f, 0.35f), 6);
	}
	if (bSelected)
	{
		const float SelR = R * (1.38f + 0.05f * FMath::Sin((float)Now * 4.f));
		Box(&Style.RingBrush, SelR, FLinearColor(1.f, 1.f, 1.f, 0.85f), 6);
	}
	if (bLocked)
	{
		const float BR = R * 0.5f;
		FSlateDrawElement::MakeBox(Out, Layer + 7,
			Geo.ToPaintGeometry(FVector2D(BR * 2.f, BR * 2.f), FSlateLayoutTransform(Center + FVector2D(R * 0.55f, -R * 1.25f))),
			&Style.BadgeLocked, ESlateDrawEffect::None, FLinearColor::White);
	}

	Layer += 8;
}

void SGraphCanvas::PaintLabel(const FGeometry& Geo, FSlateWindowElementList& Out, int32 Layer,
	const FVector2D& WorldPos, const FString& Text, const FLinearColor& Color, int32 FontSize, bool bBold) const
{
	const FForgeStyle& Style = FForgeStyle::Get();
	const FSlateFontInfo Font = Style.Font(FMath::Max(7, (int32)(FontSize * FMath::Min(1.f, Zoom * 1.4f))), bBold);
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D TextSize = Measure->Measure(Text, Font);
	const FVector2D Pos = WorldToScreen(WorldPos) - FVector2D(TextSize.X * 0.5f, 0.f);

	FSlateDrawElement::MakeText(Out, Layer,
		Geo.ToPaintGeometry(TextSize, FSlateLayoutTransform(Pos + FVector2D(1, 1))),
		Text, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.8f));
	FSlateDrawElement::MakeText(Out, Layer,
		Geo.ToPaintGeometry(TextSize, FSlateLayoutTransform(Pos)),
		Text, Font, ESlateDrawEffect::None, Color);
}

void SGraphCanvas::PaintNetworkView(const FGeometry& Geo, FSlateWindowElementList& Out, int32& Layer) const
{
	FNeuralNetwork* Net = const_cast<FForgeSession*>(Session)->OpenNetwork();
	if (!Net) { return; }

	// connections
	for (const FNeuralConnection& C : Net->Connections)
	{
		const FNeuralNode* A = Net->FindNode(C.Source);
		const FNeuralNode* B = Net->FindNode(C.Target);
		if (!A || !B) { continue; }

		const FLinearColor Color = FForgeStyle::ConnTypeColor((uint8)C.Type);
		const float Thickness = 1.2f + FMath::Abs(C.Weight) * 1.4f;
		const bool bJitter = (C.Type == EConnType::Unstable);
		const bool bDashed = (C.Type == EConnType::Modulatory || C.Type == EConnType::Delayed);

		PaintConnectionCurve(Geo, Out, Layer, A->Pos, B->Pos, Color, Thickness,
			FMath::Abs(C.LastSignal), C.Traffic, bJitter, C.Id == SelectedConnId, (uint32)C.Id, bDashed);

		FVector2D P1, P2;
		BezierPoints(A->Pos, B->Pos, P1, P2);
		PaintPulses(Geo, Out, Layer + 5, A->Pos, P1, P2, B->Pos, Color, C.Traffic, (C.Id % 7) / 7.f);
	}
	Layer += 8;

	// nodes
	for (const FNeuralNode& N : Net->Nodes)
	{
		const FLinearColor Color = FForgeStyle::NodeTypeColor((uint8)N.Type);
		const bool bHex = (N.Type == ENodeType::Input || N.Type == ENodeType::Output);
		PaintNodeBody(Geo, Out, Layer, N.Pos, NodeRadius, Color, FMath::Abs(N.Activation),
			NodeIconIndex(N.Type), SelectedNodeIds.Contains(N.Id), N.Id == HoveredNodeId, N.bLocked, false, bHex);

		FString Label = FString::Printf(TEXT("%s"), NodeTypeName(N.Type));
		if (N.Type == ENodeType::Input || N.Type == ENodeType::Output)
		{
			Label = FString::Printf(TEXT("%s-%d"), N.Type == ENodeType::Input ? TEXT("IN") : TEXT("OUT"), N.Channel + 1);
		}
		PaintLabel(Geo, Out, Layer, N.Pos + FVector2D(0, NodeRadius + 8.f), Label, FForgeStyle::TextDim(), 9);
	}

	// pending connect line
	if (bDraggingConnect && ConnectFromId != INDEX_NONE)
	{
		if (const FNeuralNode* From = Net->FindNode(ConnectFromId))
		{
			PaintConnectionCurve(Geo, Out, Layer, From->Pos, CurrentMouseWorld,
				FForgeStyle::ConnTypeColor((uint8)PendingConnType), 1.6f, 0.6f, 0.3f, false, false, 0, false);
		}
	}
	Layer += 4;
}

void SGraphCanvas::PaintBrainView(const FGeometry& Geo, FSlateWindowElementList& Out, int32& Layer) const
{
	const FForgeStyle& Style = FForgeStyle::Get();
	const FBrainGraph& Brain = Session->Brain;

	// region links
	for (const FRegionLink& L : Brain.Links)
	{
		const FBrainRegion* A = Brain.FindRegion(L.Source);
		const FBrainRegion* B = Brain.FindRegion(L.Target);
		if (!A || !B) { continue; }

		const FLinearColor Color = RegionColor(A->Role);
		PaintConnectionCurve(Geo, Out, Layer, A->Pos, B->Pos, Color, 1.6f + L.Weight,
			FMath::Abs(L.LastSignal), L.Traffic, false, L.Id == SelectedLinkId, (uint32)L.Id, false);

		FVector2D P1, P2;
		BezierPoints(A->Pos, B->Pos, P1, P2);
		PaintPulses(Geo, Out, Layer + 5, A->Pos, P1, P2, B->Pos, Color, L.Traffic, (L.Id % 5) / 5.f);
	}
	Layer += 8;

	// regions
	for (const FBrainRegion& R : Brain.Regions)
	{
		const FLinearColor Color = RegionColor(R.Role);
		const bool bDim = !R.bAwake;

		PaintNodeBody(Geo, Out, Layer, R.Pos, RegionRadius, Color, R.Activation,
			bDim ? -1 : 8 /* brain-ish mutate icon for regions */, R.Id == SelectedRegionId,
			R.Id == HoveredRegionId, false, bDim, true);

		if (bDim)
		{
			// dashed dormant ring
			const FVector2D Center = R.Pos;
			const int32 Dashes = 12;
			for (int32 d = 0; d < Dashes; d += 2)
			{
				TArray<FVector2D> Pts;
				for (int32 s = 0; s <= 3; ++s)
				{
					const float Ang = ((float)d + s / 3.f) / Dashes * 6.2831853f + (float)Now * 0.3f;
					Pts.Add(WorldToScreen(Center + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * RegionRadius * 1.25f));
				}
				FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(), Pts, ESlateDrawEffect::None,
					FForgeStyle::TextDim().CopyWithNewOpacity(0.5f), true, 1.4f);
			}

			const bool bReady = (int32)RegionUnlockPhase(R.Role) <= (int32)Session->Phase;
			PaintLabel(Geo, Out, Layer, R.Pos + FVector2D(0, RegionRadius + 14.f), TEXT("DORMANT"),
				FForgeStyle::TextDim(), 9, true);
			if (bReady && R.Id == HoveredRegionId)
			{
				PaintLabel(Geo, Out, Layer, R.Pos + FVector2D(0, RegionRadius + 30.f),
					Session->bFreeAwaken ? TEXT("Double-click: awaken (FREE)") : TEXT("Double-click: awaken (150 energy)"),
					FForgeStyle::Gold(), 9);
			}
		}
		else
		{
			PaintLabel(Geo, Out, Layer, R.Pos + FVector2D(0, RegionRadius + 12.f), R.DisplayName(),
				R.NameKnown() ? Color : FForgeStyle::TextBright(), 11, true);

			if (!R.FullyKnown())
			{
				const FString Conf = R.DiscoveryConfidence > 0.05f
					? FString::Printf(TEXT("confidence %d%%"), FMath::RoundToInt(R.DiscoveryConfidence * 100.f))
					: TEXT("function unknown");
				PaintLabel(Geo, Out, Layer, R.Pos + FVector2D(0, RegionRadius + 30.f), Conf, FForgeStyle::TextDim(), 8);
			}

			// expanding pulse ring on hot regions
			if (R.Activation > 0.45f)
			{
				const float PT = FMath::Fmod((float)Now * 0.8f + R.Id * 0.17f, 1.f);
				const float PR = RegionRadius * (1.1f + PT * 0.9f) * Zoom;
				const FVector2D Center = WorldToScreen(R.Pos);
				FSlateDrawElement::MakeBox(Out, Layer,
					Geo.ToPaintGeometry(FVector2D(PR * 2.f, PR * 2.f), FSlateLayoutTransform(Center - FVector2D(PR, PR))),
					&Style.RingBrush, ESlateDrawEffect::None, RegionColor(R.Role).CopyWithNewOpacity((1.f - PT) * 0.5f * R.Activation));
			}
		}
	}

	// pending region connect line
	if (bDraggingConnect && ConnectFromId != INDEX_NONE)
	{
		if (const FBrainRegion* From = Brain.FindRegion(ConnectFromId))
		{
			PaintConnectionCurve(Geo, Out, Layer, From->Pos, CurrentMouseWorld,
				FForgeStyle::Cyan(), 1.8f, 0.6f, 0.3f, false, false, 0, false);
		}
	}
	Layer += 4;
}

int32 SGraphCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!Session || !FForgeStyle::Get().IsInitialized())
	{
		return LayerId;
	}

	OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));

	int32 Layer = LayerId;
	PaintGrid(AllottedGeometry, OutDrawElements, Layer);
	Layer += 1;

	if (InNetworkView())
	{
		PaintNetworkView(AllottedGeometry, OutDrawElements, Layer);
	}
	else
	{
		PaintBrainView(AllottedGeometry, OutDrawElements, Layer);
	}

	OutDrawElements.PopClip();
	return Layer;
}
