// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "Engine/EngineTypes.h"
#include "FootholdSharedSettings.generated.h"

/** Which steep surfaces the Climbing mode accepts. */
UENUM(BlueprintType)
enum class EFootholdClimbablePolicy : uint8
{
	/** Only surfaces whose component or actor carries ClimbableTag. */
	TaggedOnly,
	/** Any surface steep enough to be a wall, unless it carries NoClimbTag. */
	AnySteepSurface,
};

/**
 * Detection and geometry settings shared by every Foothold mode and transition on a Mover component.
 * Mover creates one instance per component (it's listed in each Foothold mode's SharedSettingsClasses),
 * and you edit it in the Mover component's Shared Settings list.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class FOOTHOLD_API UFootholdSharedSettings : public UObject, public IMovementSettingsInterface
{
	GENERATED_BODY()

public:
	virtual FString GetDisplayName() const override { return GetName(); }

	/** Trace channel for every Foothold probe (walls, ledges, clearance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Which surfaces the Climbing mode accepts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	EFootholdClimbablePolicy ClimbPolicy = EFootholdClimbablePolicy::TaggedOnly;

	/** Component or actor tag that makes a surface climbable (TaggedOnly policy). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	FName ClimbableTag = TEXT("Climbable");

	/** Component or actor tag that blocks climbing, ledge grabs and mantles on that surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	FName NoClimbTag = TEXT("NoClimb");

	/** Component or actor tag that blocks wall-running on that surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	FName NoWallRunTag = TEXT("NoWallRun");

	/** A surface counts as a wall when its normal is at least this many degrees from up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "30", ClampMax = "120", ForceUnits = "deg"))
	float MinWallAngle = 70.f;

	/** While hanging, how far above the capsule centre the hands are (the ledge edge sits here), in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hang Geometry", meta = (ClampMin = "0", ForceUnits = "cm"))
	float HandReachAboveCenter = 90.f;

	/** Gap kept between the capsule and the wall while hanging or climbing, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hang Geometry", meta = (ClampMin = "0", ForceUnits = "cm"))
	float WallGap = 4.f;

	/** A ledge top must extend at least this far behind the edge, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hang Geometry", meta = (ClampMin = "1", ForceUnits = "cm"))
	float MinLedgeDepth = 20.f;

	/** Ledge tops steeper than walkable are refused unless this is set (you can hang there, but not climb up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hang Geometry")
	bool bAllowHangOnUnstandableLedges = false;

	/** Walkable slope for ledge tops and landings, as cos(angle). Mover's own walking modes use 0.71 (45 degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hang Geometry", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxWalkSlopeCosine = 0.71f;

	/** Draws probes and anchors while true (also: console variable foothold.Debug). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebug = false;

	/** cos(MinWallAngle) measured against up: normals with Up dot below this are walls. */
	float GetWallUpDotMax() const { return FMath::Cos(FMath::DegreesToRadians(MinWallAngle)); }
};
