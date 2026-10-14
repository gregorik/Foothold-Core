// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "FootholdTraceLibrary.h"
#include "MoverTypes.h"

class UMoverBlackboard;

/**
 * Hand-offs between Foothold transitions and modes inside one simulation step, through Mover's sim
 * blackboard. A transition that found its target stores it stamped with the step's time; the mode it
 * enters (later in the same step) uses it instead of probing again, so both agree on the target.
 */
namespace FootholdBlackboard
{
	/** FFootholdPendingLedge: a ledge an entry transition found for the mode it enters. */
	extern FOOTHOLD_API const FName PendingLedge;

	/** double: sim time (ms) before which a given transition kind must not fire again (see Cooldown*). */
	extern FOOTHOLD_API const FName MantleCooldownUntilMs;
	extern FOOTHOLD_API const FName GrabCooldownUntilMs;
	extern FOOTHOLD_API const FName WallRunCooldownUntilMs;
	extern FOOTHOLD_API const FName ClimbCooldownUntilMs;
	extern FOOTHOLD_API const FName LadderCooldownUntilMs;
	extern FOOTHOLD_API const FName GlideCooldownUntilMs;
	extern FOOTHOLD_API const FName SlideCooldownUntilMs;
}

/** A ledge found by an entry transition, for the mode it's entering. */
struct FFootholdPendingLedge
{
	FFootholdLedge Ledge;
	FName TargetMode;
	double SimTimeMs = -1.0;
};

struct FOOTHOLD_API FFootholdBlackboardUtils
{
	static void SetPendingLedge(UMoverBlackboard* Blackboard, const FMoverTimeStep& TimeStep, FName TargetMode, const FFootholdLedge& Ledge);

	/** The pending ledge for TargetMode stored during this time step, if any. */
	static bool ConsumePendingLedge(UMoverBlackboard* Blackboard, const FMoverTimeStep& TimeStep, FName TargetMode, FFootholdLedge& OutLedge);

	static void SetCooldown(UMoverBlackboard* Blackboard, FName Key, const FMoverTimeStep& TimeStep, float Seconds);
	static bool IsCoolingDown(const UMoverBlackboard* Blackboard, FName Key, const FMoverTimeStep& TimeStep);
};
