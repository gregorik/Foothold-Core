// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdDefaults.h"
#include "MovementMode.h"
#include "MovementModeTransition.h"
#include "MoverComponent.h"
#include "Modes/FootholdLadderMode.h"
#include "Modes/FootholdLedgeHangMode.h"
#include "Modes/FootholdMantleMode.h"
#include "Transitions/FootholdLadderStartTransition.h"
#include "Transitions/FootholdLedgeGrabTransition.h"
#include "Transitions/FootholdAirMantleTransition.h"
#include "FootholdTypes.h"
#include "Transitions/FootholdTransitionBase.h"

// Foothold Core has three of Foothold's seven modes. Climbing, Glide, Wall-run and Slide are in Foothold Pro; their names,
// kinds and sync-state fields stay in the shared types, so a project can swap Core for Pro without losing data.
TArray<FFootholdDefaults::FModeEntry> FFootholdDefaults::GetModes()
{
	TArray<FModeEntry> Modes;
	Modes.Add({ FootholdModeNames::LedgeHang, UFootholdLedgeHangMode::StaticClass() });
	Modes.Add({ FootholdModeNames::Ladder, UFootholdLadderMode::StaticClass() });
	Modes.Add({ FootholdModeNames::Mantle, UFootholdMantleMode::StaticClass() });
	return Modes;
}

TArray<TSubclassOf<UBaseMovementModeTransition>> FFootholdDefaults::GetGlobalTransitions()
{
	// Order is priority: the first transition that fires wins.
	TArray<TSubclassOf<UBaseMovementModeTransition>> Transitions;
	Transitions.Add(UFootholdAirMantleTransition::StaticClass());	// low ledges: pull straight up
	Transitions.Add(UFootholdLedgeGrabTransition::StaticClass());	// ledges at hand height: hang
	Transitions.Add(UFootholdLadderStartTransition::StaticClass());	// walk into a ladder: climb it
	return Transitions;
}

TSet<FName> FFootholdDefaults::GetCoveredModes(const UMoverComponent& /*Mover*/)
{
	// Pro leaves its slide out of pawns with their own "Sliding" mode (GASP's); Core has no mode a pawn could already have.
	return TSet<FName>();
}

int32 FFootholdDefaults::AddToMover(UMoverComponent& Mover, EObjectFlags ObjectFlags, const TSet<FName>* SkipModes)
{
	auto Skipped = [SkipModes](FName Mode) { return SkipModes && SkipModes->Contains(Mode); };
	int32 Added = 0;
	for (const FModeEntry& Entry : GetModes())
	{
		if (!Mover.MovementModes.Contains(Entry.Name) && Entry.Class && !Skipped(Entry.Name))
		{
			UBaseMovementMode* Mode = NewObject<UBaseMovementMode>(&Mover, Entry.Class, MakeUniqueObjectName(&Mover, Entry.Class, *FString::Printf(TEXT("Foothold%sMode"), *Entry.Name.ToString())), ObjectFlags);
			Mover.MovementModes.Add(Entry.Name, Mode);
			++Added;
		}
	}

	for (const TSubclassOf<UBaseMovementModeTransition>& TransitionClass : GetGlobalTransitions())
	{
		const bool bHasOne = Mover.Transitions.ContainsByPredicate([&TransitionClass](const UBaseMovementModeTransition* Existing)
		{
			return Existing && Existing->GetClass() == TransitionClass;
		});
		const UFootholdTransitionBase* Defaults = TransitionClass ? Cast<UFootholdTransitionBase>(TransitionClass->GetDefaultObject()) : nullptr;
		if (!bHasOne && TransitionClass && !(Defaults && Skipped(Defaults->TargetMode)))
		{
			Mover.Transitions.Add(NewObject<UBaseMovementModeTransition>(&Mover, TransitionClass, NAME_None, ObjectFlags));
			++Added;
		}
	}
	return Added;
}
