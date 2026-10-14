// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Modes/FootholdModeBase.h"
#include "FootholdMantleMode.generated.h"

/**
 * Pulls the capsule up onto a ledge: it rises in front of the wall until the feet clear the top,
 * then moves over onto it, and hands over to the ground mode. The path is a function of start, end
 * and time only, so every machine computes the same positions. Animation is cosmetic: use the
 * mantle clip driven by FFootholdAnimData::MantleProgress, or an entry montage on UFootholdComponent.
 *
 * Entered from the air (UFootholdAirMantleTransition), from a ledge hang (climb up), from the top of
 * a climb, or from the top of a ladder. It finds its ledge again on entry.
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTHOLD_API UFootholdMantleMode : public UFootholdModeBase
{
	GENERATED_BODY()

public:
	UFootholdMantleMode();

	virtual EFootholdKind GetFootholdKind() const override { return EFootholdKind::Mantle; }

	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	/** Speed of the pull-up phase: with the height it sets how long the rise takes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "1", ForceUnits = "cm/s"))
	float RiseSpeed = 260.f;

	/** Speed of the move over the top: with the distance it sets how long that phase takes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "1", ForceUnits = "cm/s"))
	float OverSpeed = 300.f;

	/** Shortest rise and shortest move-over, so small mantles don't snap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "0.05", ForceUnits = "s"))
	float MinPhaseDuration = 0.18f;

	/** Longest whole mantle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "0.1", ForceUnits = "s"))
	float MaxDuration = 1.2f;

	/** Highest edge above the feet the mode will mantle onto when it has to find the ledge itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "0", ForceUnits = "cm"))
	float MaxHeight = 260.f;

	/** Speed along the facing when the mantle hands over to walking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float ExitSpeed = 150.f;

	/** The mantle aborts to the air mode if the capsule ends a step this far from where the path says. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "1", ForceUnits = "cm"))
	float BlockedTolerance = 10.f;

	/** After a mantle can't start or gets blocked, entry transitions wait this long before trying again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "0", ForceUnits = "s"))
	float RetryCooldown = 0.25f;

	/** True when the capsule can pull up onto Ledge: valid, room to stand, and room to rise in front of the wall. */
	static bool IsMantleable(const FFootholdQueryContext& Ctx, const FFootholdLedge& Ledge);

	/** The point on the mantle path at Alpha (0..1), given its start, end and up direction. */
	static FVector EvaluatePath(const FVector& Start, const FVector& End, const FVector& Up, float RiseShare, float Alpha);

protected:
	/** Sets up the mantle state on entry. Returns false when there's nothing to mantle onto. */
	bool BeginMantle(const FSimulationTickParams& Params, const FFootholdSyncState* PreviousFoothold, FFootholdSyncState& Out) const;
};
