// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdBlackboard.h"
#include "MoveLibrary/MoverBlackboard.h"

namespace FootholdBlackboard
{
	const FName PendingLedge = TEXT("Foothold.PendingLedge");
	const FName MantleCooldownUntilMs = TEXT("Foothold.MantleCooldownUntilMs");
	const FName GrabCooldownUntilMs = TEXT("Foothold.GrabCooldownUntilMs");
	const FName WallRunCooldownUntilMs = TEXT("Foothold.WallRunCooldownUntilMs");
	const FName ClimbCooldownUntilMs = TEXT("Foothold.ClimbCooldownUntilMs");
	const FName LadderCooldownUntilMs = TEXT("Foothold.LadderCooldownUntilMs");
	const FName GlideCooldownUntilMs = TEXT("Foothold.GlideCooldownUntilMs");
	const FName SlideCooldownUntilMs = TEXT("Foothold.SlideCooldownUntilMs");
}

void FFootholdBlackboardUtils::SetPendingLedge(UMoverBlackboard* Blackboard, const FMoverTimeStep& TimeStep, FName TargetMode, const FFootholdLedge& Ledge)
{
	if (!Blackboard)
	{
		return;
	}
	FFootholdPendingLedge Pending;
	Pending.Ledge = Ledge;
	Pending.TargetMode = TargetMode;
	Pending.SimTimeMs = TimeStep.BaseSimTimeMs;
	Blackboard->Set(FootholdBlackboard::PendingLedge, Pending);
}

bool FFootholdBlackboardUtils::ConsumePendingLedge(UMoverBlackboard* Blackboard, const FMoverTimeStep& TimeStep, FName TargetMode, FFootholdLedge& OutLedge)
{
	if (!Blackboard)
	{
		return false;
	}
	FFootholdPendingLedge Pending;
	if (!Blackboard->TryGet(FootholdBlackboard::PendingLedge, Pending))
	{
		return false;
	}
	Blackboard->Invalidate(FootholdBlackboard::PendingLedge);
	if (Pending.TargetMode != TargetMode || !FMath::IsNearlyEqual(Pending.SimTimeMs, TimeStep.BaseSimTimeMs, 0.01))
	{
		return false;
	}
	OutLedge = Pending.Ledge;
	return true;
}

void FFootholdBlackboardUtils::SetCooldown(UMoverBlackboard* Blackboard, FName Key, const FMoverTimeStep& TimeStep, float Seconds)
{
	if (Blackboard)
	{
		Blackboard->Set(Key, TimeStep.BaseSimTimeMs + Seconds * 1000.0);
	}
}

bool FFootholdBlackboardUtils::IsCoolingDown(const UMoverBlackboard* Blackboard, FName Key, const FMoverTimeStep& TimeStep)
{
	double Until = 0.0;
	return Blackboard && Blackboard->TryGet(Key, Until) && TimeStep.BaseSimTimeMs < Until;
}
