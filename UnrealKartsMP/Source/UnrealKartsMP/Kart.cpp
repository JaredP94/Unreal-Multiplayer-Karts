// Fill out your copyright notice in the Description page of Project Settings.


#include "Kart.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// Sets default values
AKart::AKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bReplicateMovement = false;

	MovementComponent = CreateDefaultSubobject<UKartMovementComponent>(TEXT("MovementComponent"));
	MovementReplicationComponent = CreateDefaultSubobject<UKartMovementReplicationComponent>(TEXT("MovementReplicationComponent"));
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

FString AKart::GetEnumText(ENetRole OwnerRole)
{
	switch (OwnerRole)
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

// Called to bind functionality to input
void AKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AKart::MoveRight);
}