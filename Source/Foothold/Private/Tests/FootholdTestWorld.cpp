// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "Tests/FootholdTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/WorldSettings.h"
#include "MoverComponent.h"
#include "EngineUtils.h"
#include "UObject/Package.h"

FFootholdTestWorld::FFootholdTestWorld(bool bWithFloor)
{
	World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("FootholdTestWorld")));
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);

	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	// Without a game mode (this world has no game instance to make one) nothing marks the world as
	// "begun play": actors never get BeginPlay and Mover ignores QueueNextMode (it checks
	// HasBegunPlay()). The world settings actor does what the game state would have done.
	World->GetWorldSettings()->NotifyBeginPlay();
	ensureMsgf(World->HasBegunPlay(), TEXT("Foothold test world failed to begin play"));

	if (bWithFloor)
	{
		SpawnBox(FVector(0, 0, -50), FVector(2000, 2000, 50));
	}
}

FFootholdTestWorld::~FFootholdTestWorld()
{
	if (World)
	{
		World->EndPlay(EEndPlayReason::Quit);
		// Destroy the world while its context still exists: child actors are destroyed with it.
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World = nullptr;
	}
}

AStaticMeshActor* FFootholdTestWorld::SpawnBox(const FVector& Center, const FVector& HalfExtent, const FRotator& Rotation, const TArray<FName>& Tags)
{
	static UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	check(Cube);

	AStaticMeshActor* Box = Spawn<AStaticMeshActor>(Center, Rotation);
	UStaticMeshComponent* Mesh = Box->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(Cube);
	Mesh->SetWorldScale3D(HalfExtent / 50.f);	// the engine cube is 100 cm
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	for (const FName& Tag : Tags)
	{
		Box->Tags.AddUnique(Tag);
		Mesh->ComponentTags.AddUnique(Tag);
	}
	return Box;
}

void FFootholdTestWorld::Tick(float Seconds, float Dt)
{
	const int32 Steps = FMath::Max(1, FMath::RoundToInt(Seconds / Dt));
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		StepOnce(Dt);
	}
}

void FFootholdTestWorld::StepOnce(float Dt)
{
	// Tick functions are queued at most once per engine frame (GFrameCounter). An automation test runs
	// inside a single engine frame, so advance the counter or every step after the first is skipped.
	++GFrameCounter;
	World->Tick(LEVELTICK_All, Dt);
	ElapsedSeconds += Dt;
}

bool FFootholdTestWorld::TickUntil(TFunctionRef<bool()> Predicate, float MaxSeconds, float Dt)
{
	const int32 Steps = FMath::Max(1, FMath::RoundToInt(MaxSeconds / Dt));
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		if (Predicate())
		{
			return true;
		}
		StepOnce(Dt);
	}
	return Predicate();
}

TArray<FName> FFootholdTestWorld::RecordModes(const UMoverComponent* Mover, float Seconds, TFunction<bool()> StopWhen, float Dt)
{
	TArray<FName> Modes;
	Modes.Add(Mover->GetMovementModeName());
	const int32 Steps = FMath::Max(1, FMath::RoundToInt(Seconds / Dt));
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		StepOnce(Dt);
		const FName Mode = Mover->GetMovementModeName();
		if (Modes.Last() != Mode)
		{
			Modes.Add(Mode);
		}
		if (StopWhen && StopWhen())
		{
			break;
		}
	}
	return Modes;
}

#endif
