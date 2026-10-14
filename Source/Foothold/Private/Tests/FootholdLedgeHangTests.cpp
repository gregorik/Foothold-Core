// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/StaticMeshActor.h"
#include "FootholdComponent.h"
#include "Tests/FootholdTestWorld.h"
#include "Tests/FootholdTestCharacter.h"
#include "FootholdTypes.h"
#include "MoverComponent.h"
#include "MoverSimulationTypes.h"

namespace UE::Foothold::Tests
{
	// A 250 cm wall, face at X=100, 400 cm long (Y from -200 to 200), and a pawn falling beside it
	// with its hands just under the top: it should catch the edge.
	struct FHangScene
	{
		AStaticMeshActor* Wall = nullptr;
		AFootholdTestCharacter* Pawn = nullptr;
	};

	static FHangScene SetUpHang(FFootholdTestWorld& W, const FQuat& Frame = FQuat::Identity)
	{
		FHangScene Scene;
		Scene.Wall = W.SpawnBox(Frame.RotateVector(FVector(150, 0, 125)), FVector(50, 200, 125), Frame.Rotator());
		Scene.Pawn = W.Spawn<AFootholdTestCharacter>(Frame.RotateVector(FVector(58, 0, 190)), Frame.Rotator());
		return Scene;
	}

	static bool WaitForMode(FFootholdTestWorld& W, const AFootholdTestCharacter* Pawn, FName Mode, float Seconds)
	{
		const UMoverComponent* Mover = Pawn->GetMoverComponent();
		return W.TickUntil([Mover, Mode]() { return Mover->GetMovementModeName() == Mode; }, Seconds);
	}
}

