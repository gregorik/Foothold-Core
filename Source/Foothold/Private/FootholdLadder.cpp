// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdLadder.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "FootholdLadderComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdLadder)

namespace UE::Foothold::LadderMesh
{
	constexpr float RailThickness = 6.f;
	constexpr float RungThickness = 4.f;
	constexpr float CubeSize = 100.f;	// the engine cube is 100 cm
}

AFootholdLadder::AFootholdLadder()
{
	PrimaryActorTick.bCanEverTick = false;

	// Replicated, so that a ladder spawned at runtime (the demo course's, or one from game code) is net-addressable:
	// the Ladder mode's sync state points at its component. Static, so it rarely needs an update.
	bReplicates = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(1.f);

	Ladder = CreateDefaultSubobject<UFootholdLadderComponent>(TEXT("Ladder"));
	SetRootComponent(Ladder);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Rails = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rails"));
	Rails->SetupAttachment(Ladder);
	Rungs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rungs"));
	Rungs->SetupAttachment(Ladder);

	for (UInstancedStaticMeshComponent* Part : { Rails.Get(), Rungs.Get() })
	{
		Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		if (Cube.Succeeded())
		{
			Part->SetStaticMesh(Cube.Object);
		}
	}
}

void AFootholdLadder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildMesh();
}

void AFootholdLadder::RebuildMesh()
{
	using namespace UE::Foothold::LadderMesh;

	Rails->ClearInstances();
	Rungs->ClearInstances();

	const float Height = Ladder->Height;
	const float HalfWidth = Ladder->Width * 0.5f;

	// Rails: thin boxes at +/- half the width, bottom to top, in component space.
	for (const float Side : { -1.f, 1.f })
	{
		const FVector Center(0.f, Side * HalfWidth, Height * 0.5f);
		const FVector Scale(RailThickness / CubeSize, RailThickness / CubeSize, Height / CubeSize);
		Rails->AddInstance(FTransform(FRotator::ZeroRotator, Center, Scale));
	}

	// Rungs: one every RungSpacing, starting one spacing above the bottom.
	const float Spacing = FMath::Max(Ladder->RungSpacing, 10.f);
	for (float Z = Spacing; Z <= Height + KINDA_SMALL_NUMBER; Z += Spacing)
	{
		const FVector Center(0.f, 0.f, Z);
		const FVector Scale(RungThickness / CubeSize, (Ladder->Width + RailThickness) / CubeSize, RungThickness / CubeSize);
		Rungs->AddInstance(FTransform(FRotator::ZeroRotator, Center, Scale));
	}
}

int32 AFootholdLadder::GetRungCount() const
{
	return Rungs->GetInstanceCount();
}
