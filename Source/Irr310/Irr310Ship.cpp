// Fill out your copyright notice in the Description page of Project Settings.

#include "Irr310.h"
#include "Irr310Ship.h"
#include "Irr310ShipModule.h"
#include "Irr310ShipEngine.h"

AIrr310Ship::AIrr310Ship(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	UE_LOG(LogTemp, Warning, TEXT("new AIrr310Ship"));
	Mass = 1000;
	TickSumForce = FVector::ZeroVector;
	TickSumTorque = FVector::ZeroVector;
}

void AIrr310Ship::Tick(float deltaTime)
{
	Super::Tick(deltaTime);
	
	AutoPilotSubTick(deltaTime);

	// Tick Modules
	TArray<UActorComponent*> Modules = GetComponentsByClass(UIrr310ShipModule::StaticClass());
	for (int32 i = 0; i < Modules.Num(); i++) {
		UIrr310ShipModule* Module = Cast<UIrr310ShipModule>(Modules[i]);
		Module->TickModule(this, deltaTime);
	}
	
	PhysicSubTick(deltaTime);

}

void AIrr310Ship::AddForceAtLocation(FVector force, FVector applicationPoint)
{
	TickSumForce += force;

	

	// TODO extract com
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector COM = c->GetBodyInstance()->GetCOMPosition();

	FVector ApplicationOffset = (applicationPoint - COM) / 100;

	TickSumTorque += FVector::CrossProduct(ApplicationOffset, force);

	/*UE_LOG(LogTemp, Warning, TEXT("0 - AddForceAtLocation: force = %s"), *force.ToString());
	UE_LOG(LogTemp, Warning, TEXT("0 - AddForceAtLocation: ap = %s"), *applicationPoint.ToString());
	UE_LOG(LogTemp, Warning, TEXT("0 - AddForceAtLocation: COM = %s"), *COM.ToString());
	UE_LOG(LogTemp, Warning, TEXT("0 - AddForceAtLocation: FVector::CrossProduct(ApplicationOffset, force) = %s"), *FVector::CrossProduct(ApplicationOffset, force).ToString());
	UE_LOG(LogTemp, Warning, TEXT("0 - AddForceAtLocation: TickSumTorque = %s"), *TickSumTorque.ToString());*/

}

FVector AIrr310Ship::GetLinearSpeed()
{

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	FVector WorldVelocity = c->GetPhysicsLinearVelocity();


	//FVector LocalVelocity = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(WorldVelocity);
	
	return WorldVelocity / 100;
}

float AIrr310Ship::GetAltitude() {
	return GetActorLocation().Z / 100;
}


void AIrr310Ship::PhysicSubTick(float deltaTime)
{
	
	TArray<UActorComponent*> Modules = GetComponentsByClass(UIrr310ShipModule::StaticClass());
	for (int32 i = 0; i < Modules.Num(); i++) {
		UIrr310ShipModule* Module = Cast<UIrr310ShipModule>(Modules[i]);
		Module->TickModule(this, deltaTime);
	}

	

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	//UE_LOG(LogTemp, Warning, TEXT("01 - Acceleration: %s"), *Acceleration.ToString());

	// Gravity
	FVector Gravity = FVector(0, 0, -13.62); // 13.62 m/s²

	//UE_LOG(LogTemp, Warning, TEXT("02 - Acceleration: %s"), *Acceleration.ToString());

	// Levitation
	FVector Levitation = FVector(0, 0, 2.612622450e9 / FMath::Square(GetActorLocation().Z / 100 + 13850)); // At 13850 m gravity and levitation ar equal.


	// Air resistance
	float Ro = 1.6550; // Air density
	float A = 1 * 0.5;
	float SquareVelocity = (c->GetPhysicsLinearVelocity() / 100).SizeSquared(); // UE4 is in cm
	FVector VelocityDirection = c->GetPhysicsLinearVelocity();
	VelocityDirection.Normalize();

	FVector AirResistance = -VelocityDirection * 0.5f * Ro * A * SquareVelocity;

	FVector Acceleration = FVector(0);
	Acceleration += Gravity;
	Acceleration += Levitation;
	Acceleration += AirResistance / Mass;
	Acceleration += TickSumForce / Mass;

	//UE_LOG(LogTemp, Warning, TEXT("03 - Gravity: %s"), *Gravity.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("03 - Levitation: %s"), *Levitation.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("03 - AirResistance: %s"), *AirResistance.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("03 - Acceleration: %s"), *Acceleration.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("04 - GetActorLocation: %s"), *GetActorLocation().ToString());
	//UE_LOG(LogTemp, Warning, TEXT("04 - GetTransform().GetTranslation(): %s"), *GetTransform().GetTranslation().ToString());




	//AddActorWorldRotation(AngularVelocity * deltaTime);
	//AddActorWorldOffset(LinearVelocity * deltaTime);


	//UE_LOG(LogTemp, Warning, TEXT("1 - GetPhysicsLinearVelocity: %s"), *c->GetPhysicsLinearVelocity().ToString());
	//UE_LOG(LogTemp, Warning, TEXT("2 - deltaTime: %f"), deltaTime);
	//UE_LOG(LogTemp, Warning, TEXT("3 - Acceleration: %s"), *Acceleration.ToString());
	///TEXT(c->GetPhysicsLinearVelocity()

	c->SetPhysicsLinearVelocity(Acceleration * deltaTime * 100, true); // Multiply by 100 because UE4 works in cm
	//c->SetPhysicsAngularVelocity(FVector());

	//UE_LOG(LogTemp, Warning, TEXT("4 - GetPhysicsLinearVelocity: %s"), *c->GetPhysicsLinearVelocity().ToString());

	// Rotation

	FVector InertiaTensor = FVector(100, 100, 100);
	// TODO FIX : no intertia component must be null
	//FVector WorldInertiaTensor = c->GetComponentToWorld().GetRotation().RotateVector(InertiaTensor);
	FVector WorldInertiaTensor = InertiaTensor;

	float SquareAngularVelocity = (c->GetPhysicsAngularVelocity()).SizeSquared(); // UE4 is in cm
	FVector AngularVelocityDirection = c->GetPhysicsAngularVelocity();
	AngularVelocityDirection.Normalize();

	FVector AngularAirResistance = -AngularVelocityDirection * 0.5f * Ro * SquareAngularVelocity;


	FVector AngularAcceleration = FVector(0);
	AngularAcceleration += TickSumTorque / WorldInertiaTensor;
	AngularAcceleration += AngularAirResistance / WorldInertiaTensor;


	/*UE_LOG(LogTemp, Warning, TEXT("2 - GetPhysicsAngularVelocity: %s"), *c->GetPhysicsAngularVelocity().ToString());
	UE_LOG(LogTemp, Warning, TEXT("3 - AngularAcceleration: %s"), *AngularAcceleration.ToString());
	UE_LOG(LogTemp, Warning, TEXT("3 - TickSumTorque: %s"), *TickSumTorque.ToString());
	UE_LOG(LogTemp, Warning, TEXT("3 - AngularAirResistance: %s"), *AngularAirResistance.ToString());
	UE_LOG(LogTemp, Warning, TEXT("3 - WorldInertiaTensor: %s"), *WorldInertiaTensor.ToString());
	*/

	c->SetPhysicsAngularVelocity(AngularAcceleration * deltaTime, true);


	// Reset force and torque for next tick
	TickSumForce = FVector::ZeroVector;
	TickSumTorque = FVector::ZeroVector;
}

