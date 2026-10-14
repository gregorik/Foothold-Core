// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdDemoCourse.h"
#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "FootholdLadder.h"
#include "FootholdLadderComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MoverComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdDemoCourse)

const FName AFootholdDemoCourse::MantleSection(TEXT("Mantle"));
const FName AFootholdDemoCourse::LedgeSection(TEXT("Ledge"));
const FName AFootholdDemoCourse::LadderSection(TEXT("Ladder"));
const FName AFootholdDemoCourse::TowerSection(TEXT("Tower"));
const FName AFootholdDemoCourse::TowerTop(TEXT("TowerTop"));

namespace UE::Foothold::Demo
{
	/** Capsule centre of a standing GASP/Manny character (half height 88) on the course floor. */
	static constexpr float StandZ = 90.f;
	static constexpr float ReferenceHalfHeight = 88.f;
	static constexpr int32 LabelCount = 5;
}

AFootholdDemoCourse::AFootholdDemoCourse()
{
	PrimaryActorTick.bCanEverTick = false;
	// Spawned on a server, clients build the same course from the replicated actor.
	bReplicates = true;
	bAlwaysRelevant = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	auto MakeMeshes = [this](const TCHAR* Name)
	{
		UInstancedStaticMeshComponent* Mesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Root);
		Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		return Mesh;
	};
	FloorMeshes = MakeMeshes(TEXT("Floor"));
	MantleMeshes = MakeMeshes(TEXT("MantleBlocks"));
	LedgeMeshes = MakeMeshes(TEXT("LedgeWall"));
	TowerMeshes = MakeMeshes(TEXT("MantleTower"));

	LadderActor = CreateDefaultSubobject<UChildActorComponent>(TEXT("Ladder"));
	LadderActor->SetupAttachment(Root);
	LadderActor->SetChildActorClass(AFootholdLadder::StaticClass());

	for (int32 Index = 0; Index < UE::Foothold::Demo::LabelCount; ++Index)
	{
		UTextRenderComponent* Label = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Label%d"), Index));
		Label->SetupAttachment(Root);
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetVerticalAlignment(EVRTA_TextBottom);
		Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Labels.Add(Label);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	BaseMaterial = Material.Object;
}

void AFootholdDemoCourse::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void AFootholdDemoCourse::BeginPlay()
{
	Super::BeginPlay();
	// A course saved into a level before its section starts were saved with it comes back without them in a cooked
	// game (no construction script runs there). Rebuild it: the geometry is the same.
	if (LocalStarts.IsEmpty())
	{
		Rebuild();
	}
}

void AFootholdDemoCourse::AddBox(UInstancedStaticMeshComponent* Mesh, const FVector& Center, const FVector& Size, const FRotator& Rotation)
{
	Mesh->AddInstance(FTransform(Rotation, Center, Size / 100.f));	// the engine cube is 100 cm, centred
}

void AFootholdDemoCourse::AddBlock(UInstancedStaticMeshComponent* Mesh, float FrontX, float Y, const FVector& Size)
{
	AddBox(Mesh, FVector(FrontX + Size.X * 0.5f, Y, Size.Z * 0.5f), Size);
}

void AFootholdDemoCourse::SetColor(UInstancedStaticMeshComponent* Mesh, const FLinearColor& Color)
{
	Mesh->SetStaticMesh(CubeMesh);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Mesh->SetMaterial(0, Material);
	}
}

void AFootholdDemoCourse::PlaceLabel(int32 Index, const FVector& Location, const FString& Text)
{
	UTextRenderComponent* Label = Labels.IsValidIndex(Index) ? Labels[Index].Get() : nullptr;
	if (!Label)
	{
		return;
	}
	Label->SetRelativeLocationAndRotation(Location, FRotator(0.f, 180.f, 0.f));	// readable from the course start
	Label->SetText(FText::AsCultureInvariant(Text));	// not localized; FromString text warns when the level is cooked
	Label->SetWorldSize(LabelSize);
	Label->SetTextRenderColor(FColor::White);
	Label->SetVisibility(bShowLabels);
}

