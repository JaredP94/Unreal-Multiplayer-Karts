// Fill out your copyright notice in the Description page of Project Settings.


#include "KartMovementReplicationComponent.h"
#include "GameFramework/Actor.h"
#include "UnrealNetwork.h"

// Sets default values for this component's properties
UKartMovementReplicationComponent::UKartMovementReplicationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


// Called when the game starts
void UKartMovementReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UKartMovementComponent>();
	
}


// Called every frame
void UKartMovementReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MovementComponent)
		return;

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		FKartMove Move = MovementComponent->CreateMove(DeltaTime);
		MovementComponent->SimulateMove(Move);
		UnacknowledgedMoveQueue.Add(Move);
		Server_SendMove(Move);
	}

	if (GetOwnerRole() == ROLE_Authority && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		FKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	if (GetOwnerRole() == ROLE_SimulatedProxy)
		MovementComponent->SimulateMove(ServerState.LastMove);
}

void UKartMovementReplicationComponent::ClearAcknowledgedMoves(FKartMove LastMove)
{
	TArray<FKartMove> NewMoveQueue;

	for (const FKartMove& Move : UnacknowledgedMoveQueue)
	{
		if (Move.Time > LastMove.Time)
			NewMoveQueue.Add(Move);
	}

	UnacknowledgedMoveQueue = NewMoveQueue;
}

void UKartMovementReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKartMovementReplicationComponent, ServerState);
}

void UKartMovementReplicationComponent::OnRep_ServerState()
{
	if (!MovementComponent)
		return;

	GetOwner()->SetActorTransform(ServerState.Tranform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FKartMove& Move : UnacknowledgedMoveQueue)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UKartMovementReplicationComponent::Server_SendMove_Implementation(FKartMove Move)
{
	if (!MovementComponent)
		return;

	MovementComponent->SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Tranform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

bool UKartMovementReplicationComponent::Server_SendMove_Validate(FKartMove Move)
{
	return true; // TODO: Implement validation
}