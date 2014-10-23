// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include "Irr310Ship.generated.h"

/**
 * 
 */
UCLASS()
class IRR310_API AIrr310Ship : public APawn
{
	GENERATED_UCLASS_BODY()

	virtual void Tick(float deltaTime) override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:
	FVector GetLinearSpeed();

	FVector GetLocalLinearSpeed();

	float GetAltitude();

	FVector GetLocation();

	FVector getLocalAngularVelocity();

	FVector getWorldAngularVelocity();

	void AddForceAtLocation(FVector force, FVector applicationPoint);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
		float Mass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310AutoPilot)
		FVector LocalLinearVelocityTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310AutoPilot)
		FVector LocalAngularVelocityTarget;

	static const FName LookUpBinding;
	static const FName LookRightBinding;

private:
	

	FVector TickSumForce;
	FVector TickSumTorque;

	void PhysicSubTick(float deltaTime);
	
	void AutoPilotSubTick(float deltaTime);

	float* ComputeLinearVelocityStabilisation(TArray<UActorComponent*>& Engines, FVector LocalTargetSpeed);
	float* ComputeAngularVelocityStabilisation(TArray<UActorComponent*>& Engines, FVector LocalTargetSpeed);

	float* ComputeAngularControl(TArray<UActorComponent*>& Engines, FVector LocalShipAxis, FVector TargetAxis);
	float* ComputePositionWithoutRotationControl(TArray<UActorComponent*>& Engines, FVector TargetLocation, float speed);
	float* ComputePositionWithRotationControl(TArray<UActorComponent*>& Engines, FVector TargetLocation, float speed);
	


	// Actions
	void OnIncreaseLinearVelocity();
	void OnDecreaseLinearVelocity();
	void OnKillLinearVelocity();
	void OnRandomizeRotationSpeed();
	void OnPichtCommand(float Val);
	void OnYawCommand(float Val);
};
