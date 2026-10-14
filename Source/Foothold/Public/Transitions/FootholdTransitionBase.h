// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "MovementModeTransition.h"
#include "FootholdTraceLibrary.h"
#include "FootholdTransitionBase.generated.h"

class UFootholdSharedSettings;
struct FMoverTickStartData;

/**
 * Base for Foothold's entry transitions. They go in the Mover component's global Transitions list,
 * so the project's own modes (GASP's Walking and Falling, for example) need no changes. Each one only
 * fires from the modes listed in AllowedFromModes, and only into a target mode that's registered.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class FOOTHOLD_API UFootholdTransitionBase : public UBaseMovementModeTransition
{
	GENERATED_BODY()

public:
	UFootholdTransitionBase();

	/** The mode this transition enters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold")
	FName TargetMode;

	/** The transition only fires while the Mover is in one of these modes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold")
	TArray<FName> AllowedFromModes;

protected:
	/** True when the current mode is allowed and the target mode exists on this Mover. */
	bool CanFire(const FSimulationTickParams& Params) const;

	const UFootholdSharedSettings* GetFootholdSettings() const;
	FVector GetUp() const;
	FFootholdQueryContext MakeContext(const FSimulationTickParams& Params) const;

	/** Velocity at the start of the tick, in world space. */
	static FVector GetStartVelocity(const FMoverTickStartData& StartState);

	/** The planar move input, or the planar facing when there's no input and bUseFacingWithoutInput. */
	FVector GetPlanarIntent(const FSimulationTickParams& Params, bool bUseFacingWithoutInput) const;
};
