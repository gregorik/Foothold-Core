// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FootholdTestWorld.h"
#include "FootholdLadder.h"
#include "FootholdLadderComponent.h"
#include "FootholdSharedSettings.h"
#include "FootholdTraceLibrary.h"

namespace UE::Foothold::Tests
{
	// A test capsule the size of the UE5 mannequin's: radius 34, half-height 88.
	static FFootholdQueryContext MakeContext(UWorld* World, const FVector& Center, const UFootholdSharedSettings* Settings,
		const FVector& Up = FVector::UpVector, const FVector& Forward = FVector::ForwardVector)
	{
		return FFootholdQueryContext::Make(World, Center, Up, Forward, 34.f, 88.f, Settings);
	}

	static UFootholdSharedSettings* MakeSettings()
	{
		return NewObject<UFootholdSharedSettings>(GetTransientPackage());
	}

	// A 100 cm deep wall whose face is at X=100 and whose top is TopZ above the floor.
	static void SpawnLedgeWall(FFootholdTestWorld& W, float TopZ, const TArray<FName>& Tags = TArray<FName>())
	{
		W.SpawnBox(FVector(150, 0, TopZ * 0.5f), FVector(50, 200, TopZ * 0.5f), FRotator::ZeroRotator, Tags);
	}
}

using namespace UE::Foothold::Tests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceFindWallHitsBoxAhead, "Foothold.Trace.FindWall.HitsBoxAhead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceFindWallHitsBoxAhead::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	SpawnLedgeWall(W, 200.f);
	// Capsule surface at X=64: the wall is 36 cm away.
	const FFootholdQueryContext Ctx = MakeContext(W.World, FVector(30, 0, 90), MakeSettings());

	const FFootholdWallHit Hit = UFootholdTraceLibrary::FindWall(Ctx, FVector::ForwardVector, 50.f);
	TestEqual(TEXT("distance from the capsule surface"), Hit.Distance, 36.f, 1.f);
	TestTrue(TEXT("hit the wall"), Hit.bHit);
	TestEqual(TEXT("wall face X"), Hit.Location.X, 100.0, 1.0);
	TestTrue(TEXT("normal faces the capsule"), Hit.Normal.Equals(FVector(-1, 0, 0), 0.01));

	const FFootholdWallHit Miss = UFootholdTraceLibrary::FindWall(Ctx, FVector::ForwardVector, 20.f);
	TestFalse(TEXT("no hit when the wall is out of reach"), Miss.bHit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceClimbableNeedsTag, "Foothold.Trace.FindWall.ClimbableNeedsTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceClimbableNeedsTag::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	UFootholdSharedSettings* Settings = MakeSettings();
	W.SpawnBox(FVector(150, 0, 200), FVector(50, 100, 200), FRotator::ZeroRotator, { TEXT("Climbable") });
	W.SpawnBox(FVector(-150, 0, 200), FVector(50, 100, 200));
	const FFootholdQueryContext Ctx = MakeContext(W.World, FVector(0, 0, 90), Settings);

	TestTrue(TEXT("tagged wall is climbable"), UFootholdTraceLibrary::FindWall(Ctx, FVector::ForwardVector, 80.f).bClimbable);
	TestFalse(TEXT("untagged wall is not climbable"), UFootholdTraceLibrary::FindWall(Ctx, FVector::BackwardVector, 80.f).bClimbable);

	Settings->ClimbPolicy = EFootholdClimbablePolicy::AnySteepSurface;
	TestTrue(TEXT("any steep surface policy accepts the untagged wall"), UFootholdTraceLibrary::FindWall(Ctx, FVector::BackwardVector, 80.f).bClimbable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceFindLedgeFindsBoxTop, "Foothold.Trace.FindLedge.FindsBoxTop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceFindLedgeFindsBoxTop::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	UFootholdSharedSettings* Settings = MakeSettings();
	SpawnLedgeWall(W, 180.f);
	const FFootholdQueryContext Ctx = MakeContext(W.World, FVector(40, 0, 88), Settings);

	const FFootholdLedge Ledge = UFootholdTraceLibrary::FindLedge(Ctx, FVector::ForwardVector, 100.f, 250.f);
	TestTrue(TEXT("valid"), Ledge.bValid);
	TestEqual(TEXT("edge X"), Ledge.EdgePoint.X, 100.0, 2.0);
	TestEqual(TEXT("edge Z"), Ledge.EdgePoint.Z, 180.0, 1.0);
	TestEqual(TEXT("height above feet"), Ledge.HeightAboveFeet, 180.f, 1.f);
	TestTrue(TEXT("wall normal"), Ledge.WallNormal.Equals(FVector(-1, 0, 0), 0.01));
	TestEqual(TEXT("hang X keeps radius plus gap off the wall"), Ledge.HangLocation.X, 100.0 - 34.0 - Settings->WallGap, 1.0);
	TestEqual(TEXT("hang Z puts the hands at the edge"), Ledge.HangLocation.Z, 180.0 - Settings->HandReachAboveCenter, 1.0);
	TestTrue(TEXT("hang capsule is clear"), Ledge.bHangClear);
	TestTrue(TEXT("can stand on top"), Ledge.bCanStand);
	TestEqual(TEXT("stand Z"), Ledge.StandLocation.Z, 180.0 + 88.0 + 2.0, 1.5);
	TestTrue(TEXT("stand location is on top of the box"), Ledge.StandLocation.X > 100.0 + 34.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceFindLedgeLowObstacle, "Foothold.Trace.FindLedge.FindsLowObstacleTop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceFindLedgeLowObstacle::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	SpawnLedgeWall(W, 90.f);
	const FFootholdQueryContext Ctx = MakeContext(W.World, FVector(40, 0, 88), MakeSettings());

	const FFootholdLedge Ledge = UFootholdTraceLibrary::FindLedge(Ctx, FVector::ForwardVector, 40.f, 200.f);
	TestTrue(TEXT("valid"), Ledge.bValid);
	TestEqual(TEXT("edge Z"), Ledge.EdgePoint.Z, 90.0, 1.0);
	TestTrue(TEXT("can stand on top"), Ledge.bCanStand);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceFindLedgeRejectsSteepTop, "Foothold.Trace.FindLedge.RejectsSteepTop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceFindLedgeRejectsSteepTop::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	UFootholdSharedSettings* Settings = MakeSettings();
	SpawnLedgeWall(W, 150.f);
	// A plate tilted 60 degrees covers the wall top, so the "top" the probe finds is too steep to stand on.
	W.SpawnBox(FVector(130, 0, 190), FVector(60, 200, 5), FRotator(60, 0, 0));
	const FFootholdQueryContext Ctx = MakeContext(W.World, FVector(40, 0, 88), Settings);

	TestFalse(TEXT("steep top refused"), UFootholdTraceLibrary::FindLedge(Ctx, FVector::ForwardVector, 100.f, 250.f).bValid);

	Settings->bAllowHangOnUnstandableLedges = true;
	const FFootholdLedge Allowed = UFootholdTraceLibrary::FindLedge(Ctx, FVector::ForwardVector, 100.f, 250.f);
	TestTrue(TEXT("allowed when the setting says so"), Allowed.bValid);
	TestFalse(TEXT("but you can't stand there"), Allowed.bCanStand);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceFindLedgeRejectsTooHigh, "Foothold.Trace.FindLedge.RejectsTooHigh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceFindLedgeRejectsTooHigh::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	SpawnLedgeWall(W, 300.f);
	const FFootholdQueryContext Ctx = MakeContext(W.World, FVector(40, 0, 88), MakeSettings());
	TestFalse(TEXT("a 300 cm top is out of a 250 cm reach"), UFootholdTraceLibrary::FindLedge(Ctx, FVector::ForwardVector, 100.f, 250.f).bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceFindLedgeNoClimbTag, "Foothold.Trace.FindLedge.RespectsNoClimbTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceFindLedgeNoClimbTag::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	SpawnLedgeWall(W, 180.f, { TEXT("NoClimb") });
	const FFootholdQueryContext Ctx = MakeContext(W.World, FVector(40, 0, 88), MakeSettings());
	TestFalse(TEXT("NoClimb ledge refused"), UFootholdTraceLibrary::FindLedge(Ctx, FVector::ForwardVector, 100.f, 250.f).bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceCanStandFalseUnderCeiling, "Foothold.Trace.FindLedge.CanStandFalseUnderCeiling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceCanStandFalseUnderCeiling::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	SpawnLedgeWall(W, 180.f);
	// A ceiling 60 cm above the top, starting 10 cm behind the edge: room to hang, no room to stand.
	W.SpawnBox(FVector(260, 0, 260), FVector(150, 200, 20));
	const FFootholdQueryContext Ctx = MakeContext(W.World, FVector(40, 0, 88), MakeSettings());

	const FFootholdLedge Ledge = UFootholdTraceLibrary::FindLedge(Ctx, FVector::ForwardVector, 100.f, 250.f);
	TestTrue(TEXT("still a valid ledge"), Ledge.bValid);
	TestTrue(TEXT("hang capsule is clear"), Ledge.bHangClear);
	TestFalse(TEXT("can't stand under the ceiling"), Ledge.bCanStand);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceFindLadderWithinDistance, "Foothold.Trace.FindLadder.WithinDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceFindLadderWithinDistance::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	// Ladder at X=100 facing -X, so its climbers stand at X=55.
	AFootholdLadder* Ladder = W.Spawn<AFootholdLadder>(FVector(100, 0, 0), FRotator(0, 180, 0));
	const UFootholdSharedSettings* Settings = MakeSettings();

	float Position = -1.f;
	UFootholdLadderComponent* Found = UFootholdTraceLibrary::FindLadder(MakeContext(W.World, FVector(30, 0, 88), Settings), 60.f, Position);
	TestTrue(TEXT("found the ladder"), Found == Ladder->GetLadder());
	TestEqual(TEXT("position along the ladder"), Position, 88.f, 1.f);

	TestNull(TEXT("too far to the side"), UFootholdTraceLibrary::FindLadder(MakeContext(W.World, FVector(30, 300, 88), Settings), 60.f, Position));
	TestNull(TEXT("behind the ladder"), UFootholdTraceLibrary::FindLadder(MakeContext(W.World, FVector(160, 0, 88), Settings), 60.f, Position));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdTraceWorksWithRotatedUp, "Foothold.Trace.WorksWithRotatedUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdTraceWorksWithRotatedUp::RunTest(const FString& Parameters)
{
	// The FindsBoxTop scene, rotated so that "up" is +Y.
	FFootholdTestWorld W(/*bWithFloor*/ false);
	const FQuat Q = FQuat::FindBetweenNormals(FVector::UpVector, FVector::RightVector);
	W.SpawnBox(Q.RotateVector(FVector(150, 0, 90)), FVector(50, 200, 90), Q.Rotator());
	W.SpawnBox(Q.RotateVector(FVector(0, 0, -50)), FVector(2000, 2000, 50), Q.Rotator());	// the floor, rotated too

	const FVector Up = Q.RotateVector(FVector::UpVector);
	const FVector Forward = Q.RotateVector(FVector::ForwardVector);
	const FFootholdQueryContext Ctx = MakeContext(W.World, Q.RotateVector(FVector(40, 0, 88)), MakeSettings(), Up, Forward);

	const FFootholdLedge Ledge = UFootholdTraceLibrary::FindLedge(Ctx, Forward, 100.f, 250.f);
	TestTrue(TEXT("valid with a sideways up"), Ledge.bValid);
	TestTrue(TEXT("edge in the rotated frame"), Ledge.EdgePoint.Equals(Q.RotateVector(FVector(100, 0, 180)), 2.5));
	TestEqual(TEXT("height above feet"), Ledge.HeightAboveFeet, 180.f, 1.f);
	TestTrue(TEXT("can stand"), Ledge.bCanStand);
	return true;
}

#endif
