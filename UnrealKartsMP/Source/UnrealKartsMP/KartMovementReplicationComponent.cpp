// Fill out your copyright notice in the Description page of Project Settings.


#include "KartMovementReplicationComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
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

	FKartMove LastMove = MovementComponent->GetLastMove();

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoveQueue.Add(LastMove);
		Server_SendMove(LastMove);
	}

	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
		UpdateServerState(LastMove);

	if (GetOwnerRole() == ROLE_SimulatedProxy)
		ClientTick(DeltaTime);
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

void UKartMovementReplicationComponent::UpdateServerState(const FKartMove & Move)
{
	ServerState.LastMove = Move;
	ServerState.Tranform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UKartMovementReplicationComponent::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) 
		return;

	if (!MovementComponent) 
		return;

	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	FHermiteCubicSpline Spline = CreateSpline();

	InterpolateLocation(Spline, LerpRatio);
	InterpolateVelocity(Spline, LerpRatio);
	InterpolateRotation(LerpRatio);
}

void UKartMovementReplicationComponent::AutonomousProxy_OnRep_ServerState()
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

void UKartMovementReplicationComponent::SimulatedProxy_OnRep_ServerState()
{
	if (!MovementComponent) 
		return;

	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	if (MeshOffsetRoot != nullptr)
	{
		ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
	}

	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Tranform);
}

FHermiteCubicSpline UKartMovementReplicationComponent::CreateSpline()
{
	FHermiteCubicSpline Spline;
	Spline.TargetLocation = ServerState.Tranform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();

	return Spline;
}

void UKartMovementReplicationComponent::InterpolateLocation(const FHermiteCubicSpline & Spline, float LerpRatio)
{
	FVector NewLocation = Spline.InterpolateLocation(LerpRatio);

	if (MeshOffsetRoot != nullptr)
		MeshOffsetRoot->SetWorldLocation(NewLocation);
}

void UKartMovementReplicationComponent::InterpolateVelocity(const FHermiteCubicSpline & Spline, float LerpRatio)
{
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative();
	MovementComponent->SetVelocity(NewVelocity);
}

void UKartMovementReplicationComponent::InterpolateRotation(float LerpRatio)
{
	FQuat TargetRotation = ServerState.Tranform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();

	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	if (MeshOffsetRoot != nullptr)
		MeshOffsetRoot->SetWorldRotation(NewRotation);
}

float UKartMovementReplicationComponent::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdates * 100;
}

void UKartMovementReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKartMovementReplicationComponent, ServerState);
}

void UKartMovementReplicationComponent::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UKartMovementReplicationComponent::SetMeshOffsetRoot(USceneComponent * Root)
{
	MeshOffsetRoot = Root;
}

void UKartMovementReplicationComponent::Server_SendMove_Implementation(FKartMove Move)
{
	if (!MovementComponent)
		return;

	ClientSimulatedTime += Move.DeltaTime;

	MovementComponent->SimulateMove(Move);
	UpdateServerState(Move);
}

bool UKartMovementReplicationComponent::Server_SendMove_Validate(FKartMove Move)
{
	float ProposedTime = ClientSimulatedTime + Move.DeltaTime;
	bool ClientNotRunningAhead = ProposedTime < GetWorld()->TimeSeconds;

	if (!ClientNotRunningAhead) 
	{
		UE_LOG(LogTemp, Error, TEXT("Client is running too fast."));
		return false;
	}

	if (!Move.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Received invalid move."));
		return false;
	}

	return true;
}