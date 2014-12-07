// Fill out your copyright notice in the Description page of Project Settings.

#include "Irr310.h"
#include "Irr310ShipControlSurface.h"
#include "Irr310Ship.h"

#include "Irr310PhysicHelper.h"


UIrr310ShipControlSurface::UIrr310ShipControlSurface(const class FPostConstructInitializeProperties& PCIP)
	:Super(PCIP)
{
	UE_LOG(LogTemp, Warning, TEXT("new UIrr310ShipEngine"));
	PrimaryComponentTick.bCanEverTick = true;
	
	MinimumAngle = -30;
	MaximumAngle = 30;
	CurrentAngle = 0;
	TargetAngle = 0;
	LocalSurfaceNeutralLocalAxis = LocalSurfaceAxis;

	//AttachChildren()

	//Surface = 1;
}

void UIrr310ShipControlSurface::TickModule(AIrr310Ship* Ship, float DeltaTime)
{

	

	CurrentAngle = TargetAngle;
	CurrentAngle = FMath::Clamp(CurrentAngle, MinimumAngle, MaximumAngle);


	//CurrentAngle = 20;

	USceneComponent* child = GetChildComponent(0);
	if (child != NULL) {

		
		float VisualMaxSpeed = 30; // 5°/s
		VisualAngle += FMath::Clamp(CurrentAngle * 2 - VisualAngle, -VisualMaxSpeed * DeltaTime, VisualMaxSpeed * DeltaTime);

		child->SetRelativeRotation(FRotator(VisualAngle, 0, 0));
	}

	//AttachChildren
	
	// Compute surface axis from current angle
	//UE_LOG(LogTemp, Warning, TEXT("CurrentAngle: %f"), CurrentAngle);
	LocalSurfaceAxis = LocalSurfaceNeutralLocalAxis.RotateAngleAxis(-CurrentAngle, LocalLeadingEdgeAxis);

	Super::TickModule(Ship, DeltaTime);
}



void UIrr310ShipControlSurface::SetTargetThrustRatio(float ratio) {

	//ratio = -1.0;

	ratio = FMath::Clamp(ratio, -1.f, 1.f);
	
	if (ratio > 0) {
		TargetAngle = GetTargetAngleForAngleOfAttack(0) * (1-ratio)  + GetTargetAngleForAngleOfAttack(15) * ratio;
	}
	else {
		TargetAngle = GetTargetAngleForAngleOfAttack(0) * (1 + ratio) + GetTargetAngleForAngleOfAttack(-15) * - ratio;
	}
}



float UIrr310ShipControlSurface::GetCurrentMaxThrust() const
{
	// The max Thrust is the lift on 15° of angle of attack, or if not possible the angle of attack the nearest

	AIrr310Ship* Ship = Cast<AIrr310Ship>(GetOwner());

	// TODO Cache these operations
	// Warning, it's the neutral axis
	FVector SurfaceAxis = GetComponentToWorld().GetRotation().RotateVector(LocalSurfaceNeutralLocalAxis);
	FVector LeadingEdgeAxis = GetComponentToWorld().GetRotation().RotateVector(LocalLeadingEdgeAxis);

	FVector LinearSpeed = Ship->GetLinearSpeed();


	FVector ForceOffset = GetComponentToWorld().GetRotation().RotateVector(LocalForceOffset);
	FVector ForceCenterLocation = GetComponentLocation() / 100;
	ForceCenterLocation += ForceOffset;


	//Add linear speed induced by rotation
	FVector Distance = (ForceCenterLocation - Ship->GetLocation());
	FVector LocalLinearSpeed = FVector::CrossProduct(Ship->getWorldAngularVelocity(), Distance) * PI / 180;

	LinearSpeed += LocalLinearSpeed;

	// Current Lift

	// Chord axis
	FVector ChordAxis = FVector::CrossProduct(LeadingEdgeAxis, SurfaceAxis);
	ChordAxis.Normalize();

	FVector AirSpeedInLeadingPlane = FVector::CrossProduct(LeadingEdgeAxis, FVector::CrossProduct(LinearSpeed, LeadingEdgeAxis));
	FVector AirSpeedInLeadingPlaneAxis = AirSpeedInLeadingPlane;
	AirSpeedInLeadingPlaneAxis.Normalize();

	float Dot = FVector::DotProduct(ChordAxis, AirSpeedInLeadingPlaneAxis);

	float LiftSign = -FMath::Sign(FVector::DotProduct(SurfaceAxis, AirSpeedInLeadingPlaneAxis));


	// Neutral Angle of attack
	float AngleOfAttack = FMath::RadiansToDegrees(FMath::Acos(Dot));


	//Max lift for AngleOfAttack = 15

	float TargetAngle = 15 - LiftSign * AngleOfAttack;

	TargetAngle = FMath::Clamp(TargetAngle, MinimumAngle, MaximumAngle);

	float MaxAngleOfAttack = AngleOfAttack + TargetAngle;


	float Ro = 1.6550; // Air density
	float LiftRatio = Irr310PhysicHelper::ComputeLiftRatio(MaxLiftRatio, FMath::Abs(MaxAngleOfAttack));

	float Lift = Irr310PhysicHelper::ComputeLift(AirSpeedInLeadingPlane.SizeSquared(), Ro, LiftRatio, Surface);

	//UE_LOG(LogTemp, Warning, TEXT("Max Lift: %f"), Lift);

	return Lift;
}

