// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdLadderComponent.h"
#include "FootholdLadder.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "FootholdWorldSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdLadderComponent)

FVector UFootholdLadderComponent::GetBottom() const
{
	return GetComponentLocation();
}

FVector UFootholdLadderComponent::GetTop() const
{
	return GetBottom() + GetAxis() * Height;
}

FVector UFootholdLadderComponent::GetAxis() const
{
	return GetUpVector();
}

FVector UFootholdLadderComponent::GetFacing() const
{
	return GetForwardVector();
}

FVector UFootholdLadderComponent::GetRight() const
{
	return GetRightVector();
}

float UFootholdLadderComponent::ProjectToLadder(const FVector& WorldLocation) const
{
	const float Along = FVector::DotProduct(WorldLocation - GetBottom(), GetAxis());
	return FMath::Clamp(Along, 0.f, Height);
}

FVector UFootholdLadderComponent::GetClimbLocation(float LadderPosition) const
{
	return GetBottom() + GetAxis() * FMath::Clamp(LadderPosition, 0.f, Height) + GetFacing() * StandOffDistance;
}

float UFootholdLadderComponent::GetNearestRungPosition(float LadderPosition) const
{
	const float Spacing = FMath::Max(RungSpacing, 1.f);
	return FMath::Clamp(FMath::RoundToFloat(LadderPosition / Spacing) * Spacing, 0.f, Height);
}

UFootholdLadderComponent::UFootholdLadderComponent()
{
	// The Ladder mode's sync state refers to this component, so a ladder spawned at runtime must replicate for
	// clients to resolve it (its owner has to replicate too, as AFootholdLadder does).
	SetIsReplicatedByDefault(true);
}

void UFootholdLadderComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFootholdLadderComponent, Height);
	DOREPLIFETIME(UFootholdLadderComponent, Width);
	DOREPLIFETIME(UFootholdLadderComponent, RungSpacing);
	DOREPLIFETIME(UFootholdLadderComponent, StandOffDistance);
	DOREPLIFETIME(UFootholdLadderComponent, bMantleOffTop);
}

void UFootholdLadderComponent::OnRep_Shape()
{
	if (AFootholdLadder* Owner = Cast<AFootholdLadder>(GetOwner()))
	{
		Owner->RebuildMesh();
	}
}

void UFootholdLadderComponent::OnRegister()
{
	Super::OnRegister();
	if (UWorld* World = GetWorld())
	{
		if (UFootholdWorldSubsystem* Subsystem = World->GetSubsystem<UFootholdWorldSubsystem>())
		{
			Subsystem->RegisterLadder(this);
		}
	}
}

void UFootholdLadderComponent::OnUnregister()
{
	if (UWorld* World = GetWorld())
	{
		if (UFootholdWorldSubsystem* Subsystem = World->GetSubsystem<UFootholdWorldSubsystem>())
		{
			Subsystem->UnregisterLadder(this);
		}
	}
	Super::OnUnregister();
}
