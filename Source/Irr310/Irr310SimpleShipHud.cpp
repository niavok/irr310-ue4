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
	//HUDFont->Set
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
		FVector2D ScaleVec(HUDYRatio * 0.7f, HUDYRatio * 0.7f);

		FVector Speed = Ship->GetLocalLinearSpeed();
		FVector WorldSpeed = Ship->GetLinearSpeed();
		FVector Location = Ship->GetLocation();
		FVector LocalLinearVelocityTarget = Ship->LocalLinearVelocityTarget;
		FVector LocalAngularVelocityTarget = Ship->LocalAngularVelocityTarget;
		
		FRotator WorldRotation = Ship->GetActorRotation();
		
		FVector LocalAngularVelocity = Ship->getLocalAngularVelocity();
		FVector WorldAngularVelocity = Ship->getWorldAngularVelocity();

		FVector LocationTarget = Ship->LocationTarget;

		float leftMargin = 50;
		float line = 455;
		float lineHeight = 20;


		// Target Speed
		FCanvasTextItem TargetPositionTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("SpeedFormat", "Target location: X = {0} m,Y = {1} m,Z = {2} m"),
			FText::AsNumber(LocationTarget.X), FText::AsNumber(LocationTarget.Y), FText::AsNumber(LocationTarget.Z)),
			HUDFont, FLinearColor::Black);
		TargetPositionTextItem.Scale = ScaleVec;
		Canvas->DrawItem(TargetPositionTextItem);
		line += lineHeight;

		// Target Speed
		FCanvasTextItem TargetLinearSpeedTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("SpeedFormat", "Target local speed: X = {0} m/s,Y = {1} m/s,Z = {2} m/s"),
			FText::AsNumber(LocalLinearVelocityTarget.X), FText::AsNumber(LocalLinearVelocityTarget.Y), FText::AsNumber(LocalLinearVelocityTarget.Z)),
			HUDFont, FLinearColor::Black);
		TargetLinearSpeedTextItem.Scale = ScaleVec;
		Canvas->DrawItem(TargetLinearSpeedTextItem);
		line += lineHeight;

		// Target Angular Speed
		FCanvasTextItem TargetAngularSpeedTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("SpeedFormat", "Target local angular speed: Roll = {0} °/s, Pitch = {1} °/s, Yaw = {2} °/s"),
			FText::AsNumber(LocalAngularVelocityTarget.X), FText::AsNumber(LocalAngularVelocityTarget.Y), FText::AsNumber(LocalAngularVelocityTarget.Z)),
			HUDFont, FLinearColor::Black);
		TargetAngularSpeedTextItem.Scale = ScaleVec;
		Canvas->DrawItem(TargetAngularSpeedTextItem);
		line += lineHeight;

		// Local Speed
		FCanvasTextItem LinearSpeedTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("SpeedFormat", "Local speed: X = {0} m/s, Y = {1} m/s, Z = {2} m/s"),
			FText::AsNumber(Speed.X), FText::AsNumber(Speed.Y), FText::AsNumber(Speed.Z)),
			HUDFont, FLinearColor::Black);
		LinearSpeedTextItem.Scale = ScaleVec;
		Canvas->DrawItem(LinearSpeedTextItem);
		line += lineHeight;

		// Local Rotation Speed
		FCanvasTextItem LocalAngularSpeedTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("SpeedFormat", "Local angular speed: Roll = {0} °/s, Pitch = {1} °/s, Yaw = {2} °/s"),
			FText::AsNumber(LocalAngularVelocity.X), FText::AsNumber(LocalAngularVelocity.Y), FText::AsNumber(LocalAngularVelocity.Z)),
			HUDFont, FLinearColor::Black);
		LocalAngularSpeedTextItem.Scale = ScaleVec;
		Canvas->DrawItem(LocalAngularSpeedTextItem);
		line += lineHeight;

		// Location
		FCanvasTextItem AltitudeTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("AltitudeFormat", "Location: X = {0} m, Y = {1} m, Altitude: {2} m"),
			FText::AsNumber(Location.X), FText::AsNumber(Location.Y), FText::AsNumber(Location.Z)),
			HUDFont, FLinearColor::Black);
		AltitudeTextItem.Scale = ScaleVec;
		Canvas->DrawItem(AltitudeTextItem);
		line += lineHeight;

		// World Speed
		FCanvasTextItem LinearWorldSpeedTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("SpeedFormat", "World speed: X = {0} m/s, Y = {1} m/s, Z = {2} m/s"),
			FText::AsNumber(WorldSpeed.X), FText::AsNumber(WorldSpeed.Y), FText::AsNumber(WorldSpeed.Z)),
			HUDFont, FLinearColor::Black);
		LinearWorldSpeedTextItem.Scale = ScaleVec;
		Canvas->DrawItem(LinearWorldSpeedTextItem);
		line += lineHeight;

		// World Rotation
		FCanvasTextItem WorldRotationTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("SpeedFormat", "World rotation: Roll = {0} °, Pitch = {1} °, Yaw = {2} °"),
			FText::AsNumber(WorldRotation.Roll), FText::AsNumber(WorldRotation.Pitch), FText::AsNumber(WorldRotation.Yaw)),
			HUDFont, FLinearColor::Black);
		WorldRotationTextItem.Scale = ScaleVec;
		Canvas->DrawItem(WorldRotationTextItem);
		line += lineHeight;

		// World Rotation Speed
		FCanvasTextItem WorldAngularSpeedTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("SpeedFormat", "World angular speed: Roll = {0} °/s, Pitch = {1} °/s, Yaw = {2} °/s"),
			FText::AsNumber(WorldAngularVelocity.X), FText::AsNumber(WorldAngularVelocity.Y), FText::AsNumber(WorldAngularVelocity.Z)),
			HUDFont, FLinearColor::Black);
		WorldAngularSpeedTextItem.Scale = ScaleVec;
		Canvas->DrawItem(WorldAngularSpeedTextItem);
		line += lineHeight;
	
		// World Rotation Speed
		FCanvasTextItem ControlModeTextItem(FVector2D(HUDXRatio * leftMargin, HUDYRatio * line),
			FText::Format(LOCTEXT("Mode", "Autopilot Mode= {0}"),
			FText::AsNumber(Ship->ControlMode)),
			HUDFont, FLinearColor::Black);
		ControlModeTextItem.Scale = ScaleVec;
		Canvas->DrawItem(ControlModeTextItem);
		line += lineHeight;
	}
}

#undef LOCTEXT_NAMESPACE