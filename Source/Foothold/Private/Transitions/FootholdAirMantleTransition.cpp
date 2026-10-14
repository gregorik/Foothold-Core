// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Transitions/FootholdAirMantleTransition.h"
#include "FootholdBlackboard.h"
#include "Modes/FootholdMantleMode.h"
#include "FootholdTypes.h"
#include "MoverSimulationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdAirMantleTransition)

UFootholdAirMantleTransition::UFootholdAirMantleTransition()
{
	TargetMode = FootholdModeNames::Mantle;
	AllowedFromModes.Add(DefaultModeNames::Falling);
	AllowedFromModes.Add(FootholdModeNames::Gliding);
}

FTransitionEvalResult UFootholdAirMantleTransition::Evaluate_Implementation(const FSimulationTickParams& Params) const
{
	if (!CanFire(Params) || FFootholdBlackboardUtils::IsCoolingDown(Params.SimBlackboard, FootholdBlackboard::MantleCooldownUntilMs, Params.TimeStep))
	{
		return FTransitionEvalResult::NoTransition;
	}

	const FVector Up = GetUp();
	if (FVector::DotProduct(GetStartVelocity(Params.StartState), Up) > MaxUpSpeed)
	{
		return FTransitionEvalResult::NoTransition;
	}

	const FVector Intent = GetPlanarIntent(Params, /*bUseFacingWithoutInput*/ false);
	if (Intent.IsNearlyZero())
	{
		return FTransitionEvalResult::NoTransition;
	}

	const FFootholdQueryContext Ctx = MakeContext(Params);
	const FFootholdLedge Ledge = UFootholdTraceLibrary::FindLedge(Ctx, Intent, MinHeight, MaxHeight, Reach);
	if (FVector::DotProduct(Intent, -Ledge.WallNormal) >= MinInputTowardWall && UFootholdMantleMode::IsMantleable(Ctx, Ledge))
	{
		FFootholdBlackboardUtils::SetPendingLedge(Params.SimBlackboard, Params.TimeStep, TargetMode, Ledge);
		return FTransitionEvalResult(TargetMode);
	}
	return FTransitionEvalResult::NoTransition;
}
