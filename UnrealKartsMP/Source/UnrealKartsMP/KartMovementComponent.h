// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KartMovementComponent.generated.h"

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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREALKARTSMP_API UKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UKartMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SimulateMove(const FKartMove & Move);
	FKartMove CreateMove(float DeltaTime);
	void SetVelocity(FVector Value);
	FVector GetVelocity();
	void SetSteeringThrow(float Value);
	void SetThrottle(float Value);

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

	void ApplyRotation(float DeltaTime, float SteeringThrow);
	void UpdateLocationFromVelocity(float DeltaTime);
	FVector GetAirResistance();
	FVector GetRollingResistance();
		
	FVector Velocity;
	float Throttle;
	float SteeringThrow;
};
