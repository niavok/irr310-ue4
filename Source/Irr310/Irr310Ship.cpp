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

FVector AIrr310Ship::GetLocalLinearSpeed()
{

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	FVector WorldVelocity = c->GetPhysicsLinearVelocity();


	FVector LocalVelocity = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(WorldVelocity);

	return LocalVelocity / 100;
}

float AIrr310Ship::GetAltitude() {
	return GetActorLocation().Z / 100;
}

// Estimators
static FVector getDeltaVToEnginesRatio(TArray<UActorComponent*>& Engines, float Mass, float ThrustRatio);
static FVector getDeltaVToEngineRatio(UIrr310ShipEngine* Engine, float Mass, float ThrustRatio);
static float getEnginesToRatioDuration(TArray<UActorComponent*>& Engines, float ThrustRatio);
static float getEngineToRatioDuration(UIrr310ShipEngine* Engines, float ThrustRatio);
static FVector getTotalMaxThrust(TArray<UActorComponent*>& Engines);
// Tools
static float computeSoftAngularCommand(float measure, float mMaxAngularSpeed, float mSoftModeAngularLimit, float command);
static FVector computeAngularAirResistance(FVector velocity, float Ro, float A);


void AIrr310Ship::PhysicSubTick(float deltaTime)
{

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	//UE_LOG(LogTemp, Warning, TEXT("01 - Acceleration: %s"), *Acceleration.ToString());

	// Gravity
	FVector Gravity = FVector(0, 0, -13.62); // 13.62 m/s�

	//UE_LOG(LogTemp, Warning, TEXT("02 - Acceleration: %s"), *Acceleration.ToString());

	// Levitation
	FVector Levitation = FVector(0, 0, 2.612622450e9 / FMath::Square(GetActorLocation().Z / 100 + 13850)); // At 13850 m gravity and levitation ar equal.


	// Air resistance
	float Ro = 1.6550; // Air density
	float A = 1 * 0.5;
	FVector AirResistance = computeAngularAirResistance(c->GetPhysicsLinearVelocity() / 100, Ro, A);

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

	FVector InertiaTensor = FVector(200, 200, 200);
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


void AIrr310Ship::AutoPilotSubTick(float deltaTime)
{

	//TODO take air resistance, gravity and levitation
	
	// Basic auto pilot, stabilise to 10 m/s max front velocity
	FVector LocalTargetSpeed = FVector(20,0,0);
	


	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	TArray<UActorComponent*> Engines = GetComponentsByClass(UIrr310ShipEngine::StaticClass());


	FVector WorldTargetSpeed = c->GetComponentToWorld().GetRotation().RotateVector(LocalTargetSpeed);

	FVector WorldVelocity = GetLinearSpeed();
	


	// Compute the equilibrium Thrust: Target Air Resistance = Total Thurst
	float Ro = 1.6550; // Air density
	float A = 1 * 0.5;
	FVector TargetAirResistance = computeAngularAirResistance(WorldTargetSpeed, Ro, A);
	FVector CurrentAirResistance = computeAngularAirResistance(WorldVelocity, Ro, A);
	
	FVector TotalMaxThrust = getTotalMaxThrust(Engines);
	float FinalThurstRatio = TargetAirResistance.Size() / TotalMaxThrust.Size();
		


	//float timeToMatch = getMatchThurstDuration(Engines, TargetAirResistance);
	

	// Compute time to get this trust
	FVector WorldVelocityToEnginesStop = GetLinearSpeed();
	
	
	FVector LocalVelocityToEnginesStop = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(WorldVelocityToEnginesStop);

	float ThrustCommand = 0;


	if (LocalVelocityToEnginesStop.X > LocalTargetSpeed.X + 0.5) {
		ThrustCommand = 0;
	} else if (LocalVelocityToEnginesStop.X > LocalTargetSpeed.X) {
		ThrustCommand = FinalThurstRatio;
	} else {
		WorldVelocityToEnginesStop += getDeltaVToEnginesRatio(Engines, Mass, FinalThurstRatio);
		// Check if air resistant won't make the estimation optimist.
		WorldVelocityToEnginesStop += getEnginesToRatioDuration(Engines, FinalThurstRatio) * CurrentAirResistance / Mass; // Assusme the air resistance will be almost constant during all the process. It's wrong, but it's better than noting

		LocalVelocityToEnginesStop = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(WorldVelocityToEnginesStop);




		if (LocalVelocityToEnginesStop.X < LocalTargetSpeed.X - 0.5) {
			ThrustCommand = 1;
		} else if (LocalVelocityToEnginesStop.X < LocalTargetSpeed.X) {
			ThrustCommand = (1 - 2 * (LocalTargetSpeed.X - LocalVelocityToEnginesStop.X)) * FinalThurstRatio + 2 * (LocalTargetSpeed.X - LocalVelocityToEnginesStop.X);
		} else {
			ThrustCommand = FinalThurstRatio;
		}
	}

	
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);
		Engine->SetTargetThrustRatio(ThrustCommand);
	}

	FVector COM = c->GetBodyInstance()->GetCOMPosition();

	// Simple kill rot
	FVector AngularVelocity = c->GetPhysicsAngularVelocity();
	

	FVector AngularVelocityCommand = FVector(0, 0, 0);
	FVector aim = FVector(0, 0, 0);
	float MaxAngularSpeed = 360;
	float SoftModeAngularLimit = 2;

	aim.X = computeSoftAngularCommand(AngularVelocity.X, MaxAngularSpeed, SoftModeAngularLimit, AngularVelocityCommand.X);
	aim.Y = computeSoftAngularCommand(AngularVelocity.Y, MaxAngularSpeed, SoftModeAngularLimit, AngularVelocityCommand.Y);
	aim.Z = computeSoftAngularCommand(AngularVelocity.Z, MaxAngularSpeed, SoftModeAngularLimit, AngularVelocityCommand.Z);


	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);


		
		FVector EngineOffset = (Engine->GetComponentLocation() - COM) / 100;
		
		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);

		FVector TorqueDirection = FVector::CrossProduct(EngineOffset, WorldThurstAxis);
		TorqueDirection.Normalize();

		float alpha = 0;

		if (FMath::Abs(TorqueDirection.X) > 0.1 && FMath::Abs(aim.X) > 0.001)
		{
			alpha += (Engine->MaxThrust - Engine->MinThrust) * aim.X * (TorqueDirection.X > 0 ? 1 : -1);
		}

		if (FMath::Abs(TorqueDirection.Y) > 0.1 && FMath::Abs(aim.Y) > 0.001)
		{
			alpha += (Engine->MaxThrust - Engine->MinThrust) * aim.Y * (TorqueDirection.Y > 0 ? 1 : -1);
		}

		if (FMath::Abs(TorqueDirection.Z) > 0.1 && FMath::Abs(aim.Z) > 0.001)
		{
			alpha += (Engine->MaxThrust - Engine->MinThrust) * aim.Z * (TorqueDirection.Z > 0 ? 1 : -1);
		}

		Engine->TargetThrust = Engine->TargetThrust - alpha;
	}




}





