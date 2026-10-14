// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdTypes.h"
#include "Components/PrimitiveComponent.h"
#include "FootholdLadderComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdTypes)

namespace FootholdModeNames
{
	const FName Climbing = TEXT("Climbing");
	const FName LedgeHang = TEXT("LedgeHang");
	const FName Ladder = TEXT("Ladder");
	const FName Mantle = TEXT("Mantle");
	const FName Gliding = TEXT("Gliding");
	const FName WallRun = TEXT("WallRun");
	const FName Slide = TEXT("Slide");
}

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Foothold_Active, "Foothold.Active", "A Foothold traversal mode is active");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Foothold_Climbing, "Foothold.Climbing", "Foothold free climbing");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Foothold_LedgeHang, "Foothold.LedgeHang", "Foothold hanging from a ledge");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Foothold_Ladder, "Foothold.Ladder", "Foothold on a ladder");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Foothold_Mantle, "Foothold.Mantle", "Foothold mantling onto a ledge");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Foothold_Gliding, "Foothold.Gliding", "Foothold gliding");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Foothold_WallRun, "Foothold.WallRun", "Foothold wall running");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Foothold_Sliding, "Foothold.Sliding", "Foothold sliding");

namespace UE::Foothold::Private
{
	// Anchors and positions this far apart (cm) between client and server trigger a correction.
	constexpr float ReconcileDistance = 2.f;

	static void SerializeObject(FArchive& Ar, UPackageMap* Map, TWeakObjectPtr<UObject>& Ptr, UClass* Class)
	{
		UObject* Object = Ptr.Get();
		if (Map)
		{
			Map->SerializeObject(Ar, Class, Object);
		}
		else
		{
			Ar << Object;
		}
		if (Ar.IsLoading())
		{
			Ptr = Object;
		}
	}

	template<typename T>
	static void SerializeWeak(FArchive& Ar, UPackageMap* Map, TWeakObjectPtr<T>& Ptr)
	{
		TWeakObjectPtr<UObject> AsObject = Ptr.Get();
		SerializeObject(Ar, Map, AsObject, T::StaticClass());
		if (Ar.IsLoading())
		{
			Ptr = Cast<T>(AsObject.Get());
		}
	}
}

bool FFootholdSyncState::UsesAnchor() const
{
	return Kind == EFootholdKind::Climb || Kind == EFootholdKind::LedgeHang || Kind == EFootholdKind::WallRun;
}

FMoverDataStructBase* FFootholdSyncState::Clone() const
{
	return new FFootholdSyncState(*this);
}

bool FFootholdSyncState::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	using namespace UE::Foothold::Private;

	Super::NetSerialize(Ar, Map, bOutSuccess);

	uint8 KindByte = static_cast<uint8>(Kind);
	Ar << KindByte;
	Kind = static_cast<EFootholdKind>(KindByte);
	Ar << Phase;
	Ar << TimeInMode;
	Ar << Timer;

	switch (Kind)
	{
	case EFootholdKind::Climb:
	case EFootholdKind::LedgeHang:
		Ar << AnchorLocation;
		Ar << SurfaceNormal;
		Ar << TravelDirection;
		Ar << AnchorComponentLocation;
		SerializeWeak(Ar, Map, AnchorComponent);
		break;

	case EFootholdKind::Ladder:
		Ar << LadderPosition;
		SerializeWeak(Ar, Map, Ladder);
		break;

	case EFootholdKind::Mantle:
		Ar << MantleStart;
		Ar << MantleEnd;
		Ar << MantleDuration;
		Ar << SurfaceNormal;
		break;

	case EFootholdKind::Glide:
	case EFootholdKind::Slide:
		Ar << TravelDirection;
		break;

	case EFootholdKind::WallRun:
		Ar << AnchorLocation;
		Ar << SurfaceNormal;
		Ar << TravelDirection;
		break;

	default:
		break;
	}

	bOutSuccess = !Ar.IsError();
	return true;
}

void FFootholdSyncState::ToString(FAnsiStringBuilderBase& Out) const
{
	Super::ToString(Out);
	Out.Appendf("Foothold Kind: %d  Phase: %d  TimeInMode: %.3f  Timer: %.3f\n", static_cast<int32>(Kind), Phase, TimeInMode, Timer);
	Out.Appendf("Anchor: %s  Normal: %s  Travel: %s\n", TCHAR_TO_ANSI(*AnchorLocation.ToCompactString()), TCHAR_TO_ANSI(*SurfaceNormal.ToCompactString()), TCHAR_TO_ANSI(*TravelDirection.ToCompactString()));
	Out.Appendf("Mantle: %s -> %s over %.3f  Ladder: %s @ %.1f\n", TCHAR_TO_ANSI(*MantleStart.ToCompactString()), TCHAR_TO_ANSI(*MantleEnd.ToCompactString()), MantleDuration, TCHAR_TO_ANSI(*GetNameSafe(Ladder.Get())), LadderPosition);
}

bool FFootholdSyncState::ShouldReconcile(const FMoverDataStructBase& AuthorityState) const
{
	using namespace UE::Foothold::Private;

	const FFootholdSyncState& Authority = static_cast<const FFootholdSyncState&>(AuthorityState);
	if (Kind != Authority.Kind || Phase != Authority.Phase)
	{
		return true;
	}

	switch (Kind)
	{
	case EFootholdKind::Climb:
	case EFootholdKind::LedgeHang:
	case EFootholdKind::WallRun:
		return FVector::DistSquared(AnchorLocation, Authority.AnchorLocation) > FMath::Square(ReconcileDistance);

	case EFootholdKind::Ladder:
		return Ladder != Authority.Ladder || FMath::Abs(LadderPosition - Authority.LadderPosition) > ReconcileDistance;

	case EFootholdKind::Mantle:
		return FVector::DistSquared(MantleEnd, Authority.MantleEnd) > FMath::Square(ReconcileDistance);

	default:
		return false;
	}
}

