// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Tests/FootholdTestCharacter.h"
#include "Backends/MoverStandaloneLiaison.h"
#include "FootholdMoverComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdTestCharacter)

AFootholdTestCharacter::AFootholdTestCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Mover->BackendClass = UMoverStandaloneLiaisonComponent::StaticClass();
}
