// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootholdDemoCourse.generated.h"

class APawn;
class UChildActorComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UTextRenderComponent;

/**
 * A procedural test course for Foothold Core's moves, built from engine cubes in OnConstruction: mantle blocks, a
 * ledge wall, a ladder to a platform, and a tower of 13 mantle steps. Drop it in a level, or type Foothold.SpawnDemo in
 * the console. The floor's top is at the actor's origin; the course runs along +X.
 */
UCLASS(Blueprintable, ClassGroup = (Foothold), meta = (DisplayName = "Foothold Demo Course"))
class FOOTHOLD_API AFootholdDemoCourse : public AActor
{
	GENERATED_BODY()

public:
	static const FName MantleSection;
	static const FName LedgeSection;
	static const FName LadderSection;
	static const FName TowerSection;
	/** The top of the tower, facing back down the course. */
	static const FName TowerTop;

	AFootholdDemoCourse();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Clears and rebuilds the course. */
	UFUNCTION(BlueprintCallable, Category = "Foothold|Demo")
	void Rebuild();

	/**
	 * Where a standing character's capsule centre goes to try a section, facing the obstacle (world space).
	 * Identity if Section isn't one of the section names.
	 */
	UFUNCTION(BlueprintPure, Category = "Foothold|Demo")
	FTransform GetSectionStart(FName Section) const;

	/** Where the player starts, facing down the course (world space, capsule centre). */
	UFUNCTION(BlueprintPure, Category = "Foothold|Demo")
	FTransform GetPlayerStart() const;

	/** The sections in course order, then TowerTop. */
	UFUNCTION(BlueprintPure, Category = "Foothold|Demo")
	static TArray<FName> GetSectionNames();

	/**
	 * Spawns a course at Location (default: 500 cm in front of PlayerPawn, on the ground below it) and
	 * moves PlayerPawn to its start. Backs Foothold.SpawnDemo.
	 */
	static AFootholdDemoCourse* SpawnDemo(UWorld* World, const TOptional<FVector>& Location, APawn* PlayerPawn);

	/** Moves a pawn, through its Mover component when it has one. */
	static void MovePawnTo(APawn* Pawn, const FTransform& Where);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Demo")
	bool bShowLabels = true;

	/** Height of the label text, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Demo", meta = (ClampMin = "10"))
	float LabelSize = 45.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Foothold|Demo")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Foothold|Demo")
	TObjectPtr<UInstancedStaticMeshComponent> FloorMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Foothold|Demo")
	TObjectPtr<UInstancedStaticMeshComponent> MantleMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Foothold|Demo")
	TObjectPtr<UInstancedStaticMeshComponent> LedgeMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Foothold|Demo")
	TObjectPtr<UInstancedStaticMeshComponent> TowerMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Foothold|Demo")
	TObjectPtr<UChildActorComponent> LadderActor;

	UPROPERTY(VisibleAnywhere, Category = "Foothold|Demo")
	TArray<TObjectPtr<UTextRenderComponent>> Labels;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	/**
	 * Section starts in actor space. Saved with the actor: a cooked game loads a placed course without running
	 * OnConstruction, so starts that only lived in memory would be gone.
	 */
	UPROPERTY()
	TMap<FName, FTransform> LocalStarts;

	void AddBox(UInstancedStaticMeshComponent* Mesh, const FVector& Center, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator);
	/** A box standing on the floor (Z = 0) with its front face at FrontX. */
	void AddBlock(UInstancedStaticMeshComponent* Mesh, float FrontX, float Y, const FVector& Size);
	void SetColor(UInstancedStaticMeshComponent* Mesh, const FLinearColor& Color);
	void PlaceLabel(int32 Index, const FVector& Location, const FString& Text);
};
