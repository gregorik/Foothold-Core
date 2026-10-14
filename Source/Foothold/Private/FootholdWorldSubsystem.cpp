// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdWorldSubsystem.h"
#include "FootholdLadderComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdWorldSubsystem)

void UFootholdWorldSubsystem::RegisterLadder(UFootholdLadderComponent* Ladder)
{
	Ladders.RemoveAll([](const TWeakObjectPtr<UFootholdLadderComponent>& Entry) { return !Entry.IsValid(); });
	Ladders.AddUnique(Ladder);
}

void UFootholdWorldSubsystem::UnregisterLadder(UFootholdLadderComponent* Ladder)
{
	Ladders.RemoveAll([Ladder](const TWeakObjectPtr<UFootholdLadderComponent>& Entry) { return !Entry.IsValid() || Entry.Get() == Ladder; });
}
