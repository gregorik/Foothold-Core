// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "FootholdDemoCourse.h"
#include "FootholdLadder.h"
#include "FootholdSharedSettings.h"
#include "FootholdTraceLibrary.h"
#include "MoverComponent.h"
#include "MoverSimulationTypes.h"
#include "Tests/FootholdTestCharacter.h"
#include "Tests/FootholdTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdDemoCourseBuilds, "Foothold.Demo.CourseBuildsAllSections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdDemoCourseBuilds::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	AFootholdDemoCourse* Course = W.Spawn<AFootholdDemoCourse>(FVector(1000, 2000, 0), FRotator(0, 30, 0));
	if (!TestNotNull(TEXT("course spawns"), Course))
	{
		return false;
	}

	TArray<UTextRenderComponent*> Labels;
	Course->GetComponents(Labels);
	TestEqual(TEXT("a label per section, and one at the start"), Labels.Num(), 5);

	int32 Ladders = 0;
	for (TActorIterator<AFootholdLadder> It(W.World); It; ++It)
	{
		++Ladders;
	}
	TestEqual(TEXT("one ladder"), Ladders, 1);

	const FTransform LedgeStart = Course->GetSectionStart(AFootholdDemoCourse::LedgeSection);
	TestFalse(TEXT("the ledge section has a start point"), LedgeStart.Equals(FTransform::Identity));
	UFootholdSharedSettings* Settings = NewObject<UFootholdSharedSettings>();
	const FFootholdQueryContext Ctx = FFootholdQueryContext::Make(W.World, LedgeStart.GetLocation(), FVector::UpVector, LedgeStart.GetRotation().GetForwardVector(), 34.f, 88.f, Settings);
	const FFootholdLedge Ledge = UFootholdTraceLibrary::FindLedge(Ctx, LedgeStart.GetRotation().GetForwardVector(), 100.f, 320.f, 200.f);
	TestTrue(TEXT("the ledge wall top is found from its start point"), Ledge.bValid);
	TestTrue(TEXT("the ledge wall can be hung from"), Ledge.bHangClear);

	for (const FName Section : AFootholdDemoCourse::GetSectionNames())
	{
		TestFalse(*FString::Printf(TEXT("section %s has a start point"), *Section.ToString()), Course->GetSectionStart(Section).Equals(FTransform::Identity));
	}
	const FTransform TowerTop = Course->GetSectionStart(AFootholdDemoCourse::TowerTop);
	FHitResult Top;
	TestTrue(TEXT("the tower top is 18 m up and stands on the tower"),
		W.World->LineTraceSingleByChannel(Top, TowerTop.GetLocation(), TowerTop.GetLocation() - FVector(0, 0, 300), ECC_Visibility)
		&& Top.ImpactPoint.Z - Course->GetActorLocation().Z > 1800.0);

	const FTransform PlayerStart = Course->GetPlayerStart();
	FHitResult Ground;
	TestTrue(TEXT("the player start stands on the course floor"),
		W.World->LineTraceSingleByChannel(Ground, PlayerStart.GetLocation(), PlayerStart.GetLocation() - FVector(0, 0, 300), ECC_Visibility));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdDemoCourseRebuilds, "Foothold.Demo.RebuildIsRepeatable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdDemoCourseRebuilds::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	auto CountInstances = [](const AFootholdDemoCourse* Course)
	{
		TArray<UInstancedStaticMeshComponent*> Meshes;
		Course->GetComponents(Meshes);
		int32 Count = 0;
		for (const UInstancedStaticMeshComponent* Mesh : Meshes)
		{
			Count += Mesh->GetInstanceCount();
		}
		return Count;
	};
	AFootholdDemoCourse* Course = W.Spawn<AFootholdDemoCourse>(FVector::ZeroVector, FRotator::ZeroRotator);
	const int32 Built = CountInstances(Course);
	TestTrue(TEXT("the course has geometry"), Built > 15);
	Course->Rebuild();
	TestEqual(TEXT("rebuilding gives the same course, not a second copy"), CountInstances(Course), Built);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdDemoSpawnMovesPawn, "Foothold.Demo.SpawnDemoMovesPawnToStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdDemoSpawnMovesPawn::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	AFootholdTestCharacter* Pawn = W.Spawn<AFootholdTestCharacter>(FVector(0, 0, 92), FRotator(0, 90, 0));
	W.Tick(0.2f);

	const AFootholdDemoCourse* Course = AFootholdDemoCourse::SpawnDemo(W.World, TOptional<FVector>(), Pawn);
	if (!TestNotNull(TEXT("course spawns in front of the pawn"), Course))
	{
		return false;
	}
	TestEqual(TEXT("course faces where the pawn faced"), Course->GetActorRotation().Yaw, 90.0, 1.0);
	TestEqual(TEXT("course is 5 m in front"), Course->GetActorLocation().Y, 500.0, 1.0);
	TestEqual(TEXT("course floor sits on the ground"), Course->GetActorLocation().Z, 2.0, 1.0);

	W.Tick(0.5f);
	const FVector Start = Course->GetPlayerStart().GetLocation();
	TestTrue(TEXT("pawn moved to the course start"), FVector::Dist2D(Pawn->GetActorLocation(), Start) < 20.f);
	TestEqual(TEXT("pawn stands on the course floor"), Pawn->GetMoverComponent()->GetMovementModeName(), DefaultModeNames::Walking);
	return true;
}

#endif
