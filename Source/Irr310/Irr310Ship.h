// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include "Irr310FlightRecorder.h"
#include "Irr310Ship.generated.h"

/**
 * 
 */
UCLASS()
class IRR310_API AIrr310Ship : public APawn
{
	GENERATED_UCLASS_BODY()

	virtual void Tick(float DeltaSeconds) override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:
	FVector GetLinearSpeed() const;

	FVector GetLocalLinearSpeed() const;

	float GetAltitude() const;

	FVector GetLocation() const;

	FVector getLocalAngularVelocity() const;

	FVector getWorldAngularVelocity() const;

	void AddForceAtLocation(FVector force, FVector applicationPoint);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float Mass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float MagneticMass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	FVector LocalCOM;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float InertiaTensor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310AutoPilot)
	FVector LocalLinearVelocityTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310AutoPilot)
	FVector LocalAngularVelocityTarget;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Irr310AutoPilot)
	FVector FrontAxisTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Irr310AutoPilot)
	FVector LocationTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Irr310AutoPilot)
	int32 ControlMode;

	static const FName LookUpBinding;
	static const FName LookRightBinding;

private:
	
	Irr310FlightRecorder FlightRecorder;

	// Temporary variable reset each tich
	FVector TickSumForce;
	FVector TickSumTorque;
	FVector COM;


	void PhysicSubTick(float DeltaSeconds);
	
	void AutoPilotSubTick(float DeltaSeconds);

	float* ComputeLinearVelocityStabilisation(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector WorldTargetSpeed, float ThrustAngleLimit) const;
	float* ComputeLinearVelocityStabilisationWithRotation(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector LocalTargetSpeed) const;
	
	float* ComputeAngularVelocityStabilisation(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector LocalTargetSpeed) const;
	
	float* ComputeAngularControl(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector LocalShipAxis, FVector TargetAxis) const;


	float* ComputePositionWithoutRotationControlOld(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector TargetLocation, float speed) const;
	float* ComputePositionWithoutRotationControl(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector TargetLocation, FVector SpeedAtTarget, FVector TargetSpeed, float maxSpeed) const;

	float* ComputePositionWithRotationControl(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector TargetLocation, float speed) const;
	


	// Actions
	void OnIncreaseLinearVelocity();
	void OnDecreaseLinearVelocity();
	void OnKillLinearVelocity();
	void OnRandomizeRotationSpeed();

	void OnIncrementMode();
	void OnDecrementMode();

	void OnPichtCommand(float Val);
	void OnYawCommand(float Val);
	void OnRollCommand(float Val);
	void OnThrustCommand(float Val);
};
