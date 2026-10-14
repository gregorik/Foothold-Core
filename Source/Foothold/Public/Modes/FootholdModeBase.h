// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "FootholdTraceLibrary.h"
#include "FootholdTypes.h"
#include "FootholdModeBase.generated.h"

class UCommonLegacyMovementSettings;
class UFootholdSharedSettings;
struct FMovementRecord;

/**
 * Base for Foothold's Mover modes. It registers the shared settings every Foothold mode uses and
 * gives the helpers they share: the Foothold sync state, probe contexts, a swept move that writes
 * Mover's default sync state, and exits to other modes.
 *
 * Foothold modes keep their state in FFootholdSyncState and copy it forward every tick, so it goes
 * away when the Mover leaves Foothold's modes. They run on the game thread (bSupportsAsync = false),
 * like Mover's own Walking, Falling and Flying modes.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class FOOTHOLD_API UFootholdModeBase : public UBaseMovementMode
{
	GENERATED_BODY()

public:
	UFootholdModeBase();

	/** Which Foothold kind this mode writes into FFootholdSyncState. */
	virtual EFootholdKind GetFootholdKind() const { return EFootholdKind::None; }

	/** Name of the Blueprint user-defined input field read as "crouch held" (GASP: WantsToCrouch). None to disable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input")
	FName CrouchInputFieldPrefix = TEXT("WantsToCrouch");

	/** After this mode hands over to the air mode, a glide can't start for this long (so the jump that let go doesn't also glide). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input", meta = (ClampMin = "0", ForceUnits = "s"))
	float GlideCooldownAfterExit = 0.2f;

protected:
	virtual void OnRegistered(const FName ModeName, const FMoverSimContext& SimContext) override;
	virtual void OnUnregistered(const FMoverSimContext& SimContext) override;

	/** The Foothold state at the start of the tick, if any. */
	static const FFootholdSyncState* GetStartFoothold(const FMoverTickStartData& StartState);

	/** True when the start state was written by this mode (the mode is continuing rather than starting). */
	bool IsContinuing(const FFootholdSyncState* StartFoothold) const;

	/** The Foothold state to write this tick: a copy of the start state when continuing, otherwise a fresh one. */
	FFootholdSyncState& WriteFoothold(FMoverTickEndData& OutputState, const FFootholdSyncState* StartFoothold) const;

	/** A probe context for this tick's moving components and up direction. */
	FFootholdQueryContext MakeContext(const FSimulationTickParams& Params) const;

	FVector GetUp() const;
	static float GetDeltaSeconds(const FSimulationTickParams& Params) { return Params.TimeStep.StepMs * 0.001f; }

	/** A rotation facing Forward, upright along Up. */
	FQuat MakeFacing(const FVector& Forward) const;

	/**
	 * Moves the updated component by Delta (swept, sliding along what it hits when bSlide), turns it to
	 * Rotation, and writes Mover's default sync state with the resulting velocity. Returns the blocking
	 * hit of the first sweep, if any.
	 */
	FHitResult CommitMove(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Delta, const FQuat& Rotation, bool bSlide = true) const;

	/** Writes Mover's default sync state for the current transform with an explicit velocity (no movement). */
	void CommitHold(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Velocity, const FQuat& Rotation) const;

	/** Ends the mode: Mover switches to NextMode, handing it RemainingMs of this tick. */
	static void ExitTo(FMoverTickEndData& OutputState, FName NextMode, float RemainingMs);

	/** Exits to the air mode at once, giving the whole tick to it, with an outgoing velocity. */
	void ExitToAir(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Velocity) const;

	FName GetAirModeName() const;
	FName GetGroundModeName() const;

	/** Input helpers bound to this mode's settings. */
	bool WantsDrop(const FMoverTickStartData& StartState, const FVector& FlatSurfaceNormal) const;
	bool IsCrouchHeld(const FMoverTickStartData& StartState) const;

	/** Shared settings, cached when the mode registers. */
	UPROPERTY(Transient)
	TObjectPtr<const UFootholdSharedSettings> FootholdSettings;

	UPROPERTY(Transient)
	TObjectPtr<const UCommonLegacyMovementSettings> LegacySettings;
};
