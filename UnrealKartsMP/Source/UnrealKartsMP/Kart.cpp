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

	if (IsLocallyControlled())
	{
		FKartMove Move = CreateMove(DeltaTime);

		if (!HasAuthority()) 
		{
			UnacknowledgedMoveQueue.Add(Move);
			UE_LOG(LogTemp, Warning, TEXT("Queue length: %d"), UnacknowledgedMoveQueue.Num());
		}

		Server_SendMove(Move);
		SimulateMove(Move);
	}

	DrawDebugString(GetWorld(), FVector(0, 0, 150), GetEnumText(Role), this, FColor::Blue, DeltaTime);
}

void AKart::MoveForward(float Value)
{
	Throttle = Value;
}

void AKart::MoveRight(float Value)
{
	SteeringThrow = Value;
}

void AKart::ApplyRotation(float DeltaTime, float SteeringThrow)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurnRadius * SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);
	Velocity = RotationDelta.RotateVector(Velocity);
	AddActorWorldRotation(RotationDelta);
}

void AKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * DeltaTime * 100; // cm

	FHitResult Hit;
	AddActorWorldOffset(Translation, true, &Hit);

	if (Hit.IsValidBlockingHit())
		Velocity = FVector::ZeroVector;
}

FVector AKart::GetAirResistance()
{
	return (-Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient);
}

FVector AKart::GetRollingResistance()
{
	float GravitationalAcceleration = -(GetWorld()->GetGravityZ() / 100);
	float NormalForce = Mass * GravitationalAcceleration;
	return (-Velocity.GetSafeNormal() * RollingCoefficient * NormalForce);
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

void AKart::SimulateMove(FKartMove Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity += Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);
}

FKartMove AKart::CreateMove(float DeltaTime)
{
	FKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->TimeSeconds;

	return Move;
}

void AKart::ClearAcknowledgedMoves(FKartMove LastMove)
{
	TArray<FKartMove> NewMoveQeue;

	for (const FKartMove& Move : UnacknowledgedMoveQueue)
	{
		if (Move.Time > LastMove.Time)
			NewMoveQeue.Add(Move);
	}

	UnacknowledgedMoveQueue = NewMoveQeue;
}

void AKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKart, ServerState);
}

void AKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Tranform);
	Velocity = ServerState.Velocity;

	ClearAcknowledgedMoves(ServerState.LastMove);
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
	SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Tranform = GetActorTransform();
	ServerState.Velocity = Velocity;
}

bool AKart::Server_SendMove_Validate(FKartMove Move)
{
	return true; // TODO: Implement validation
}