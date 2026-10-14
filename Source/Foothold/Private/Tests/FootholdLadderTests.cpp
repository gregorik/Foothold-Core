// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FootholdTestWorld.h"
#include "FootholdLadder.h"
#include "FootholdLadderComponent.h"
#include "FootholdWorldSubsystem.h"
#include "UObject/CoreNet.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLadderRegistersAndProjects, "Foothold.Ladder.RegistersAndProjects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootholdLadderRegistersAndProjects::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	AFootholdLadder* Ladder = W.Spawn<AFootholdLadder>(FVector(100, 0, 0), FRotator(0, 90, 0));
	Ladder->GetLadder()->Height = 300.f;
	Ladder->RebuildMesh();

	UFootholdWorldSubsystem* Subsystem = W.World->GetSubsystem<UFootholdWorldSubsystem>();
	if (!TestNotNull(TEXT("subsystem"), Subsystem))
	{
		return false;
	}
	TestEqual(TEXT("one ladder registered"), Subsystem->GetLadders().Num(), 1);

	const UFootholdLadderComponent* Comp = Ladder->GetLadder();
	TestTrue(TEXT("facing is the actor's forward"), Comp->GetFacing().Equals(Ladder->GetActorForwardVector(), 1e-3));
	TestTrue(TEXT("bottom is the actor location"), Comp->GetBottom().Equals(FVector(100, 0, 0), 1e-2));
	TestTrue(TEXT("top is 300 cm up"), Comp->GetTop().Equals(FVector(100, 0, 300), 1e-2));

	const FVector InFront = Comp->GetBottom() + Comp->GetFacing() * 45.f;
	TestEqual(TEXT("projects mid-height"), Comp->ProjectToLadder(InFront + FVector(0, 0, 150)), 150.f, 0.01f);
	TestEqual(TEXT("clamps above the top"), Comp->ProjectToLadder(InFront + FVector(0, 0, 900)), 300.f, 0.01f);
	TestEqual(TEXT("clamps below the bottom"), Comp->ProjectToLadder(InFront - FVector(0, 0, 50)), 0.f, 0.01f);
	TestTrue(TEXT("climb point is in front of the ladder"), Comp->GetClimbLocation(120.f).Equals(Comp->GetBottom() + FVector(0, 0, 120) + Comp->GetFacing() * Comp->StandOffDistance, 1e-2));
	TestTrue(TEXT("built rails and rungs"), Ladder->GetRungCount() >= 9);

	Ladder->Destroy();
	W.Tick(0.05f);
	TestEqual(TEXT("unregisters when destroyed"), Subsystem->GetLadders().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLadderNearestRung, "Foothold.Ladder.NearestRung",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootholdLadderNearestRung::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	AFootholdLadder* Ladder = W.Spawn<AFootholdLadder>(FVector::ZeroVector);
	UFootholdLadderComponent* Comp = Ladder->GetLadder();
	Comp->RungSpacing = 30.f;
	Comp->Height = 300.f;
	TestEqual(TEXT("snaps down"), Comp->GetNearestRungPosition(44.f), 30.f, 0.01f);
	TestEqual(TEXT("snaps up"), Comp->GetNearestRungPosition(46.f), 60.f, 0.01f);
	TestEqual(TEXT("never past the top"), Comp->GetNearestRungPosition(299.f), 300.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdLadderReplicates, "Foothold.Ladder.ReplicatesForRuntimeSpawns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootholdLadderReplicates::RunTest(const FString& Parameters)
{
	// The Ladder mode's sync state points at the ladder component, so clients must be able to resolve it even
	// when the ladder is spawned at runtime (the demo course's child actor, a ladder spawned by game code):
	// the actor replicates, and so do the settings that shape the climb.
	const AFootholdLadder* Ladder = GetDefault<AFootholdLadder>();
	TestTrue(TEXT("AFootholdLadder replicates"), Ladder->GetIsReplicated());
	TestTrue(TEXT("its ladder component replicates"), Ladder->GetLadder()->GetIsReplicated());

	// As the net driver does: replication indices first, then the registered properties.
	UFootholdLadderComponent::StaticClass()->SetUpRuntimeReplicationData();
	TArray<FLifetimeProperty> Props;
	Ladder->GetLadder()->GetLifetimeReplicatedProps(Props);
	TSet<FName> Names;
	for (const FLifetimeProperty& Prop : Props)
	{
		for (TFieldIterator<FProperty> It(UFootholdLadderComponent::StaticClass()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Net) && It->RepIndex == Prop.RepIndex)
			{
				Names.Add(It->GetFName());
			}
		}
	}
	for (const TCHAR* Name : { TEXT("Height"), TEXT("Width"), TEXT("RungSpacing"), TEXT("StandOffDistance"), TEXT("bMantleOffTop") })
	{
		TestTrue(FString::Printf(TEXT("%s replicates"), Name), Names.Contains(FName(Name)));
	}
	return true;
}

#endif
