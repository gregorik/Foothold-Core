// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FootholdAnimData.h"
#include "FootholdTypes.h"
#include "MoverSimulationTypes.h"
#include "FootholdComponent.generated.h"

class UAnimMontage;
class UInputAction;
class UMoverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFootholdModeChangedSignature, EFootholdKind, PreviousKind, EFootholdKind, NewKind);

/**
 * The one component a pawn needs alongside its Mover component:
 * - it adds FFootholdInputs to every Mover input command (drop, glide and climb requests, crouch held);
 * - it turns the Foothold sync state into FFootholdAnimData for animation, every frame;
 * - it fires OnFootholdModeChanged and plays optional montages when a mode starts.
 * Mover finds it automatically as an input producer.
 */
UCLASS(ClassGroup = (Foothold), meta = (BlueprintSpawnableComponent))
class FOOTHOLD_API UFootholdComponent : public UActorComponent, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	UFootholdComponent();

	/** Let go of the current ledge, wall or ladder on the next movement step. */
	UFUNCTION(BlueprintCallable, Category = "Foothold|Input")
	void RequestDrop();

	/** Start or stop gliding on the next movement step (jumping in the air does the same by default). */
	UFUNCTION(BlueprintCallable, Category = "Foothold|Input")
	void RequestGlideToggle();

	/** Ask to start climbing on the next movement step (only needed when climbing requires a request). */
	UFUNCTION(BlueprintCallable, Category = "Foothold|Input")
	void RequestClimbToggle();

	/** Holding crouch lets go of holds. GASP's crouch input is also read automatically. */
	UFUNCTION(BlueprintCallable, Category = "Foothold|Input")
	void SetCrouchHeld(bool bHeld);

	/** The latest animation data. Safe to call from animation worker threads (returns a copy). */
	UFUNCTION(BlueprintPure, Category = "Foothold|Animation", meta = (BlueprintThreadSafe))
	FFootholdAnimData GetAnimData() const;

	/** The Foothold mode the Mover is in, or None. */
	UFUNCTION(BlueprintPure, Category = "Foothold")
	EFootholdKind GetCurrentKind() const { return CurrentKind; }

	/** Fires on the game thread when the Mover enters or leaves a Foothold mode. */
	UPROPERTY(BlueprintAssignable, Category = "Foothold")
	FFootholdModeChangedSignature OnFootholdModeChanged;

	/** Optional: a montage to play on the owner's skeletal mesh when a Foothold mode starts (cosmetic only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation")
	TMap<EFootholdKind, TObjectPtr<UAnimMontage>> EntryMontages;

	/** Seconds for the traversal pose to blend in when a Foothold mode starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation", meta = (ClampMin = "0"))
	float PoseBlendInTime = 0.15f;

	/** Seconds for the traversal pose to blend out when a Foothold mode ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation", meta = (ClampMin = "0"))
	float PoseBlendOutTime = 0.25f;

	/** Glide: degrees of bank per degree/second of turn rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation", meta = (ClampMin = "0"))
	float GlideBankPerTurnRate = 0.35f;

	/** Hands are placed this far either side of the body's centre line (half the shoulder width). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ClampMin = "0", ForceUnits = "cm"))
	float HandSpacing = 22.f;

	/** Feet are placed this far either side of the centre line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ClampMin = "0", ForceUnits = "cm"))
	float FootSpacing = 16.f;

	/** Hanging: how far below the edge the feet rest against the wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ClampMin = "0", ForceUnits = "cm"))
	float HangFootDrop = 150.f;

	/** Climbing: hand height above the capsule centre, and foot height below it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ForceUnits = "cm"))
	float ClimbHandHeight = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ForceUnits = "cm"))
	float ClimbFootDepth = 65.f;

	/**
	 * Climbing, ladders and hanging: hands and feet rest on holds while the body moves, and move to the next hold, one
	 * diagonal pair at a time, once the body is half a step past them. This is the body travel per step.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ClampMin = "5", ForceUnits = "cm"))
	float StepLength = 45.f;

	/** Seconds a hand or foot takes to move to its next hold (shorter when the body moves fast). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ClampMin = "0.05", ForceUnits = "s"))
	float StepTime = 0.22f;

	/** How far a moving hand or foot comes off the surface on its way to the next hold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ClampMin = "0", ForceUnits = "cm"))
	float StepLift = 8.f;

	/** Gap kept between hands or feet and the surface they rest on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ClampMin = "0", ForceUnits = "cm"))
	float ContactGap = 4.f;

	/** Gripping a ledge: how far out from the wall face the wrists sit, so the palms land on the edge and the fingers on the top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ForceUnits = "cm"))
	float HandGripOut = 3.f;

	/** Gripping a ledge: wrist height above the top surface, so curled fingers wrap the edge instead of sinking into it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Targets", meta = (ForceUnits = "cm"))
	float HandGripUp = 5.f;

	/** In the air, lift the arms toward a ledge the pawn can hang from before it grabs, so the grab reads as a catch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Reach")
	bool bReachForLedges = true;

	/** Reach for edges up to this far above the hands... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Reach", meta = (ClampMin = "0", ForceUnits = "cm"))
	float LedgeReachAbove = 100.f;

	/** ...and down to this far below them (the ledge grab's own window starts 35 cm below). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Reach", meta = (ClampMin = "0", ForceUnits = "cm"))
	float LedgeReachBelow = 35.f;

	/** How far ahead of the capsule to look for the wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Reach", meta = (ClampMin = "0", ForceUnits = "cm"))
	float LedgeReachDistance = 60.f;

	/** Seconds for the arms to come up fully (and to come down again when there's nothing to reach for). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Animation|Reach", meta = (ClampMin = "0"))
	float LedgeReachBlendTime = 0.15f;

	/** Optional Enhanced Input actions, bound to the owning pawn's input component when it's local. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input")
	TObjectPtr<UInputAction> DropAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input")
	TObjectPtr<UInputAction> GlideAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input")
	TObjectPtr<UInputAction> ClimbToggleAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input")
	TObjectPtr<UInputAction> CrouchAction;

	// IMoverInputProducerInterface
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

	// UActorComponent
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/** Rebuilds AnimData from the Mover's latest state. */
	virtual void UpdateAnimData(float DeltaTime);

	/** Fills hand and foot targets for the current kind. */
	virtual void UpdateLimbTargets(const struct FFootholdSyncState& State, float DeltaTime);

	/**
	 * Moves hands and feet between holds (world space). Each limb rests on its hold until the body is half a step past it,
	 * then moves to a hold half a step ahead of where it belongs; the diagonal pairs (left hand and right foot, right hand
	 * and left foot) take turns. Ideal: where each limb belongs now (left hand, right hand, left foot, right foot);
	 * bUse: which limbs hold on; Velocity: the body's velocity along the surface; Stride: body travel per step of one
	 * limb; Settle: how far a limb may sit from where it belongs once the body stops; Snap: puts a point on a hold (the
	 * wall, a rung, the edge). bWholeStrides: each move goes exactly one Stride along the travel from the limb's hold
	 * (rung to rung). Out: each limb's position on the surface; OutLift: how far it comes off the surface mid-move.
	 */
	void StepLimbs(float DeltaTime, const FVector (&Ideal)[4], const bool (&bUse)[4], const FVector& Velocity, float Stride, float Settle,
		TFunctionRef<FVector(int32, const FVector&)> Snap, FVector (&Out)[4], float (&OutLift)[4], bool bWholeStrides = false);

	/** Updates the pre-grab reach (AnimData.ReachWeight and targets): rises below a hangable ledge in the air, fades otherwise. */
	virtual void UpdateLedgeReach(float DeltaTime);

	/** Where a wrist goes to grip an edge (EdgePoint on the wall face's top line, WallNormal pointing away from it). */
	FVector GetGripPoint(const FVector& EdgePoint, const FVector& WallNormal, const FVector& Up) const;

	void HandleKindChanged(EFootholdKind PreviousKind, EFootholdKind NewKind);
	void TryBindInput();

	/** Logs every Mover mode change at Verbose: "[NetMode/Role] Pawn Old -> New @frame N". */
	UFUNCTION()
	void HandleMoverModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName);

	/**
	 * Refreshes the animation data as soon as Mover has finalized a step, so the pose node sees a new mode on that
	 * mode's first frame. Anim blueprints that read Mover's mode directly (GASP's does) switch that frame; a frame late,
	 * their own pose for the new state would show first.
	 */
	UFUNCTION()
	void HandleMoverPostFinalize(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState);

	/** The frame the animation data was last refreshed (it refreshes once a frame: after Mover's step, or on tick). */
	uint64 AnimDataFrame = TNumericLimits<uint64>::Max();

	UPROPERTY(Transient)
	TObjectPtr<UMoverComponent> MoverComponent;

	EFootholdKind CurrentKind = EFootholdKind::None;
	EFootholdKind PosedKind = EFootholdKind::None;

	/** A hand or foot resting on a hold (Alpha 1), or moving From one To the next. */
	struct FLimbStep
	{
		FVector From = FVector::ZeroVector;
		FVector To = FVector::ZeroVector;
		float Alpha = 1.f;
		float Duration = 0.2f;
		bool bPlaced = false;
	};

	/** Left hand, right hand, left foot, right foot. */
	FLimbStep LimbSteps[4];

	/** The diagonal pair that moved last: 0 = left hand and right foot, 1 = right hand and left foot. */
	int32 LastStepPair = 1;

	/** Travel along the surface (AnimData.CycleDistance) at which the next pair moves, and whether a move is due. */
	float NextStepAt = 0.f;
	bool bStepDue = false;

	mutable FCriticalSection AnimDataLock;
	FFootholdAnimData AnimData;

	bool bPendingDrop = false;
	bool bPendingGlideToggle = false;
	bool bPendingClimbToggle = false;
	bool bCrouchHeld = false;
	bool bInputBound = false;
};
