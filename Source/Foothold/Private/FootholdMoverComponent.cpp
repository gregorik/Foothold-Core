// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdMoverComponent.h"
#include "UObject/UnrealType.h"
#include "FootholdDefaults.h"
#include "MovementModeTransition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdMoverComponent)

UFootholdMoverComponent::UFootholdMoverComponent()
{
	for (const FFootholdDefaults::FModeEntry& Entry : FFootholdDefaults::GetModes())
	{
		const FName SubobjectName(*FString::Printf(TEXT("Foothold%sMode"), *Entry.Name.ToString()));
		UObject* Mode = CreateDefaultSubobject(SubobjectName, UBaseMovementMode::StaticClass(), Entry.Class.Get(), /*bIsRequired*/ true, /*bIsTransient*/ false);
		MovementModes.Add(Entry.Name, CastChecked<UBaseMovementMode>(Mode));
	}

	int32 Index = 0;
	for (const TSubclassOf<UBaseMovementModeTransition>& TransitionClass : FFootholdDefaults::GetGlobalTransitions())
	{
		const FName SubobjectName(*FString::Printf(TEXT("FootholdTransition%d"), Index++));
		UObject* Transition = CreateDefaultSubobject(SubobjectName, UBaseMovementModeTransition::StaticClass(), TransitionClass.Get(), /*bIsRequired*/ true, /*bIsTransient*/ false);
		Transitions.Add(CastChecked<UBaseMovementModeTransition>(Transition));
	}

	// Blueprint children validate this template, which never registers, and Mover only makes shared
	// settings on register and on edit: create the ones the default modes and transitions ask for here.
	TArray<UClass*> SettingsClasses;
	auto Collect = [&SettingsClasses](const TArray<TSubclassOf<UObject>>& Classes)
	{
		for (const TSubclassOf<UObject>& Class : Classes)
		{
			if (Class)
			{
				SettingsClasses.AddUnique(Class.Get());
			}
		}
	};
	for (const TPair<FName, TObjectPtr<UBaseMovementMode>>& Mode : MovementModes)
	{
		if (Mode.Value)
		{
			Collect(Mode.Value->SharedSettingsClasses);
			for (const UBaseMovementModeTransition* Transition : Mode.Value->Transitions)
			{
				if (Transition)
				{
					Collect(Transition->SharedSettingsClasses);
				}
			}
		}
	}
	for (const UBaseMovementModeTransition* Transition : Transitions)
	{
		if (Transition)
		{
			Collect(Transition->SharedSettingsClasses);
		}
	}
	SettingsClasses.Sort([](const UClass& A, const UClass& B) { return A.GetPathName() < B.GetPathName(); });	// Mover's order
	const FArrayProperty* SettingsProperty = FindFProperty<FArrayProperty>(UMoverComponent::StaticClass(), TEXT("SharedSettings"));	// private
	const FObjectPropertyBase* SettingProperty = SettingsProperty ? CastField<FObjectPropertyBase>(SettingsProperty->Inner) : nullptr;
	if (!ensureMsgf(SettingProperty, TEXT("UMoverComponent::SharedSettings changed; Foothold can't pre-create shared settings.")))
	{
		return;
	}
	FScriptArrayHelper Settings(SettingsProperty, SettingsProperty->ContainerPtrToValuePtr<void>(this));
	for (UClass* Class : SettingsClasses)
	{
		const FName SubobjectName(*FString::Printf(TEXT("SharedSettings_%s"), *Class->GetName()));
		UObject* Setting = CreateDefaultSubobject(SubobjectName, UObject::StaticClass(), Class, /*bIsRequired*/ true, /*bIsTransient*/ false);
		SettingProperty->SetObjectPropertyValue(Settings.GetRawPtr(Settings.AddValue()), Setting);
	}
}
