// Fill out your copyright notice in the Description page of Project Settings.

#include "Irr310.h"
#include "Irr310ShipFin.h"
#include "Irr310Ship.h"

#include "Irr310PhysicHelper.h"


UIrr310ShipFin::UIrr310ShipFin(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	UE_LOG(LogTemp, Warning, TEXT("new UIrr310ShipEngine"));
	PrimaryComponentTick.bCanEverTick = true;
	Surface = 1;
	LocalSurfaceAxis = FVector(1, 0, 0);
	LocalLeadingEdgeAxis = FVector(0, 1, 0);
	MaxLiftRatio = 1;
}

void UIrr310ShipFin::TickModule(AIrr310Ship* Ship, float DeltaTime)
{
	Super::TickModule(Ship, DeltaTime);

	FVector LinearSpeed = Ship->GetLinearSpeed();


	FVector ForceOffset = GetComponentToWorld().GetRotation().RotateVector(LocalForceOffset);
	FVector ForceCenterLocation = GetComponentLocation()/100;
	ForceCenterLocation += ForceOffset;
	

	//Add linear speed induced by rotation
	FVector Distance = (ForceCenterLocation - Ship->GetLocation());
	FVector LocalLinearSpeed = FVector::CrossProduct(Ship->getWorldAngularVelocity(), Distance) * PI / 180;

	LinearSpeed += LocalLinearSpeed;

	FVector LinearSpeedAxis = LinearSpeed;
	LinearSpeedAxis.Normalize();


	FVector SurfaceAxis = GetComponentToWorld().GetRotation().RotateVector(LocalSurfaceAxis);
	FVector LeadingEdgeAxis = GetComponentToWorld().GetRotation().RotateVector(LocalLeadingEdgeAxis);

	// The fin has 2 effect, a form drag and a lift.
	// The form drag is in the axis for speed.

	// Drag
	
	SurfaceAxis.Normalize();

	float dot = FVector::DotProduct(LinearSpeedAxis, SurfaceAxis);

	float Ro = 1.6550; // Air density
	float A = FMath::Abs(dot) * Surface * 1; // Flat surface, be exposed surface depend on attack angle



	FVector AirResistance = Irr310PhysicHelper::ComputeLinearAirResistance(LinearSpeed, Ro, A);
	Ship->AddForceAtLocation(AirResistance, ForceCenterLocation*100); // TOD unit


	// Lift
	// Depend of angle of attack, angle between the air speed and the fin in the leading eadge axis

	// Chord axis
	FVector ChordAxis = FVector::CrossProduct(LeadingEdgeAxis, SurfaceAxis);
	ChordAxis.Normalize();

	FVector AirSpeedInLeadingPlane = FVector::CrossProduct(LeadingEdgeAxis, FVector::CrossProduct(LinearSpeed, LeadingEdgeAxis));
	FVector AirSpeedInLeadingPlaneAxis = AirSpeedInLeadingPlane;
	AirSpeedInLeadingPlaneAxis.Normalize();

	float Dot = FVector::DotProduct(ChordAxis, AirSpeedInLeadingPlaneAxis);

	float AngleOfAttack = FMath::RadiansToDegrees(FMath::Acos(Dot));

	float LiftSign = FMath::Sign(FVector::DotProduct(SurfaceAxis, AirSpeedInLeadingPlaneAxis));

	float LiftRatio = Irr310PhysicHelper::ComputeLiftRatio(MaxLiftRatio, AngleOfAttack);
	float Lift = Irr310PhysicHelper::ComputeLift(AirSpeedInLeadingPlane.SizeSquared(), Ro, LiftRatio, Surface);


	FVector LiftAxis = - LiftSign * FVector::CrossProduct(AirSpeedInLeadingPlaneAxis, LeadingEdgeAxis);

	Ship->AddForceAtLocation(Lift * LiftAxis, ForceCenterLocation*100); // TOD unit
}
