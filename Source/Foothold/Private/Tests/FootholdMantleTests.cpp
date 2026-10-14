// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FootholdTestWorld.h"
#include "Tests/FootholdTestCharacter.h"
#include "FootholdTypes.h"
#include "MoverComponent.h"
#include "MoverSimulationTypes.h"

namespace UE::Foothold::Tests
{
	// A 120 cm block whose face is at X=100, and a pawn standing just in front of it.
	static AFootholdTestCharacter* SetUpMantle(FFootholdTestWorld& W, bool bCeiling)
	{
		W.SpawnBox(FVector(200, 0, 60), FVector(100, 200, 60));
		if (bCeiling)
		{
			// 60 cm above the top, starting 10 cm behind the edge: too low to stand under, and too tall
			// (180 to 400 cm) to be a ledge itself.
			W.SpawnBox(FVector(260, 0, 290), FVector(150, 200, 110));
		}
		AFootholdTestCharacter* Pawn = W.Spawn<AFootholdTestCharacter>(FVector(30, 0, 90));
		W.TickUntil([Pawn]() { return Pawn->GetMoverComponent()->GetMovementModeName() == DefaultModeNames::Walking; }, 2.f);
		return Pawn;
	}

	static FVector RunMantle(FFootholdTestWorld& W, AFootholdTestCharacter* Pawn, TArray<FName>& OutModes)
	{
		Pawn->SetScriptedMove(FVector(1, 0, 0));
		Pawn->PressJump();
		// Stop once the pawn is walking again after a mantle, before it walks off the far side.
		const UMoverComponent* Mover = Pawn->GetMoverComponent();
		bool bMantled = false;
		OutModes = W.RecordModes(Mover, 3.f, [Mover, &bMantled]()
		{
			bMantled |= Mover->GetMovementModeName() == FootholdModeNames::Mantle;
			return bMantled && Mover->GetMovementModeName() == DefaultModeNames::Walking;
		});
		Pawn->SetScriptedMove(FVector::ZeroVector);
		return Pawn->GetActorLocation();
	}
}

using namespace UE::Foothold::Tests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdMantleClimbsOntoBox, "Foothold.Modes.Mantle.ClimbsOntoBox",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdMantleClimbsOntoBox::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	AFootholdTestCharacter* Pawn = SetUpMantle(W, /*bCeiling*/ false);
	TArray<FName> Modes;
	const FVector End = RunMantle(W, Pawn, Modes);

	TestTrue(TEXT("went through Mantle"), Modes.Contains(FootholdModeNames::Mantle));
	TestEqual(TEXT("ends walking"), Pawn->GetMoverComponent()->GetMovementModeName(), DefaultModeNames::Walking);
	TestEqual(TEXT("standing on the box (capsule centre 88 above the 120 cm top)"), End.Z, 208.0, 4.0);
	TestTrue(TEXT("on top, past the edge"), End.X > 100.0 + 34.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdMantleRejectsBlockedTop, "Foothold.Modes.Mantle.RejectsBlockedTop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdMantleRejectsBlockedTop::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	AFootholdTestCharacter* Pawn = SetUpMantle(W, /*bCeiling*/ true);
	TArray<FName> Modes;
	const FVector End = RunMantle(W, Pawn, Modes);

	TestFalse(TEXT("never mantles under a low ceiling"), Modes.Contains(FootholdModeNames::Mantle));
	TestTrue(TEXT("stays in front of the box"), End.X < 100.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdMantleDeterministic, "Foothold.Modes.Mantle.Deterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdMantleDeterministic::RunTest(const FString& Parameters)
{
	FVector Ends[2];
	for (int32 Run = 0; Run < 2; ++Run)
	{
		FFootholdTestWorld W;
		AFootholdTestCharacter* Pawn = SetUpMantle(W, false);
		TArray<FName> Modes;
		Ends[Run] = RunMantle(W, Pawn, Modes);
	}
	TestTrue(TEXT("identical runs end at the same place"), Ends[0].Equals(Ends[1], 0.01));
	return true;
}

#endif
