// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FootholdTestWorld.h"
#include "FootholdTestTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdSmokeModeTicks, "Foothold.Smoke.CustomModeTicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootholdSmokeModeTicks::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	AFootholdTestPawn* Pawn = W.Spawn<AFootholdTestPawn>(FVector(0, 0, 200));
	if (!TestNotNull(TEXT("pawn spawned"), Pawn))
	{
		return false;
	}

	UFootholdSmokeMode::TickCount = 0;
	Pawn->Mover->QueueNextMode(TEXT("FootholdSmoke"));
	W.Tick(0.5f);

	TestEqual(TEXT("mover is in the smoke mode"), Pawn->Mover->GetMovementModeName(), FName(TEXT("FootholdSmoke")));
	TestTrue(TEXT("smoke mode simulation ticked"), UFootholdSmokeMode::TickCount > 0);
	return true;
}

#endif
