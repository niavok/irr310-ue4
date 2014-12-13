// Copyright 2014 Frédéric Bertolus, Inc. All Rights Reserved.
#pragma once

class AIrr310Ship;


class VelocityConstraints {
public:
	VelocityConstraints(FVector Axis, float Velocity, float Tolerance) :
		Axis(Axis),
		Velocity(Velocity),
		Tolerance(Tolerance){}

	FVector Axis;
	float Velocity;
	float Tolerance;

};

class Irr310AutoPilot
{
public:
	Irr310AutoPilot(AIrr310Ship* Ship);

	virtual ~Irr310AutoPilot();

	/**
	 * Initialize auto pilot for now compute
	 */
	void Init();

	/**
	 * Compute order stack to produce engine commands
	 */
	TArray<float*> Compute();

	/**
	 * Prefered speed if there is no specific constain
	 */
	void SetCrusingSpeed(float Speed, float Tolerance);

	/**
	* Max speed if there is no specific constain
	*/
	void AddMaxSpeedTarget(float Speed, float Tolerance, float priority);

	void AddAltitudeTarget(float Altitude, float Tolerance);

	void AddHeadingTarget(float Heading, float Tolerance);

	void AddVelocityTarget(FVector Axis, float Velocity, float Tolerance);

	void AddAngularVelocityTarget(FVector Axis, float AngluarVelocity, float Tolerance);

	void AddAngleTarget(FVector Axis, FVector Target, float Tolerance);

	// Go to way point
	void AddWaypoint(FVector TargetLocation, FVector LocationTolerance, FVector, TArray<VelocityConstraints> VelocityTarget, FVector WaypointVelocity);

private:
	AIrr310Ship* Ship;
	

};
