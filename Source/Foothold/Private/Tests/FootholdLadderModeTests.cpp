// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FootholdLadder.h"
#include "FootholdComponent.h"
#include "FootholdLadderComponent.h"
#include "Tests/FootholdTestWorld.h"
#include "Tests/FootholdTestCharacter.h"
#include "FootholdTypes.h"
#include "MoverComponent.h"
#include "MoverSimulationTypes.h"

namespace UE::Foothold::Tests
{
	// A 400 cm ladder at X=100 facing -X, up to a platform behind it whose face is at X=105 and top
	// at 400; a pawn standing in front, walking into the ladder.
	struct FLadderScene
	{
		AFootholdLadder* Ladder = nullptr;
		AFootholdTestCharacter* Pawn = nullptr;
	};

	static FLadderScene SetUpLadder(FFootholdTestWorld& W)
	{
		FLadderScene Scene;
		W.SpawnBox(FVector(205, 0, 200), FVector(100, 200, 200));
		Scene.Ladder = W.Spawn<AFootholdLadder>(FVector(100, 0, 0), FRotator(0, 180, 0));
		Scene.Ladder->GetLadder()->Height = 400.f;
		Scene.Ladder->RebuildMesh();
		Scene.Pawn = W.Spawn<AFootholdTestCharacter>(FVector(20, 0, 90));
		W.TickUntil([Pawn = Scene.Pawn]() { return Pawn->GetMoverComponent()->GetMovementModeName() == DefaultModeNames::Walking; }, 2.f);
		return Scene;
	}

	static bool GetOnLadder(FFootholdTestWorld& W, AFootholdTestCharacter* Pawn)
	{
		Pawn->SetScriptedMove(FVector(1, 0, 0));
		const UMoverComponent* Mover = Pawn->GetMoverComponent();
		return W.TickUntil([Mover]() { return Mover->GetMovementModeName() == FootholdModeNames::Ladder; }, 1.5f);
	}

	static float GetLadderPosition(const AFootholdTestCharacter* Pawn)
	{
		const FFootholdSyncState* State = Pawn->GetMoverComponent()->GetSyncState().SyncStateCollection.FindDataByType<FFootholdSyncState>();
		return State ? State->LadderPosition : -1.f;
	}
}

