// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "FootholdSharedSettings.h"
#include "FootholdTraceLibrary.h"
#include "FootholdLadderComponent.h"
#include "FootholdLog.h"
#include "FootholdTypes.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "MoverComponent.h"
#include "MoverSimulationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdComponent)

UFootholdComponent::UFootholdComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	// After Mover has applied the frame's movement. Animation reads the result on its next update.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UFootholdComponent::RequestDrop()
{
	bPendingDrop = true;
}

void UFootholdComponent::RequestGlideToggle()
{
	bPendingGlideToggle = true;
}

void UFootholdComponent::RequestClimbToggle()
{
	bPendingClimbToggle = true;
}

void UFootholdComponent::SetCrouchHeld(bool bHeld)
{
	bCrouchHeld = bHeld;
}

FFootholdAnimData UFootholdComponent::GetAnimData() const
{
	FScopeLock Lock(&AnimDataLock);
	return AnimData;
}

void UFootholdComponent::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	FFootholdInputs& Inputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FFootholdInputs>();
	Inputs.bDropPressed = bPendingDrop;
	Inputs.bGlideTogglePressed = bPendingGlideToggle;
	Inputs.bClimbTogglePressed = bPendingClimbToggle;
	Inputs.bCrouchHeld = bCrouchHeld;

	bPendingDrop = false;
	bPendingGlideToggle = false;
	bPendingClimbToggle = false;
}

void UFootholdComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		MoverComponent = Owner->FindComponentByClass<UMoverComponent>();
		if (!MoverComponent)
		{
			UE_LOG(LogFoothold, Warning, TEXT("%s: UFootholdComponent needs a Mover component on the same actor. Foothold animation data stays empty."), *Owner->GetName());
		}
		else
		{
			MoverComponent->OnMovementModeChanged.AddDynamic(this, &UFootholdComponent::HandleMoverModeChanged);
			MoverComponent->OnPostFinalize.AddDynamic(this, &UFootholdComponent::HandleMoverPostFinalize);
		}
	}
}

void UFootholdComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MoverComponent)
	{
		MoverComponent->OnMovementModeChanged.RemoveDynamic(this, &UFootholdComponent::HandleMoverModeChanged);
		MoverComponent->OnPostFinalize.RemoveDynamic(this, &UFootholdComponent::HandleMoverPostFinalize);
	}
	Super::EndPlay(EndPlayReason);
}

void UFootholdComponent::HandleMoverModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !UE_LOG_ACTIVE(LogFoothold, Verbose))
	{
		return;
	}
	const TCHAR* NetMode = TEXT("Standalone");
	switch (Owner->GetNetMode())
	{
	case NM_DedicatedServer:	NetMode = TEXT("DedicatedServer"); break;
	case NM_ListenServer:		NetMode = TEXT("ListenServer"); break;
	case NM_Client:				NetMode = TEXT("Client"); break;
	default:					break;
	}
	const FString Role = StaticEnum<ENetRole>()->GetNameStringByValue(Owner->GetLocalRole()).RightChop(5);	// drop "ROLE_"
	UE_LOG(LogFoothold, Verbose, TEXT("[%s/%s] %s %s -> %s @frame %llu"), NetMode, *Role, *Owner->GetName(),
		*PreviousMovementModeName.ToString(), *NewMovementModeName.ToString(), static_cast<uint64>(GFrameCounter));
}

void UFootholdComponent::HandleMoverPostFinalize(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState)
{
	if (AnimDataFrame != GFrameCounter)
	{
		AnimDataFrame = GFrameCounter;
		UpdateAnimData(GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f);
	}
}

void UFootholdComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TryBindInput();
	// Usually done already this frame, right after Mover's step (HandleMoverPostFinalize).
	if (AnimDataFrame != GFrameCounter)
	{
		AnimDataFrame = GFrameCounter;
		UpdateAnimData(DeltaTime);
	}
}