// Estimator 

// Estimate how must delta v the engine will produce if stopped now
FVector getDeltaVToEnginesRatio(TArray<UActorComponent*>& Engines, float Mass, float ThrustRatio) {

	FVector DeltaVToEnginesStop = FVector::ZeroVector;

	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		DeltaVToEnginesStop += getDeltaVToEngineRatio(Engine, Mass, ThrustRatio);
	}

	return DeltaVToEnginesStop;
}

FVector getDeltaVToEngineRatio(UIrr310ShipEngine* Engine, float Mass, float ThrustRatio) {
	
	// Linear integrale below a y = ax + b curve with a  = ThrustVariationSpeed and  b = CurrentThrust :
	// b^2 / 2a
	FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
	WorldThurstAxis.Normalize();
	float DeltaThurst = FMath::Abs(Engine->CurrentThrust - Engine->MaxThrust * ThrustRatio);
	return WorldThurstAxis * 0.5 * (DeltaThurst * DeltaThurst) / (Engine->ThrustVariationSpeed * Mass);
}

static float getEnginesToRatioDuration(TArray<UActorComponent*>& Engines, float ThrustRatio) {
	float TimeToStop = 0;

	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		TimeToStop = FMath::Max(TimeToStop, getEngineToRatioDuration(Engine, ThrustRatio));
	}

	return TimeToStop;
}

static float getEngineToRatioDuration(UIrr310ShipEngine* Engine, float ThrustRatio) {
	return FMath::Abs(Engine->CurrentThrust - Engine->MaxThrust * ThrustRatio) / Engine->ThrustVariationSpeed;
}

static FVector getTotalMaxThrust(TArray<UActorComponent*>& Engines) {
	FVector TotalMaxThrust = FVector::ZeroVector;
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		TotalMaxThrust += Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis) * Engine->MaxThrust;
	}

	return TotalMaxThrust;
}

/*static float getMatchThurstDuration(TArray<UActorComponent*>& Engines,FVector TargetAirResistance) {
	
	FVector MaxTotalThrust = FVector::ZeroVector;
	FVector CurrentTotalThrust = FVector::ZeroVector;
	
	// Find max total thurst
	float TimeToStop = 0;

	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		MaxTotalThrust += Engine->ThrustAxis * Engine->MaxThrust;
		MaxTotalThrust += Engine->ThrustAxis * Engine->CurrentThrust;
		TimeToStop = FMath::Max(TimeToStop, getEngineStopDuration(Engine));
	}

	// Find current total

	// find target thurst ratio

	//Find Max time to get this ratio

}*/

// Tools
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

static FVector computeAngularAirResistance(FVector Velocity, float Ro, float A) {
	float SquareVelocity = Velocity.SizeSquared(); // UE4 is in cm
	FVector VelocityDirection = Velocity;
	VelocityDirection.Normalize();

	return -VelocityDirection * 0.5f * Ro * A * SquareVelocity;
}

