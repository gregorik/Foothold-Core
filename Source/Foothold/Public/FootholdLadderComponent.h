// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FootholdLadderComponent.generated.h"

/**
 * Marks a ladder for Foothold's Ladder mode. Place it at the bottom centre of the ladder, with +X
 * pointing out of the ladder (toward the climber) and +Z up the ladder. Add it to your own ladder
 * meshes, or use AFootholdLadder, which builds a simple ladder around it.
 */
UCLASS(ClassGroup = (Foothold), meta = (BlueprintSpawnableComponent), HideCategories = (Mobility, LOD, Collision, Physics, Rendering))
class FOOTHOLD_API UFootholdLadderComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFootholdLadderComponent();

	/** Climbable length along +Z, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Shape, Category = "Foothold|Ladder", meta = (ClampMin = "50", ForceUnits = "cm"))
	float Height = 400.f;

	/** Distance between the rails, in cm. Hand targets are placed on the rails. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Shape, Category = "Foothold|Ladder", meta = (ClampMin = "20", ForceUnits = "cm"))
	float Width = 60.f;

	/** Distance between rungs, in cm. A climber who stops eases to the nearest rung. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Shape, Category = "Foothold|Ladder", meta = (ClampMin = "10", ForceUnits = "cm"))
	float RungSpacing = 30.f;

	/** How far in front of the ladder (along +X) the climber's capsule centre is held, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Shape, Category = "Foothold|Ladder", meta = (ClampMin = "0", ForceUnits = "cm"))
	float StandOffDistance = 45.f;

	/** If true, climbing past the top mantles onto whatever is up there. If false, the climber stops at the top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Shape, Category = "Foothold|Ladder")
	bool bMantleOffTop = true;

	/** World-space bottom of the climbable axis (the component location). */
	UFUNCTION(BlueprintPure, Category = "Foothold|Ladder")
	FVector GetBottom() const;

	/** World-space top of the climbable axis. */
	UFUNCTION(BlueprintPure, Category = "Foothold|Ladder")
	FVector GetTop() const;

	/** Unit vector up the ladder (component +Z). */
	UFUNCTION(BlueprintPure, Category = "Foothold|Ladder")
	FVector GetAxis() const;

	/** Unit vector out of the ladder toward the climber (component +X). The climber faces the opposite way. */
	UFUNCTION(BlueprintPure, Category = "Foothold|Ladder")
	FVector GetFacing() const;

	/** Unit vector along the rungs (component +Y). */
	UFUNCTION(BlueprintPure, Category = "Foothold|Ladder")
	FVector GetRight() const;

	/** Distance along the axis (0 at the bottom, clamped to [0, Height]) of the point nearest WorldLocation. */
	UFUNCTION(BlueprintPure, Category = "Foothold|Ladder")
	float ProjectToLadder(const FVector& WorldLocation) const;

	/** Where a climber's capsule centre is held at a given position along the axis. */
	UFUNCTION(BlueprintPure, Category = "Foothold|Ladder")
	FVector GetClimbLocation(float LadderPosition) const;

	/** The rung position nearest to LadderPosition, clamped to [0, Height]. */
	UFUNCTION(BlueprintPure, Category = "Foothold|Ladder")
	float GetNearestRungPosition(float LadderPosition) const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	/** A ladder spawned at runtime gets its shape from the server: rebuild an AFootholdLadder's mesh to match. */
	UFUNCTION()
	void OnRep_Shape();
};