void UFootholdComponent::UpdateAnimData(float DeltaTime)
{
	if (!MoverComponent)
	{
		return;
	}

	const FMoverSyncState& SyncState = MoverComponent->GetSyncState();
	const FFootholdSyncState* State = SyncState.SyncStateCollection.FindDataByType<FFootholdSyncState>();
	const EFootholdKind NewKind = (State && MoverComponent->HasGameplayTag(Foothold_Active, /*bExactMatch*/ true)) ? State->Kind : EFootholdKind::None;

	if (NewKind != CurrentKind)
	{
		const EFootholdKind PreviousKind = CurrentKind;
		CurrentKind = NewKind;
		HandleKindChanged(PreviousKind, NewKind);
	}

	FScopeLock Lock(&AnimDataLock);

	UpdateLedgeReach(DeltaTime);

	// Blend weight: in while a mode runs, out after it ends. The last kind stays posed while blending out.
	if (CurrentKind != EFootholdKind::None)
	{
		if (PosedKind == EFootholdKind::None)
		{
			// Grabbing out of a reach: the arms are already up at the edge, so start the pose that far in.
			AnimData.Weight = FMath::Max(AnimData.Weight, AnimData.ReachWeight);
		}
		PosedKind = CurrentKind;
		AnimData.Weight = PoseBlendInTime > 0.f ? FMath::Min(1.f, AnimData.Weight + DeltaTime / PoseBlendInTime) : 1.f;
	}
	else
	{
		AnimData.Weight = PoseBlendOutTime > 0.f ? FMath::Max(0.f, AnimData.Weight - DeltaTime / PoseBlendOutTime) : 0.f;
		if (AnimData.Weight <= 0.f)
		{
			PosedKind = EFootholdKind::None;
		}
	}
	AnimData.Kind = PosedKind;

	if (CurrentKind == EFootholdKind::None || !State)
	{
		return;
	}

	const FVector Up = MoverComponent->GetUpDirection();
	FVector Facing = MoverComponent->GetUpdatedComponentTransform().GetRotation().GetForwardVector();
	Facing = FVector::VectorPlaneProject(Facing, Up).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(Up, Facing).GetSafeNormal();
	const FVector Velocity = MoverComponent->GetVelocity();

	AnimData.Up = Up;
	AnimData.Facing = Facing;
	AnimData.SurfaceNormal = State->SurfaceNormal;
	AnimData.SurfacePoint = State->AnchorLocation;
	AnimData.LocalVelocity = FVector(FVector::DotProduct(Velocity, Right), FVector::DotProduct(Velocity, Up), FVector::DotProduct(Velocity, Facing));
	AnimData.TimeInMode = State->TimeInMode;
	AnimData.MantleAlpha = (State->Kind == EFootholdKind::Mantle && State->MantleDuration > 0.f) ? FMath::Clamp(State->TimeInMode / State->MantleDuration, 0.f, 1.f) : 0.f;
	AnimData.MantleRiseShare = (State->Kind == EFootholdKind::Mantle && State->MantleDuration > 0.f) ? FMath::Clamp(State->Timer / State->MantleDuration, 0.05f, 0.95f) : 0.5f;
	const bool bMantling = State->Kind == EFootholdKind::Mantle;
	AnimData.MantleTravel = bMantling ? State->MantleEnd - State->MantleStart : FVector::ZeroVector;
	AnimData.MantleProgress = bMantling ? MoverComponent->GetUpdatedComponentTransform().GetLocation() - State->MantleStart : FVector::ZeroVector;
	AnimData.CycleDistance += FVector::VectorPlaneProject(Velocity, State->SurfaceNormal.IsNearlyZero() ? Up : State->SurfaceNormal).Size() * DeltaTime;

	// Glide banks into turns (the glide mode keeps its turn rate, deg/s, in Timer).
	const float TargetBank = State->Kind == EFootholdKind::Glide ? FMath::Clamp(State->Timer * GlideBankPerTurnRate, -45.f, 45.f) : 0.f;
	AnimData.BankDegrees = FMath::FInterpTo(AnimData.BankDegrees, TargetBank, DeltaTime, 6.f);

	UpdateLimbTargets(*State, DeltaTime);
}

