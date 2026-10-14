// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Transitions/FootholdTransitionBase.h"
#include "FootholdLadderStartTransition.generated.h"

/**
 * Next to a ladder (in front of it, within reach of its climbing line): walking toward it, or
 * falling past it, gets on.
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTHOLD_API UFootholdLadderStartTransition : public UFootholdTransitionBase
{
	GENERATED_BODY()

public:
	UFootholdLadderStartTransition();

	virtual FTransitionEvalResult Evaluate_Implementation(const FSimulationTickParams& Params) const override;

	/** From the ground: how far from the ladder's climbing line the capsule may be. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm"))
	float GroundReach = 60.f;

	/** From the air: how far from the ladder's climbing line the capsule may be. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm"))
	float AirReach = 50.f;

	/** From the ground, move input must point at the ladder at least this much (dot product). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Ladder", meta = (ClampMin = "0", ClampMax = "1"))
	float MinTowardLadder = 0.5f;
};
