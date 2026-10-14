// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "FootholdMoverComponent.generated.h"

/**
 * Mover's character component with every Foothold mode and entry transition already added
 * (Walking, Falling and Flying come from UCharacterMoverComponent). Use it on new C++ or Blueprint
 * pawns. For an existing pawn, call FFootholdDefaults::AddToMover (Foothold Pro also has a one-click editor setup).
 */
UCLASS(ClassGroup = (Foothold), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class FOOTHOLD_API UFootholdMoverComponent : public UCharacterMoverComponent
{
	GENERATED_BODY()

public:
	UFootholdMoverComponent();
};
