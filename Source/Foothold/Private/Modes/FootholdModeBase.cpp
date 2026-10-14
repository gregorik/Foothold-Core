// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Modes/FootholdModeBase.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "FootholdBlackboard.h"
#include "FootholdInputReader.h"
#include "FootholdLog.h"
#include "FootholdSharedSettings.h"
#include "MoveLibrary/MovementRecord.h"
#include "MoveLibrary/MovementUtils.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdModeBase)

UFootholdModeBase::UFootholdModeBase()
{
	SharedSettingsClasses.Add(UFootholdSharedSettings::StaticClass());
	SharedSettingsClasses.Add(UCommonLegacyMovementSettings::StaticClass());
	GameplayTags.AddTag(Foothold_Active);
	bSupportsAsync = false;
}

void UFootholdModeBase::OnRegistered(const FName ModeName, const FMoverSimContext& SimContext)
{
	Super::OnRegistered(ModeName, SimContext);

	const UMoverComponent* Mover = GetMoverComponent();
	FootholdSettings = Mover ? Mover->FindSharedSettings<UFootholdSharedSettings>() : nullptr;
	LegacySettings = Mover ? Mover->FindSharedSettings<UCommonLegacyMovementSettings>() : nullptr;
	if (!FootholdSettings)
	{
		UE_LOG(LogFoothold, Warning, TEXT("%s: no UFootholdSharedSettings on the Mover component; the mode will exit at once."), *GetPathNameSafe(this));
	}
}

void UFootholdModeBase::OnUnregistered(const FMoverSimContext& SimContext)
{
	FootholdSettings = nullptr;
	LegacySettings = nullptr;
	Super::OnUnregistered(SimContext);
}

const FFootholdSyncState* UFootholdModeBase::GetStartFoothold(const FMoverTickStartData& StartState)
{
	return StartState.SyncState.SyncStateCollection.FindDataByType<FFootholdSyncState>();
}

bool UFootholdModeBase::IsContinuing(const FFootholdSyncState* StartFoothold) const
{
	return StartFoothold && StartFoothold->Kind == GetFootholdKind();
}

FFootholdSyncState& UFootholdModeBase::WriteFoothold(FMoverTickEndData& OutputState, const FFootholdSyncState* StartFoothold) const
{
	FFootholdSyncState& Out = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FFootholdSyncState>();
	if (StartFoothold && StartFoothold != &Out)
	{
		Out = *StartFoothold;
	}
	return Out;
}

FFootholdQueryContext UFootholdModeBase::MakeContext(const FSimulationTickParams& Params) const
{
	return FFootholdQueryContext::Make(Params.MovingComps, FootholdSettings, GetUp());
}

FVector UFootholdModeBase::GetUp() const
{
	const UMoverComponent* Mover = GetMoverComponent();
	return Mover ? Mover->GetUpDirection() : FVector::UpVector;
}

FQuat UFootholdModeBase::MakeFacing(const FVector& Forward) const
{
	const FVector Up = GetUp();
	FVector Flat = FVector::VectorPlaneProject(Forward, Up).GetSafeNormal();
	if (Flat.IsNearlyZero())
	{
		Flat = FVector::VectorPlaneProject(FVector::ForwardVector, Up).GetSafeNormal();
	}
	return FRotationMatrix::MakeFromXZ(Flat, Up).ToQuat();
}

FHitResult UFootholdModeBase::CommitMove(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Delta, const FQuat& Rotation, bool bSlide) const
{
	const float DeltaSeconds = GetDeltaSeconds(Params);
	FMovementRecord Record;
	Record.SetDeltaSeconds(DeltaSeconds);

	FHitResult Hit(1.f);
	if (bSlide)
	{
		UMovementUtils::TrySafeMoveAndSlideUpdatedComponent(Params.MovingComps, Delta, Rotation, /*bSweep*/ true, Hit, ETeleportType::None, Record);
	}
	else
	{
		UMovementUtils::TrySafeMoveUpdatedComponent(Params.MovingComps, Delta, Rotation, /*bSweep*/ true, Hit, ETeleportType::None, Record);
	}

	USceneComponent* Updated = Params.MovingComps.UpdatedComponent.Get();
	const FVector Velocity = DeltaSeconds > UE_SMALL_NUMBER ? Record.GetRelevantVelocity() : FVector::ZeroVector;
	CommitHold(Params, OutputState, Velocity, Updated ? Updated->GetComponentQuat() : Rotation);
	return Hit;
}

void UFootholdModeBase::CommitHold(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Velocity, const FQuat& Rotation) const
{
	USceneComponent* Updated = Params.MovingComps.UpdatedComponent.Get();
	if (!Updated)
	{
		return;
	}

	if (!Updated->GetComponentQuat().Equals(Rotation, 1e-4f))
	{
		FHitResult RotHit;
		UMovementUtils::TrySafeMoveUpdatedComponentNoMovementRecord(Params.MovingComps, FVector::ZeroVector, Rotation, /*bSweep*/ true, RotHit, ETeleportType::None);
	}

	FMoverDefaultSyncState& OutSync = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FMoverDefaultSyncState>();
	OutSync.SetTransforms_WorldSpace(Updated->GetComponentLocation(), Updated->GetComponentRotation(), Velocity, FVector::ZeroVector, nullptr);
	Updated->ComponentVelocity = Velocity;

	if (UMoverComponent* Mover = GetMoverComponent())
	{
		if (UMoverBlackboard* Blackboard = Mover->GetSimBlackboard_Mutable())
		{
			Blackboard->Invalidate(CommonBlackboard::LastFloorResult);
			Blackboard->Invalidate(CommonBlackboard::LastFoundDynamicMovementBase);
		}
	}
}

void UFootholdModeBase::ExitTo(FMoverTickEndData& OutputState, FName NextMode, float RemainingMs)
{
	OutputState.MovementEndState.NextModeName = NextMode;
	OutputState.MovementEndState.RemainingMs = RemainingMs;
}

void UFootholdModeBase::ExitToAir(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Velocity) const
{
	// The press that ended a hold (jump off a ledge, a wall jump) must not also start a glide.
	UMoverComponent* Mover = GetMoverComponent();
	FFootholdBlackboardUtils::SetCooldown(Mover ? Mover->GetSimBlackboard_Mutable() : nullptr, FootholdBlackboard::GlideCooldownUntilMs, Params.TimeStep, GlideCooldownAfterExit);
	USceneComponent* Updated = Params.MovingComps.UpdatedComponent.Get();
	CommitHold(Params, OutputState, Velocity, Updated ? Updated->GetComponentQuat() : FQuat::Identity);
	ExitTo(OutputState, GetAirModeName(), Params.TimeStep.StepMs);
}

FName UFootholdModeBase::GetAirModeName() const
{
	return LegacySettings ? LegacySettings->AirMovementModeName : DefaultModeNames::Falling;
}

FName UFootholdModeBase::GetGroundModeName() const
{
	return LegacySettings ? LegacySettings->GroundMovementModeName : DefaultModeNames::Walking;
}

bool UFootholdModeBase::WantsDrop(const FMoverTickStartData& StartState, const FVector& FlatSurfaceNormal) const
{
	return FFootholdInputReader::WantsDrop(StartState, FlatSurfaceNormal, CrouchInputFieldPrefix);
}

bool UFootholdModeBase::IsCrouchHeld(const FMoverTickStartData& StartState) const
{
	return FFootholdInputReader::IsCrouchHeld(StartState, CrouchInputFieldPrefix);
}
