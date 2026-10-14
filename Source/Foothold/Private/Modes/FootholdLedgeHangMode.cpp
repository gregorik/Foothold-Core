// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Modes/FootholdLedgeHangMode.h"
#include "Components/PrimitiveComponent.h"
#include "FootholdBlackboard.h"
#include "FootholdInputReader.h"
#include "FootholdSharedSettings.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdLedgeHangMode)

namespace UE::Foothold::Hang
{
	// The ledge's component may drift this far (cm) between steps before the hold breaks.
	constexpr float MaxAnchorDrift = 2.f;
	// Input counts as toward/away from the wall, or along the edge, past this dot product.
	constexpr float IntentDot = 0.7f;
	constexpr float LateralDeadZone = 0.1f;
	// How far past the capsule's side the shimmy looks for more ledge.
	constexpr float LookAhead = 10.f;
	// Reach used when probing the edge from the hang position.
	constexpr float ProbeReach = 30.f;
}

UFootholdLedgeHangMode::UFootholdLedgeHangMode()
{
	GameplayTags.AddTag(Foothold_LedgeHang);
}

void UFootholdLedgeHangMode::GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	OutProposedMove = FProposedMove();
}

FVector UFootholdLedgeHangMode::GetHangLocation(const FVector& EdgePoint, const FVector& WallNormal, const FVector& Up, float CapsuleRadius, const UFootholdSharedSettings& Settings)
{
	return EdgePoint + WallNormal * (CapsuleRadius + Settings.WallGap) - Up * Settings.HandReachAboveCenter;
}

FFootholdLedge UFootholdLedgeHangMode::ProbeEdge(const FFootholdQueryContext& Ctx, const FVector& HangCenter, const FVector& WallNormal) const
{
	FFootholdQueryContext Probe = Ctx;
	Probe.Location = HangCenter;
	const float Reach = FootholdSettings->HandReachAboveCenter + Ctx.CapsuleHalfHeight;
	return UFootholdTraceLibrary::FindLedge(Probe, -WallNormal, Reach - EdgeSearchMargin, Reach + EdgeSearchMargin, UE::Foothold::Hang::ProbeReach);
}

bool UFootholdLedgeHangMode::BeginHang(const FSimulationTickParams& Params, const FFootholdSyncState* PreviousFoothold, FFootholdSyncState& Out) const
{
	if (!FootholdSettings)
	{
		return false;
	}
	const FFootholdQueryContext Ctx = MakeContext(Params);

	FFootholdLedge Ledge;
	UMoverComponent* Mover = GetMoverComponent();
	if (!FFootholdBlackboardUtils::ConsumePendingLedge(Mover ? Mover->GetSimBlackboard_Mutable() : nullptr, Params.TimeStep, FootholdModeNames::LedgeHang, Ledge))
	{
		// No hand-off: hang from whatever is in front (or where the previous Foothold mode was holding).
		FVector Direction = Ctx.Forward;
		if (PreviousFoothold && !PreviousFoothold->SurfaceNormal.IsNearlyZero())
		{
			Direction = -PreviousFoothold->SurfaceNormal;
		}
		Ledge = ProbeEdge(Ctx, Ctx.Location, -Direction);
	}
	if (!Ledge.bValid || !Ledge.bHangClear)
	{
		return false;
	}

	Out = FFootholdSyncState();
	Out.Kind = EFootholdKind::LedgeHang;
	Out.AnchorLocation = Ledge.EdgePoint;
	Out.SurfaceNormal = Ledge.WallNormal;
	Out.TravelDirection = Ledge.EdgeDirection;
	Out.AnchorComponent = Ledge.Component;
	Out.AnchorComponentLocation = Ledge.Component.IsValid() ? Ledge.Component->GetComponentLocation() : FVector::ZeroVector;
	Out.MantleStart = Ctx.Location;	// where the settle starts
	return true;
}

void UFootholdLedgeHangMode::LetGo(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Velocity) const
{
	UMoverComponent* Mover = GetMoverComponent();
	FFootholdBlackboardUtils::SetCooldown(Mover ? Mover->GetSimBlackboard_Mutable() : nullptr, FootholdBlackboard::GrabCooldownUntilMs, Params.TimeStep, RegrabDelay);
	ExitToAir(Params, OutputState, Velocity);
}

void UFootholdLedgeHangMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	using namespace UE::Foothold::Hang;

	USceneComponent* Updated = Params.MovingComps.UpdatedComponent.Get();
	if (!Updated)
	{
		return;
	}

	const FFootholdSyncState* StartFoothold = GetStartFoothold(Params.StartState);
	FFootholdSyncState State;
	if (IsContinuing(StartFoothold))
	{
		State = *StartFoothold;
	}
	else if (!BeginHang(Params, StartFoothold, State))
	{
		LetGo(Params, OutputState, FVector::ZeroVector);
		return;
	}

	const float DeltaSeconds = GetDeltaSeconds(Params);
	const FVector Up = GetUp();
	const FFootholdQueryContext Ctx = MakeContext(Params);
	const FVector Input = FVector::VectorPlaneProject(FFootholdInputReader::GetMoveInput(Params.StartState), Up);

	// The ledge moved (or was destroyed): the hold breaks.
	if (State.AnchorComponent.IsValid())
	{
		if (FVector::Dist(State.AnchorComponent->GetComponentLocation(), State.AnchorComponentLocation) > MaxAnchorDrift)
		{
			LetGo(Params, OutputState, FVector::ZeroVector);
			return;
		}
	}
	else if (!State.AnchorComponent.IsExplicitlyNull())
	{
		LetGo(Params, OutputState, FVector::ZeroVector);
		return;
	}

	// Let go.
	if (WantsDrop(Params.StartState, State.SurfaceNormal))
	{
		LetGo(Params, OutputState, State.SurfaceNormal * DropPushSpeed);
		return;
	}

	// Climb up: jump, or keep pushing toward the wall.
	const float TowardWall = FVector::DotProduct(Input, -State.SurfaceNormal);
	const float AwayFromWall = FVector::DotProduct(Input, State.SurfaceNormal);
	State.Timer = TowardWall > IntentDot ? State.Timer + DeltaSeconds : (AwayFromWall > IntentDot ? State.Timer - DeltaSeconds : 0.f);
	const bool bWantsUp = FFootholdInputReader::JumpJustPressed(Params.StartState) || (ClimbUpHoldTime > 0.f && State.Timer >= ClimbUpHoldTime);
	if (bWantsUp && GetMoverComponent()->FindMovementModeByName(FootholdModeNames::Mantle))
	{
		const FFootholdLedge Ledge = ProbeEdge(Ctx, Ctx.Location, State.SurfaceNormal);
		if (Ledge.bValid && Ledge.bCanStand)
		{
			FFootholdBlackboardUtils::SetPendingLedge(GetMoverComponent()->GetSimBlackboard_Mutable(), Params.TimeStep, FootholdModeNames::Mantle, Ledge);
			CommitHold(Params, OutputState, FVector::ZeroVector, Updated->GetComponentQuat());
			FFootholdSyncState& Out = WriteFoothold(OutputState, &State);
			Out.Timer = 0.f;
			ExitTo(OutputState, FootholdModeNames::Mantle, Params.TimeStep.StepMs);
			return;
		}
	}

	// Climb down onto a climbable wall below.
	if (ClimbDownHoldTime > 0.f && -State.Timer >= ClimbDownHoldTime && GetMoverComponent()->FindMovementModeByName(FootholdModeNames::Climbing))
	{
		const FFootholdWallHit Wall = UFootholdTraceLibrary::FindWall(Ctx, -State.SurfaceNormal, FootholdSettings->WallGap + 15.f);
		if (Wall.bHit && Wall.bClimbable)
		{
			CommitHold(Params, OutputState, FVector::ZeroVector, Updated->GetComponentQuat());
			FFootholdSyncState& Out = WriteFoothold(OutputState, &State);
			Out.Timer = 0.f;
			ExitTo(OutputState, FootholdModeNames::Climbing, Params.TimeStep.StepMs);
			return;
		}
	}

	// Shimmy along the edge, re-anchoring so the hang follows the ledge; stop where it ends.
	const float Lateral = FVector::DotProduct(Input, State.TravelDirection);
	if (FMath::Abs(Lateral) > LateralDeadZone && State.TimeInMode >= SettleTime)
	{
		const FVector Side = State.TravelDirection * FMath::Sign(Lateral);
		const FVector Here = GetHangLocation(State.AnchorLocation, State.SurfaceNormal, Up, Ctx.CapsuleRadius, *FootholdSettings);
		const FFootholdLedge Ahead = ProbeEdge(Ctx, Here + Side * (Ctx.CapsuleRadius + LookAhead), State.SurfaceNormal);
		const bool bAheadOk = Ahead.bValid && Ahead.bHangClear
			&& FMath::Abs(FVector::DotProduct(Ahead.EdgePoint - State.AnchorLocation, Up)) <= MaxEdgeStep;
		if (bAheadOk)
		{
			const FVector Step = State.TravelDirection * (Lateral * ShimmySpeed * DeltaSeconds);
			const FFootholdLedge Next = ProbeEdge(Ctx, Here + Step, State.SurfaceNormal);
			if (Next.bValid && Next.bHangClear && FMath::Abs(FVector::DotProduct(Next.EdgePoint - State.AnchorLocation, Up)) <= MaxEdgeStep)
			{
				State.AnchorLocation = Next.EdgePoint;
				State.SurfaceNormal = Next.WallNormal;
				// Keep the edge direction's sign stable so "right" stays right.
				State.TravelDirection = FVector::DotProduct(Next.EdgeDirection, State.TravelDirection) >= 0.f ? Next.EdgeDirection : -Next.EdgeDirection;
				if (Next.Component.IsValid())
				{
					State.AnchorComponent = Next.Component;
					State.AnchorComponentLocation = Next.Component->GetComponentLocation();
				}
			}
		}
	}

	// Settle into the hang position, then hold it.
	const FVector HangLocation = GetHangLocation(State.AnchorLocation, State.SurfaceNormal, Up, Ctx.CapsuleRadius, *FootholdSettings);
	FVector Target = HangLocation;
	const float NewTime = State.TimeInMode + DeltaSeconds;
	if (SettleTime > 0.f && NewTime < SettleTime)
	{
		const float Alpha = FMath::SmoothStep(0.f, 1.f, NewTime / SettleTime);
		Target = FMath::Lerp(State.MantleStart, HangLocation, Alpha);
	}

	CommitMove(Params, OutputState, Target - Updated->GetComponentLocation(), MakeFacing(-State.SurfaceNormal), /*bSlide*/ false);
	FFootholdSyncState& Out = WriteFoothold(OutputState, &State);
	Out.TimeInMode = NewTime;
	Out.Phase = NewTime < SettleTime ? 0 : 1;
}
