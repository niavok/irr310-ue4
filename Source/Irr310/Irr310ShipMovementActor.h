// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Irr310ShipModule.h"
#include "Irr310ShipMovementActor.generated.h"

class AIrr310Ship;

/**
 * 
 */
UCLASS(abstract)
class IRR310_API UIrr310ShipMovementActor : public UIrr310ShipModule
{
	GENERATED_UCLASS_BODY()

public:
	//virtual void TickModule(AIrr310Ship* ship, float deltaTime) override;


	virtual FVector GetCurrentThurstAxis() const { check(0 && "You must override this");  return FVector::ZeroVector; };

	virtual float GetCurrentMaxThrust() const { check(0 && "You must override this");  return 0; };

	virtual float GetCurrentMinThrust() const { check(0 && "You must override this");  return 0; };

	virtual FVector GetThrustLocation() const { check(0 && "You must override this");  return FVector::ZeroVector; };

	/**
	  * Configure the target thrust
	  * 1 for max trust
	  * 0 for no thrust
	  * -1 for max reverse thrust
	  */
	virtual void SetTargetThrustRatio(float ratio) { check(0 && "You must override this"); };
};
