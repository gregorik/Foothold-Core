// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MoverSimulationTypes.h"
#include "FootholdCharacter.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UFootholdComponent;
class UFootholdMoverComponent;
class UInputAction;
class UInputMappingContext;
class UMoverComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
struct FInputActionValue;

/**
 * A minimal Mover pawn with every Foothold mode: capsule, mesh, third-person camera, Foothold Mover
 * and Foothold components. Assign the input actions (or drive it from code with SetScriptedMove and
 * PressJump) and a skeletal mesh with an anim blueprint that reads UFootholdComponent::GetAnimData().
 *
 * Its input matches GASP's Mover pawn: camera-relative directional move input, orientation toward
 * the move direction, and jump flagged only on the frame it's pressed.
 */
UCLASS(Blueprintable, ClassGroup = (Foothold))
class FOOTHOLD_API AFootholdCharacter : public APawn, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	AFootholdCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Foothold")
	UMoverComponent* GetMoverComponent() const;

	UFUNCTION(BlueprintPure, Category = "Foothold")
	UFootholdComponent* GetFootholdComponent() const { return Foothold; }

	/** Drives movement from code (bots, tests): a world-space direction, length 0..1. Overrides player input until cleared with zero. */
	UFUNCTION(BlueprintCallable, Category = "Foothold|Input")
	void SetScriptedMove(FVector WorldDirection);

	/** Presses jump for the next movement step. */
	UFUNCTION(BlueprintCallable, Category = "Foothold|Input")
	void PressJump();

	virtual FVector GetVelocity() const override;

	// IMoverInputProducerInterface
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foothold|Input")
	TObjectPtr<UInputAction> JumpAction;

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	void OnMove(const FInputActionValue& Value);
	void OnMoveCompleted(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJump(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foothold")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foothold")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foothold")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foothold")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foothold")
	TObjectPtr<UFootholdMoverComponent> Mover;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foothold")
	TObjectPtr<UFootholdComponent> Foothold;

	FVector2D PlayerMoveInput = FVector2D::ZeroVector;
	FVector ScriptedMove = FVector::ZeroVector;
	bool bUseScriptedMove = false;
	bool bJumpPending = false;
};
