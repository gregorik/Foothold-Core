// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FootholdTraceLibrary.generated.h"

class UFootholdLadderComponent;
class UFootholdSharedSettings;
class UPrimitiveComponent;
class UWorld;
struct FMovingComponentSet;

/**
 * Everything a Foothold probe needs to know about the capsule asking. Every direction and height is
 * measured relative to Up, so the probes work with Mover gravity and up-direction overrides.
 */
struct FOOTHOLD_API FFootholdQueryContext
{
	UWorld* World = nullptr;
	const AActor* IgnoreActor = nullptr;
	const UFootholdSharedSettings* Settings = nullptr;

	/** Capsule centre. */
	FVector Location = FVector::ZeroVector;
	FVector Up = FVector::UpVector;
	/** Planar facing direction (perpendicular to Up). */
	FVector Forward = FVector::ForwardVector;

	float CapsuleRadius = 34.f;
	float CapsuleHalfHeight = 88.f;

	/** Object type and responses used for capsule clearance tests (the moving primitive's own, when known). */
	ECollisionChannel ClearanceChannel = ECC_Pawn;
	FCollisionResponseParams ClearanceResponses;

	FVector GetFeet() const { return Location - Up * CapsuleHalfHeight; }
	FQuat GetCapsuleRotation() const;
	ECollisionChannel GetTraceChannel() const;
	FCollisionQueryParams MakeQueryParams(FName TraceTag) const;

	/** Builds a context from a capsule description (used by tests and by code without a Mover). */
	static FFootholdQueryContext Make(UWorld* World, const FVector& Location, const FVector& Up, const FVector& Forward,
		float CapsuleRadius, float CapsuleHalfHeight, const UFootholdSharedSettings* Settings, const AActor* IgnoreActor = nullptr);

	/** Builds a context from a Mover's moving components. Up is usually MoverComponent->GetUpDirection(). */
	static FFootholdQueryContext Make(const FMovingComponentSet& MovingComps, const UFootholdSharedSettings* Settings, const FVector& Up);
};

/** A wall found by FindWall. */
USTRUCT(BlueprintType)
struct FOOTHOLD_API FFootholdWallHit
{
	GENERATED_BODY()

	/** A steep enough surface was hit within reach. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bHit = false;

	/** The climbing policy accepts this wall. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bClimbable = false;

	/** The wall is near-vertical and not tagged NoWallRun. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bWallRunnable = false;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector Location = FVector::ZeroVector;

	/** Surface normal, pointing away from the wall. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector Normal = FVector::ZeroVector;

	/** Normal flattened onto the plane perpendicular to Up. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector FlatNormal = FVector::ZeroVector;

	/** Distance from the capsule surface to the wall, along the probe. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	float Distance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	TWeakObjectPtr<UPrimitiveComponent> Component;
};

/** A ledge found by FindLedge: a walkable top surface above a wall, within the asked height range. */
USTRUCT(BlueprintType)
struct FOOTHOLD_API FFootholdLedge
{
	GENERATED_BODY()

	/** A top surface was found in range, deep enough, not tagged NoClimb, and walkable (or allowed by the settings). */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bValid = false;

	/** A capsule hanging from the edge fits. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bHangClear = false;

	/** A standing capsule fits on top, on walkable ground. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	bool bCanStand = false;

	/** The point on the edge, where the wall face meets the top. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector EdgePoint = FVector::ZeroVector;

	/** Wall normal at the edge, flattened (perpendicular to Up), pointing away from the wall. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector WallNormal = FVector::ZeroVector;

	/** Normal of the top surface. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector TopNormal = FVector::ZeroVector;

	/** Direction along the edge (Up x WallNormal). */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector EdgeDirection = FVector::ZeroVector;

	/** Capsule centre for hanging with the hands on the edge. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector HangLocation = FVector::ZeroVector;

	/** Capsule centre for standing on top, just behind the edge. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	FVector StandLocation = FVector::ZeroVector;

	/** Height of the edge above the asking capsule's feet. */
	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	float HeightAboveFeet = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = Foothold)
	TWeakObjectPtr<UPrimitiveComponent> Component;
};

/**
 * Foothold's probes. They're deterministic, use only scene queries, and are safe to call from Mover
 * simulation code (game thread).
 */
UCLASS()
class FOOTHOLD_API UFootholdTraceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Looks for a steep surface along Direction (flattened against Up), within Distance of the capsule
	 * surface, at HeightOffset above the capsule centre.
	 */
	static FFootholdWallHit FindWall(const FFootholdQueryContext& Ctx, const FVector& Direction, float Distance, float HeightOffset = 0.f);

	/**
	 * Looks for a ledge along Direction whose edge is between MinHeightAboveFeet and MaxHeightAboveFeet.
	 * The wall can be anywhere from the feet up to just above the hands.
	 */
	static FFootholdLedge FindLedge(const FFootholdQueryContext& Ctx, const FVector& Direction, float MinHeightAboveFeet, float MaxHeightAboveFeet, float Reach = 60.f);

	/** Finds the nearest registered ladder whose climb line is within MaxDistance, in front of the ladder. */
	static UFootholdLadderComponent* FindLadder(const FFootholdQueryContext& Ctx, float MaxDistance, float& OutLadderPosition);

	/** Steep enough, not tagged NoClimb, and accepted by the climbing policy. */
	static bool IsClimbable(const FHitResult& Hit, const UFootholdSharedSettings& Settings, const FVector& Up);

	/** True when the hit's component or actor carries Tag. */
	static bool HasTag(const FHitResult& Hit, FName Tag);

	/** True when the asking capsule, inflated by Inflate cm (negative shrinks), fits at Center without blocking overlaps. */
	static bool IsCapsuleClear(const FFootholdQueryContext& Ctx, const FVector& Center, float Inflate = -2.f);

	/** Walkable under the settings' slope limit, measured against Up. */
	static bool IsWalkable(const FVector& Normal, const FVector& Up, float MaxWalkSlopeCosine);

	/** Line trace along -Up from Start for Distance. Returns false on no hit or a start inside geometry. */
	static bool TraceDown(const FFootholdQueryContext& Ctx, const FVector& Start, float Distance, FHitResult& OutHit);
};
