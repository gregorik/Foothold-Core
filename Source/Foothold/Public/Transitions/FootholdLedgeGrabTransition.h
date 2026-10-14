// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Transitions/FootholdTransitionBase.h"
#include "FootholdLedgeGrabTransition.generated.h"

/**
 * In the air with a ledge at hand height in front (or in the move direction): grab it and hang.
 * Doesn't fire while rising fast, or for a moment after letting go of a ledge.
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTHOLD_API UFootholdLedgeGrabTransition : public UFootholdTransitionBase
{
	GENERATED_BODY()

public:
	UFootholdLedgeGrabTransition();

	virtual FTransitionEvalResult Evaluate_Implementation(const FSimulationTickParams& Params) const override;

	/** Accepted edge heights extend this far below and above the hands' reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Grab", meta = (ClampMin = "0", ForceUnits = "cm"))
	float BelowReach = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Grab", meta = (ClampMin = "0", ForceUnits = "cm"))
	float AboveReach = 25.f;

	/** How far ahead of the capsule to look for the wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Grab", meta = (ClampMin = "0", ForceUnits = "cm"))
	float Reach = 30.f;

	/** No grab while rising faster than this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Grab", meta = (ForceUnits = "cm/s"))
	float MaxUpSpeed = 150.f;

	/** The facing (or move input) must point at the wall at least this much (dot product). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Grab", meta = (ClampMin = "0", ClampMax = "1"))
	float MinTowardWall = 0.5f;
};
