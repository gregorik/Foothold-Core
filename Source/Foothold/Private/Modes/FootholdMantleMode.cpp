// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Modes/FootholdMantleMode.h"
#include "FootholdBlackboard.h"
#include "FootholdInputReader.h"
#include "FootholdSharedSettings.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdMantleMode)

namespace UE::Foothold::Mantle
{
	// Lowest edge the mode accepts when it looks for the ledge itself.
	constexpr float MinEdgeHeight = 20.f;
	// How far ahead the mode looks for the wall on entry.
	constexpr float EntryReach = 45.f;

	static float Ease(float X)
	{
		X = FMath::Clamp(X, 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}
}

UFootholdMantleMode::UFootholdMantleMode()
{
	GameplayTags.AddTag(Foothold_Mantle);
	GameplayTags.AddTag(Mover_SkipAnimRootMotion);
}

void UFootholdMantleMode::GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	// The path is evaluated in SimulationTick.
	OutProposedMove = FProposedMove();
}

FVector UFootholdMantleMode::EvaluatePath(const FVector& Start, const FVector& End, const FVector& Up, float InRiseShare, float Alpha)
{
	using namespace UE::Foothold::Mantle;
	const FVector RiseEnd = Start + Up * FVector::DotProduct(End - Start, Up);
	if (Alpha <= InRiseShare)
	{
		return FMath::Lerp(Start, RiseEnd, Ease(Alpha / InRiseShare));
	}
	return FMath::Lerp(RiseEnd, End, Ease((Alpha - InRiseShare) / (1.f - InRiseShare)));
}

bool UFootholdMantleMode::IsMantleable(const FFootholdQueryContext& Ctx, const FFootholdLedge& Ledge)
{
	if (!Ledge.bValid || !Ledge.bCanStand)
	{
		return false;
	}
	// The capsule rises straight up in front of the wall first: that column must be free.
	const FVector RiseEnd = Ctx.Location + Ctx.Up * FVector::DotProduct(Ledge.StandLocation - Ctx.Location, Ctx.Up);
	return UFootholdTraceLibrary::IsCapsuleClear(Ctx, RiseEnd);
}

bool UFootholdMantleMode::BeginMantle(const FSimulationTickParams& Params, const FFootholdSyncState* PreviousFoothold, FFootholdSyncState& Out) const
{
	using namespace UE::Foothold::Mantle;

	if (!FootholdSettings)
	{
		return false;
	}
	const FFootholdQueryContext Ctx = MakeContext(Params);
	const FVector Up = Ctx.Up;

	// Look where the previous Foothold mode was holding on, else where the input points, else ahead.
	FVector Direction = FVector::ZeroVector;
	if (PreviousFoothold && !PreviousFoothold->SurfaceNormal.IsNearlyZero())
	{
		Direction = -PreviousFoothold->SurfaceNormal;
	}
	if (FVector::VectorPlaneProject(Direction, Up).IsNearlyZero())
	{
		Direction = FFootholdInputReader::GetMoveInput(Params.StartState);
	}
	if (FVector::VectorPlaneProject(Direction, Up).IsNearlyZero())
	{
		Direction = Ctx.Forward;
	}

	// An entry transition that already found the ledge this step hands it over; otherwise look.
	FFootholdLedge Ledge;
	UMoverComponent* Mover = GetMoverComponent();
	if (!FFootholdBlackboardUtils::ConsumePendingLedge(Mover ? Mover->GetSimBlackboard_Mutable() : nullptr, Params.TimeStep, FootholdModeNames::Mantle, Ledge))
	{
		Ledge = UFootholdTraceLibrary::FindLedge(Ctx, Direction, MinEdgeHeight, MaxHeight, EntryReach);
	}
	if (!IsMantleable(Ctx, Ledge))
	{
		return false;
	}

	const float Rise = FMath::Max(0.f, FVector::DotProduct(Ledge.StandLocation - Ctx.Location, Up));
	const float Over = FVector::VectorPlaneProject(Ledge.StandLocation - Ctx.Location, Up).Size();
	float RiseTime = FMath::Max(MinPhaseDuration, Rise / RiseSpeed);
	float OverTime = FMath::Max(MinPhaseDuration, Over / OverSpeed);
	const float Scale = FMath::Min(1.f, MaxDuration / (RiseTime + OverTime));
	RiseTime *= Scale;
	OverTime *= Scale;

	Out = FFootholdSyncState();
	Out.Kind = EFootholdKind::Mantle;
	Out.MantleStart = Ctx.Location;
	Out.MantleEnd = Ledge.StandLocation;
	Out.MantleDuration = RiseTime + OverTime;
	Out.Timer = RiseTime;	// how much of the duration is the rise
	Out.SurfaceNormal = Ledge.WallNormal;
	Out.AnchorLocation = Ledge.EdgePoint;
	return true;
}

void UFootholdMantleMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	USceneComponent* Updated = Params.MovingComps.UpdatedComponent.Get();
	if (!Updated)
	{
		return;
	}

	const FFootholdSyncState* StartFoothold = GetStartFoothold(Params.StartState);
	const float DeltaSeconds = GetDeltaSeconds(Params);

	FFootholdSyncState State;
	if (IsContinuing(StartFoothold))
	{
		State = *StartFoothold;
	}
	else if (!BeginMantle(Params, StartFoothold, State))
	{
		// Nothing to mantle onto: back to the air, and keep the entry transitions from retrying at once.
		UMoverComponent* Mover = GetMoverComponent();
		FFootholdBlackboardUtils::SetCooldown(Mover ? Mover->GetSimBlackboard_Mutable() : nullptr, FootholdBlackboard::MantleCooldownUntilMs, Params.TimeStep, RetryCooldown);
		const FMoverDefaultSyncState* StartSync = Params.StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
		ExitToAir(Params, OutputState, StartSync ? StartSync->GetVelocity_WorldSpace() : FVector::ZeroVector);
		return;
	}

	const FVector Up = GetUp();
	const FQuat Facing = MakeFacing(-State.SurfaceNormal);
	const float NewTime = State.TimeInMode + DeltaSeconds;
	const float Alpha = State.MantleDuration > 0.f ? FMath::Min(1.f, NewTime / State.MantleDuration) : 1.f;
	const float RiseShare = State.MantleDuration > 0.f ? FMath::Clamp(State.Timer / State.MantleDuration, 0.05f, 0.95f) : 0.5f;
	const FVector Target = EvaluatePath(State.MantleStart, State.MantleEnd, Up, RiseShare, Alpha);

	CommitMove(Params, OutputState, Target - Updated->GetComponentLocation(), Facing, /*bSlide*/ false);

	FFootholdSyncState& Out = WriteFoothold(OutputState, &State);
	Out.TimeInMode = NewTime;
	Out.Phase = Alpha <= RiseShare ? 0 : 1;

	if (FVector::Dist(Updated->GetComponentLocation(), Target) > BlockedTolerance)
	{
		UMoverComponent* Mover = GetMoverComponent();
		FFootholdBlackboardUtils::SetCooldown(Mover ? Mover->GetSimBlackboard_Mutable() : nullptr, FootholdBlackboard::MantleCooldownUntilMs, Params.TimeStep, RetryCooldown);
		ExitTo(OutputState, GetAirModeName(), 0.f);
		return;
	}

	if (Alpha >= 1.f)
	{
		const float OvershootMs = FMath::Max(0.f, (NewTime - State.MantleDuration) * 1000.f);
		CommitHold(Params, OutputState, Facing.GetForwardVector() * ExitSpeed, Facing);
		ExitTo(OutputState, GetGroundModeName(), FMath::Min(OvershootMs, Params.TimeStep.StepMs));
	}
}
