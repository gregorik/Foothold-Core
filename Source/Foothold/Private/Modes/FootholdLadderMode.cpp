// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Modes/FootholdLadderMode.h"
#include "FootholdBlackboard.h"
#include "FootholdInputReader.h"
#include "FootholdLadderComponent.h"
#include "FootholdSharedSettings.h"
#include "Modes/FootholdMantleMode.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdLadderMode)

namespace UE::Foothold::LadderClimb
{
	constexpr float InputDeadZone = 0.1f;
	// Mantling off the top looks for edges between these heights above the feet.
	constexpr float TopMantleMinHeight = 20.f;
	constexpr float TopMantleMaxHeight = 260.f;
	constexpr float TopMantleReach = 60.f;
}

UFootholdLadderMode::UFootholdLadderMode()
{
	GameplayTags.AddTag(Foothold_Ladder);
}

void UFootholdLadderMode::GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	OutProposedMove = FProposedMove();
}

void UFootholdLadderMode::Leave(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Velocity) const
{
	UMoverComponent* Mover = GetMoverComponent();
	FFootholdBlackboardUtils::SetCooldown(Mover ? Mover->GetSimBlackboard_Mutable() : nullptr, FootholdBlackboard::LadderCooldownUntilMs, Params.TimeStep, RegrabDelay);
	ExitToAir(Params, OutputState, Velocity);
}

bool UFootholdLadderMode::TryMantleOffTop(const FSimulationTickParams& Params, const FFootholdSyncState& State, FMoverTickEndData& OutputState) const
{
	using namespace UE::Foothold::LadderClimb;

	UMoverComponent* Mover = GetMoverComponent();
	const UFootholdLadderComponent* Ladder = State.Ladder.Get();
	if (!Ladder || !Ladder->bMantleOffTop || !Mover->FindMovementModeByName(FootholdModeNames::Mantle))
	{
		return false;
	}

	const FFootholdQueryContext Ctx = MakeContext(Params);
	const FFootholdLedge Ledge = UFootholdTraceLibrary::FindLedge(Ctx, -Ladder->GetFacing(), TopMantleMinHeight, TopMantleMaxHeight, TopMantleReach);
	if (!UFootholdMantleMode::IsMantleable(Ctx, Ledge))
	{
		return false;
	}

	FFootholdBlackboardUtils::SetPendingLedge(Mover->GetSimBlackboard_Mutable(), Params.TimeStep, FootholdModeNames::Mantle, Ledge);
	CommitHold(Params, OutputState, FVector::ZeroVector, Params.MovingComps.UpdatedComponent->GetComponentQuat());
	WriteFoothold(OutputState, &State);
	ExitTo(OutputState, FootholdModeNames::Mantle, Params.TimeStep.StepMs);
	return true;
}

void UFootholdLadderMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	using namespace UE::Foothold::LadderClimb;

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
	else
	{
		float Position = 0.f;
		UFootholdLadderComponent* Found = FootholdSettings ? UFootholdTraceLibrary::FindLadder(MakeContext(Params), GrabDistance, Position) : nullptr;
		if (!Found)
		{
			Leave(Params, OutputState, FVector::ZeroVector);
			return;
		}
		State.Kind = EFootholdKind::Ladder;
		State.Ladder = Found;
		State.LadderPosition = Position;
		State.SurfaceNormal = Found->GetFacing();
		State.MantleStart = Updated->GetComponentLocation();	// where the entry move starts
	}

	const UFootholdLadderComponent* Ladder = State.Ladder.Get();
	if (!Ladder)
	{
		Leave(Params, OutputState, FVector::ZeroVector);	// the ladder was destroyed
		return;
	}

	const float DeltaSeconds = GetDeltaSeconds(Params);
	const FVector Up = GetUp();
	const FVector Facing = Ladder->GetFacing();
	State.SurfaceNormal = Facing;

	if (WantsDrop(Params.StartState, Facing) || FFootholdInputReader::JumpJustPressed(Params.StartState))
	{
		Leave(Params, OutputState, Facing * JumpOffSpeed + Up * JumpOffUpSpeed);
		return;
	}

	const FVector Input = FVector::VectorPlaneProject(FFootholdInputReader::GetMoveInput(Params.StartState), Up);
	const float Climb = FVector::DotProduct(Input, -Facing);

	if (FMath::Abs(Climb) > InputDeadZone)
	{
		// Hands at the top: mantle off. At the bottom: step onto the floor.
		if (Climb > 0.f && State.LadderPosition >= FMath::Max(Ladder->Height - TopOutBelowTop, 0.f) - KINDA_SMALL_NUMBER && TryMantleOffTop(Params, State, OutputState))
		{
			return;
		}
		if (Climb < 0.f)
		{
			const FFootholdQueryContext Ctx = MakeContext(Params);
			FHitResult Floor;
			if (FootholdSettings && UFootholdTraceLibrary::TraceDown(Ctx, Ctx.Location, Ctx.CapsuleHalfHeight + FloorStepOffDistance, Floor)
				&& UFootholdTraceLibrary::IsWalkable(Floor.ImpactNormal, Up, FootholdSettings->MaxWalkSlopeCosine))
			{
				CommitHold(Params, OutputState, FVector::ZeroVector, Updated->GetComponentQuat());
				ExitTo(OutputState, GetGroundModeName(), Params.TimeStep.StepMs);
				return;
			}
			if (State.LadderPosition <= KINDA_SMALL_NUMBER)
			{
				Leave(Params, OutputState, FVector::ZeroVector);	// bottom of a ladder that ends in the air
				return;
			}
		}
		State.LadderPosition = FMath::Clamp(State.LadderPosition + Climb * ClimbSpeed * DeltaSeconds, 0.f, Ladder->Height);
	}
	else
	{
		const float Rung = Ladder->GetNearestRungPosition(State.LadderPosition);
		State.LadderPosition = FMath::FInterpConstantTo(State.LadderPosition, Rung, DeltaSeconds, RungSettleSpeed);
	}

	FVector Target = Ladder->GetClimbLocation(State.LadderPosition);
	const float NewTime = State.TimeInMode + DeltaSeconds;
	if (EntryTime > 0.f && NewTime < EntryTime)
	{
		Target = FMath::Lerp(State.MantleStart, Target, FMath::SmoothStep(0.f, 1.f, NewTime / EntryTime));
	}

	CommitMove(Params, OutputState, Target - Updated->GetComponentLocation(), MakeFacing(-Facing), /*bSlide*/ false);
	FFootholdSyncState& Out = WriteFoothold(OutputState, &State);
	Out.TimeInMode = NewTime;
	Out.AnchorLocation = Ladder->GetBottom() + Ladder->GetAxis() * State.LadderPosition;
}
