// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MovementMode.h"
#include "MoverSimulationTypes.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "FootholdTestTypes.generated.h"

class UCapsuleComponent;

/** Test-only mode: counts simulation ticks and holds still. */
UCLASS(NotBlueprintable, HideDropdown)
class UFootholdSmokeMode : public UBaseMovementMode
{
	GENERATED_BODY()

public:
	static int32 TickCount;

	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;
};

/** Test-only character mover: the default set plus the smoke mode, on the standalone backend. */
UCLASS(NotBlueprintable, HideDropdown)
class UFootholdSmokeMoverComponent : public UCharacterMoverComponent
{
	GENERATED_BODY()

public:
	UFootholdSmokeMoverComponent();
};

/** Test-only pawn: a capsule and a smoke mover, with scripted input. */
UCLASS(NotBlueprintable, HideDropdown)
class AFootholdTestPawn : public APawn, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	AFootholdTestPawn();

	UPROPERTY()
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY()
	TObjectPtr<UFootholdSmokeMoverComponent> Mover;

	FVector ScriptedMove = FVector::ZeroVector;
	bool bJumpThisFrame = false;

	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;
};
