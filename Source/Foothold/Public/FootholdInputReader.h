// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"

struct FFootholdInputs;
struct FMoverDataCollection;
struct FMoverTickStartData;

/**
 * Reads what Foothold needs from a Mover input command. Everything is read from the command itself
 * (never from game-thread state), so it gives the same answers on the server, on the owning client
 * and during resimulation.
 */
struct FOOTHOLD_API FFootholdInputReader
{
	/** World-space move intent, length 0..1. Velocity-type input is normalised. */
	static FVector GetMoveInput(const FMoverTickStartData& Start);

	/** Jump was pressed this command (GASP sets this only on the press frame). */
	static bool JumpJustPressed(const FMoverTickStartData& Start);

	/** Foothold's own input block, if a UFootholdComponent (or your code) added one. */
	static const FFootholdInputs* GetFootholdInputs(const FMoverTickStartData& Start);

	/**
	 * Crouch is held: FFootholdInputs::bCrouchHeld, or a bool in a Blueprint user-defined input struct
	 * whose name starts with UserStructFieldPrefix (GASP's S_MoverCustomInputs has WantsToCrouch).
	 */
	static bool IsCrouchHeld(const FMoverTickStartData& Start, FName UserStructFieldPrefix);

	/**
	 * Let go of a hold: an explicit drop request, crouch held, or jump while the move input points away
	 * from the surface (dot with FlatSurfaceNormal of 0.5 or more).
	 */
	static bool WantsDrop(const FMoverTickStartData& Start, const FVector& FlatSurfaceNormal, FName UserStructFieldPrefix);

	/** Finds a bool whose name starts with FieldPrefix in any Blueprint user-defined struct in the collection. */
	static bool ReadUserDefinedBool(const FMoverDataCollection& Collection, FName FieldPrefix, bool& bOutValue);
};