FVector UFootholdComponent::GetGripPoint(const FVector& EdgePoint, const FVector& WallNormal, const FVector& Up) const
{
	return EdgePoint + WallNormal * HandGripOut + Up * HandGripUp;
}

void UFootholdComponent::UpdateLedgeReach(float DeltaTime)
{
	float Target = 0.f;
	const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(MoverComponent->GetUpdatedComponent());
	const UFootholdSharedSettings* Settings = MoverComponent->FindSharedSettings<UFootholdSharedSettings>();
	if (bReachForLedges && CurrentKind == EFootholdKind::None && Capsule && Settings && GetWorld()
		&& MoverComponent->GetMovementModeName() == DefaultModeNames::Falling)
	{
		const FVector Up = MoverComponent->GetUpDirection();
		const FVector Planar = FVector::VectorPlaneProject(MoverComponent->GetVelocity(), Up);
		const FVector Ahead = Planar.Size() > 50.f ? Planar.GetSafeNormal() : FVector::VectorPlaneProject(Capsule->GetForwardVector(), Up).GetSafeNormal();
		if (!Ahead.IsNearlyZero())
		{
			// The ledge grab's own query, with a window that starts higher: the arms go up before the grab can happen.
			const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			const FFootholdQueryContext Ctx = FFootholdQueryContext::Make(GetWorld(), Capsule->GetComponentLocation(), Up, Ahead,
				Capsule->GetScaledCapsuleRadius(), HalfHeight, Settings, GetOwner());
			const float HandReach = Settings->HandReachAboveCenter + HalfHeight;
			const FFootholdLedge Ledge = UFootholdTraceLibrary::FindLedge(Ctx, Ahead, HandReach - LedgeReachBelow, HandReach + LedgeReachAbove, LedgeReachDistance);
			if (Ledge.bValid && Ledge.bHangClear && FVector::DotProduct(Ahead, -Ledge.WallNormal) >= 0.5f)
			{
				// Full reach until the edge is 60% of the window above the hands, then easing off to nothing at the top of it.
				const float Above = static_cast<float>(FVector::DotProduct(Ledge.EdgePoint - Ctx.GetFeet(), Up)) - HandReach;
				const float FullUpTo = LedgeReachAbove * 0.6f;
				Target = Above <= FullUpTo ? 1.f : FMath::Clamp(1.f - (Above - FullUpTo) / FMath::Max(LedgeReachAbove - FullUpTo, 1.f), 0.f, 1.f);
				const FVector Right = FVector::CrossProduct(Up, -Ledge.WallNormal).GetSafeNormal();
				const FVector Grip = GetGripPoint(Ledge.EdgePoint, Ledge.WallNormal, Up);
				AnimData.LeftReachTarget = Grip - Right * HandSpacing;
				AnimData.RightReachTarget = Grip + Right * HandSpacing;
				AnimData.ReachNormal = Ledge.WallNormal;
				AnimData.Up = Up;
				AnimData.Facing = -Ledge.WallNormal;
			}
		}
	}
	const float Step = LedgeReachBlendTime > 0.f ? DeltaTime / LedgeReachBlendTime : 1.f;
	AnimData.ReachWeight = FMath::Clamp(AnimData.ReachWeight + FMath::Clamp(Target - AnimData.ReachWeight, -Step, Step), 0.f, 1.f);
}