void AFootholdDemoCourse::Rebuild()
{
	using namespace UE::Foothold::Demo;

	for (UInstancedStaticMeshComponent* Mesh : { FloorMeshes.Get(), MantleMeshes.Get(), LedgeMeshes.Get(), TowerMeshes.Get() })
	{
		Mesh->ClearInstances();
	}
	SetColor(FloorMeshes, FLinearColor(0.045f, 0.05f, 0.055f));
	SetColor(MantleMeshes, FLinearColor(1.f, 0.48f, 0.1f));
	SetColor(LedgeMeshes, FLinearColor(0.1f, 0.45f, 1.f));
	SetColor(TowerMeshes, FLinearColor(1.f, 0.8f, 0.15f));
	LocalStarts.Reset();

	// Floor: top at Z = 0, from behind the start to past the tower, wide enough to drift off line.
	AddBox(FloorMeshes, FVector(3000.f, 0.f, -50.f), FVector(8400.f, 3000.f, 100.f));
	PlaceLabel(0, FVector(-300.f, 0.f, 220.f), TEXT("FOOTHOLD CORE DEMO<br>walk forward"));

	// 1. Mantle blocks.
	AddBlock(MantleMeshes, 600.f, -600.f, FVector(200.f, 300.f, 60.f));
	AddBlock(MantleMeshes, 600.f, 0.f, FVector(200.f, 300.f, 100.f));
	AddBlock(MantleMeshes, 600.f, 600.f, FVector(200.f, 300.f, 140.f));
	LocalStarts.Add(MantleSection, FTransform(FVector(500.f, 600.f, StandZ)));	// the 140 cm block: lower ones are a hop-on for high jumpers
	PlaceLabel(1, FVector(400.f, 0.f, 320.f), TEXT("1  MANTLE<br>jump at 60 / 100 / 140 cm"));

	// 2. Ledge wall: too tall to jump-mantle (310 cm), 6 m to shimmy along, deep enough to climb onto.
	AddBlock(LedgeMeshes, 1600.f, 0.f, FVector(150.f, 600.f, 310.f));
	LocalStarts.Add(LedgeSection, FTransform(FVector(1560.f, -200.f, StandZ)));
	PlaceLabel(2, FVector(1400.f, 0.f, 360.f), TEXT("2  LEDGE<br>jump to hang, shimmy, climb up"));

	// 3. Ladder up to a 4 m platform.
	AddBlock(FloorMeshes, 2700.f, 0.f, FVector(400.f, 400.f, 400.f));
	LadderActor->SetRelativeLocationAndRotation(FVector(2690.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f));
	if (AFootholdLadder* Ladder = Cast<AFootholdLadder>(LadderActor->GetChildActor()))
	{
		Ladder->GetLadder()->Height = 400.f;
		Ladder->RebuildMesh();
	}
	LocalStarts.Add(LadderSection, FTransform(FVector(2590.f, 0.f, StandZ)));
	PlaceLabel(3, FVector(2400.f, -400.f, 320.f), TEXT("3  LADDER<br>walk into it"));

	// 4. Tower: 13 mantle steps of 140 cm to an 18 m top. (In Foothold Pro you glide off it.)
	constexpr int32 TowerSteps = 13;
	constexpr float StepRise = 140.f;
	constexpr float StepDepth = 150.f;
	constexpr float TowerX = 3900.f;
	for (int32 Step = 1; Step <= TowerSteps; ++Step)
	{
		AddBlock(TowerMeshes, TowerX + StepDepth * (Step - 1), 0.f, FVector(StepDepth, 300.f, StepRise * Step));
	}
	const float TopX = TowerX + StepDepth * TowerSteps;
	const float TopZ = StepRise * TowerSteps;
	AddBlock(TowerMeshes, TopX, 0.f, FVector(400.f, 400.f, TopZ));
	LocalStarts.Add(TowerSection, FTransform(FVector(TowerX - 100.f, 0.f, StandZ)));
	LocalStarts.Add(TowerTop, FTransform(FRotator(0.f, 180.f, 0.f), FVector(TopX + 200.f, 0.f, TopZ + StandZ)));
	PlaceLabel(4, FVector(TowerX - 200.f, -400.f, 320.f), TEXT("4  MANTLE TOWER<br>13 jumps to 18 m"));
}

FTransform AFootholdDemoCourse::GetSectionStart(FName Section) const
{
	const FTransform* Local = LocalStarts.Find(Section);
	return Local ? *Local * GetActorTransform() : FTransform::Identity;
}

FTransform AFootholdDemoCourse::GetPlayerStart() const
{
	return FTransform(FVector(-600.f, 0.f, UE::Foothold::Demo::StandZ)) * GetActorTransform();
}

TArray<FName> AFootholdDemoCourse::GetSectionNames()
{
	return { MantleSection, LedgeSection, LadderSection, TowerSection, TowerTop };
}

void AFootholdDemoCourse::MovePawnTo(APawn* Pawn, const FTransform& Where)
{
	if (!Pawn)
	{
		return;
	}
	// Starts are for a 88 cm half-height capsule; keep other pawns' feet on the same floor.
	const FVector Location = Where.GetLocation() + FVector(0.f, 0.f, Pawn->GetSimpleCollisionHalfHeight() - UE::Foothold::Demo::ReferenceHalfHeight);
	const FRotator Rotation(0.f, Where.Rotator().Yaw, 0.f);
	if (UMoverComponent* Mover = Pawn->FindComponentByClass<UMoverComponent>())
	{
		TSharedPtr<FTeleportEffect> Teleport = MakeShared<FTeleportEffect>();
		Teleport->TargetLocation = Location;
		Teleport->bUseActorRotation = false;
		Teleport->TargetRotation = Rotation;
		Mover->QueueInstantMovementEffect(Teleport);
	}
	else
	{
		Pawn->TeleportTo(Location, Rotation);
	}
	if (AController* Controller = Pawn->GetController())
	{
		Controller->SetControlRotation(Rotation);
	}
}

AFootholdDemoCourse* AFootholdDemoCourse::SpawnDemo(UWorld* World, const TOptional<FVector>& Location, APawn* PlayerPawn)
{
	if (!World)
	{
		return nullptr;
	}
	FVector Where = Location.Get(FVector::ZeroVector);
	FRotator Facing = FRotator::ZeroRotator;
	if (!Location.IsSet() && PlayerPawn)
	{
		Facing = FRotator(0.f, PlayerPawn->GetActorRotation().Yaw, 0.f);
		Where = PlayerPawn->GetActorLocation() + Facing.Vector() * 500.f;
		FHitResult Ground;
		const FCollisionQueryParams Query(TEXT("FootholdSpawnDemo"), false, PlayerPawn);
		Where = World->LineTraceSingleByChannel(Ground, Where + FVector(0.f, 0.f, 200.f), Where - FVector(0.f, 0.f, 5000.f), ECC_Visibility, Query)
			? Ground.ImpactPoint + FVector(0.f, 0.f, 2.f)	// just above the existing ground
			: Where - FVector(0.f, 0.f, PlayerPawn->GetSimpleCollisionHalfHeight());
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFootholdDemoCourse* Course = World->SpawnActor<AFootholdDemoCourse>(Where, Facing, Params);
	if (Course && PlayerPawn)
	{
		MovePawnTo(PlayerPawn, Course->GetPlayerStart());
	}
	return Course;
}
