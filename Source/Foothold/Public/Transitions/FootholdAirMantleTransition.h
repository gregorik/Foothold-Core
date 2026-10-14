// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Transitions/FootholdTransitionBase.h"
#include "FootholdAirMantleTransition.generated.h"

/**
 * In the air, moving toward a ledge whose top is low enough to pull up onto: mantle onto it.
 * Higher ledges are left to the ledge grab.
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTHOLD_API UFootholdAirMantleTransition : public UFootholdTransitionBase
{
	GENERATED_BODY()

public:
	UFootholdAirMantleTransition();

	virtual FTransitionEvalResult Evaluate_Implementation(const FSimulationTickParams& Params) const override;

	/** Lowest edge above the feet that counts (lower than this, walking handles it as a step). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "0", ForceUnits = "cm"))
	float MinHeight = 45.f;

	/** Highest edge above the feet that's mantled from the air. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "0", ForceUnits = "cm"))
	float MaxHeight = 150.f;

	/** How far ahead of the capsule to look for the wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "0", ForceUnits = "cm"))
	float Reach = 35.f;

	/** No mantle while rising faster than this (lets a jump carry over low obstacles first). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ForceUnits = "cm/s"))
	float MaxUpSpeed = 450.f;

	/** Move input must point at the wall at least this much (dot product). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Mantle", meta = (ClampMin = "0", ClampMax = "1"))
	float MinInputTowardWall = 0.5f;
};
