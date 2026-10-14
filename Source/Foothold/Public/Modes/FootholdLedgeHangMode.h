// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Modes/FootholdModeBase.h"
#include "FootholdLedgeHangMode.generated.h"

/**
 * Hanging from a ledge by the hands. Move input along the edge shimmies (and stops where the ledge
 * ends); jump, or holding toward the wall, climbs up (Mantle); jump while pulling away, or crouch,
 * lets go; holding away from a climbable wall climbs down onto it (Climbing).
 *
 * The capsule hangs HandReachAboveCenter below the edge and WallGap off the wall (shared settings).
 * If the ledge moves, the hold breaks and the pawn falls.
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTHOLD_API UFootholdLedgeHangMode : public UFootholdModeBase
{
	GENERATED_BODY()

public:
	UFootholdLedgeHangMode();

	virtual EFootholdKind GetFootholdKind() const override { return EFootholdKind::LedgeHang; }

	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	/** Sideways speed along the edge at full input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Hang", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float ShimmySpeed = 130.f;

	/** Seconds to settle from the grab point into the hang position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Hang", meta = (ClampMin = "0", ForceUnits = "s"))
	float SettleTime = 0.15f;

	/** Holding toward the wall this long climbs up, without a jump. Zero disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Hang", meta = (ClampMin = "0", ForceUnits = "s"))
	float ClimbUpHoldTime = 0.35f;

	/** Holding away from a climbable wall this long climbs down onto it. Zero disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Hang", meta = (ClampMin = "0", ForceUnits = "s"))
	float ClimbDownHoldTime = 0.35f;

	/** Speed away from the wall when letting go. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Hang", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float DropPushSpeed = 150.f;

	/** After letting go, ledge grabs wait this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Hang", meta = (ClampMin = "0", ForceUnits = "s"))
	float RegrabDelay = 0.4f;

	/** A shimmy stops where the next stretch of edge is higher or lower than this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Hang", meta = (ClampMin = "0", ForceUnits = "cm"))
	float MaxEdgeStep = 20.f;

	/** Edge heights (above the feet) the mode accepts when it has to find the ledge itself, around the hang reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Hang", meta = (ClampMin = "0", ForceUnits = "cm"))
	float EdgeSearchMargin = 35.f;

	/** The capsule centre hanging from EdgePoint. */
	static FVector GetHangLocation(const FVector& EdgePoint, const FVector& WallNormal, const FVector& Up, float CapsuleRadius, const class UFootholdSharedSettings& Settings);

protected:
	bool BeginHang(const FSimulationTickParams& Params, const FFootholdSyncState* PreviousFoothold, FFootholdSyncState& Out) const;

	/** Looks for the edge again in front of a capsule centred at HangCenter. */
	FFootholdLedge ProbeEdge(const FFootholdQueryContext& Ctx, const FVector& HangCenter, const FVector& WallNormal) const;

	void LetGo(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Velocity) const;
};
