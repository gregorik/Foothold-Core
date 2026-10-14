// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Transitions/FootholdLadderStartTransition.h"
#include "FootholdBlackboard.h"
#include "FootholdLadderComponent.h"
#include "FootholdTypes.h"
#include "MoverSimulationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdLadderStartTransition)

UFootholdLadderStartTransition::UFootholdLadderStartTransition()
{
	TargetMode = FootholdModeNames::Ladder;
	AllowedFromModes.Add(DefaultModeNames::Walking);
	AllowedFromModes.Add(DefaultModeNames::Falling);
	AllowedFromModes.Add(FootholdModeNames::Gliding);
}

FTransitionEvalResult UFootholdLadderStartTransition::Evaluate_Implementation(const FSimulationTickParams& Params) const
{
	if (!CanFire(Params) || FFootholdBlackboardUtils::IsCoolingDown(Params.SimBlackboard, FootholdBlackboard::LadderCooldownUntilMs, Params.TimeStep))
	{
		return FTransitionEvalResult::NoTransition;
	}

	const bool bOnGround = Params.StartState.SyncState.MovementMode == DefaultModeNames::Walking;
	float Position = 0.f;
	const UFootholdLadderComponent* Ladder = UFootholdTraceLibrary::FindLadder(MakeContext(Params), bOnGround ? GroundReach : AirReach, Position);
	if (!Ladder)
	{
		return FTransitionEvalResult::NoTransition;
	}

	if (bOnGround)
	{
		const FVector Intent = GetPlanarIntent(Params, /*bUseFacingWithoutInput*/ false);
		if (Intent.IsNearlyZero() || FVector::DotProduct(Intent, -Ladder->GetFacing()) < MinTowardLadder)
		{
			return FTransitionEvalResult::NoTransition;
		}
	}
	return FTransitionEvalResult(TargetMode);
}
