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
	LocalLinearVelocityTarget = FVector::ZeroVector;
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
	
	//Configure camera
	if (InputComponent)
	{
		USpringArmComponent* InternalCamera = Cast<USpringArmComponent>(GetComponentByClass(USpringArmComponent::StaticClass()));
		FRotator HeadRotation = InternalCamera->RelativeRotation;
		HeadRotation.Pitch += InputComponent->GetAxisValue(LookUpBinding);
		HeadRotation.Yaw += InputComponent->GetAxisValue(LookRightBinding);
		InternalCamera->RelativeRotation = HeadRotation;
	}

	PhysicSubTick(deltaTime);

}

const FName AIrr310Ship::LookUpBinding("LookUp");
const FName AIrr310Ship::LookRightBinding("LookRight");

void AIrr310Ship::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAxis("PitchCommand", this, &AIrr310Ship::OnPichtCommand);
	InputComponent->BindAxis("YawCommand", this, &AIrr310Ship::OnYawCommand);
	InputComponent->BindAxis(LookUpBinding);
	InputComponent->BindAxis(LookRightBinding);

	InputComponent->BindAction("IncreaseLinearVelocity", IE_Pressed, this, &AIrr310Ship::OnIncreaseLinearVelocity);
	InputComponent->BindAction("DecreaseLinearVelocity", IE_Pressed, this, &AIrr310Ship::OnDecreaseLinearVelocity);
	InputComponent->BindAction("KillLinearVelocity", IE_Pressed, this, &AIrr310Ship::OnKillLinearVelocity);

	InputComponent->BindAction("RandomizeRotationSpeed", IE_Pressed, this, &AIrr310Ship::OnRandomizeRotationSpeed);

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

FVector AIrr310Ship::GetLocation() {
	return GetActorLocation() / 100;
}

// Estimators
static FVector getDeltaVToEnginesRatio(TArray<UActorComponent*>& Engines, float Mass, float ThrustRatio);
static FVector getDeltaVToEngineRatio(UIrr310ShipEngine* Engine, float Mass, float ThrustRatio);
static float getEnginesToRatioDuration(TArray<UActorComponent*>& Engines, float ThrustRatio);
static float getEngineToRatioDuration(UIrr310ShipEngine* Engines, float ThrustRatio);
static FVector getTotalMaxThrust(TArray<UActorComponent*>& Engines);
static FVector getTotalMaxLocalThrust(UPrimitiveComponent* root, TArray<UActorComponent*>& Engines);
static FVector getTotalMaxThrustInAxis(TArray<UActorComponent*>& Engines, FVector Axis);
// Tools
static float computeSoftAngularCommand(float measure, float mMaxAngularSpeed, float mSoftModeAngularLimit, float command);
static FVector computeAngularAirResistance(FVector velocity, float Ro, float A);


void AIrr310Ship::PhysicSubTick(float deltaTime)
{

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

	// Basic auto pilot

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	TArray<UActorComponent*> Engines = GetComponentsByClass(UIrr310ShipEngine::StaticClass());


	TArray<float*> EngineCommands;


	// Keep head up
	//EngineCommands.Add(ComputeAngularControl(Engines, FVector(0, 0, 1), FVector(0, 0, 1)));
	
	//EngineCommands.Add(ComputeAngularControl(Engines, FVector(1, 0, 0), FVector(1,0,0)));
	
	EngineCommands.Add(ComputePositionWithRotationControl(Engines, LocationTarget, LocalLinearVelocityTarget.Size()));

	
	//EngineCommands.Add(ComputeLinearVelocityStabilisation(Engines, LocalLinearVelocityTarget));
	//EngineCommands.Add(ComputeAngularVelocityStabilisation(Engines, LocalAngularVelocityTarget));


	FVector target = c->GetComponentToWorld().GetRotation().RotateVector(LocalLinearVelocityTarget);
	

	//EngineCommands.Add(ComputeLinearVelocityStabilisation(Engines, target));
	//EngineCommands.Add(ComputeAngularVelocityStabilisation(Engines, LocalAngularVelocityTarget));


	for (int32 EngineIndex = 0; EngineIndex < Engines.Num(); EngineIndex++) {
		float ThrustRatio = 0;
		for (int32 CommandIndex = 0; CommandIndex < EngineCommands.Num(); CommandIndex++) {
			ThrustRatio += EngineCommands[CommandIndex][EngineIndex];
		}
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[EngineIndex]);
		Engine->SetTargetThrustRatio(ThrustRatio);
	}

	//Clear
	for (int32 CommandIndex = 0; CommandIndex < EngineCommands.Num(); CommandIndex++) {
		delete[] EngineCommands[CommandIndex];
	}

}

