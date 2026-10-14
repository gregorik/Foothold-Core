// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class UBaseMovementMode;
class UBaseMovementModeTransition;
class UMoverComponent;

/** The standard Foothold set: which modes go under which names, and which global transitions enter them. */
struct FOOTHOLD_API FFootholdDefaults
{
	struct FModeEntry
	{
		FName Name;
		TSubclassOf<UBaseMovementMode> Class;
	};

	/** Foothold's modes and the names they're registered under. */
	static TArray<FModeEntry> GetModes();

	/** Foothold's entry transitions, added to the Mover component's global Transitions. */
	static TArray<TSubclassOf<UBaseMovementModeTransition>> GetGlobalTransitions();

	/**
	 * Adds every missing Foothold mode and transition to a Mover component (or a Mover component
	 * template, when ObjectFlags carries RF_ArchetypeObject). Existing entries are left alone, and so are
	 * the modes in SkipModes and the transitions into them. Returns how many objects were added.
	 */
	static int32 AddToMover(UMoverComponent& Mover, EObjectFlags ObjectFlags = RF_NoFlags, const TSet<FName>* SkipModes = nullptr);

	/**
	 * Foothold modes a pawn already has its own version of, which Foothold leaves out: a pawn with a "Sliding" mode (GASP's
	 * motion-matched slide) keeps its slide.
	 */
	static TSet<FName> GetCoveredModes(const UMoverComponent& Mover);
};
