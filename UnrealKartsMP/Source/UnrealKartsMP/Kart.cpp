// Fill out your copyright notice in the Description page of Project Settings.


#include "Kart.h"

#include "Components/InputComponent.h"
#include "Quat.h"
#include "Engine/World.h"

// Sets default values
AKart::AKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AKart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;

	Velocity += Acceleration * DeltaTime;

	ApplyRotation(DeltaTime);

	UpdateLocationFromVelocity(DeltaTime);
}

void AKart::ApplyRotation(float DeltaTime)
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

// Called to bind functionality to input
void AKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AKart::Server_MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AKart::Server_MoveRight);
}

void AKart::Server_MoveForward_Implementation(float Value)
{
	Throttle = Value;
}

bool AKart::Server_MoveForward_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}

void AKart::Server_MoveRight_Implementation(float Value)
{
	SteeringThrow = Value;
}

bool AKart::Server_MoveRight_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}