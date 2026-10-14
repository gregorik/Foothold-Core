// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Transitions/FootholdTransitionBase.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "FootholdInputReader.h"
#include "FootholdSharedSettings.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoverSimulationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdTransitionBase)

UFootholdTransitionBase::UFootholdTransitionBase()
{
	SharedSettingsClasses.Add(UFootholdSharedSettings::StaticClass());
	SharedSettingsClasses.Add(UCommonLegacyMovementSettings::StaticClass());
	bSupportsAsync = false;
}

bool UFootholdTransitionBase::CanFire(const FSimulationTickParams& Params) const
{
	if (TargetMode.IsNone() || !AllowedFromModes.Contains(Params.StartState.SyncState.MovementMode))
	{
		return false;
	}
	const UMoverComponent* Mover = GetMoverComponent();
	return Mover && Mover->FindMovementModeByName(TargetMode) != nullptr && GetFootholdSettings() != nullptr;
}

const UFootholdSharedSettings* UFootholdTransitionBase::GetFootholdSettings() const
{
	const UMoverComponent* Mover = GetMoverComponent();
	return Mover ? Mover->FindSharedSettings<UFootholdSharedSettings>() : nullptr;
}

FVector UFootholdTransitionBase::GetUp() const
{
	const UMoverComponent* Mover = GetMoverComponent();
	return Mover ? Mover->GetUpDirection() : FVector::UpVector;
}

FFootholdQueryContext UFootholdTransitionBase::MakeContext(const FSimulationTickParams& Params) const
{
	return FFootholdQueryContext::Make(Params.MovingComps, GetFootholdSettings(), GetUp());
}

FVector UFootholdTransitionBase::GetStartVelocity(const FMoverTickStartData& StartState)
{
	const FMoverDefaultSyncState* Sync = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	return Sync ? Sync->GetVelocity_WorldSpace() : FVector::ZeroVector;
}

FVector UFootholdTransitionBase::GetPlanarIntent(const FSimulationTickParams& Params, bool bUseFacingWithoutInput) const
{
	const FVector Up = GetUp();
	const FVector Input = FVector::VectorPlaneProject(FFootholdInputReader::GetMoveInput(Params.StartState), Up);
	if (!Input.IsNearlyZero(0.1f))
	{
		return Input.GetSafeNormal();
	}
	if (bUseFacingWithoutInput)
	{
		if (const USceneComponent* Updated = Params.MovingComps.UpdatedComponent.Get())
		{
			return FVector::VectorPlaneProject(Updated->GetForwardVector(), Up).GetSafeNormal();
		}
	}
	return FVector::ZeroVector;
}
