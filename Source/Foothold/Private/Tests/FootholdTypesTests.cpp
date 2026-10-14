// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FootholdTypes.h"
#include "FootholdSharedSettings.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdSyncStateRoundTrip, "Foothold.Types.SyncStateNetSerializeRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootholdSyncStateRoundTrip::RunTest(const FString& Parameters)
{
	FFootholdSyncState A;
	A.Kind = EFootholdKind::Mantle;
	A.Phase = 1;
	A.MantleStart = FVector(1, 2, 3);
	A.MantleEnd = FVector(4, 5, 6);
	A.MantleDuration = 0.6f;
	A.TimeInMode = 0.25f;

	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	bool bOk = false;
	A.NetSerialize(Writer, nullptr, bOk);
	TestTrue(TEXT("write ok"), bOk);

	FFootholdSyncState B;
	FMemoryReader Reader(Bytes);
	B.NetSerialize(Reader, nullptr, bOk);
	TestTrue(TEXT("read ok"), bOk);

	TestEqual(TEXT("kind"), B.Kind, A.Kind);
	TestEqual(TEXT("phase"), B.Phase, A.Phase);
	TestEqual(TEXT("mantle end"), B.MantleEnd, A.MantleEnd);
	TestEqual(TEXT("mantle duration"), B.MantleDuration, A.MantleDuration);
	TestFalse(TEXT("no reconcile when equal"), B.ShouldReconcile(A));

	A.Kind = EFootholdKind::LedgeHang;
	B.Kind = EFootholdKind::LedgeHang;
	B.AnchorLocation = A.AnchorLocation + FVector(0, 0, 5);
	TestTrue(TEXT("reconcile on a 5 cm anchor error"), B.ShouldReconcile(A));

	B.AnchorLocation = A.AnchorLocation + FVector(0, 0, 1);
	TestFalse(TEXT("no reconcile on a 1 cm anchor error"), B.ShouldReconcile(A));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdSyncStateSlide, "Foothold.Types.SlideStateKeepsItsDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFootholdSyncStateSlide::RunTest(const FString& Parameters)
{
	FFootholdSyncState A;
	A.Kind = EFootholdKind::Slide;
	A.TravelDirection = FVector(0, 1, 0);
	A.TimeInMode = 0.5f;
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	bool bOk = false;
	A.NetSerialize(Writer, nullptr, bOk);
	FFootholdSyncState B;
	FMemoryReader Reader(Bytes);
	B.NetSerialize(Reader, nullptr, bOk);
	TestEqual(TEXT("kind"), B.Kind, EFootholdKind::Slide);
	TestEqual(TEXT("travel direction"), B.TravelDirection, A.TravelDirection);
	TestEqual(TEXT("mode name"), FFootholdKindUtils::GetModeName(EFootholdKind::Slide), FootholdModeNames::Slide);
	TestEqual(TEXT("kind from mode name"), FFootholdKindUtils::FromModeName(FootholdModeNames::Slide), EFootholdKind::Slide);
	TestEqual(TEXT("tag"), FFootholdKindUtils::GetTag(EFootholdKind::Slide), Foothold_Sliding.GetTag());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdSyncStateCompactWhenIdle, "Foothold.Types.SyncStateIsCompactForSimpleKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootholdSyncStateCompactWhenIdle::RunTest(const FString& Parameters)
{
	auto SizeOf = [](const FFootholdSyncState& State)
	{
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes);
		bool bOk = false;
		const_cast<FFootholdSyncState&>(State).NetSerialize(Writer, nullptr, bOk);
		return Bytes.Num();
	};

	FFootholdSyncState Glide;
	Glide.Kind = EFootholdKind::Glide;
	FFootholdSyncState Mantle;
	Mantle.Kind = EFootholdKind::Mantle;
	TestTrue(TEXT("glide state serializes smaller than mantle state"), SizeOf(Glide) < SizeOf(Mantle));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdInputsMerge, "Foothold.Types.InputsMergeKeepsPresses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootholdInputsMerge::RunTest(const FString& Parameters)
{
	FFootholdInputs Prior;
	Prior.bDropPressed = true;
	Prior.bGlideTogglePressed = true;

	FFootholdInputs Current;
	Current.Merge(Prior);
	TestTrue(TEXT("drop press survives merge"), Current.bDropPressed);
	TestTrue(TEXT("glide press survives merge"), Current.bGlideTogglePressed);
	TestFalse(TEXT("climb toggle not invented"), Current.bClimbTogglePressed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootholdSettingsDefaults, "Foothold.Types.SharedSettingsDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootholdSettingsDefaults::RunTest(const FString& Parameters)
{
	const UFootholdSharedSettings* Settings = GetDefault<UFootholdSharedSettings>();
	TestEqual(TEXT("climbable tag"), Settings->ClimbableTag, FName(TEXT("Climbable")));
	TestEqual(TEXT("policy is tagged-only"), Settings->ClimbPolicy, EFootholdClimbablePolicy::TaggedOnly);
	TestTrue(TEXT("implements the Mover settings interface"), Settings->GetClass()->ImplementsInterface(UMovementSettingsInterface::StaticClass()));
	TestEqual(TEXT("mode name"), FootholdModeNames::LedgeHang, FName(TEXT("LedgeHang")));
	TestTrue(TEXT("tag is valid"), Foothold_Active.GetTag().IsValid());
	return true;
}

#endif