float* AIrr310Ship::ComputeLinearVelocityStabilisation(TArray<UActorComponent*>& Engines, FVector WorldTargetSpeed)
{

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	//FVector WorldTargetSpeed = c->GetComponentToWorld().GetRotation().RotateVector(LocalTargetSpeed);
	FVector WorldVelocity = GetLinearSpeed();
	FVector WorldTargetSpeedAxis = WorldTargetSpeed;
	WorldTargetSpeedAxis.Normalize();
	FVector DeltaVelocity = WorldTargetSpeed - WorldVelocity;
	FVector DeltaVelocityAxis = DeltaVelocity;
	DeltaVelocityAxis.Normalize();

	// Compute the equilibrium Thrust: Target Air Resistance = Total Thurst
	float Ro = 1.6550; // Air density
	float A = 1 * 0.5;
	FVector TargetAirResistance = computeAngularAirResistance(WorldTargetSpeed, Ro, A);
	FVector CurrentAirResistance = computeAngularAirResistance(WorldVelocity, Ro, A);

	FVector TotalMaxThrust = getTotalMaxThrustInAxis(Engines, WorldTargetSpeedAxis);
	float FinalThurstRatio = TargetAirResistance.Size() / TotalMaxThrust.Size();



	//float timeToMatch = getMatchThurstDuration(Engines, TargetAirResistance);


	// Compute time to get this trust
	//FVector CurrentVelocity = GetLinearSpeed();


	//FVector LocalVelocityToEnginesStop = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(WorldVelocityToEnginesStop);


	FVector WorldVelocityToEnginesStop = WorldVelocity;
	WorldVelocityToEnginesStop += getDeltaVToEnginesRatio(Engines, Mass, FinalThurstRatio);
	// Check if air resistant won't make the estimation optimist.
	WorldVelocityToEnginesStop += getEnginesToRatioDuration(Engines, FinalThurstRatio) * CurrentAirResistance / Mass; // Assusme the air resistance will be almost constant during all the process. It's wrong, but it's better than noting



	
	FVector DeltaVelocityToStop = WorldTargetSpeed - WorldVelocityToEnginesStop;
	FVector DeltaVelocityToStopAxis = DeltaVelocityToStop;
	DeltaVelocityToStopAxis.Normalize();

	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
		float dot = FVector::DotProduct(WorldThurstAxis, WorldTargetSpeedAxis);
		float DeltaSpeedInThurstAxis = FVector::DotProduct(WorldThurstAxis, DeltaVelocity);
		float ThrustCommand = 0;
		

		if (DeltaSpeedInThurstAxis < -0.5) {
			//If a engine contribute to the axis, and speed in his thust direction  is too hight : stop it
			ThrustCommand = 0;
			// TODO reverse thrust if possible
		}
		else if (DeltaSpeedInThurstAxis < 0) {
			ThrustCommand = FinalThurstRatio * dot;
		}
		else {
			float StopDot = FVector::DotProduct(WorldThurstAxis, DeltaVelocityToStop);
			float DeltaStopSpeedInThurstAxis = FVector::DotProduct(WorldThurstAxis, DeltaVelocityToStop);

			if (DeltaStopSpeedInThurstAxis > 1) {
				ThrustCommand = 1;
			}
			else if (DeltaStopSpeedInThurstAxis > 0) {
				ThrustCommand = (1 - DeltaStopSpeedInThurstAxis) * FinalThurstRatio + DeltaStopSpeedInThurstAxis;
			}
			else {
				ThrustCommand = FinalThurstRatio;
			}
		}
		command[i] = ThrustCommand;
	}


	/*

	float ThrustCommand = 0;


	if (LocalVelocityToEnginesStop > LocalTargetSpeed.X + 0.5) {
		ThrustCommand = 0;
	}
	else if (LocalVelocityToEnginesStop.X > LocalTargetSpeed.X) {
		ThrustCommand = FinalThurstRatio;
	}
	else {
		WorldVelocityToEnginesStop += getDeltaVToEnginesRatio(Engines, Mass, FinalThurstRatio);
		// Check if air resistant won't make the estimation optimist.
		WorldVelocityToEnginesStop += getEnginesToRatioDuration(Engines, FinalThurstRatio) * CurrentAirResistance / Mass; // Assusme the air resistance will be almost constant during all the process. It's wrong, but it's better than noting

		LocalVelocityToEnginesStop = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(WorldVelocityToEnginesStop);

		// TODO use also gravity and levitation


		if (LocalVelocityToEnginesStop.X < LocalTargetSpeed.X - 0.5) {
			ThrustCommand = 1;
		}
		else if (LocalVelocityToEnginesStop.X < LocalTargetSpeed.X) {
			ThrustCommand = (1 - 2 * (LocalTargetSpeed.X - LocalVelocityToEnginesStop.X)) * FinalThurstRatio + 2 * (LocalTargetSpeed.X - LocalVelocityToEnginesStop.X);
		}
		else {
			ThrustCommand = FinalThurstRatio;
		}
	}

	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);
		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
		float dot = FVector::DotProduct(WorldThurstAxis, WorldTargetSpeed);
		//Wrong, make per component 
		command[i] = ThrustCommand * dot;
	}*/
	return command;
}