float UIrr310ShipControlSurface::GetCurrentMinThrust() const
{
	// The max Thrust is the lift on 15° of angle of attack, or if not possible the angle of attack the nearest

	AIrr310Ship* Ship = Cast<AIrr310Ship>(GetOwner());

	// TODO Cache these operations
	// Warning, it's the neutral axis
	FVector SurfaceAxis = GetComponentToWorld().GetRotation().RotateVector(LocalSurfaceNeutralLocalAxis);
	FVector LeadingEdgeAxis = GetComponentToWorld().GetRotation().RotateVector(LocalLeadingEdgeAxis);

	FVector LinearSpeed = Ship->GetLinearSpeed();


	FVector ForceOffset = GetComponentToWorld().GetRotation().RotateVector(LocalForceOffset);
	FVector ForceCenterLocation = GetComponentLocation() / 100;
	ForceCenterLocation += ForceOffset;


	//Add linear speed induced by rotation
	FVector Distance = (ForceCenterLocation - Ship->GetLocation());
	FVector LocalLinearSpeed = FVector::CrossProduct(Ship->getWorldAngularVelocity(), Distance) * PI / 180;

	LinearSpeed += LocalLinearSpeed;

	// Current Lift

	// Chord axis
	FVector ChordAxis = FVector::CrossProduct(LeadingEdgeAxis, SurfaceAxis);
	ChordAxis.Normalize();

	FVector AirSpeedInLeadingPlane = FVector::CrossProduct(LeadingEdgeAxis, FVector::CrossProduct(LinearSpeed, LeadingEdgeAxis));
	FVector AirSpeedInLeadingPlaneAxis = AirSpeedInLeadingPlane;
	AirSpeedInLeadingPlaneAxis.Normalize();

	float Dot = FVector::DotProduct(ChordAxis, AirSpeedInLeadingPlaneAxis);

	float LiftSign =- FMath::Sign(FVector::DotProduct(SurfaceAxis, AirSpeedInLeadingPlaneAxis));


	// Neutral Angle of attack
	float AngleOfAttack = FMath::RadiansToDegrees(FMath::Acos(Dot));


	//Max lift for AngleOfAttack = 15

	float TargetAngle = -15 - LiftSign * AngleOfAttack;

	TargetAngle = FMath::Clamp(TargetAngle, MinimumAngle, MaximumAngle);

	float MaxAngleOfAttack = AngleOfAttack + TargetAngle;


	float Ro = 1.6550; // Air density
	float LiftRatio = Irr310PhysicHelper::ComputeLiftRatio(MaxLiftRatio, FMath::Abs(MaxAngleOfAttack));

	float Lift = -Irr310PhysicHelper::ComputeLift(AirSpeedInLeadingPlane.SizeSquared(), Ro, LiftRatio, Surface);
	//UE_LOG(LogTemp, Warning, TEXT("Min Lift: %f"), Lift);
	return Lift;
}

float UIrr310ShipControlSurface::GetTargetAngleForAngleOfAttack(float TargetAngleOfAttack) {
	// The max Thrust is the lift on 15° of angle of attack, or if not possible the angle of attack the nearest

	AIrr310Ship* Ship = Cast<AIrr310Ship>(GetOwner());

	// TODO Cache these operations
	// Warning, it's the neutral axis
	FVector SurfaceAxis = GetComponentToWorld().GetRotation().RotateVector(LocalSurfaceNeutralLocalAxis);
	FVector LeadingEdgeAxis = GetComponentToWorld().GetRotation().RotateVector(LocalLeadingEdgeAxis);

	FVector LinearSpeed = Ship->GetLinearSpeed();


	FVector ForceOffset = GetComponentToWorld().GetRotation().RotateVector(LocalForceOffset);
	FVector ForceCenterLocation = GetComponentLocation() / 100;
	ForceCenterLocation += ForceOffset;


	//Add linear speed induced by rotation
	FVector Distance = (ForceCenterLocation - Ship->GetLocation());
	FVector LocalLinearSpeed = FVector::CrossProduct(Ship->getWorldAngularVelocity(), Distance) * PI / 180;

	LinearSpeed += LocalLinearSpeed;

	// Current Lift

	// Chord axis
	FVector ChordAxis = FVector::CrossProduct(LeadingEdgeAxis, SurfaceAxis);
	ChordAxis.Normalize();

	FVector AirSpeedInLeadingPlane = FVector::CrossProduct(LeadingEdgeAxis, FVector::CrossProduct(LinearSpeed, LeadingEdgeAxis));
	FVector AirSpeedInLeadingPlaneAxis = AirSpeedInLeadingPlane;
	AirSpeedInLeadingPlaneAxis.Normalize();

	float Dot = FVector::DotProduct(ChordAxis, AirSpeedInLeadingPlaneAxis);

	float LiftSign = -FMath::Sign(FVector::DotProduct(SurfaceAxis, AirSpeedInLeadingPlaneAxis));


	// Neutral Angle of attack
	float AngleOfAttack = FMath::RadiansToDegrees(FMath::Acos(Dot));


	//Max lift for AngleOfAttack = 15

	float TargetAngle = TargetAngleOfAttack - LiftSign * AngleOfAttack;

	TargetAngle = FMath::Clamp(TargetAngle, MinimumAngle, MaximumAngle);

	return TargetAngle;
}