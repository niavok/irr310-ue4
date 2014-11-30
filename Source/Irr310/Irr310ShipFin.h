// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Irr310ShipModule.h"
#include "Irr310ShipFin.generated.h"

class AIrr310Ship;

/**
 * 
 */
UCLASS(Blueprintable, ClassGroup = (Irr310, Ship), meta = (BlueprintSpawnableComponent))
class IRR310_API UIrr310ShipFin : public UIrr310ShipModule
{
	GENERATED_UCLASS_BODY()

public:
	virtual void TickModule(AIrr310Ship* Ship, float DeltaTime) override;
		
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float Surface;

	/* Lift ratio at 15�*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	float MaxLiftRatio;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	FVector LocalSurfaceAxis;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	FVector LocalLeadingEdgeAxis;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Irr310Physics)
	FVector LocalForceOffset;

};