void UFootholdComponent::StepLimbs(float DeltaTime, const FVector (&Ideal)[4], const bool (&bUse)[4], const FVector& Velocity,
	float Stride, float Settle, TFunctionRef<FVector(int32, const FVector&)> Snap, FVector (&Out)[4], float (&OutLift)[4], bool bWholeStrides)
{
	const float Speed = static_cast<float>(Velocity.Size());
	const bool bMoving = Speed > 5.f;
	const float Half = Stride * 0.5f;
	// A move ends well before the body has gone another half step, so the pairs alternate instead of moving together.
	const float Duration = bMoving ? FMath::Min(StepTime, 0.4f * Stride / Speed) : StepTime;
	const FVector Ahead = bMoving ? Velocity / Speed : FVector::ZeroVector;

	// Every half step of travel the other pair is due to move.
	while (AnimData.CycleDistance >= NextStepAt)
	{
		bStepDue = true;
		NextStepAt += Half;
	}

	bool bMovingLimb = false;
	for (int32 Limb = 0; Limb < 4; ++Limb)
	{
		FLimbStep& Step = LimbSteps[Limb];
		if (!bUse[Limb])
		{
			Step.bPlaced = false;
			continue;
		}
		if (!Step.bPlaced || FVector::Dist(Step.To, Ideal[Limb]) > Stride * 2.f)
		{
			// A new hold, or the body jumped far from the old one: the limb starts where it belongs.
			Step.From = Step.To = Snap(Limb, Ideal[Limb]);
			Step.Alpha = 1.f;
			Step.bPlaced = true;
		}
		else if (Step.Alpha < 1.f)
		{
			Step.Alpha = FMath::Min(1.f, Step.Alpha + DeltaTime / FMath::Max(Step.Duration, UE_KINDA_SMALL_NUMBER));
		}
		bMovingLimb |= Step.Alpha < 1.f;
	}

	if (!bMovingLimb)
	{
		auto Behind = [&](int32 Limb) { return bUse[Limb] ? static_cast<float>(FVector::Dist(LimbSteps[Limb].To, Ideal[Limb])) : 0.f; };
		static constexpr int32 Pairs[2][2] = { { 0, 3 }, { 1, 2 } };	// left hand + right foot, right hand + left foot
		const float Worst[2] = { FMath::Max(Behind(0), Behind(3)), FMath::Max(Behind(1), Behind(2)) };
		int32 Pair = INDEX_NONE;
		if (bMoving && bStepDue)
		{
			Pair = 1 - LastStepPair;
		}
		else
		{
			// Fallen too far behind (a hop), or stopped away from where they belong: the pair furthest out catches up.
			const float Limit = bMoving ? Stride * 1.5f : Settle;
			const int32 Worse = Worst[1] > Worst[0] ? 1 : 0;
			Pair = Worst[Worse] > Limit ? Worse : INDEX_NONE;
		}
		if (Pair != INDEX_NONE)
		{
			bStepDue = false;
			LastStepPair = Pair;
			for (const int32 Limb : Pairs[Pair])
			{
				FLimbStep& Step = LimbSteps[Limb];
				const FVector To = bMoving && bWholeStrides ? Snap(Limb, Step.To + Ahead * Stride) : Snap(Limb, Ideal[Limb] + Velocity * Duration + Ahead * Half);
				if (bUse[Limb] && !To.Equals(Step.To, 0.5))
				{
					Step.From = Step.To;
					Step.To = To;
					Step.Alpha = 0.f;
					Step.Duration = Duration;
				}
			}
		}
	}

	for (int32 Limb = 0; Limb < 4; ++Limb)
	{
		const FLimbStep& Step = LimbSteps[Limb];
		OutLift[Limb] = 0.f;
		if (Step.Alpha >= 1.f)
		{
			Out[Limb] = Step.To;
			continue;
		}
		// Off the surface and back on, the arc as high as the move is long (up to StepLift).
		const float Lift = StepLift * FMath::Min(1.f, static_cast<float>(FVector::Dist(Step.From, Step.To)) / FMath::Max(Half, 1.f));
		Out[Limb] = FMath::Lerp(Step.From, Step.To, FMath::SmoothStep(0.f, 1.f, Step.Alpha));
		OutLift[Limb] = Lift * FMath::Sin(Step.Alpha * UE_PI);
	}
}

