// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdModule.h"
#include "Engine/World.h"
#include "FootholdDemoCourse.h"
#include "FootholdLog.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

namespace UE::Foothold::Console
{
	static void SpawnDemo(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}
		TOptional<FVector> Location;
		if (Args.Num() >= 3)
		{
			Location = FVector(FCString::Atod(*Args[0]), FCString::Atod(*Args[1]), FCString::Atod(*Args[2]));
		}
		const APlayerController* Player = World->GetFirstPlayerController();
		const AFootholdDemoCourse* Course = AFootholdDemoCourse::SpawnDemo(World, Location, Player ? Player->GetPawn() : nullptr);
		UE_LOG(LogFoothold, Display, TEXT("Foothold.SpawnDemo: %s"), Course ? *Course->GetActorLocation().ToString() : TEXT("failed"));
	}

	static FAutoConsoleCommandWithWorldAndArgs SpawnDemoCommand(
		TEXT("Foothold.SpawnDemo"),
		TEXT("Foothold.SpawnDemo [X Y Z]: spawns the Foothold demo course (default: 5 m in front of the player) and moves the player to its start. Use foothold.Debug 1 to draw probes."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnDemo));
}

void FFootholdModule::StartupModule()
{
	UE_LOG(LogFoothold, Log, TEXT("Foothold: started"));
}

void FFootholdModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FFootholdModule, Foothold)
