// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Irr310ShipFin.h"
#include "Irr310ShipControlSurface.generated.h"

class AIrr310Ship;

/**
 * 
 */
UCLASS(Blueprintable, ClassGroup = (Irr310, Ship), meta = (BlueprintSpawnableComponent))
class IRR310_API UIrr310ShipControlSurface : public UIrr310ShipFin
{
	GENERATED_UCLASS_BODY()

public:

	virtual void TickModule(AIrr310Ship* Ship, float DeltaTime) override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float MinimumAngle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float MaximumAngle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float CurrentAngle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float TargetAngle;

	FVector LocalSurfaceNeutralLocalAxis;

	virtual float GetCurrentMaxThrust() const;

	virtual float GetCurrentMinThrust() const;

	/**
	* Configure the target thrust
	* 1 for max trust
	* 0 for no thrust
	* -1 for max reverse thrust
	*/
	virtual void SetTargetThrustRatio(float ratio);

private:
	float GetTargetAngleForAngleOfAttack(float TargetAngleOfAttack);

	float VisualAngle;
};