float* AIrr310Ship::ComputeLinearVelocityStabilisationWithRotation(TArray<UActorComponent*>& Engines, FVector WorldTargetSpeed) {

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	FVector DeltatSpeed = WorldTargetSpeed - GetLinearSpeed();

	FVector DeltaSpeedDirection = DeltatSpeed;
	DeltaSpeedDirection.Normalize();

	FVector LocalMaxThrustDirection = getTotalMaxLocalThrust(c, Engines);
	LocalMaxThrustDirection.Normalize();

	FVector MaxThrustDirection = getTotalMaxThrust(Engines);
	MaxThrustDirection.Normalize();

	float dot = FVector::DotProduct(DeltaSpeedDirection, MaxThrustDirection);
	if (dot < 0.99) {
		dot = 9;
	}
	else {
		dot = (dot - 0.99) * 100;
	}

	/*
	dot = FMath::Clamp(dot, 0.0f, 1.0f);
	dot = FMath::Square(dot);*/
	
	

	float* angularCommand;
	float* positionCommand;
	if (DeltatSpeed.Size() > 5) {
		angularCommand = ComputeAngularControl(Engines, LocalMaxThrustDirection, DeltaSpeedDirection);
		positionCommand = ComputeLinearVelocityStabilisation(Engines, WorldTargetSpeed * dot);
	} else {
		angularCommand = new float[Engines.Num()];
		for (int32 i = 0; i < Engines.Num(); i++) {
			angularCommand[i] = 0;
		}
		positionCommand = ComputeLinearVelocityStabilisation(Engines, WorldTargetSpeed);
	}
	

	// Merge
	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		command[i] = angularCommand[i] + positionCommand[i];
	}

	delete[] angularCommand;
	delete[] positionCommand;

	return command;




}


