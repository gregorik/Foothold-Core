// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Modes/FootholdModeBase.h"
#include "FootholdLadderMode.generated.h"

class UFootholdLadderComponent;

/**
 * Climbing a UFootholdLadderComponent: held in front of the ladder, facing it. Move input toward
 * the ladder climbs, away descends; with no input the climber settles on the nearest rung. Past the
 * top it mantles off (if the ladder allows it), at the bottom it steps off onto the floor, and jump
 * pushes off backwards.
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTHOLD_API UFootholdLadderMode : public UFootholdModeBase
{
	GENERATED_BODY()

public:
	UFootholdLadderMode();

	virtual EFootholdKind GetFootholdKind() const override { return EFootholdKind::Ladder; }

	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	/** Climbing speed at full input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float ClimbSpeed = 150.f;

	/** Speed of the settle onto the nearest rung when there's no input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float RungSettleSpeed = 60.f;

	/** Seconds to move from where the ladder was grabbed into the climbing position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "s"))
	float EntryTime = 0.2f;

	/** Jumping off: speed away from the ladder, and upward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float JumpOffSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm/s"))
	float JumpOffUpSpeed = 250.f;

	/** After leaving a ladder, ladder grabs wait this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "s"))
	float RegrabDelay = 0.4f;

	/** How far from the ladder's climbing line the mode looks for it on entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm"))
	float GrabDistance = 80.f;

	/** Descending steps off onto walkable floor this close under the feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm"))
	float FloorStepOffDistance = 6.f;

	/**
	 * Climbing up, the mantle off the top starts once the capsule centre is this far below the top of the ladder: the
	 * hands are at the top, as when climbing a wall tops out. If there's nothing to mantle onto there, the climb goes on.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm"))
	float TopOutBelowTop = 60.f;

protected:
	void Leave(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Velocity) const;
	bool TryMantleOffTop(const FSimulationTickParams& Params, const FFootholdSyncState& State, FMoverTickEndData& OutputState) const;
};