void UFootholdComponent::UpdateLimbTargets(const FFootholdSyncState& State, float DeltaTime)
{
	AnimData.bLeftHandTargetValid = false;
	AnimData.bRightHandTargetValid = false;
	AnimData.bLeftFootTargetValid = false;
	AnimData.bRightFootTargetValid = false;
	AnimData.LimbLift = FVector4(0.0, 0.0, 0.0, 0.0);

	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	const FVector Up = AnimData.Up;
	const FVector Center = Owner->GetActorLocation();
	const FVector Normal = State.SurfaceNormal;
	const FVector Facing = Normal.IsNearlyZero() ? AnimData.Facing : -Normal;
	const FVector Right = FVector::CrossProduct(Up, Facing).GetSafeNormal();	// the climber's right; left is -Right
	const FVector Velocity = MoverComponent->GetVelocity();

	// Left hand, right hand, left foot, right foot.
	float Lift[4] = { 0.f, 0.f, 0.f, 0.f };
	auto SetLimbs = [this, &Lift](const FVector (&At)[4], const bool (&bUse)[4])
	{
		AnimData.LimbLift = FVector4(Lift[0], Lift[1], Lift[2], Lift[3]);
		AnimData.LeftHandTarget = At[0];
		AnimData.RightHandTarget = At[1];
		AnimData.LeftFootTarget = At[2];
		AnimData.RightFootTarget = At[3];
		AnimData.bLeftHandTargetValid = bUse[0];
		AnimData.bRightHandTargetValid = bUse[1];
		AnimData.bLeftFootTargetValid = bUse[2];
		AnimData.bRightFootTargetValid = bUse[3];
	};

	// A point on the surface plane (through Anchor, along Normal), held ContactGap off it.
	auto OnSurface = [&](const FVector& Point)
	{
		return FVector::PointPlaneProject(Point, State.AnchorLocation, Normal) + Normal * ContactGap;
	};

	// Is there surface to rest on at Point (a short probe toward the surface)?
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootholdLimbProbe), false, Owner);
	auto HasSurface = [&](const FVector& Point)
	{
		FHitResult Hit;
		return World->LineTraceSingleByChannel(Hit, Point + Normal * 20.f, Point - Normal * 30.f, ECC_Visibility, Params);
	};

	FVector At[4];
	switch (State.Kind)
	{
	case EFootholdKind::LedgeHang:
	{
		// Hand over hand along the edge; the feet against the wall below, if there is wall there.
		const FVector Edge = GetGripPoint(State.AnchorLocation, Normal, Up);
		const FVector FootBase = OnSurface(State.AnchorLocation - Up * HangFootDrop);
		const bool bFeet = HasSurface(FootBase);
		const FVector Ideal[4] = { Edge - Right * HandSpacing, Edge + Right * HandSpacing, FootBase - Right * FootSpacing, FootBase + Right * FootSpacing };
		const bool bUse[4] = { true, true, bFeet, bFeet };
		auto Snap = [&](int32 Limb, const FVector& Point)
		{
			const FVector Base = Limb < 2 ? Edge : FootBase;
			return Base + Right * FVector::DotProduct(Point - Base, Right);
		};
		StepLimbs(DeltaTime, Ideal, bUse, Right * FVector::DotProduct(Velocity, Right), StepLength, 3.f, Snap, At, Lift);
		SetLimbs(At, bUse);
		break;
	}

	case EFootholdKind::Climb:
	{
		const FVector Ideal[4] = {
			OnSurface(Center + Up * ClimbHandHeight - Right * HandSpacing), OnSurface(Center + Up * ClimbHandHeight + Right * HandSpacing),
			OnSurface(Center - Up * ClimbFootDepth - Right * FootSpacing), OnSurface(Center - Up * ClimbFootDepth + Right * FootSpacing) };
		const bool bUse[4] = { true, true, true, true };
		auto Snap = [&](int32, const FVector& Point) { return OnSurface(Point); };
		StepLimbs(DeltaTime, Ideal, bUse, FVector::VectorPlaneProject(Velocity, Normal), StepLength, StepLength * 0.25f, Snap, At, Lift);
		SetLimbs(At, bUse);
		break;
	}

	case EFootholdKind::Ladder:
	{
		const UFootholdLadderComponent* Ladder = State.Ladder.Get();
		if (!Ladder)
		{
			break;
		}
		const float Spacing = FMath::Max(Ladder->RungSpacing, 1.f);
		const FVector Axis = Ladder->GetAxis();
		const FVector Out = Ladder->GetFacing();
		const FVector Bottom = Ladder->GetBottom();
		// Rails, from the climber's point of view (the climber faces into the ladder).
		const FVector RailSide = FVector::DotProduct(Ladder->GetRight(), Right) >= 0.f ? Ladder->GetRight() : -Ladder->GetRight();
		const FVector HalfWidth = RailSide * (Ladder->Width * 0.5f);
		const FVector Hands = Bottom + Axis * (State.LadderPosition + ClimbHandHeight) + Out * ContactGap;
		const FVector Feet = Bottom + Axis * (State.LadderPosition - ClimbFootDepth) + Out * (ContactGap * 2.f);
		const FVector Ideal[4] = { Hands - HalfWidth, Hands + HalfWidth, Feet - RailSide * FootSpacing, Feet + RailSide * FootSpacing };
		const bool bUse[4] = { true, true, true, true };
		auto Snap = [&](int32, const FVector& Point)
		{
			const float Along = static_cast<float>(FVector::DotProduct(Point - Bottom, Axis));
			const float Rung = FMath::Clamp(FMath::RoundToFloat(Along / Spacing) * Spacing, 0.f, Ladder->Height);
			return Point + Axis * (Rung - Along);
		};
		// Hand over hand, a whole number of rungs at a time.
		const float Stride = Spacing * FMath::Max(1, FMath::RoundToInt(StepLength / Spacing));
		StepLimbs(DeltaTime, Ideal, bUse, Axis * FVector::DotProduct(Velocity, Axis), Stride, Spacing * 0.5f + 1.f, Snap, At, Lift, /*bWholeStrides*/ true);
		SetLimbs(At, bUse);
		break;
	}

	case EFootholdKind::Mantle:
	{
		// Hands push down on the top while the body comes up and over it (the pose node eases them off at the end). The
		// feet push off their holds on the wall, step up over the edge onto the top (the lead, right, foot as the hips
		// near the edge, the other once the body is up) and come in under the body to stand.
		const float Alpha = AnimData.MantleAlpha;
		const FVector Edge = State.AnchorLocation - Normal * (ContactGap * 1.5f) + Up * HandGripUp;
		const bool bHands = Alpha < 0.8f;
		const bool bUse[4] = { bHands, bHands, true, true };
		At[0] = Edge - Right * HandSpacing;
		At[1] = Edge + Right * HandSpacing;

		constexpr float AnkleHeight = 10.f;
		constexpr float StepIn = 25.f;
		constexpr float FootSpeed = 400.f;
		const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(MoverComponent->GetUpdatedComponent());
		const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.f;
		auto Above = [&](const FVector& Point) { return static_cast<float>(FVector::DotProduct(Point - State.AnchorLocation, Up)); };
		auto Inside = [&](const FVector& Point) { return static_cast<float>(FVector::DotProduct(Point - State.AnchorLocation, -Normal)); };
		const float Height = Above(Center);	// the body's height over the edge: negative hanging, HalfHeight standing on top
		const FVector WallFoot = OnSurface(State.MantleStart - Up * ClimbFootDepth);
		const bool bWallFeet = Above(WallFoot) < 0.f && HasSurface(WallFoot);
		const FVector Under = Center - Up * (HalfHeight - AnkleHeight);
		const FVector Top = State.AnchorLocation - Normal * StepIn + Up * AnkleHeight;
		const FVector OverEdge = State.AnchorLocation + Normal * (ContactGap * 3.f) + Up * (AnkleHeight + 15.f);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FVector Offset = Right * (Side == 0 ? -FootSpacing : FootSpacing);
			const float LeaveAt = Side == 1 ? -35.f : 20.f;
			const FVector Goal = Offset + (Alpha >= 0.85f ? Under : Height >= LeaveAt ? Top : bWallFeet ? WallFoot : Under);
			FLimbStep& Foot = LimbSteps[2 + Side];
			if (!Foot.bPlaced)
			{
				Foot.To = Offset + (bWallFeet ? WallFoot : Under);
				Foot.bPlaced = true;
			}
			// A foot below the edge on its way to the top goes up past the edge first, never through the corner.
			const FVector Next = Inside(Goal) > 0.f && Above(Foot.To) < AnkleHeight - 1.f && Inside(Foot.To) < 1.f ? OverEdge + Offset : Goal;
			const FVector ToNext = Next - Foot.To;
			const float Distance = static_cast<float>(ToNext.Size());
			if (Distance > UE_KINDA_SMALL_NUMBER)
			{
				// At most FootSpeed, easing in at the end.
				Foot.To += ToNext * (FMath::Min(Distance, FMath::Min(FootSpeed, Distance * 12.f) * DeltaTime) / Distance);
			}
			At[2 + Side] = Foot.To;
		}
		SetLimbs(At, bUse);
		break;
	}

	default:
		break;	// glide and wall-run make their own limb targets in the pose node
	}
}