float* AIrr310Ship::ComputeAngularVelocityStabilisation(TArray<UActorComponent*>& Engines, FVector LocalTargetSpeed)
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector COM = c->GetBodyInstance()->GetCOMPosition();

	// Simple kill rot
	FVector AngularVelocity = c->GetPhysicsAngularVelocity();
	

	FVector AngularVelocityCommand = c->GetComponentToWorld().GetRotation().RotateVector(LocalTargetSpeed);

	FVector DeltaVelocity = AngularVelocityCommand - AngularVelocity;
	FVector DeltaVelocityDirection = DeltaVelocity;
	DeltaVelocityDirection.Normalize();
	FVector aim = FVector(0, 0, 0);
	float MaxAngularSpeed = 360;
	float SoftModeAngularLimit = 90;

	aim.X = computeSoftAngularCommand(AngularVelocity.X, MaxAngularSpeed, SoftModeAngularLimit, AngularVelocityCommand.X);
	aim.Y = computeSoftAngularCommand(AngularVelocity.Y, MaxAngularSpeed, SoftModeAngularLimit, AngularVelocityCommand.Y);
	aim.Z = computeSoftAngularCommand(AngularVelocity.Z, MaxAngularSpeed, SoftModeAngularLimit, AngularVelocityCommand.Z);

	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);


		
		FVector EngineOffset = (Engine->GetComponentLocation() - COM) / 100;
		
		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);

		FVector TorqueDirection = FVector::CrossProduct(EngineOffset, WorldThurstAxis);
		TorqueDirection.Normalize();

		float alpha = 0;
		float ramp = 1;
		if (DeltaVelocity.SizeSquared() < 1) {
			ramp = DeltaVelocity.SizeSquared();
		}

		alpha = ramp * FVector::DotProduct(TorqueDirection, DeltaVelocityDirection);

		/*if (FMath::Abs(TorqueDirection.X) > 0.1 && FMath::Abs(aim.X) > 0.001)
		{
			alpha += - aim.X * (TorqueDirection.X > 0 ? 1 : -1);
		}

		if (FMath::Abs(TorqueDirection.Y) > 0.1 && FMath::Abs(aim.Y) > 0.001)
		{
			alpha += - aim.Y * (TorqueDirection.Y > 0 ? 1 : -1);
		}

		if (FMath::Abs(TorqueDirection.Z) > 0.1 && FMath::Abs(aim.Z) > 0.001)
		{
			alpha += - aim.Z * (TorqueDirection.Z > 0 ? 1 : -1);
		}*/

		command[i] = alpha;
	}
	return command;
}

/**
 * Align LocalShipAxis to TargetAxis
 */
float* AIrr310Ship::ComputeAngularControl(TArray<UActorComponent*>& Engines, FVector LocalShipAxis, FVector TargetAxis)
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector WorldShipAxis = c->GetComponentToWorld().GetRotation().RotateVector(LocalShipAxis);

	WorldShipAxis.Normalize();
	TargetAxis.Normalize();

	float dot = FVector::DotProduct(WorldShipAxis, TargetAxis);



	FVector RotationDirection = FVector::CrossProduct(WorldShipAxis, TargetAxis);

	if (dot < 0) {
		//Invert and maximize
		if (RotationDirection.Size() == 0) {
			RotationDirection = FVector(1, 1, 1) - TargetAxis;
			
		}
		//RotationDirection *= -1;
		RotationDirection.Normalize();
	}

	/*
	// Soften the end only
	RotationDirection.X = FMath::Pow(RotationDirection.X,3);
	RotationDirection.Y = FMath::Pow(RotationDirection.Y, 3);
	RotationDirection.Z = FMath::Pow(RotationDirection.Z, 3);
	*/

	RotationDirection *= 30;

	RotationDirection.ClampMaxSize(20);

	
	//TODO Improve


	// Check what is the faster angle is 

	// TODO Evaluate if and intermediate rotation help

	// TODO pass ComputeAngularVelocityStabilisation in world space
	FVector WorldRotationDirection = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(RotationDirection);

	return ComputeAngularVelocityStabilisation(Engines, WorldRotationDirection);

}

