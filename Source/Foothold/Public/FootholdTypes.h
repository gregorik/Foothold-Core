// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "MoverTypes.h"
#include "NativeGameplayTags.h"
#include "FootholdTypes.generated.h"

class UFootholdLadderComponent;
class UPrimitiveComponent;

/** Mover mode names registered by Foothold. Use these as keys in a Mover component's MovementModes map. */
namespace FootholdModeNames
{
	extern FOOTHOLD_API const FName Climbing;
	extern FOOTHOLD_API const FName LedgeHang;
	extern FOOTHOLD_API const FName Ladder;
	extern FOOTHOLD_API const FName Mantle;
	extern FOOTHOLD_API const FName Gliding;
	extern FOOTHOLD_API const FName WallRun;
	/** "Slide", not "Sliding": GASP's pawn has a Sliding mode of its own. */
	extern FOOTHOLD_API const FName Slide;
}

/** Added by every Foothold mode. Systems that should stand down during traversal (foot placement, GASP's foot locking) check this. */
FOOTHOLD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Foothold_Active);
FOOTHOLD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Foothold_Climbing);
FOOTHOLD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Foothold_LedgeHang);
FOOTHOLD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Foothold_Ladder);
FOOTHOLD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Foothold_Mantle);
FOOTHOLD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Foothold_Gliding);
FOOTHOLD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Foothold_WallRun);
FOOTHOLD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Foothold_Sliding);

/** Which Foothold behaviour is running. None when the Mover is in any other mode. */
UENUM(BlueprintType)
enum class EFootholdKind : uint8
{
	None,
	Climb,
	LedgeHang,
	Ladder,
	Mantle,
	Glide,
	WallRun,
	Slide,
};

/**
 * Per-mode simulation state, carried in the Mover sync state while a Foothold mode is active.
 * It is not a persistent sync type: each Foothold mode copies it forward every tick, so it disappears
 * on its own when the Mover moves to a mode that isn't Foothold's. Only the fields a Kind uses are
 * sent over the network.
 */
USTRUCT(BlueprintType)
struct FOOTHOLD_API FFootholdSyncState : public FMoverDataStructBase
{
	GENERATED_BODY()

	/** Which mode wrote this state. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	EFootholdKind Kind = EFootholdKind::None;

	/** Mode-specific sub-state (for example the mantle's rise and slide phases). */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	uint8 Phase = 0;

	/** Seconds since the current mode started. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	float TimeInMode = 0.f;

	/** World-space point the mode is attached to: the ledge edge, the wall contact, the ladder axis point. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	FVector AnchorLocation = FVector::ZeroVector;

	/** Normal of the surface the mode is attached to, pointing away from it. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	FVector SurfaceNormal = FVector::ZeroVector;

	/** Glide and wall-run: travel direction. Hang: the edge direction. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	FVector TravelDirection = FVector::ZeroVector;

	/** Mantle: capsule centre at the start of the mantle. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	FVector MantleStart = FVector::ZeroVector;

	/** Mantle: capsule centre standing on top. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	FVector MantleEnd = FVector::ZeroVector;

	/** Mantle: total seconds. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	float MantleDuration = 0.f;

	/** Ladder: centimetres from the bottom along the ladder axis. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	float LadderPosition = 0.f;

	/** General-purpose timer (hang: how long forward has been held; wall-run: time without a wall). */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	float Timer = 0.f;

	/** Component the anchor sits on. A move of more than 2 cm per tick releases the hold. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	TWeakObjectPtr<UPrimitiveComponent> AnchorComponent;

	/** Where AnchorComponent was when the anchor was last taken. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	FVector AnchorComponentLocation = FVector::ZeroVector;

	/** Ladder: the ladder being climbed. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Foothold)
	TWeakObjectPtr<UFootholdLadderComponent> Ladder;

	/** True for the kinds that hold an anchor point (and so reconcile on anchor error). */
	bool UsesAnchor() const;

	virtual FMoverDataStructBase* Clone() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	virtual void ToString(FAnsiStringBuilderBase& Out) const override;
	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthorityState) const override;
	virtual void Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct) override;
};

template<>
struct TStructOpsTypeTraits<FFootholdSyncState> : public TStructOpsTypeTraitsBase2<FFootholdSyncState>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};

/**
 * Foothold's input block. UFootholdComponent adds it to each input command. The presses are one-shot:
 * they're true for one produced command and cleared afterwards. Merge ORs them so a press survives
 * when frames are combined.
 */
USTRUCT(BlueprintType)
struct FOOTHOLD_API FFootholdInputs : public FMoverDataStructBase
{
	GENERATED_BODY()

	/** Let go of a ledge, wall or ladder. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Foothold)
	bool bDropPressed = false;

	/** Start or stop gliding. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Foothold)
	bool bGlideTogglePressed = false;

	/** Start climbing (only needed when a climb-start transition requires it). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Foothold)
	bool bClimbTogglePressed = false;

	/** Crouch is held. Lets go of holds, like a drop. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Foothold)
	bool bCrouchHeld = false;

	bool HasAnyPress() const { return bDropPressed || bGlideTogglePressed || bClimbTogglePressed; }

	virtual FMoverDataStructBase* Clone() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	virtual void ToString(FAnsiStringBuilderBase& Out) const override;
	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthorityState) const override;
	virtual void Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct) override;
	virtual void Merge(const FMoverDataStructBase& From) override;
};

template<>
struct TStructOpsTypeTraits<FFootholdInputs> : public TStructOpsTypeTraitsBase2<FFootholdInputs>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};

/** Converts between EFootholdKind and Foothold's mode names and tags. */
struct FOOTHOLD_API FFootholdKindUtils
{
	static FName GetModeName(EFootholdKind Kind);
	static EFootholdKind FromModeName(FName ModeName);
	static FGameplayTag GetTag(EFootholdKind Kind);
};
