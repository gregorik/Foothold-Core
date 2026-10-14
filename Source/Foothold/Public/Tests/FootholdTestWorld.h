// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "Templates/Function.h"

class AStaticMeshActor;
class UMoverComponent;

/**
 * Test support (excluded from the Fab package with the rest of Source/.../Tests).
 * A throwaway game world for Foothold automation tests. It has an optional 40 m floor at z=0,
 * spawns test geometry, and ticks the world in fixed steps so Mover simulates deterministically.
 */
struct FOOTHOLD_API FFootholdTestWorld
{
	explicit FFootholdTestWorld(bool bWithFloor = true);
	~FFootholdTestWorld();

	FFootholdTestWorld(const FFootholdTestWorld&) = delete;
	FFootholdTestWorld& operator=(const FFootholdTestWorld&) = delete;

	/** Spawns a box (an engine cube scaled to HalfExtent) that blocks everything. Tags go on the actor and the component. */
	AStaticMeshActor* SpawnBox(const FVector& Center, const FVector& HalfExtent, const FRotator& Rotation = FRotator::ZeroRotator, const TArray<FName>& Tags = TArray<FName>());

	template<class T>
	T* Spawn(const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<T>(Location, Rotation, Params);
	}

	/** Ticks the world for Seconds in fixed steps of Dt. */
	void Tick(float Seconds, float Dt = 1.f / 60.f);

	/** One fixed step: advances the engine frame counter and ticks the world. */
	void StepOnce(float Dt = 1.f / 60.f);

	/** Ticks until Predicate returns true or MaxSeconds pass. Returns whether the predicate became true. */
	bool TickUntil(TFunctionRef<bool()> Predicate, float MaxSeconds, float Dt = 1.f / 60.f);

	/**
	 * Ticks for Seconds (or until StopWhen returns true) and returns every distinct movement mode the
	 * Mover passed through, in order, starting with the current one.
	 */
	TArray<FName> RecordModes(const UMoverComponent* Mover, float Seconds, TFunction<bool()> StopWhen = nullptr, float Dt = 1.f / 60.f);

	UWorld* World = nullptr;
	float ElapsedSeconds = 0.f;
};

#endif