using namespace UE::Foothold::Tests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLadderClimbsToTop, "Foothold.Modes.Ladder.ClimbsToTop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLadderClimbsToTop::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FLadderScene Scene = SetUpLadder(W);
	if (!TestTrue(TEXT("gets on the ladder"), GetOnLadder(W, Scene.Pawn)))
	{
		return false;
	}
	W.Tick(0.3f);
	TestEqual(TEXT("held in front of the ladder"), Scene.Pawn->GetActorLocation().X, 55.0, 2.0);
	TestTrue(TEXT("faces the ladder"), Scene.Pawn->GetActorForwardVector().Equals(FVector(1, 0, 0), 0.02));

	const UMoverComponent* Mover = Scene.Pawn->GetMoverComponent();
	bool bMantled = false;
	const TArray<FName> Modes = W.RecordModes(Mover, 6.f, [Mover, &bMantled]()
	{
		bMantled |= Mover->GetMovementModeName() == FootholdModeNames::Mantle;
		return bMantled && Mover->GetMovementModeName() == DefaultModeNames::Walking;
	});
	TestTrue(TEXT("mantles off the top"), Modes.Contains(FootholdModeNames::Mantle));
	TestEqual(TEXT("walking on the platform"), Mover->GetMovementModeName(), DefaultModeNames::Walking);
	TestEqual(TEXT("on the 400 cm platform"), Scene.Pawn->GetActorLocation().Z, 490.0, 4.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLadderTopOutAtHands, "Foothold.Modes.Ladder.TopOutStartsWithHandsAtTheTop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLadderTopOutAtHands::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FLadderScene Scene = SetUpLadder(W);
	if (!TestTrue(TEXT("gets on the ladder"), GetOnLadder(W, Scene.Pawn)))
	{
		return false;
	}
	const UMoverComponent* Mover = Scene.Pawn->GetMoverComponent();
	if (!TestTrue(TEXT("mantles off the top"), W.TickUntil([Mover]() { return Mover->GetMovementModeName() == FootholdModeNames::Mantle; }, 6.f)))
	{
		return false;
	}
	// The 400 cm ladder's top is the platform's edge. Climbing a wall tops out with the hands at the edge; so should a ladder.
	const double CentreBelowTop = 400.0 - Scene.Pawn->GetActorLocation().Z;
	AddInfo(FString::Printf(TEXT("capsule centre %.1f cm below the top when the mantle starts"), CentreBelowTop));
	TestTrue(TEXT("top-out starts with the hands at the top, the body still below it"), CentreBelowTop > 40.0 && CentreBelowTop < 80.0);
	TestTrue(TEXT("and ends on the platform"), W.TickUntil([Mover]() { return Mover->GetMovementModeName() == DefaultModeNames::Walking; }, 3.f)
		&& FMath::Abs(Scene.Pawn->GetActorLocation().Z - 490.0) < 4.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLadderDescends, "Foothold.Modes.Ladder.DescendsToFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLadderDescends::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FLadderScene Scene = SetUpLadder(W);
	if (!TestTrue(TEXT("gets on the ladder"), GetOnLadder(W, Scene.Pawn)))
	{
		return false;
	}
	W.Tick(1.f);
	TestTrue(TEXT("climbed up"), Scene.Pawn->GetActorLocation().Z > 180.0);

	Scene.Pawn->SetScriptedMove(FVector(-1, 0, 0));
	const UMoverComponent* Mover = Scene.Pawn->GetMoverComponent();
	TestTrue(TEXT("steps off at the bottom"), W.TickUntil([Mover]() { return Mover->GetMovementModeName() == DefaultModeNames::Walking; }, 3.f));
	TestEqual(TEXT("on the floor"), Scene.Pawn->GetActorLocation().Z, 90.0, 4.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLadderDestroyed, "Foothold.Modes.Ladder.ExitsWhenLadderDestroyed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLadderDestroyed::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FLadderScene Scene = SetUpLadder(W);
	if (!TestTrue(TEXT("gets on the ladder"), GetOnLadder(W, Scene.Pawn)))
	{
		return false;
	}
	W.Tick(0.6f);
	Scene.Ladder->Destroy();
	W.StepOnce();
	W.StepOnce();
	TestEqual(TEXT("falls when the ladder goes away"), Scene.Pawn->GetMoverComponent()->GetMovementModeName(), DefaultModeNames::Falling);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLadderSnapsToRungs, "Foothold.Modes.Ladder.SnapsToRungs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLadderSnapsToRungs::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FLadderScene Scene = SetUpLadder(W);
	if (!TestTrue(TEXT("gets on the ladder"), GetOnLadder(W, Scene.Pawn)))
	{
		return false;
	}
	W.Tick(0.83f);
	Scene.Pawn->SetScriptedMove(FVector::ZeroVector);
	W.Tick(0.6f);

	const float Position = GetLadderPosition(Scene.Pawn);
	const float Spacing = Scene.Ladder->GetLadder()->RungSpacing;
	const float FromRung = FMath::Abs(Position - FMath::RoundToFloat(Position / Spacing) * Spacing);
	TestEqual(TEXT("still on the ladder"), Scene.Pawn->GetMoverComponent()->GetMovementModeName(), FootholdModeNames::Ladder);
	TestTrue(TEXT("settled on a rung"), FromRung < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLadderHandsMoveRungToRung, "Foothold.Modes.Ladder.HandsMoveRungToRung",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLadderHandsMoveRungToRung::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FLadderScene Scene = SetUpLadder(W);
	if (!TestTrue(TEXT("gets on the ladder"), GetOnLadder(W, Scene.Pawn)))
	{
		return false;
	}
	W.Tick(0.2f);

	// Hands travel from rung to rung (no jump of a whole rung in one frame) and rest on a rung between moves.
	const UFootholdComponent* Foothold = Scene.Pawn->GetFootholdComponent();
	const float Spacing = Scene.Ladder->GetLadder()->RungSpacing;
	const double Bottom = Scene.Ladder->GetLadder()->GetBottom().Z;
	FFootholdAnimData Last = Foothold->GetAnimData();
	double LargestJump = 0.0;
	int32 RestingOffRung = 0, BothMoving = 0;
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		W.Tick(1.f / 60.f);
		const FFootholdAnimData Now = Foothold->GetAnimData();
		const FVector Hands[2] = { Now.LeftHandTarget, Now.RightHandTarget };
		const FVector Before[2] = { Last.LeftHandTarget, Last.RightHandTarget };
		bool bMoved[2];
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double Jump = FVector::Dist(Hands[Side], Before[Side]);
			LargestJump = FMath::Max(LargestJump, Jump);
			bMoved[Side] = Jump > 0.25;
			const double Height = Hands[Side].Z - Bottom;
			RestingOffRung += !bMoved[Side] && FMath::Abs(Height - FMath::RoundToDouble(Height / Spacing) * Spacing) > 1.0;
		}
		BothMoving += bMoved[0] && bMoved[1];
		Last = Now;
	}
	AddInfo(FString::Printf(TEXT("largest hand move in one frame %.1f cm; resting off a rung %d; both moving %d"), LargestJump, RestingOffRung, BothMoving));
	TestEqual(TEXT("still on the ladder"), Scene.Pawn->GetMoverComponent()->GetMovementModeName(), FootholdModeNames::Ladder);
	TestTrue(TEXT("no hand jumps a rung in one frame"), LargestJump < Spacing * 0.4);
	TestEqual(TEXT("resting hands are on rungs"), RestingOffRung, 0);
	TestEqual(TEXT("one hand moves at a time"), BothMoving, 0);
	return true;
}

#endif
