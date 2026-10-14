// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "FootholdCharacter.h"
#include "FootholdTestCharacter.generated.h"

/** Test-only: AFootholdCharacter on Mover's standalone backend, so a plain test world can simulate it. */
UCLASS(NotBlueprintable, HideDropdown)
class FOOTHOLD_API AFootholdTestCharacter : public AFootholdCharacter
{
	GENERATED_BODY()

public:
	AFootholdTestCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
