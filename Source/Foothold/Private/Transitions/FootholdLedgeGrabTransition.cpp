// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Transitions/FootholdLedgeGrabTransition.h"
#include "FootholdBlackboard.h"
#include "FootholdSharedSettings.h"
#include "FootholdTypes.h"
#include "MoverSimulationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdLedgeGrabTransition)

UFootholdLedgeGrabTransition::UFootholdLedgeGrabTransition()
{
	TargetMode = FootholdModeNames::LedgeHang;
	AllowedFromModes.Add(DefaultModeNames::Falling);
	AllowedFromModes.Add(FootholdModeNames::Gliding);
}

FTransitionEvalResult UFootholdLedgeGrabTransition::Evaluate_Implementation(const FSimulationTickParams& Params) const
{
	if (!CanFire(Params) || FFootholdBlackboardUtils::IsCoolingDown(Params.SimBlackboard, FootholdBlackboard::GrabCooldownUntilMs, Params.TimeStep))
	{
		return FTransitionEvalResult::NoTransition;
	}

	const FVector Up = GetUp();
	if (FVector::DotProduct(GetStartVelocity(Params.StartState), Up) > MaxUpSpeed)
	{
		return FTransitionEvalResult::NoTransition;
	}

	const FVector Intent = GetPlanarIntent(Params, /*bUseFacingWithoutInput*/ true);
	if (Intent.IsNearlyZero())
	{
		return FTransitionEvalResult::NoTransition;
	}

	const FFootholdQueryContext Ctx = MakeContext(Params);
	const float HandReach = Ctx.Settings->HandReachAboveCenter + Ctx.CapsuleHalfHeight;
	const FFootholdLedge Ledge = UFootholdTraceLibrary::FindLedge(Ctx, Intent, HandReach - BelowReach, HandReach + AboveReach, Reach);
	if (Ledge.bValid && Ledge.bHangClear && FVector::DotProduct(Intent, -Ledge.WallNormal) >= MinTowardWall)
	{
		FFootholdBlackboardUtils::SetPendingLedge(Params.SimBlackboard, Params.TimeStep, TargetMode, Ledge);
		return FTransitionEvalResult(TargetMode);
	}
	return FTransitionEvalResult::NoTransition;
}
