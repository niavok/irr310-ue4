// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Irr310ShipModule.h"
#include "Irr310ShipEngine.generated.h"

class AIrr310Ship;

/**
 * 
 */
UCLASS(Blueprintable, ClassGroup = (Irr310, Ship), meta = (BlueprintSpawnableComponent))
class IRR310_API UIrr310ShipEngine : public UIrr310ShipModule
{
	GENERATED_UCLASS_BODY()

public:
	virtual void TickModule(AIrr310Ship* ship, float deltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float CurrentThrust;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float MinThrust;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,  Category = Irr310Physics)
	float MaxThrust;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float TargetThrust;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float ThrustVariationSpeed;

	/**
	  * Configure the target thrust
	  * 1 for max trust
	  * 0 for no thrust
	  * -1 for max reverse thrust
	  */
	void SetTargetThrustRatio(float ratio);

};
