// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdTestTypes.h"
#include "Backends/MoverStandaloneLiaison.h"
#include "Components/CapsuleComponent.h"
#include "MoverDataModelTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdTestTypes)

int32 UFootholdSmokeMode::TickCount = 0;

void UFootholdSmokeMode::GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	OutProposedMove = FProposedMove();
}

void UFootholdSmokeMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	++TickCount;

	const FMoverDefaultSyncState* Start = Params.StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	FMoverDefaultSyncState& Out = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FMoverDefaultSyncState>();
	if (Start)
	{
		Out = *Start;
	}
}

UFootholdSmokeMoverComponent::UFootholdSmokeMoverComponent()
{
	MovementModes.Add(TEXT("FootholdSmoke"), CreateDefaultSubobject<UFootholdSmokeMode>(TEXT("FootholdSmokeMode")));
	BackendClass = UMoverStandaloneLiaisonComponent::StaticClass();
}

AFootholdTestPawn::AFootholdTestPawn()
{
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(34.f, 88.f);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	SetRootComponent(Capsule);

	Mover = CreateDefaultSubobject<UFootholdSmokeMoverComponent>(TEXT("Mover"));
}

void AFootholdTestPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	FCharacterDefaultInputs& Inputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	Inputs.SetMoveInput(EMoveInputType::DirectionalIntent, ScriptedMove);
	Inputs.OrientationIntent = ScriptedMove.GetSafeNormal();
	Inputs.ControlRotation = FRotator::ZeroRotator;
	Inputs.bIsJumpJustPressed = bJumpThisFrame;
	Inputs.bIsJumpPressed = bJumpThisFrame;
	bJumpThisFrame = false;
}
