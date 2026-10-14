// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdInputReader.h"
#include "FootholdTypes.h"
#include "MoverDataModelTypes.h"
#include "MoverSimulationTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/UnrealType.h"
#include "UserDefinedStructSupport.h"

namespace UE::Foothold::Input
{
	// Velocity-type move input at or above this speed (cm/s) reads as full intent.
	constexpr float FullIntentSpeed = 100.f;
	// Move input pointing away from the surface by at least this much (dot) turns a jump into a drop.
	constexpr float AwayFromSurfaceDot = 0.5f;
}

FVector FFootholdInputReader::GetMoveInput(const FMoverTickStartData& Start)
{
	const FCharacterDefaultInputs* Inputs = Start.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
	if (!Inputs)
	{
		return FVector::ZeroVector;
	}

	const FVector Move = Inputs->GetMoveInput_WorldSpace();
	switch (Inputs->GetMoveInputType())
	{
	case EMoveInputType::DirectionalIntent:
		return Move.GetClampedToMaxSize(1.f);
	case EMoveInputType::Velocity:
		return Move.GetSafeNormal() * FMath::Min(1.f, Move.Size() / UE::Foothold::Input::FullIntentSpeed);
	default:
		return FVector::ZeroVector;
	}
}

bool FFootholdInputReader::JumpJustPressed(const FMoverTickStartData& Start)
{
	const FCharacterDefaultInputs* Inputs = Start.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
	return Inputs && Inputs->bIsJumpJustPressed;
}

const FFootholdInputs* FFootholdInputReader::GetFootholdInputs(const FMoverTickStartData& Start)
{
	return Start.InputCmd.InputCollection.FindDataByType<FFootholdInputs>();
}

bool FFootholdInputReader::IsCrouchHeld(const FMoverTickStartData& Start, FName UserStructFieldPrefix)
{
	if (const FFootholdInputs* Foothold = GetFootholdInputs(Start))
	{
		if (Foothold->bCrouchHeld)
		{
			return true;
		}
	}

	bool bUserCrouch = false;
	return !UserStructFieldPrefix.IsNone() && ReadUserDefinedBool(Start.InputCmd.InputCollection, UserStructFieldPrefix, bUserCrouch) && bUserCrouch;
}

bool FFootholdInputReader::WantsDrop(const FMoverTickStartData& Start, const FVector& FlatSurfaceNormal, FName UserStructFieldPrefix)
{
	if (const FFootholdInputs* Foothold = GetFootholdInputs(Start))
	{
		if (Foothold->bDropPressed)
		{
			return true;
		}
	}
	if (IsCrouchHeld(Start, UserStructFieldPrefix))
	{
		return true;
	}
	return JumpJustPressed(Start) && FVector::DotProduct(GetMoveInput(Start), FlatSurfaceNormal) >= UE::Foothold::Input::AwayFromSurfaceDot;
}

bool FFootholdInputReader::ReadUserDefinedBool(const FMoverDataCollection& Collection, FName FieldPrefix, bool& bOutValue)
{
	static const FName UserDefinedWrapperName(TEXT("MoverUserDefinedDataStruct"));
	const FString Prefix = FieldPrefix.ToString();

	for (const TSharedPtr<FMoverDataStructBase>& Data : Collection.GetDataArray())
	{
		if (!Data.IsValid() || !Data->GetScriptStruct() || Data->GetScriptStruct()->GetFName() != UserDefinedWrapperName)
		{
			continue;
		}

		const FMoverUserDefinedDataStruct* Wrapper = static_cast<const FMoverUserDefinedDataStruct*>(Data.Get());
		const UScriptStruct* UserStruct = Wrapper->StructInstance.GetScriptStruct();
		const uint8* Memory = Wrapper->StructInstance.GetMemory();
		if (!UserStruct || !Memory)
		{
			continue;
		}

		for (TFieldIterator<FBoolProperty> It(UserStruct); It; ++It)
		{
			// User-defined struct fields are stored as "WantsToCrouch_28_<guid>"; the authored name is the readable part.
			const FString AuthoredName = UserStruct->GetAuthoredNameForField(*It);
			if (AuthoredName.StartsWith(Prefix) || It->GetName().StartsWith(Prefix))
			{
				bOutValue = It->GetPropertyValue_InContainer(Memory);
				return true;
			}
		}
	}
	return false;
}
