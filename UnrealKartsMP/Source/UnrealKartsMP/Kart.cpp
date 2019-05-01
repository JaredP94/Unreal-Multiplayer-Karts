// Fill out your copyright notice in the Description page of Project Settings.


#include "Kart.h"

#include "Components/InputComponent.h"
#include "Quat.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"

// Sets default values
AKart::AKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	MovementComponent = CreateDefaultSubobject<UKartMovementComponent>(TEXT("MovementComponent"));
}

// Called when the game starts or when spawned
void AKart::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
		NetUpdateFrequency = 1;
}

// Called every frame
void AKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!MovementComponent)
		return;

	if (Role == ROLE_AutonomousProxy)
	{
		FKartMove Move = MovementComponent->CreateMove(DeltaTime);
		MovementComponent->SimulateMove(Move);
		UnacknowledgedMoveQueue.Add(Move);
		Server_SendMove(Move);
	}

	if (Role == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy)
	{
		FKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	if (Role == ROLE_SimulatedProxy)
		MovementComponent->SimulateMove(ServerState.LastMove);

	DrawDebugString(GetWorld(), FVector(0, 0, 150), GetEnumText(Role), this, FColor::Blue, DeltaTime);
}

void AKart::MoveForward(float Value)
{
	if (!MovementComponent)
		return;

	MovementComponent->SetThrottle(Value);
}

void AKart::MoveRight(float Value)
{
	if (!MovementComponent)
		return;

	MovementComponent->SetSteeringThrow(Value);
}

FString AKart::GetEnumText(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomousProxy";
	case ROLE_Authority:
		return "Authority";
	case ROLE_MAX:
		return "Max";
	default:
		return "Error";
	}
}

void AKart::ClearAcknowledgedMoves(FKartMove LastMove)
{
	TArray<FKartMove> NewMoveQueue;

	for (const FKartMove& Move : UnacknowledgedMoveQueue)
	{
		if (Move.Time > LastMove.Time)
			NewMoveQueue.Add(Move);
	}

	UnacknowledgedMoveQueue = NewMoveQueue;
}

void AKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKart, ServerState);
}

void AKart::OnRep_ServerState()
{
	if (!MovementComponent)
		return;

	SetActorTransform(ServerState.Tranform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FKartMove& Move : UnacknowledgedMoveQueue)
	{
		MovementComponent->SimulateMove(Move);
	}
}

// Called to bind functionality to input
void AKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AKart::MoveRight);
}

void AKart::Server_SendMove_Implementation(FKartMove Move)
{
	if (!MovementComponent)
		return;

	MovementComponent->SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Tranform = GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

bool AKart::Server_SendMove_Validate(FKartMove Move)
{
	return true; // TODO: Implement validation
}