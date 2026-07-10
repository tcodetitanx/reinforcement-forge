#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Core/ForgeSession.h"

/**
 * The living graph view. Renders either the whole-brain region graph or an open
 * region's subnetwork, entirely procedurally (guide sections 18-22): layered
 * bezier glows, traveling pulses, electric jitter on unstable links, breathing
 * node halos. Handles pan/zoom/select/drag/connect/enter-region.
 */
class SGraphCanvas : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGraphCanvas) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FForgeSession* InSession);

	// ---- selection / hover state read by the editor screen
	TArray<int32> SelectedNodeIds;     // network view
	int32 SelectedConnId = INDEX_NONE;
	int32 SelectedRegionId = INDEX_NONE; // brain view
	int32 SelectedLinkId = INDEX_NONE;
	int32 HoveredNodeId = INDEX_NONE;
	int32 HoveredRegionId = INDEX_NONE;

	/** Arm connect mode: the next drag from a node creates a connection. */
	bool bConnectArmed = false;
	EConnType PendingConnType = EConnType::Excitatory;

	/** Pattern stamping: when set, next click stamps this pattern. */
	int32 PendingPatternId = INDEX_NONE;

	/** Pending add-node: when set, next click adds a node of this type. */
	int32 PendingAddNodeType = INDEX_NONE;

	void ClearSelection();
	void FocusContent();               // auto-fit view to content

	// SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(800, 600); }
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	FForgeSession* Session = nullptr;

	// view transform
	FVector2D ViewOffset = FVector2D(400, 300);
	float Zoom = 0.8f;
	bool bViewInitialized = false;
	double Now = 0.0;

	// interaction state
	bool bPanning = false;
	bool bDraggingNode = false;
	bool bDraggingConnect = false;
	int32 DragNodeId = INDEX_NONE;
	int32 ConnectFromId = INDEX_NONE;
	FVector2D LastMouseScreen = FVector2D::ZeroVector;
	FVector2D CurrentMouseWorld = FVector2D::ZeroVector;
	float MouseDownTime = 0.f;
	FVector2D MouseDownScreen = FVector2D::ZeroVector;

	bool InNetworkView() const { return Session && Session->OpenRegionId != INDEX_NONE; }

	FVector2D WorldToScreen(const FVector2D& World) const { return (World - ViewOffset) * Zoom; }
	FVector2D ScreenToWorld(const FVector2D& Screen) const { return Screen / Zoom + ViewOffset; }

	// hit testing (world space)
	int32 HitTestNode(const FVector2D& World) const;
	int32 HitTestRegion(const FVector2D& World) const;
	int32 HitTestConnection(const FVector2D& World) const;
	int32 HitTestRegionLink(const FVector2D& World) const;

	static void BezierPoints(const FVector2D& A, const FVector2D& B, FVector2D& OutP1, FVector2D& OutP2);
	static FVector2D EvalBezier(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float T);

	// paint helpers
	void PaintGrid(const FGeometry& Geo, FSlateWindowElementList& Out, int32 Layer) const;
	void PaintNetworkView(const FGeometry& Geo, FSlateWindowElementList& Out, int32& Layer) const;
	void PaintBrainView(const FGeometry& Geo, FSlateWindowElementList& Out, int32& Layer) const;
	void PaintConnectionCurve(const FGeometry& Geo, FSlateWindowElementList& Out, int32 Layer,
		const FVector2D& SrcW, const FVector2D& DstW, const FLinearColor& Color, float CoreThickness,
		float SignalStrength, float Traffic, bool bUnstableJitter, bool bSelected, uint32 JitterSeed, bool bDashed) const;
	void PaintPulses(const FGeometry& Geo, FSlateWindowElementList& Out, int32 Layer,
		const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3,
		const FLinearColor& Color, float Traffic, float PhaseSeed) const;
	void PaintNodeBody(const FGeometry& Geo, FSlateWindowElementList& Out, int32& Layer,
		const FVector2D& WorldPos, float WorldRadius, const FLinearColor& Color, float Activation,
		int32 IconIndex, bool bSelected, bool bHovered, bool bLocked, bool bDim, bool bHexFrame) const;
	void PaintLabel(const FGeometry& Geo, FSlateWindowElementList& Out, int32 Layer,
		const FVector2D& WorldPos, const FString& Text, const FLinearColor& Color, int32 FontSize, bool bBold = false) const;
};
