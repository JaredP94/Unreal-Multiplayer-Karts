// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Kart.generated.h"

USTRUCT()
struct FKartMove
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Throttle;
	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;
	UPROPERTY()
	float Time;
};

USTRUCT()
struct FKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Tranform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FKartMove LastMove;
};

UCLASS()
class UNREALKARTSMP_API AKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(EditAnywhere)
	float Mass = 1000; // kg

	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000; // N

	UPROPERTY(EditAnywhere)
	float MinTurnRadius = 10; // m

	UPROPERTY(EditAnywhere)
	float DragCoefficient = 15; // kg/m

	UPROPERTY(EditAnywhere)
	float RollingCoefficient = 0.015; // kg/m

	UPROPERTY(Replicated)
	float Throttle;

	UPROPERTY(Replicated)
	float SteeringThrow;

	UFUNCTION()
	void OnRep_ServerState();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FKartMove Move);

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FKartState ServerState;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void ApplyRotation(float DeltaTime, float SteeringThrow);
	void UpdateLocationFromVelocity(float DeltaTime);
	FVector GetAirResistance();
	FVector GetRollingResistance();
	FString GetEnumText(ENetRole Role);
	void SimulateMove(FKartMove Move);
	FKartMove CreateMove(float DeltaTime);
	void ClearAcknowledgedMoves(FKartMove LastMove);

	FVector Velocity;
	TArray<FKartMove> UnacknowledgedMoveQueue;
};
