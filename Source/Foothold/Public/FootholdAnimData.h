// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "FootholdTypes.h"
#include "FootholdAnimData.generated.h"

/**
 * What an animation graph needs to pose a Foothold mode. UFootholdComponent fills it on the game
 * thread after Mover moves, and anim graphs read it from any thread (it's a copy).
 */
USTRUCT(BlueprintType)
struct FOOTHOLD_API FFootholdAnimData
{
	GENERATED_BODY()

	/** The Foothold mode being posed. While blending out it keeps the last mode. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	EFootholdKind Kind = EFootholdKind::None;

	/** Blend weight of the Foothold pose: 0 = locomotion only, 1 = full traversal pose. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	float Weight = 0.f;

	/** Velocity in the pose frame, in cm/s: X right, Y up, Z forward (toward the surface for wall modes). */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector LocalVelocity = FVector::ZeroVector;

	/** Normal of the surface being held, pointing away from it. Zero for glide. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector SurfaceNormal = FVector::ZeroVector;

	/** World-space point on the surface being held (with SurfaceNormal, its plane): the wall contact, the ledge edge, the ladder axis point. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector SurfacePoint = FVector::ZeroVector;

	/** World-space forward of the pose (into the wall for wall modes, along travel for glide and wall-run). */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector Facing = FVector::ForwardVector;

	/** World-space up used by the movement. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector Up = FVector::UpVector;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector LeftHandTarget = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector RightHandTarget = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector LeftFootTarget = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector RightFootTarget = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bLeftHandTargetValid = false;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bRightHandTargetValid = false;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bLeftFootTargetValid = false;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bRightFootTargetValid = false;

	/**
	 * How far each limb comes off the surface on its way to its next hold, in cm: left hand, right hand, left foot,
	 * right foot. The targets above stay on the surface; the procedural pose lifts the limb along SurfaceNormal.
	 */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector4 LimbLift = FVector4(0.0, 0.0, 0.0, 0.0);

	/** Distance travelled along the surface while in this mode, in cm. Drives procedural reach cycles. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	float CycleDistance = 0.f;

	/** Mantle: 0 at the start, 1 standing on top. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	float MantleAlpha = 0.f;

	/** Mantle: the share of it spent rising (the rest moves over the top). */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	float MantleRiseShare = 0.5f;

	/** Mantle: the capsule's whole move, from where it started to where it will stand (world, cm). */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector MantleTravel = FVector::ZeroVector;

	/** Mantle: how far along that move the capsule is now (world, cm, from the start). */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector MantleProgress = FVector::ZeroVector;

	/** Glide and wall-run: roll of the body in degrees (positive = right side down). */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	float BankDegrees = 0.f;

	/**
	 * Reaching for a ledge before grabbing it, 0..1. While in the air below a ledge the pawn can hang from, the arms
	 * lift toward the reach targets; it fades out once a Foothold mode takes over.
	 */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	float ReachWeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector LeftReachTarget = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector RightReachTarget = FVector::ZeroVector;

	/** Normal of the wall under the ledge being reached for, pointing away from it. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector ReachNormal = FVector::ZeroVector;

	/** Seconds in the current Foothold mode. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	float TimeInMode = 0.f;
};
