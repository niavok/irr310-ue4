// Fill out your copyright notice in the Description page of Project Settings.

#include "Irr310.h"
#include "Irr310ShipModule.h"
#include "Irr310Ship.h"

UIrr310ShipModule::UIrr310ShipModule(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	UE_LOG(LogTemp, Warning, TEXT("new UIrr310ShipModule"));
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	bTickInEditor = true;
}


void UIrr310ShipModule::TickModule(AIrr310Ship* ship, float deltaTime)
{
	//UE_LOG(LogTemp, Warning, TEXT("UIrr310ShipModule::TickComponent"));
}