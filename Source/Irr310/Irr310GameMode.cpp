// Fill out your copyright notice in the Description page of Project Settings.

#include "Irr310.h"
#include "Irr310GameMode.h"
#include "Irr310SimpleShipHud.h"

AIrr310GameMode::AIrr310GameMode(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	HUDClass = AIrr310SimpleShipHUD::StaticClass();
}


