// Fill out your copyright notice in the Description page of Project Settings.

#include "Irr310.h"
#include "Irr310ShipEngine.h"
#include "Irr310Ship.h"


UIrr310ShipEngine::UIrr310ShipEngine(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	UE_LOG(LogTemp, Warning, TEXT("new UIrr310ShipEngine"));
	PrimaryComponentTick.bCanEverTick = true;
	CurrentThrust = 0;
	MinThrust = 0;
	MaxThrust = 1000;
	TargetThrust = 0;
	ThrustVariationSpeed = 10000;
	ThrustAxis = FVector(0, 0, 1);
}

void UIrr310ShipEngine::TickModule(AIrr310Ship* ship, float deltaTime)
{
	Super::TickModule(ship, deltaTime);
	//UE_LOG(LogTemp, Warning, TEXT("UIrr310ShipEngine::TickModule"));
	
	// Apply thrust variation to reach target
	if (CurrentThrust < TargetThrust) {
		CurrentThrust += ThrustVariationSpeed * deltaTime;
	}
	else if (CurrentThrust > TargetThrust)
	{
		CurrentThrust -= ThrustVariationSpeed * deltaTime;
	}

	CurrentThrust = FMath::Clamp(CurrentThrust, MinThrust, MaxThrust);



	FVector ThrustNoise; // Little random because engine are not perfectly precise
	ThrustNoise.X = FMath::FRandRange(-0.01, 0.01) * CurrentThrust;
	ThrustNoise.Y = FMath::FRandRange(-0.01, 0.01) * CurrentThrust;
	ThrustNoise.Z = FMath::FRandRange(-0.05, 0.05) * CurrentThrust;
	FVector LocalThrust = ThrustAxis * CurrentThrust + ThrustNoise * 0.0;

	FVector WorldThrust = GetComponentToWorld().GetRotation().RotateVector(LocalThrust);

	//UE_LOG(LogTemp, Warning, TEXT("1 - WorldThrust: %s"), *WorldThrust.ToString());


	ship->AddForceAtLocation(WorldThrust, GetComponentLocation());
}

void UIrr310ShipEngine::SetTargetThrustRatio(float ratio) {
	if (ratio > 0) {
		TargetThrust = ratio * MaxThrust;
	} else if (ratio < 0) {
		TargetThrust = - MinThrust * ratio;
	}
	else {
		TargetThrust = 0;
	}
}