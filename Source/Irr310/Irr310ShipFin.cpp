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
}

void UIrr310ShipFin::TickModule(AIrr310Ship* Ship, float DeltaTime)
{
	Super::TickModule(Ship, DeltaTime);

	FVector LinearSpeed = Ship->GetLinearSpeed();

	//Add linear speed induced by rotation
	FVector Distance = (GetComponentLocation()/100 - Ship->GetLocation());
	FVector LocalLinearSpeed = FVector::CrossProduct(Ship->getWorldAngularVelocity(), Distance) * PI / 180;

	LinearSpeed += LocalLinearSpeed;

	FVector LinearSpeedAxis = LinearSpeed;
	LinearSpeedAxis.Normalize();

	FVector WorldSurfaceAxis = GetComponentToWorld().GetRotation().RotateVector(SurfaceAxis);
	WorldSurfaceAxis.Normalize();

	float dot = FVector::DotProduct(LinearSpeedAxis, WorldSurfaceAxis);

	float Ro = 1.6550; // Air density
	float A = FMath::Abs(dot) * Surface * 1; // Flat surface, be exposed surface depend on attack angle



	FVector AirResistance = Irr310PhysicHelper::ComputeLinearAirResistance(LinearSpeed, Ro, A);

	FVector ResultForce = WorldSurfaceAxis * AirResistance.Size() * - FMath::Sign(dot);

	Ship->AddForceAtLocation(ResultForce, GetComponentLocation());
}