using namespace UE::Foothold::Tests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLedgeHangGrabsWhileFalling, "Foothold.Modes.LedgeHang.GrabsWhileFalling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLedgeHangGrabsWhileFalling::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FHangScene Scene = SetUpHang(W);
	if (!TestTrue(TEXT("grabs the ledge"), WaitForMode(W, Scene.Pawn, FootholdModeNames::LedgeHang, 1.f)))
	{
		return false;
	}
	W.Tick(0.3f);	// settle into the hang
	const FVector Settled = Scene.Pawn->GetActorLocation();
	W.Tick(1.f);
	const FVector Later = Scene.Pawn->GetActorLocation();

	TestEqual(TEXT("still hanging"), Scene.Pawn->GetMoverComponent()->GetMovementModeName(), FootholdModeNames::LedgeHang);
	TestTrue(TEXT("holds still within 2 cm"), Settled.Equals(Later, 2.0));
	TestTrue(TEXT("no velocity while hanging"), Scene.Pawn->GetMoverComponent()->GetVelocity().Size() < 1.0);
	TestEqual(TEXT("hands at the edge: capsule centre 90 cm under the top"), Later.Z, 250.0 - 90.0, 3.0);
	TestEqual(TEXT("held off the wall"), Later.X, 100.0 - 34.0 - 4.0, 3.0);
	TestTrue(TEXT("faces the wall"), Scene.Pawn->GetActorForwardVector().Equals(FVector(1, 0, 0), 0.02));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLedgeHangShimmies, "Foothold.Modes.LedgeHang.ShimmiesAndStopsAtEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLedgeHangShimmies::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FHangScene Scene = SetUpHang(W);
	if (!TestTrue(TEXT("grabs the ledge"), WaitForMode(W, Scene.Pawn, FootholdModeNames::LedgeHang, 1.f)))
	{
		return false;
	}
	W.Tick(0.3f);

	Scene.Pawn->SetScriptedMove(FVector(0, 1, 0));
	W.Tick(1.f);
	const float AfterOneSecond = Scene.Pawn->GetActorLocation().Y;
	TestTrue(TEXT("moves right along the edge"), AfterOneSecond > 50.f);
	TestEqual(TEXT("keeps its height while shimmying"), Scene.Pawn->GetActorLocation().Z, 160.0, 3.0);

	W.Tick(4.f);
	const FVector End = Scene.Pawn->GetActorLocation();
	TestEqual(TEXT("still hanging at the end of the ledge"), Scene.Pawn->GetMoverComponent()->GetMovementModeName(), FootholdModeNames::LedgeHang);
	TestTrue(TEXT("stops before the end of the wall"), End.Y < 200.0 && End.Y > 120.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLedgeHangClimbsUp, "Foothold.Modes.LedgeHang.ClimbsUpOnJump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLedgeHangClimbsUp::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FHangScene Scene = SetUpHang(W);
	if (!TestTrue(TEXT("grabs the ledge"), WaitForMode(W, Scene.Pawn, FootholdModeNames::LedgeHang, 1.f)))
	{
		return false;
	}
	W.Tick(0.3f);

	Scene.Pawn->PressJump();
	const UMoverComponent* Mover = Scene.Pawn->GetMoverComponent();
	bool bMantled = false;
	const TArray<FName> Modes = W.RecordModes(Mover, 2.f, [Mover, &bMantled]()
	{
		bMantled |= Mover->GetMovementModeName() == FootholdModeNames::Mantle;
		return bMantled && Mover->GetMovementModeName() == DefaultModeNames::Walking;
	});

	TestTrue(TEXT("climbs up through Mantle"), Modes.Contains(FootholdModeNames::Mantle));
	TestEqual(TEXT("walking on top"), Mover->GetMovementModeName(), DefaultModeNames::Walking);
	TestEqual(TEXT("standing on the 250 cm top"), Scene.Pawn->GetActorLocation().Z, 338.0, 4.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLedgeHangDrops, "Foothold.Modes.LedgeHang.DropsOnBackJump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLedgeHangDrops::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FHangScene Scene = SetUpHang(W);
	if (!TestTrue(TEXT("grabs the ledge"), WaitForMode(W, Scene.Pawn, FootholdModeNames::LedgeHang, 1.f)))
	{
		return false;
	}
	W.Tick(0.3f);

	Scene.Pawn->SetScriptedMove(FVector(-1, 0, 0));
	Scene.Pawn->PressJump();
	W.StepOnce();
	W.StepOnce();
	const UMoverComponent* Mover = Scene.Pawn->GetMoverComponent();
	TestEqual(TEXT("lets go"), Mover->GetMovementModeName(), DefaultModeNames::Falling);
	TestTrue(TEXT("pushed away from the wall"), Mover->GetVelocity().X < -50.0);

	Scene.Pawn->SetScriptedMove(FVector::ZeroVector);
	const TArray<FName> Modes = W.RecordModes(Mover, 0.35f);
	TestFalse(TEXT("doesn't grab the same ledge straight back"), Modes.Contains(FootholdModeNames::LedgeHang));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLedgeHangDropsWhenLedgeMoves, "Foothold.Modes.LedgeHang.DropsWhenLedgeMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLedgeHangDropsWhenLedgeMoves::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FHangScene Scene = SetUpHang(W);
	if (!TestTrue(TEXT("grabs the ledge"), WaitForMode(W, Scene.Pawn, FootholdModeNames::LedgeHang, 1.f)))
	{
		return false;
	}
	W.Tick(0.3f);

	Scene.Wall->SetActorLocation(Scene.Wall->GetActorLocation() + FVector(0, 0, 10));
	W.StepOnce();
	W.StepOnce();
	TestEqual(TEXT("lets go when the ledge moves"), Scene.Pawn->GetMoverComponent()->GetMovementModeName(), DefaultModeNames::Falling);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLedgeHangRotatedUp, "Foothold.Modes.LedgeHang.WorksWithRotatedUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLedgeHangRotatedUp::RunTest(const FString& Parameters)
{
	// The hang scene turned on its side: "up" is +X and gravity pulls toward -X.
	FFootholdTestWorld W(/*bWithFloor*/ false);
	const FQuat Frame = FQuat::FindBetweenNormals(FVector::UpVector, FVector::ForwardVector);
	W.SpawnBox(Frame.RotateVector(FVector(0, 0, -50)), FVector(2000, 2000, 50), Frame.Rotator());
	const FHangScene Scene = SetUpHang(W, Frame);
	UMoverComponent* Mover = Scene.Pawn->GetMoverComponent();
	Mover->SetGravityOverride(true, Frame.RotateVector(FVector(0, 0, -980)));

	if (!TestTrue(TEXT("grabs the ledge with sideways gravity"), WaitForMode(W, Scene.Pawn, FootholdModeNames::LedgeHang, 1.5f)))
	{
		return false;
	}
	W.Tick(0.3f);
	const FVector Settled = Scene.Pawn->GetActorLocation();
	W.Tick(0.5f);
	TestTrue(TEXT("holds still"), Settled.Equals(Scene.Pawn->GetActorLocation(), 2.0));
	TestTrue(TEXT("hangs 90 cm under the top along the rotated up"), Settled.Equals(Frame.RotateVector(FVector(62, 0, 160)), 4.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLedgeHangMantleFeet, "Foothold.Modes.LedgeHang.MantleFeetStepOverTheEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdLedgeHangMantleFeet::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	const FHangScene Scene = SetUpHang(W);
	if (!TestTrue(TEXT("grabs the ledge"), WaitForMode(W, Scene.Pawn, FootholdModeNames::LedgeHang, 1.f)))
	{
		return false;
	}
	W.Tick(0.3f);
	Scene.Pawn->PressJump();

	// Through the whole climb-up the feet have places: pushing off the wall, stepping up over the edge, on the top, under
	// the body. They move there (no jumps) and never through the block (face X=100, top Z=250).
	const UMoverComponent* Mover = Scene.Pawn->GetMoverComponent();
	const UFootholdComponent* Foothold = Scene.Pawn->GetFootholdComponent();
	int32 Frames = 0, Unplaced = 0, Inside = 0;
	double LargestJump = 0.0;
	FFootholdAnimData Last;
	for (int32 Frame = 0; Frame < 240 && Mover->GetMovementModeName() != DefaultModeNames::Walking; ++Frame)
	{
		W.Tick(1.f / 60.f);
		if (Mover->GetMovementModeName() != FootholdModeNames::Mantle)
		{
			continue;
		}
		const FFootholdAnimData Now = Foothold->GetAnimData();
		Unplaced += !(Now.bLeftFootTargetValid && Now.bRightFootTargetValid);
		for (const FVector& Foot : { Now.LeftFootTarget, Now.RightFootTarget })
		{
			Inside += Foot.X > 100.5 && Foot.Z < 249.5;
		}
		if (Frames > 0)
		{
			LargestJump = FMath::Max3(LargestJump, FVector::Dist(Now.LeftFootTarget, Last.LeftFootTarget), FVector::Dist(Now.RightFootTarget, Last.RightFootTarget));
		}
		Last = Now;
		++Frames;
	}
	AddInfo(FString::Printf(TEXT("%d mantle frames: feet unplaced in %d, inside the block %d, largest move in a frame %.1f cm, last feet %s / %s"),
		Frames, Unplaced, Inside, LargestJump, *Last.LeftFootTarget.ToCompactString(), *Last.RightFootTarget.ToCompactString()));
	TestTrue(TEXT("climbed up through Mantle"), Frames > 10);
	TestEqual(TEXT("both feet placed through the whole mantle"), Unplaced, 0);
	TestEqual(TEXT("feet never inside the block"), Inside, 0);
	TestTrue(TEXT("no foot moves more than 8 cm in a frame"), LargestJump < 8.0);
	TestTrue(TEXT("ends with both feet on the top"), FMath::Abs(Last.LeftFootTarget.Z - 260.0) < 8.0 && FMath::Abs(Last.RightFootTarget.Z - 260.0) < 8.0
		&& Last.LeftFootTarget.X > 100.0 && Last.RightFootTarget.X > 100.0);
	return true;
}

#endif
