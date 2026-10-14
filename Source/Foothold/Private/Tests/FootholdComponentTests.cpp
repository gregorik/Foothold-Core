// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FootholdTestWorld.h"
#include "Tests/FootholdTestCharacter.h"
#include "FootholdComponent.h"
#include "FootholdInputReader.h"
#include "FootholdTypes.h"
#include "MoverDataModelTypes.h"
#include "MoverSimulationTypes.h"
#include "FootholdLadder.h"
#include "FootholdLadderComponent.h"
#include "MoverComponent.h"
#include "FootholdMoverComponent.h"
#include "FootholdLog.h"
#include "Misc/OutputDeviceRedirector.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdComponentProducesInputsOnce, "Foothold.Component.ProducesInputsOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdComponentProducesInputsOnce::RunTest(const FString& Parameters)
{
	UFootholdComponent* Comp = NewObject<UFootholdComponent>(GetTransientPackage());
	Comp->RequestDrop();
	Comp->RequestGlideToggle();

	FMoverInputCmdContext First;
	Comp->ProduceInput_Implementation(16, First);
	const FFootholdInputs* FirstInputs = First.InputCollection.FindDataByType<FFootholdInputs>();
	if (!TestNotNull(TEXT("adds Foothold inputs"), FirstInputs))
	{
		return false;
	}
	TestTrue(TEXT("drop on the first command"), FirstInputs->bDropPressed);
	TestTrue(TEXT("glide toggle on the first command"), FirstInputs->bGlideTogglePressed);

	FMoverInputCmdContext Second;
	Comp->ProduceInput_Implementation(16, Second);
	const FFootholdInputs* SecondInputs = Second.InputCollection.FindDataByType<FFootholdInputs>();
	TestTrue(TEXT("still adds the block"), SecondInputs != nullptr);
	TestFalse(TEXT("drop cleared after one command"), SecondInputs && SecondInputs->bDropPressed);
	TestFalse(TEXT("glide cleared after one command"), SecondInputs && SecondInputs->bGlideTogglePressed);

	Comp->SetCrouchHeld(true);
	FMoverInputCmdContext Third;
	Comp->ProduceInput_Implementation(16, Third);
	FMoverInputCmdContext Fourth;
	Comp->ProduceInput_Implementation(16, Fourth);
	TestTrue(TEXT("crouch is held, not one-shot"), Fourth.InputCollection.FindDataByType<FFootholdInputs>()->bCrouchHeld);
	return true;
}