void UFootholdComponent::HandleKindChanged(EFootholdKind PreviousKind, EFootholdKind NewKind)
{
	{
		FScopeLock Lock(&AnimDataLock);
		AnimData.CycleDistance = 0.f;
		AnimData.BankDegrees = 0.f;
		// New holds for the new mode; the first pair moves after a quarter step, the other half a step later.
		for (FLimbStep& Step : LimbSteps)
		{
			Step.bPlaced = false;
		}
		LastStepPair = 1;
		NextStepAt = StepLength * 0.25f;
		bStepDue = false;
	}

	OnFootholdModeChanged.Broadcast(PreviousKind, NewKind);

	if (const TObjectPtr<UAnimMontage>* Montage = EntryMontages.Find(NewKind))
	{
		if (*Montage && GetOwner())
		{
			if (USkeletalMeshComponent* Mesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
			{
				if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
				{
					AnimInstance->Montage_Play(*Montage);
				}
			}
		}
	}
}

void UFootholdComponent::TryBindInput()
{
	if (bInputBound)
	{
		return;
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
	if (!Input)
	{
		return;
	}

	if (DropAction)
	{
		Input->BindAction(DropAction, ETriggerEvent::Started, this, &UFootholdComponent::RequestDrop);
	}
	if (GlideAction)
	{
		Input->BindAction(GlideAction, ETriggerEvent::Started, this, &UFootholdComponent::RequestGlideToggle);
	}
	if (ClimbToggleAction)
	{
		Input->BindAction(ClimbToggleAction, ETriggerEvent::Started, this, &UFootholdComponent::RequestClimbToggle);
	}
	if (CrouchAction)
	{
		Input->BindActionValueLambda(CrouchAction, ETriggerEvent::Started, [this](const FInputActionValue&) { SetCrouchHeld(true); });
		Input->BindActionValueLambda(CrouchAction, ETriggerEvent::Completed, [this](const FInputActionValue&) { SetCrouchHeld(false); });
	}
	bInputBound = true;
}
