// Copyright 2014 Frédéric Bertolus, Inc. All Rights Reserved.
#pragma once

#include "Irr310.h"

class Irr310PhysicHelper
{
public:
	

	//static FVector ComputeLinearAirResistance(FVector Velocity, float Ro, float A);

	//static FVector ComputeAngularAirResistance(FVector Velocity, float Ro);

	static FVector ComputeLinearMagneticResistance(FVector Velocity, float MagneticMass);
	
	static FVector ComputeAngularMagneticResistance(FVector Velocity);

	static FVector ComputeGravity();

	/**
	Return levitation acceleration
	*/
	static FVector ComputeLevitation(FVector location);

	static float ComputeLiftRatio(float MaxLiftRatio, float  AngleOfAttack);

	static float ComputeLift(float SquareVelocity, float Ro, float LiftRatio, float Surface);

private:
	Irr310PhysicHelper();
	virtual ~Irr310PhysicHelper();
};
