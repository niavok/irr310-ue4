#include "Irr310.h"
#include "Irr310PhysicHelper.h"

Irr310PhysicHelper::Irr310PhysicHelper()
{
	
}

Irr310PhysicHelper::~Irr310PhysicHelper()
{
}
FVector Irr310PhysicHelper::ComputeLinearAirResistance(FVector Velocity, float Ro, float A) {
	float SquareVelocity = Velocity.SizeSquared(); // UE4 is in cm
	FVector VelocityDirection = Velocity;
	VelocityDirection.Normalize();

	return -VelocityDirection * 0.5f * Ro * A * SquareVelocity;
}

FVector Irr310PhysicHelper::ComputeAngularAirResistance(FVector Velocity, float Ro) {
	float SquareAngularVelocity = Velocity.SizeSquared(); // UE4 is in cm
	FVector AngularVelocityDirection = Velocity;
	AngularVelocityDirection.Normalize();

	return -AngularVelocityDirection * 0.1f * Ro * SquareAngularVelocity;
}


FVector Irr310PhysicHelper::ComputeGravity() {
	return FVector(0, 0, -13.62); // 13.62 m/s²
}


/**
Return leviration acceleration
*/
FVector Irr310PhysicHelper::ComputeLevitation(FVector location) {
	return FVector(0, 0, 2.612622450e9 / FMath::Square(location.Z + 13850)); // At 13850 m gravity and levitation ar equal.
}

float Irr310PhysicHelper::ComputeLiftRatio(float MaxLiftRatio, float  AngleOfAttack)
{
	// Parabollic curve. 0 if AngleOfAttack at 0. MaxLiftRatio if AngleOfAttack at 15°
	// 0 if AngleOfAttack > 25 : stall !
	if (AngleOfAttack > 25)  {
		return 0;
	} else {
		return (1 - FMath::Square(AngleOfAttack / 15 - 1)) * MaxLiftRatio;
	}
}

float Irr310PhysicHelper::ComputeLift(float SquareVelocity, float Ro, float LiftRatio, float Surface)
{
	return LiftRatio * Ro * Surface * SquareVelocity;
}