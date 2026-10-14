// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

#include "FootholdCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "FootholdComponent.h"
#include "FootholdMoverComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "MoverDataModelTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FootholdCharacter)

AFootholdCharacter::AFootholdCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicatingMovement(false);	// Mover replicates its own state
	bReplicates = true;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(34.f, 88.f);
	Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	SetRootComponent(Capsule);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Capsule);
	Mesh->SetRelativeLocationAndRotation(FVector(0, 0, -88), FRotator(0, -90, 0));
	Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Capsule);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->SocketOffset = FVector(0, 0, 60);
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	Mover = CreateDefaultSubobject<UFootholdMoverComponent>(TEXT("Mover"));
	Foothold = CreateDefaultSubobject<UFootholdComponent>(TEXT("Foothold"));

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
}

UMoverComponent* AFootholdCharacter::GetMoverComponent() const
{
	return Mover;
}

void AFootholdCharacter::SetScriptedMove(FVector WorldDirection)
{
	ScriptedMove = WorldDirection.GetClampedToMaxSize(1.f);
	bUseScriptedMove = !ScriptedMove.IsNearlyZero();
}

void AFootholdCharacter::PressJump()
{
	bJumpPending = true;
}

FVector AFootholdCharacter::GetVelocity() const
{
	return Mover ? Mover->GetVelocity() : Super::GetVelocity();
}

void AFootholdCharacter::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	FCharacterDefaultInputs& Inputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	FVector Move = FVector::ZeroVector;
	if (bUseScriptedMove)
	{
		Move = ScriptedMove;
	}
	else if (!PlayerMoveInput.IsNearlyZero())
	{
		const FRotator YawOnly(0.f, GetControlRotation().Yaw, 0.f);
		Move = YawOnly.RotateVector(FVector(PlayerMoveInput.Y, PlayerMoveInput.X, 0.f)).GetClampedToMaxSize(1.f);
	}

	Inputs.SetMoveInput(EMoveInputType::DirectionalIntent, Move);
	Inputs.OrientationIntent = Move.GetSafeNormal();
	Inputs.ControlRotation = Controller ? GetControlRotation() : FRotator::ZeroRotator;
	Inputs.bIsJumpJustPressed = bJumpPending;
	Inputs.bIsJumpPressed = bJumpPending;
	bJumpPending = false;
}

void AFootholdCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFootholdCharacter::OnMove);
			Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AFootholdCharacter::OnMoveCompleted);
		}
		if (LookAction)
		{
			Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFootholdCharacter::OnLook);
		}
		if (JumpAction)
		{
			Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AFootholdCharacter::OnJump);
		}
	}
}

void AFootholdCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AFootholdCharacter::OnMove(const FInputActionValue& Value)
{
	PlayerMoveInput = Value.Get<FVector2D>();
}

void AFootholdCharacter::OnMoveCompleted(const FInputActionValue& Value)
{
	PlayerMoveInput = FVector2D::ZeroVector;
}

void AFootholdCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Look = Value.Get<FVector2D>();
	AddControllerYawInput(Look.X);
	AddControllerPitchInput(Look.Y);
}

void AFootholdCharacter::OnJump(const FInputActionValue& Value)
{
	PressJump();
}
