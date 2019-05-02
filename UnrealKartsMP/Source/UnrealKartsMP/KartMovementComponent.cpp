// Fill out your copyright notice in the Description page of Project Settings.

#include "KartMovementComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Quat.h"

// Sets default values for this component's properties
UKartMovementComponent::UKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UKartMovementComponent::ApplyRotation(float DeltaTime, float SteeringThrowValue)
{
	float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurnRadius * SteeringThrowValue;
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);
	Velocity = RotationDelta.RotateVector(Velocity);
	GetOwner()->AddActorWorldRotation(RotationDelta);
}

void UKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * DeltaTime * 100; // cm

	FHitResult Hit;
	GetOwner()->AddActorWorldOffset(Translation, true, &Hit);

	if (Hit.IsValidBlockingHit())
		Velocity = FVector::ZeroVector;
}

FVector UKartMovementComponent::GetAirResistance()
{
	return (-Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient);
}

FVector UKartMovementComponent::GetRollingResistance()
{
	float GravitationalAcceleration = -(GetWorld()->GetGravityZ() / 100);
	float NormalForce = Mass * GravitationalAcceleration;
	return (-Velocity.GetSafeNormal() * RollingCoefficient * NormalForce);
}

void UKartMovementComponent::SimulateMove(const FKartMove & Move)
{
	FVector Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity += Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);
}

FKartMove UKartMovementComponent::CreateMove(float DeltaTime)
{
	FKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

	return Move;
}

void UKartMovementComponent::SetVelocity(FVector Value)
{
	Velocity = Value;
}

FVector UKartMovementComponent::GetVelocity()
{
	return Velocity;
}

void UKartMovementComponent::SetSteeringThrow(float Value)
{
	SteeringThrow = Value;
}

void UKartMovementComponent::SetThrottle(float Value)
{
	Throttle = Value;
}
