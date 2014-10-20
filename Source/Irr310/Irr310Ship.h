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
	
public:
	FVector GetLinearSpeed();

	FVector GetLocalLinearSpeed();

	float GetAltitude();

	void AddForceAtLocation(FVector force, FVector applicationPoint);

private:

	UPROPERTY(EditDefaultsOnly, Category = Irr310Physics)	
	float Mass;


	FVector TickSumForce;
	FVector TickSumTorque;

	void PhysicSubTick(float deltaTime);
	
	void AutoPilotSubTick(float deltaTime);
};
