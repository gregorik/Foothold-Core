// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FootholdWorldSubsystem.generated.h"

class UFootholdLadderComponent;

/** Keeps the Foothold ladders in a world, so detection can find them without overlap queries or collision setup. */
UCLASS()
class FOOTHOLD_API UFootholdWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterLadder(UFootholdLadderComponent* Ladder);
	void UnregisterLadder(UFootholdLadderComponent* Ladder);

	/** Registered ladders. An entry can go stale if a ladder is destroyed without unregistering, so check validity. */
	const TArray<TWeakObjectPtr<UFootholdLadderComponent>>& GetLadders() const { return Ladders; }

private:
	TArray<TWeakObjectPtr<UFootholdLadderComponent>> Ladders;
};