namespace UE::Foothold::Tests
{
	static FMoverTickStartData MakeStart(const FVector& Move, bool bJump, bool bDrop = false, bool bCrouch = false)
	{
		FMoverTickStartData Start;
		FCharacterDefaultInputs& Defaults = Start.InputCmd.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
		Defaults.SetMoveInput(EMoveInputType::DirectionalIntent, Move);
		Defaults.bIsJumpJustPressed = bJump;
		Defaults.bIsJumpPressed = bJump;
		FFootholdInputs& Foothold = Start.InputCmd.InputCollection.FindOrAddMutableDataByType<FFootholdInputs>();
		Foothold.bDropPressed = bDrop;
		Foothold.bCrouchHeld = bCrouch;
		return Start;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdInputWantsDropRules, "Foothold.Input.WantsDropRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdInputWantsDropRules::RunTest(const FString& Parameters)
{
	using namespace UE::Foothold::Tests;
	const FVector WallNormal(-1, 0, 0);	// the wall is at +X, its normal points back at us
	const FName Prefix = TEXT("WantsToCrouch");

	TestTrue(TEXT("jump while pulling away from the wall"), FFootholdInputReader::WantsDrop(MakeStart(FVector(-1, 0, 0), true), WallNormal, Prefix));
	TestFalse(TEXT("jump while pushing into the wall"), FFootholdInputReader::WantsDrop(MakeStart(FVector(1, 0, 0), true), WallNormal, Prefix));
	TestFalse(TEXT("pulling away without a jump"), FFootholdInputReader::WantsDrop(MakeStart(FVector(-1, 0, 0), false), WallNormal, Prefix));
	TestTrue(TEXT("explicit drop request"), FFootholdInputReader::WantsDrop(MakeStart(FVector::ZeroVector, false, true), WallNormal, Prefix));
	TestTrue(TEXT("crouch held"), FFootholdInputReader::WantsDrop(MakeStart(FVector::ZeroVector, false, false, true), WallNormal, Prefix));
	TestFalse(TEXT("no input"), FFootholdInputReader::WantsDrop(MakeStart(FVector::ZeroVector, false), WallNormal, Prefix));

	TestTrue(TEXT("jump just pressed"), FFootholdInputReader::JumpJustPressed(MakeStart(FVector::ZeroVector, true)));
	TestTrue(TEXT("move input in world space"), FFootholdInputReader::GetMoveInput(MakeStart(FVector(0, 1, 0), false)).Equals(FVector(0, 1, 0), 1e-3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdCharacterLandsAndWalks, "Foothold.Character.LandsAndWalks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdCharacterLandsAndWalks::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	AFootholdTestCharacter* Pawn = W.Spawn<AFootholdTestCharacter>(FVector(0, 0, 300));
	if (!TestNotNull(TEXT("spawned"), Pawn))
	{
		return false;
	}
	TestNotNull(TEXT("has a Foothold component"), Pawn->GetFootholdComponent());

	const bool bLanded = W.TickUntil([Pawn]() { return Pawn->GetMoverComponent()->GetMovementModeName() == DefaultModeNames::Walking; }, 3.f);
	TestTrue(TEXT("lands and walks"), bLanded);
	TestEqual(TEXT("capsule rests on the floor"), Pawn->GetActorLocation().Z, 88.0, 3.0);

	Pawn->SetScriptedMove(FVector(1, 0, 0));
	W.Tick(1.f);
	TestTrue(TEXT("scripted input moves it"), Pawn->GetActorLocation().X > 100.0);

	const FFootholdAnimData Anim = Pawn->GetFootholdComponent()->GetAnimData();
	TestEqual(TEXT("no Foothold kind while walking"), Anim.Kind, EFootholdKind::None);
	TestEqual(TEXT("no Foothold pose weight while walking"), Anim.Weight, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdComponentHangLimbTargets, "Foothold.Component.HangLimbTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdComponentHangLimbTargets::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	W.SpawnBox(FVector(150, 0, 125), FVector(50, 200, 125));	// wall face X=100, top 250
	AFootholdTestCharacter* Pawn = W.Spawn<AFootholdTestCharacter>(FVector(58, 0, 190));
	const UMoverComponent* Mover = Pawn->GetMoverComponent();
	if (!TestTrue(TEXT("hanging"), W.TickUntil([Mover]() { return Mover->GetMovementModeName() == FootholdModeNames::LedgeHang; }, 1.f)))
	{
		return false;
	}
	W.Tick(0.5f);
	const FFootholdAnimData Anim = Pawn->GetFootholdComponent()->GetAnimData();
	TestEqual(TEXT("posing the hang"), Anim.Kind, EFootholdKind::LedgeHang);
	TestEqual(TEXT("fully blended in"), Anim.Weight, 1.f);
	TestTrue(TEXT("both hands have targets"), Anim.bLeftHandTargetValid && Anim.bRightHandTargetValid);
	// Wrists sit just above the top so the hands grip the edge (HandGripUp).
	const double GripZ = 250.0 + Pawn->GetFootholdComponent()->HandGripUp;
	TestEqual(TEXT("left hand gripping at the edge height"), Anim.LeftHandTarget.Z, GripZ, 2.0);
	TestEqual(TEXT("right hand gripping at the edge height"), Anim.RightHandTarget.Z, GripZ, 2.0);
	TestTrue(TEXT("hands near the wall face"), FMath::Abs(Anim.LeftHandTarget.X - 100.0) < 12.0 && FMath::Abs(Anim.RightHandTarget.X - 100.0) < 12.0);
	TestTrue(TEXT("hands shoulder-width apart along the edge"), FMath::Abs(Anim.LeftHandTarget.Y - Anim.RightHandTarget.Y) > 30.0);
	TestTrue(TEXT("feet have targets against the wall"), Anim.bLeftFootTargetValid && Anim.bRightFootTargetValid);
	TestTrue(TEXT("feet below the hands"), Anim.LeftFootTarget.Z < 150.0);
	TestTrue(TEXT("left hand is on the left (facing +X, left is -Y)"), Anim.LeftHandTarget.Y < Anim.RightHandTarget.Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdComponentLadderLimbTargets, "Foothold.Component.LadderLimbTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdComponentLadderLimbTargets::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	AFootholdLadder* Ladder = W.Spawn<AFootholdLadder>(FVector(100, 0, 0), FRotator(0, 180, 0));
	AFootholdTestCharacter* Pawn = W.Spawn<AFootholdTestCharacter>(FVector(20, 0, 90));
	const UMoverComponent* Mover = Pawn->GetMoverComponent();
	W.TickUntil([Mover]() { return Mover->GetMovementModeName() == DefaultModeNames::Walking; }, 2.f);
	Pawn->SetScriptedMove(FVector(1, 0, 0));
	if (!TestTrue(TEXT("on the ladder"), W.TickUntil([Mover]() { return Mover->GetMovementModeName() == FootholdModeNames::Ladder; }, 1.5f)))
	{
		return false;
	}
	W.Tick(0.6f);
	const FFootholdAnimData Anim = Pawn->GetFootholdComponent()->GetAnimData();
	const float HalfWidth = Ladder->GetLadder()->Width * 0.5f;
	TestTrue(TEXT("hands have targets"), Anim.bLeftHandTargetValid && Anim.bRightHandTargetValid);
	TestEqual(TEXT("left hand on the left rail"), FMath::Abs(Anim.LeftHandTarget.Y), (double)HalfWidth, 2.0);
	TestEqual(TEXT("right hand on the right rail"), FMath::Abs(Anim.RightHandTarget.Y), (double)HalfWidth, 2.0);
	TestEqual(TEXT("hands at the ladder"), Anim.LeftHandTarget.X, 100.0, 6.0);
	TestTrue(TEXT("feet have targets on rungs"), Anim.bLeftFootTargetValid && Anim.bRightFootTargetValid);
	return true;
}

namespace UE::Foothold::Tests
{
	/** Collects LogFoothold lines while it exists. */
	struct FFootholdLogCapture : public FOutputDevice
	{
		TArray<FString> Lines;

		FFootholdLogCapture()
		{
			LogFoothold.SetVerbosity(ELogVerbosity::Verbose);
			GLog->AddOutputDevice(this);
		}
		virtual ~FFootholdLogCapture() override
		{
			GLog->RemoveOutputDevice(this);
			LogFoothold.SetVerbosity(ELogVerbosity::Log);
		}
		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category == LogFoothold.GetCategoryName())
			{
				Lines.Add(Message);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdComponentLogsModeChanges, "Foothold.Component.LogsModeChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdComponentLogsModeChanges::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	UE::Foothold::Tests::FFootholdLogCapture Capture;
	AFootholdTestCharacter* Pawn = W.Spawn<AFootholdTestCharacter>(FVector(0, 0, 300));
	W.TickUntil([Pawn]() { return Pawn->GetMoverComponent()->GetMovementModeName() == DefaultModeNames::Walking; }, 2.f);

	const FString* Landed = Capture.Lines.FindByPredicate([](const FString& Line) { return Line.Contains(TEXT("Falling -> Walking")); });
	if (TestNotNull(TEXT("logs the landing as a mode change"), Landed))
	{
		TestTrue(TEXT("names the role"), Landed->Contains(TEXT("[Standalone/Authority]")));
		TestTrue(TEXT("names the pawn"), Landed->Contains(Pawn->GetName()));
		TestTrue(TEXT("stamps the frame"), Landed->Contains(TEXT("@frame")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdMoverDefaultsHaveSharedSettings, "Foothold.Mover.DefaultTemplateHasSharedSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdMoverDefaultsHaveSharedSettings::RunTest(const FString& Parameters)
{
	// A Blueprint child of a pawn with this component validates its template, which never registers.
	const UFootholdMoverComponent* Template = GetDefault<UFootholdMoverComponent>();
	auto CheckClasses = [this, Template](const TArray<TSubclassOf<UObject>>& Classes, const FString& Owner)
	{
		for (const TSubclassOf<UObject>& Class : Classes)
		{
			TestNotNull(*FString::Printf(TEXT("%s finds its %s"), *Owner, *GetNameSafe(Class.Get())), Template->FindSharedSettings(Class.Get()));
		}
	};
	for (const TPair<FName, TObjectPtr<UBaseMovementMode>>& Mode : Template->MovementModes)
	{
		CheckClasses(Mode.Value->SharedSettingsClasses, Mode.Key.ToString());
		for (const UBaseMovementModeTransition* Transition : Mode.Value->Transitions)
		{
			CheckClasses(Transition->SharedSettingsClasses, Mode.Key.ToString() + TEXT(" transition"));
		}
	}
	for (const UBaseMovementModeTransition* Transition : Template->Transitions)
	{
		CheckClasses(Transition->SharedSettingsClasses, GetNameSafe(Transition));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdComponentReachesForLedge, "Foothold.Component.ReachesForLedgeWhileFalling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdComponentReachesForLedge::RunTest(const FString& Parameters)
{
	{
		FFootholdTestWorld W;
		W.SpawnBox(FVector(150, 0, 175), FVector(50, 400, 175));	// wall face X=100, top 350: above the hands, too high to grab
		AFootholdTestCharacter* Pawn = W.Spawn<AFootholdTestCharacter>(FVector(58, 0, 200));
		W.Tick(0.15f);
		const FFootholdAnimData Anim = Pawn->GetFootholdComponent()->GetAnimData();
		AddInfo(FString::Printf(TEXT("mode %s, reach %.2f, targets %s / %s"), *Pawn->GetMoverComponent()->GetMovementModeName().ToString(), Anim.ReachWeight,
			*Anim.LeftReachTarget.ToCompactString(), *Anim.RightReachTarget.ToCompactString()));
		TestEqual(TEXT("still falling"), Pawn->GetMoverComponent()->GetMovementModeName(), DefaultModeNames::Falling);
		TestTrue(TEXT("reaching for the ledge"), Anim.ReachWeight > 0.5f);
		TestTrue(TEXT("reach targets at the edge"), FMath::Abs(Anim.LeftReachTarget.X - 100.0) < 10.0 && FMath::Abs(Anim.LeftReachTarget.Z - 350.0) < 10.0
			&& FMath::Abs(Anim.RightReachTarget.Z - 350.0) < 10.0);
		TestTrue(TEXT("hands either side of the body"), Anim.RightReachTarget.Y - Anim.LeftReachTarget.Y > 30.0);
		TestTrue(TEXT("reach normal is the wall's"), Anim.ReachNormal.Equals(FVector(-1, 0, 0), 0.05));
	}
	{
		FFootholdTestWorld W;
		AFootholdTestCharacter* Pawn = W.Spawn<AFootholdTestCharacter>(FVector(58, 0, 200));
		W.Tick(0.15f);
		TestEqual(TEXT("no reach without a ledge"), Pawn->GetFootholdComponent()->GetAnimData().ReachWeight, 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdComponentHangHandsFollowShimmy, "Foothold.Component.HangHandsFollowShimmy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdComponentHangHandsFollowShimmy::RunTest(const FString& Parameters)
{
	FFootholdTestWorld W;
	W.SpawnBox(FVector(150, 0, 125), FVector(50, 400, 125));	// wall face X=100, top 250, 8 m long
	AFootholdTestCharacter* Pawn = W.Spawn<AFootholdTestCharacter>(FVector(58, -200, 190));
	const UMoverComponent* Mover = Pawn->GetMoverComponent();
	if (!TestTrue(TEXT("hanging"), W.TickUntil([Mover]() { return Mover->GetMovementModeName() == FootholdModeNames::LedgeHang; }, 1.f)))
	{
		return false;
	}
	W.Tick(0.3f);
	Pawn->SetScriptedMove(FVector(0, 1, 0));
	W.Tick(1.2f);
	Pawn->SetScriptedMove(FVector::ZeroVector);
	W.Tick(0.8f);	// hand over hand: each hand takes its turn to come back beside the body

	const FFootholdAnimData Anim = Pawn->GetFootholdComponent()->GetAnimData();
	const double BodyY = Pawn->GetActorLocation().Y;
	AddInfo(FString::Printf(TEXT("after the shimmy: body Y %.1f, hands Y %.1f / %.1f"), BodyY, Anim.LeftHandTarget.Y, Anim.RightHandTarget.Y));
	TestTrue(TEXT("shimmied right"), BodyY > -150.0);
	TestEqual(TEXT("hands centred on the body"), (Anim.LeftHandTarget.Y + Anim.RightHandTarget.Y) * 0.5, BodyY, 6.0);
	return true;
}

#endif