static float computeSoftAngularCommand(float measure, float mMaxAngularSpeed, float mSoftModeAngularLimit, float command) {
	float result;
	if (measure - (command * mMaxAngularSpeed) < -mSoftModeAngularLimit)
	{
		result = -1;
	}
	else if (measure - (command * mMaxAngularSpeed) > mSoftModeAngularLimit)
	{
		result = 1;
	}
	else
	{
		result = (1.0f / mSoftModeAngularLimit) * (measure - (command * mMaxAngularSpeed));
	}
	return result;
}

void AIrr310Ship::AutoPilotSubTick(float deltaTime)
{

	
	// Basic auto pilot, stabilise to 10 m/s max front velocity
	float TargetSpeed = 10;

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	FVector WorldVelocity = GetLinearSpeed();
	FVector LocalVelocity = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(WorldVelocity);

	float ThrustCommand = 0;
	if (LocalVelocity.X < TargetSpeed) {
		ThrustCommand = 1;
	}

	TArray<UActorComponent*> Engines = GetComponentsByClass(UIrr310ShipEngine::StaticClass());
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);
		Engine->SetTargetThrustRatio(ThrustCommand);
	}

	FVector COM = c->GetBodyInstance()->GetCOMPosition();

	// Simple kill rot
	FVector AngularVelocity = c->GetPhysicsAngularVelocity();
	

	FVector AngularVelocityCommand = FVector(0, 0, 0);
	FVector aim = FVector(0, 0, 0);
	float MaxAngularSpeed = 180;
	float SoftModeAngularLimit = 90;

	aim.X = computeSoftAngularCommand(AngularVelocity.X, MaxAngularSpeed, SoftModeAngularLimit, AngularVelocityCommand.X);
	aim.Y = computeSoftAngularCommand(AngularVelocity.Y, MaxAngularSpeed, SoftModeAngularLimit, AngularVelocityCommand.Y);
	aim.Z = computeSoftAngularCommand(AngularVelocity.Z, MaxAngularSpeed, SoftModeAngularLimit, AngularVelocityCommand.Z);


	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);


		
		FVector EngineOffset = (Engine->GetComponentLocation() - COM) / 100;
		
		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThurstAxis);

		FVector TorqueDirection = FVector::CrossProduct(EngineOffset, WorldThurstAxis);
		TorqueDirection.Normalize();

		float alpha = 0;

		if (FMath::Abs(TorqueDirection.X) > 0.001 && FMath::Abs(aim.X) > 0.001)
		{
			alpha += (Engine->MaxThrust - Engine->MinThrust) * aim.X * (TorqueDirection.X > 0 ? 1 : -1);
		}

		if (FMath::Abs(TorqueDirection.Y) > 0.001 && FMath::Abs(aim.Y) > 0.001)
		{
			alpha += (Engine->MaxThrust - Engine->MinThrust) * aim.Y * (TorqueDirection.Y > 0 ? 1 : -1);
		}

		if (FMath::Abs(TorqueDirection.Z) > 0.001 && FMath::Abs(aim.Z) > 0.001)
		{
			alpha += (Engine->MaxThrust - Engine->MinThrust) * aim.Z * (TorqueDirection.Z > 0 ? 1 : -1);
		}

		Engine->TargetThrust = Engine->TargetThrust - alpha;
	}


}
