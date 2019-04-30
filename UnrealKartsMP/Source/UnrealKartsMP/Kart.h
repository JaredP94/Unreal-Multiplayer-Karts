// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Kart.generated.h"

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

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedTranform)
	FTransform ReplicatedTranform;

	UPROPERTY(Replicated)
	FVector Velocity;

	UPROPERTY(Replicated)
	float Throttle;

	UPROPERTY(Replicated)
	float SteeringThrow;

	UFUNCTION()
	void OnRep_ReplicatedTranform();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveForward(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveRight(float Value);

	void MoveForward(float Value);
	void MoveRight(float Value);
	void ApplyRotation(float DeltaTime);
	void UpdateLocationFromVelocity(float DeltaTime);
	FVector GetAirResistance();
	FVector GetRollingResistance();
	FString GetEnumText(ENetRole Role);
};