float* AIrr310Ship::ComputePositionWithoutRotationControl(TArray<UActorComponent*>& Engines, FVector TargetLocation, float speed) {
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector DeltaPosition = TargetLocation - c->GetComponentLocation() / 100;
	FVector Speed = DeltaPosition;
	Speed.Normalize();
	//TODO find max speed
	
	


	Speed *= speed;

	return ComputeLinearVelocityStabilisation(Engines, Speed);
}

float* AIrr310Ship::ComputePositionWithRotationControl(TArray<UActorComponent*>& Engines, FVector TargetLocation, float speed) {
	
	/*UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector TargetDirection = TargetLocation - c->GetComponentLocation();
	TargetDirection.Normalize();

	FVector LocalMaxThrustDirection = getTotalMaxLocalThrust(c, Engines);
	LocalMaxThrustDirection.Normalize();

	FVector MaxThrustDirection = getTotalMaxThrust(Engines);
	MaxThrustDirection.Normalize();

	float dot = FVector::DotProduct(TargetDirection, MaxThrustDirection);
	dot = FMath::Clamp(dot, 0.0f, 1.0f);
	dot = FMath::Square(dot);


	float* angularCommand = ComputeAngularControl(Engines, LocalMaxThrustDirection, TargetDirection);
	float* positionCommand = ComputePositionWithoutRotationControl(Engines, TargetLocation, speed * dot);
	
	// Merge
	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		command[i] = angularCommand[i] + positionCommand[i];
	}

	delete[] angularCommand;
	delete[] positionCommand;

	return command;*/

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector DeltaPosition = TargetLocation - c->GetComponentLocation() / 100;
	FVector Speed = DeltaPosition;
	Speed.Normalize();
	//TODO find max speed




	Speed *= speed;

	return ComputeLinearVelocityStabilisationWithRotation(Engines, Speed);

	

}



// Bindings
void AIrr310Ship::OnIncreaseLinearVelocity()
{
	LocalLinearVelocityTarget.X += 5;
}
void AIrr310Ship::OnDecreaseLinearVelocity()
{
	LocalLinearVelocityTarget.X -= 5;
}

void AIrr310Ship::OnKillLinearVelocity()
{
	LocalLinearVelocityTarget.X = 0;
}

void AIrr310Ship::OnPichtCommand(float Val)
{
	LocalAngularVelocityTarget.Y = Val * 30;
}

void AIrr310Ship::OnYawCommand(float Val)
{
	LocalAngularVelocityTarget.Z = - Val * 30;
}


void AIrr310Ship::OnRandomizeRotationSpeed()
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	FVector randomAngularVelocity = FVector(FMath::FRandRange(-360, 360), FMath::FRandRange(-360, 360), FMath::FRandRange(-360, 360));
	c->SetPhysicsAngularVelocity(randomAngularVelocity, false);
}

FVector AIrr310Ship::getLocalAngularVelocity()
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	return c->GetComponentToWorld().GetRotation().Inverse().RotateVector(c->GetPhysicsAngularVelocity());
}

FVector AIrr310Ship::getWorldAngularVelocity()
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	return c->GetPhysicsAngularVelocity();
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

static FVector getTotalMaxLocalThrust(UPrimitiveComponent* root, TArray<UActorComponent*>& Engines) {
	return  root->GetComponentToWorld().GetRotation().Inverse().RotateVector(getTotalMaxThrust(Engines));
}

static FVector getTotalMaxThrustInAxis(TArray<UActorComponent*>& Engines, FVector Axis)
{
	Axis.Normalize();
	FVector TotalMaxThrust = FVector::ZeroVector;
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);

		float dot = FVector::DotProduct(WorldThurstAxis, Axis);
		if (dot > 0) {
			TotalMaxThrust += Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis) * Engine->MaxThrust * dot;
		}
		else if (dot < 0) {
			TotalMaxThrust -= Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis) * Engine->MinThrust * dot;
		}
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

