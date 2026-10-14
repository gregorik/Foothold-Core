// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootholdLadder.generated.h"

class UFootholdLadderComponent;
class UInstancedStaticMeshComponent;

/**
 * A ready-made ladder: two rails and evenly spaced rungs built from the engine cube, around a
 * UFootholdLadderComponent. Set the size on the Ladder component and the mesh rebuilds in the editor.
 */
UCLASS(Blueprintable, ClassGroup = (Foothold))
class FOOTHOLD_API AFootholdLadder : public AActor
{
	GENERATED_BODY()

public:
	AFootholdLadder();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Rebuilds the rails and rungs from the ladder component's Height, Width and RungSpacing. */
	UFUNCTION(BlueprintCallable, Category = "Foothold|Ladder")
	void RebuildMesh();

	UFootholdLadderComponent* GetLadder() const { return Ladder; }
	int32 GetRungCount() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foothold|Ladder")
	TObjectPtr<UFootholdLadderComponent> Ladder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foothold|Ladder")
	TObjectPtr<UInstancedStaticMeshComponent> Rails;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foothold|Ladder")
	TObjectPtr<UInstancedStaticMeshComponent> Rungs;
};