void FFootholdSyncState::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	const FFootholdSyncState& FromState = static_cast<const FFootholdSyncState&>(From);
	const FFootholdSyncState& ToState = static_cast<const FFootholdSyncState&>(To);

	*this = ToState;
	if (FromState.Kind == ToState.Kind)
	{
		AnchorLocation = FMath::Lerp(FromState.AnchorLocation, ToState.AnchorLocation, Pct);
		TimeInMode = FMath::Lerp(FromState.TimeInMode, ToState.TimeInMode, Pct);
		LadderPosition = FMath::Lerp(FromState.LadderPosition, ToState.LadderPosition, Pct);
	}
}

FMoverDataStructBase* FFootholdInputs::Clone() const
{
	return new FFootholdInputs(*this);
}

bool FFootholdInputs::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Super::NetSerialize(Ar, Map, bOutSuccess);

	uint8 Bits = (bDropPressed ? 1 : 0) | (bGlideTogglePressed ? 2 : 0) | (bClimbTogglePressed ? 4 : 0) | (bCrouchHeld ? 8 : 0);
	Ar.SerializeBits(&Bits, 4);
	if (Ar.IsLoading())
	{
		bDropPressed = (Bits & 1) != 0;
		bGlideTogglePressed = (Bits & 2) != 0;
		bClimbTogglePressed = (Bits & 4) != 0;
		bCrouchHeld = (Bits & 8) != 0;
	}

	bOutSuccess = !Ar.IsError();
	return true;
}

void FFootholdInputs::ToString(FAnsiStringBuilderBase& Out) const
{
	Super::ToString(Out);
	Out.Appendf("Foothold inputs: drop=%d glide=%d climb=%d crouch=%d\n", bDropPressed, bGlideTogglePressed, bClimbTogglePressed, bCrouchHeld);
}

bool FFootholdInputs::ShouldReconcile(const FMoverDataStructBase& AuthorityState) const
{
	const FFootholdInputs& Authority = static_cast<const FFootholdInputs&>(AuthorityState);
	return bDropPressed != Authority.bDropPressed || bGlideTogglePressed != Authority.bGlideTogglePressed
		|| bClimbTogglePressed != Authority.bClimbTogglePressed || bCrouchHeld != Authority.bCrouchHeld;
}

void FFootholdInputs::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	*this = static_cast<const FFootholdInputs&>(Pct < 0.5f ? From : To);
}

void FFootholdInputs::Merge(const FMoverDataStructBase& From)
{
	const FFootholdInputs& Prior = static_cast<const FFootholdInputs&>(From);
	bDropPressed |= Prior.bDropPressed;
	bGlideTogglePressed |= Prior.bGlideTogglePressed;
	bClimbTogglePressed |= Prior.bClimbTogglePressed;
}

FName FFootholdKindUtils::GetModeName(EFootholdKind Kind)
{
	switch (Kind)
	{
	case EFootholdKind::Climb:     return FootholdModeNames::Climbing;
	case EFootholdKind::LedgeHang: return FootholdModeNames::LedgeHang;
	case EFootholdKind::Ladder:    return FootholdModeNames::Ladder;
	case EFootholdKind::Mantle:    return FootholdModeNames::Mantle;
	case EFootholdKind::Glide:     return FootholdModeNames::Gliding;
	case EFootholdKind::WallRun:   return FootholdModeNames::WallRun;
	case EFootholdKind::Slide:     return FootholdModeNames::Slide;
	default:                       return NAME_None;
	}
}

EFootholdKind FFootholdKindUtils::FromModeName(FName ModeName)
{
	if (ModeName == FootholdModeNames::Climbing)  { return EFootholdKind::Climb; }
	if (ModeName == FootholdModeNames::LedgeHang) { return EFootholdKind::LedgeHang; }
	if (ModeName == FootholdModeNames::Ladder)    { return EFootholdKind::Ladder; }
	if (ModeName == FootholdModeNames::Mantle)    { return EFootholdKind::Mantle; }
	if (ModeName == FootholdModeNames::Gliding)   { return EFootholdKind::Glide; }
	if (ModeName == FootholdModeNames::WallRun)   { return EFootholdKind::WallRun; }
	if (ModeName == FootholdModeNames::Slide)   { return EFootholdKind::Slide; }
	return EFootholdKind::None;
}

FGameplayTag FFootholdKindUtils::GetTag(EFootholdKind Kind)
{
	switch (Kind)
	{
	case EFootholdKind::Climb:     return Foothold_Climbing;
	case EFootholdKind::LedgeHang: return Foothold_LedgeHang;
	case EFootholdKind::Ladder:    return Foothold_Ladder;
	case EFootholdKind::Mantle:    return Foothold_Mantle;
	case EFootholdKind::Glide:     return Foothold_Gliding;
	case EFootholdKind::WallRun:   return Foothold_WallRun;
	case EFootholdKind::Slide:     return Foothold_Sliding;
	default:                       return FGameplayTag();
	}
}
