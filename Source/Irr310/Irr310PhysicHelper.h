// Copyright 2014 Frédéric Bertolus, Inc. All Rights Reserved.
#pragma once

#include "fvec.h"

class Irr310PhysicHelper
{
public:
	

	static FVector ComputeLinearAirResistance(FVector Velocity, float Ro, float A);

	static FVector ComputeAngularAirResistance(FVector Velocity, float Ro);

	static FVector ComputeGravity();

	/**
	Return levitation acceleration
	*/
	static FVector ComputeLevitation(FVector location);

private:
	Irr310PhysicHelper();
	virtual ~Irr310PhysicHelper();
};
