// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/StaticMeshComponent.h"
#include "Irr310ShipModule.generated.h"

class AIrr310Ship;

/**
 * 
 */
UCLASS(Blueprintable, ClassGroup = (Irr310, Ship), meta = (BlueprintSpawnableComponent))
class IRR310_API UIrr310ShipModule : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()


		virtual void TickModule(AIrr310Ship* ship, float deltaTime);
	
};
