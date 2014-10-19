// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "Irr310.h"
#include "Irr310SimpleShipHUD.h"
#include "Irr310SimpleShipHUD.h"
#include "Irr310Ship.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "CanvasItem.h"

#define LOCTEXT_NAMESPACE "VehicleHUD"

AIrr310SimpleShipHUD::AIrr310SimpleShipHUD(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Engine/EngineFonts/RobotoDistanceField"));
	HUDFont = Font.Object;
	UE_LOG(LogTemp, Warning, TEXT("new AIrr310SimpleShipHUD"));
}

void AIrr310SimpleShipHUD::DrawHUD()
{
	Super::DrawHUD();
	//UE_LOG(LogTemp, Warning, TEXT("DrawHUD"));
	// Calculate ratio from 720p
	const float HUDXRatio = Canvas->SizeX / 1280.f;
	const float HUDYRatio = Canvas->SizeY / 720.f;

	// Get our vehicle so we can check if we are in car. If we are we don't want onscreen HUD
	AIrr310Ship* Ship = Cast<AIrr310Ship>(GetOwningPawn());
	if (Ship != nullptr)
	{
		FVector2D ScaleVec(HUDYRatio * 1.4f, HUDYRatio * 1.4f);

		float Speed = Ship->GetLinearSpeed().Size();
		float Altitude = Ship->GetAltitude();
		float AltitudeVariation = Ship->GetLinearSpeed().Z;

		// Speed
		FCanvasTextItem SpeedTextItem(FVector2D(HUDXRatio * 805.f, HUDYRatio * 455), FText::Format(LOCTEXT("SpeedFormat", "{0} m/s"), FText::AsNumber(Speed)), HUDFont, FLinearColor::Black);
		SpeedTextItem.Scale = ScaleVec;
		Canvas->DrawItem(SpeedTextItem);

		// Altitude
		FCanvasTextItem AltitudeTextItem(FVector2D(HUDXRatio * 805.f, HUDYRatio * 500), FText::Format(LOCTEXT("AltitudeFormat", "{0} m"), FText::AsNumber(Altitude)), HUDFont, FLinearColor::Black);
		AltitudeTextItem.Scale = ScaleVec;
		Canvas->DrawItem(AltitudeTextItem);

		// Altitude variation
		FCanvasTextItem AltitudeVariationTextItem(FVector2D(HUDXRatio * 805.f, HUDYRatio * 545), FText::Format(LOCTEXT("AltitudeVariationFormat", "{0} m/s"), FText::AsNumber(AltitudeVariation)), HUDFont, FLinearColor::Black);
		AltitudeVariationTextItem.Scale = ScaleVec;
		Canvas->DrawItem(AltitudeVariationTextItem);

	
	}
}

#undef LOCTEXT_NAMESPACE