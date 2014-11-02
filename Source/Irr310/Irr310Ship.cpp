// Fill out your copyright notice in the Description page of Project Settings.

#include "Irr310.h"
#include "Irr310Ship.h"
#include "Irr310ShipModule.h"
#include "Irr310ShipEngine.h"

AIrr310Ship::AIrr310Ship(const class FPostConstructInitializeProperties& PCIP):
Super(PCIP),
FlightRecorder(this)
{
	UE_LOG(LogTemp, Warning, TEXT("new AIrr310Ship"));
	Mass = 500;
	InertiaTensor = 100;
	LocalLinearVelocityTarget = FVector::ZeroVector;
	TickSumForce = FVector::ZeroVector;
	TickSumTorque = FVector::ZeroVector;
}

void AIrr310Ship::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	AutoPilotSubTick(DeltaSeconds);
	

	FlightRecorder.WriteState(DeltaSeconds);

	// Tick Modules
	TArray<UActorComponent*> Modules = GetComponentsByClass(UIrr310ShipModule::StaticClass());
	for (int32 i = 0; i < Modules.Num(); i++) {
		UIrr310ShipModule* Module = Cast<UIrr310ShipModule>(Modules[i]);
		Module->TickModule(this, DeltaSeconds);
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

	PhysicSubTick(DeltaSeconds);

	
}

const FName AIrr310Ship::LookUpBinding("LookUp");
const FName AIrr310Ship::LookRightBinding("LookRight");

void AIrr310Ship::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAxis("PitchCommand", this, &AIrr310Ship::OnPichtCommand);
	InputComponent->BindAxis("YawCommand", this, &AIrr310Ship::OnYawCommand);
	InputComponent->BindAxis("RollCommand", this, &AIrr310Ship::OnRollCommand);
	InputComponent->BindAxis("ThrustCommand", this, &AIrr310Ship::OnThrustCommand);

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
	FVector COM = c->GetComponentToWorld().TransformPosition(LocalCOM);

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
FVector getDeltaAngularVelocityToEnginesRatio(TArray<UActorComponent*>& Engines, FVector COM, float Intertia, float ThrustRatio);
FVector getAngularDeltaVToEngineRatio(UIrr310ShipEngine* Engine, FVector COM, float Intertia, float ThrustRatio);
static float getEnginesToRatioDuration(TArray<UActorComponent*>& Engines, float ThrustRatio);
static float getEngineToRatioDuration(UIrr310ShipEngine* Engines, float ThrustRatio);
static FVector getTotalMaxThrust(TArray<UActorComponent*>& Engines);
static FVector getTotalMaxLocalThrust(UPrimitiveComponent* root, TArray<UActorComponent*>& Engines);
static FVector getTotalMaxThrustInAxis(TArray<UActorComponent*>& Engines, FVector Axis, float ThurstAngleLimit);
// Tools
static float computeSoftAngularCommand(float measure, float mMaxAngularSpeed, float mSoftModeAngularLimit, float command);
static FVector computeAngularAirResistance(FVector velocity, float Ro, float A);
static FVector computeGravity();
static FVector computeLevitation(FVector location);


void AIrr310Ship::PhysicSubTick(float deltaTime)
{

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	//UE_LOG(LogTemp, Warning, TEXT("01 - Acceleration: %s"), *Acceleration.ToString());

	// Gravity
	FVector Gravity = computeGravity();

	//UE_LOG(LogTemp, Warning, TEXT("02 - Acceleration: %s"), *Acceleration.ToString());

	// Levitation
	FVector Levitation = computeLevitation(GetActorLocation() / 100); // At 13850 m gravity and levitation ar equal.


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

	//FVector InertiaTensor = FVector(100, 100, 100);
	// TODO FIX : no intertia component must be null
	//FVector WorldInertiaTensor = c->GetComponentToWorld().GetRotation().RotateVector(InertiaTensor);
	float WorldInertiaTensor = InertiaTensor;

	float SquareAngularVelocity = (c->GetPhysicsAngularVelocity()).SizeSquared(); // UE4 is in cm
	FVector AngularVelocityDirection = c->GetPhysicsAngularVelocity();
	AngularVelocityDirection.Normalize();

	FVector AngularAirResistance = -AngularVelocityDirection * 0.1f * Ro * SquareAngularVelocity;


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


	if (ControlMode == 0) {
		// Manual control
		FVector linearTarget = c->GetComponentToWorld().GetRotation().RotateVector(LocalLinearVelocityTarget);
		FVector angularTarget = c->GetComponentToWorld().GetRotation().RotateVector(LocalAngularVelocityTarget);
		EngineCommands.Add(ComputeLinearVelocityStabilisation(Engines, linearTarget, 0.5));
		//EngineCommands.Add(ComputeAngularVelocityStabilisation(Engines, angularTarget));
		// Keep head up
		//EngineCommands.Add(ComputeAngularControl(Engines, FVector(0, 0, 1), FVector(0, 0, 1)));
		//EngineCommands.Add(ComputeAngularControl(Engines, FVector(1, 0, 0), FVector(1, 0, 0)));
	} else {
	

		// Keep head up
		EngineCommands.Add(ComputeAngularControl(Engines, FVector(0, 0, 1), FVector(0, 0, 1)));
	
		//EngineCommands.Add(ComputeAngularControl(Engines, FVector(1, 0, 0), FVector(1,0,0)));
	
		// Go to target
		EngineCommands.Add(ComputePositionWithRotationControl(Engines, LocationTarget, LocalLinearVelocityTarget.Size()));

	
		//EngineCommands.Add(ComputeLinearVelocityStabilisation(Engines, FVector(40,-5,0.5), 0));

		//EngineCommands.Add(ComputeLinearVelocityStabilisation(Engines, LocalLinearVelocityTarget));
	
	
		//EngineCommands.Add(ComputeAngularVelocityStabilisation(Engines, LocalAngularVelocityTarget));


		
	}

	


	for (int32 EngineIndex = 0; EngineIndex < Engines.Num(); EngineIndex++) {
		float ThrustRatio = 0;
		for (int32 CommandIndex = 0; CommandIndex < EngineCommands.Num(); CommandIndex++) {
			ThrustRatio = ThrustRatio + EngineCommands[CommandIndex][EngineIndex];
			//ThrustRatio = FMath::Clamp(ThrustRatio, 1.f, 1.f);
		}
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[EngineIndex]);
		Engine->SetTargetThrustRatio(ThrustRatio);
	}

	//Clear
	for (int32 CommandIndex = 0; CommandIndex < EngineCommands.Num(); CommandIndex++) {
		delete[] EngineCommands[CommandIndex];
	}

}

float* AIrr310Ship::ComputeLinearVelocityStabilisation(TArray<UActorComponent*>& Engines, FVector WorldTargetSpeed, float ThrustAngleLimit)
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
	FVector GravityForce = computeGravity() * Mass;
	FVector LevitationForce = computeLevitation(GetActorLocation() / 100) * Mass;
	FVector CurrentAirResistance = computeAngularAirResistance(WorldVelocity, Ro, A);


	// Compute final thrust
	// Its the thrust to keep the WorldTargetSpeed constant if the ship is at WorldTargetSpeed
	// The final thrust is the negative off all forces : air resistance, gravity and levitation
	FVector FinalThurst = -(TargetAirResistance + GravityForce + LevitationForce);
	FVector FinalThurstAxis = FinalThurst;
	FinalThurstAxis.Normalize();

	//ThurstRatio at final velocity
	FVector FinalThurstRatioVector;

	
	if (FMath::IsNearlyZero(FinalThurst.Size())) {
		// No need to thrust compensation
		FinalThurstRatioVector = FVector::ZeroVector;
	} else {
		// Find total thurst in limit in the axis of FinalThrust
		FVector TotalMaxThrust = getTotalMaxThrustInAxis(Engines, FinalThurst, ThrustAngleLimit);
		if (TotalMaxThrust.X == 0) {
			TotalMaxThrust.X = 1;
		}
		if (TotalMaxThrust.Y == 0) {
			TotalMaxThrust.Y = 1;
		}
		if (TotalMaxThrust.Z == 0) {
			TotalMaxThrust.Z = 1;
		}
		FinalThurstRatioVector = FinalThurst / TotalMaxThrust * TotalMaxThrust.GetSignVector();
	}

	


	//float timeToMatch = getMatchThurstDuration(Engines, TargetAirResistance);


	// Compute time to get this trust
	//FVector CurrentVelocity = GetLinearSpeed();


	//FVector LocalVelocityToEnginesStop = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(WorldVelocityToEnginesStop);


	
	


	
	
	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
		float dot = FVector::DotProduct(WorldThurstAxis, WorldTargetSpeedAxis);
		if (dot < ThrustAngleLimit) {
			dot = 0;
		}
		else {
			dot = (dot - ThrustAngleLimit) / (1 - ThrustAngleLimit);
		}

		
		float FinalThurstDot = FVector::DotProduct(WorldThurstAxis, FinalThurstAxis);
		if (FinalThurstDot < ThrustAngleLimit) {
			FinalThurstDot = 0;
		}
		else {
			FinalThurstDot = (FinalThurstDot - ThrustAngleLimit) / (1 - ThrustAngleLimit);
		}

		float FinalThurstRatio = FVector::DotProduct(WorldThurstAxis, FinalThurstRatioVector);
		


		//TODO Thrust angle dot

		float DeltaSpeedInThurstAxis = FVector::DotProduct(WorldThurstAxis, DeltaVelocity);
		float ThrustCommand = 0;
		

		if (DeltaSpeedInThurstAxis < -0.5) {
			//If a engine contribute to the axis, and speed in his thust direction  is too hight : stop it
			ThrustCommand = 0;
			// TODO reverse thrust if possible
		}
		else if (DeltaSpeedInThurstAxis < 0) {
			
			ThrustCommand = FinalThurstRatio * FinalThurstDot * DeltaSpeedInThurstAxis;
		}
		else {

			// Compute delta to stop
			FVector WorldVelocityToEnginesStop = WorldVelocity;
			WorldVelocityToEnginesStop += getDeltaVToEnginesRatio(Engines, Mass, FinalThurstRatio * FinalThurstDot);
			// Check if air resistant won't make the estimation optimist.
			//WorldVelocityToEnginesStop += getEnginesToRatioDuration(Engines, FinalThurstRatio * FinalThurstDot) * CurrentAirResistance / Mass; // Assusme the air resistance will be almost constant during all the process. It's wrong, but it's better than noting


			FVector DeltaVelocityToStop = WorldTargetSpeed - WorldVelocityToEnginesStop;
			FVector DeltaVelocityToStopAxis = DeltaVelocityToStop;
			DeltaVelocityToStopAxis.Normalize();


			float StopDot = FVector::DotProduct(WorldThurstAxis, DeltaVelocityToStop);
			float DeltaStopSpeedInThurstAxis = FVector::DotProduct(WorldThurstAxis, DeltaVelocityToStop);

			if (DeltaStopSpeedInThurstAxis > 0.1) {
				ThrustCommand = 1;
			}
			else if (DeltaStopSpeedInThurstAxis > 0.001) {
				ThrustCommand = FinalThurstRatio * FinalThurstDot + DeltaStopSpeedInThurstAxis * 10;
			}
			else {
				ThrustCommand = FinalThurstRatio * FinalThurstDot;
			}
		}
		if (FMath::IsNearlyZero(ThrustCommand)) {
			ThrustCommand = 0;
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


	float dot = 1;
	

	/*
	dot = FMath::Clamp(dot, 0.0f, 1.0f);
	dot = FMath::Square(dot);*/
	
	

	float* angularCommand;
	float* positionCommand;

	float rotationNeed = FMath::Clamp(DeltatSpeed.Size() / 2, 0.01f, 1.0f);
	//UE_LOG(LogTemp, Warning, TEXT("3 - rotationNeed: %f"), rotationNeed);
	//if (DeltatSpeed.Size() > 0) {
		angularCommand = ComputeAngularControl(Engines, LocalMaxThrustDirection, DeltaSpeedDirection);
		positionCommand = ComputeLinearVelocityStabilisation(Engines, WorldTargetSpeed, 0.0);

		dot = FVector::DotProduct(DeltaSpeedDirection, MaxThrustDirection);
		//dot = FMath::Clamp(dot, 0.0f, 1.0f);
		if (dot < 0.9) {
			dot = 0;
		}
		else {
			dot = (dot - 0.9) * 10;
		}
		//dot = 1;

	/*} else {
		angularCommand = new float[Engines.Num()];
		for (int32 i = 0; i < Engines.Num(); i++) {
			angularCommand[i] = 0;
		}
		positionCommand = ComputeLinearVelocityStabilisation(Engines, WorldTargetSpeed, 0.5);
	}*/
	
		rotationNeed = FMath::Min(rotationNeed, 1- dot);
			 


	// Merge
	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		command[i] = angularCommand[i] * rotationNeed + positionCommand[i] * (0.01 + 1 - rotationNeed);
	}

	delete[] angularCommand;
	delete[] positionCommand;

	return command;




}


float* AIrr310Ship::ComputeAngularVelocityStabilisation(TArray<UActorComponent*>& Engines, FVector WorldTargetSpeed)
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector COM = c->GetComponentToWorld().TransformPosition(LocalCOM);

	// Simple kill rot
	FVector AngularVelocity = c->GetPhysicsAngularVelocity();
	

	//FVector AngularVelocityCommand = c->GetComponentToWorld().GetRotation().RotateVector(LocalTargetSpeed);

	FVector DeltaVelocity = WorldTargetSpeed - AngularVelocity;
	FVector DeltaVelocityDirection = DeltaVelocity;
	DeltaVelocityDirection.Normalize();
	FVector aim = FVector(0, 0, 0);
	float MaxAngularSpeed = 360;
	float SoftModeAngularLimit = 90;


	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);
	
		float ThrustCommand = 0;
		FVector EngineOffset = (Engine->GetComponentLocation() - COM) / 100;

		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
		FVector TorqueDirection = FVector::CrossProduct(EngineOffset, WorldThurstAxis);

		float DeltaSpeedInTorqueAxis;

		if (TorqueDirection.Size() < 0.001) {
			DeltaSpeedInTorqueAxis = 0;
		}
		else {

			TorqueDirection.Normalize();

			DeltaSpeedInTorqueAxis = FVector::DotProduct(TorqueDirection, DeltaVelocity);
		}

		float FinalThurstRatio = 0; //TODO
		float FinalThurstDot = 0; //TODO

		if (DeltaSpeedInTorqueAxis < -0.5) {
			//If a engine contribute to the axis, and speed in his thust direction  is too hight : stop it
			ThrustCommand = 0;
			// TODO reverse thrust if possible
		}
		else if (DeltaSpeedInTorqueAxis < 0) {
			ThrustCommand = FinalThurstRatio * FinalThurstDot * DeltaSpeedInTorqueAxis;
		}
		else {

			// Compute delta to stop
			FVector WorldVelocityToEnginesStop = AngularVelocity;
			WorldVelocityToEnginesStop += getDeltaAngularVelocityToEnginesRatio(Engines, COM, InertiaTensor, FinalThurstRatio * FinalThurstDot); // TODO inertia
			// Check if air resistant won't make the estimation optimist.
			//WorldVelocityToEnginesStop += getEnginesToRatioDuration(Engines, FinalThurstRatio * FinalThurstDot) * CurrentAirResistance / Mass; // Assusme the air resistance will be almost constant during all the process. It's wrong, but it's better than noting


			FVector DeltaVelocityToStop = WorldTargetSpeed - WorldVelocityToEnginesStop;
			FVector DeltaVelocityToStopAxis = DeltaVelocityToStop;
			DeltaVelocityToStopAxis.Normalize();


			float StopDot = FVector::DotProduct(TorqueDirection, DeltaVelocityToStop);
			float DeltaStopSpeedInThurstAxis = FVector::DotProduct(TorqueDirection, DeltaVelocityToStop);

			if (DeltaStopSpeedInThurstAxis > 0.1) {
				ThrustCommand = 1;
			}
			else if (DeltaStopSpeedInThurstAxis > 0.001) {
				ThrustCommand = FinalThurstRatio * FinalThurstDot + DeltaStopSpeedInThurstAxis * 10;
			}
			else {
				ThrustCommand = FinalThurstRatio * FinalThurstDot;
			}
		}
		if (FMath::IsNearlyZero(ThrustCommand)) {
			ThrustCommand = 0;
		}
		command[i] = ThrustCommand;
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


	// Accelerate as fast as possible and decelerate when the integrale of the remaining angular movement if breaking now is higther than the target
	FVector RotationDirection = FVector::CrossProduct(WorldShipAxis, TargetAxis);


	float dot = FVector::DotProduct(WorldShipAxis, TargetAxis);
	float angle = FMath::RadiansToDegrees(FMath::Acos(dot));
	
	//Angle vary with rotation direction and negative dot






	

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

	RotationDirection *= 180;

	RotationDirection.ClampMaxSize(360);

	
	//TODO Improve


	// Check what is the faster angle is 

	// TODO Evaluate if and intermediate rotation help

	// TODO pass ComputeAngularVelocityStabilisation in world space
	//FVector WorldRotationDirection = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(RotationDirection);


	float* angularCommand = ComputeAngularVelocityStabilisation(Engines, RotationDirection);
	// Soften
	float* command = new float[Engines.Num()];

	float soft = 1;
	if (dot > 0.90) {
		soft = 1 - (dot - 0.90) * 10;
	}

	for (int32 i = 0; i < Engines.Num(); i++) {
		command[i] = soft * angularCommand[i];
	}

	delete[] angularCommand;
	
	return command;
}

float* AIrr310Ship::ComputePositionWithoutRotationControl(TArray<UActorComponent*>& Engines, FVector TargetLocation, float speed) {
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector DeltaPosition = TargetLocation - c->GetComponentLocation() / 100;
	FVector Speed = DeltaPosition;
	Speed.Normalize();
	//TODO find max speed
	
	


	Speed *= speed;

	return ComputeLinearVelocityStabilisation(Engines, Speed, 0);
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


	FVector CurrentSpeedDirection = GetLinearSpeed();
	CurrentSpeedDirection.Normalize();

	//Bad speed components
	float SpeedConcordance = FMath::Clamp(FVector::DotProduct(Speed, CurrentSpeedDirection), 0.1f, 1.0f);
	

	FVector CorrectSpeed = Speed * FMath::Clamp(FVector::DotProduct(GetLinearSpeed(), Speed), 0.f, 1.f);
	FVector ParasiteSpeed = GetLinearSpeed() - CorrectSpeed;

	FVector TargetSpeed = CorrectSpeed;


	FVector DeltatSpeed = Speed - GetLinearSpeed();

	if (ParasiteSpeed.Size() < 5) {
		TargetSpeed = CorrectSpeed * ParasiteSpeed.Size()/5 + Speed * (5 - ParasiteSpeed.Size()) /5 * speed;
	}

	//UE_LOG(LogTemp, Warning, TEXT("3 - CorrectSpeed: %s"), *CorrectSpeed.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("3 - ParasiteSpeed: %s"), *ParasiteSpeed.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("3 - TargetSpeed: %s"), *TargetSpeed.ToString());


	//FVector ParasiteSpeed = DeltatSpeed.ProjectOnTo(Speed);

//	ParasiteSpeed + 


	//FVector::ProjectOnTo


	//TODO find max speed




	//Speed *= speed * FMath::Pow(SpeedConcordance, 3);
	//Speed *= speed;
	TargetSpeed = Speed * speed;
	return ComputeLinearVelocityStabilisationWithRotation(Engines, TargetSpeed);

	

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

void AIrr310Ship::OnRollCommand(float Val)
{
	LocalAngularVelocityTarget.X = -Val * 30;
}

void AIrr310Ship::OnThrustCommand(float Val)
{
	//LocalLinearVelocityTarget.X = Val * 100;
}


void AIrr310Ship::OnRandomizeRotationSpeed()
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	FVector randomAngularVelocity = FVector(FMath::FRandRange(-360, 360)*0, FMath::FRandRange(-360, 360) *0, FMath::FRandRange(-360, 360));
	c->SetPhysicsAngularVelocity(randomAngularVelocity, false);

	//FVector randomVelocity = FVector(FMath::FRandRange(-1000, 1000), FMath::FRandRange(-1000, 1000), FMath::FRandRange(-1000, 1000));

	//c->SetPhysicsLinearVelocity(randomVelocity, false);
	

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


// Estimate how must angular delta v the engine will produce if stopped now
FVector getDeltaAngularVelocityToEnginesRatio(TArray<UActorComponent*>& Engines, FVector COM, float Intertia, float ThrustRatio) {
	FVector AngularDeltaVToEnginesStop = FVector::ZeroVector;

	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		AngularDeltaVToEnginesStop += getAngularDeltaVToEngineRatio(Engine,COM, Intertia, ThrustRatio);
	}

	return AngularDeltaVToEnginesStop;
}

FVector getAngularDeltaVToEngineRatio(UIrr310ShipEngine* Engine, FVector COM, float Intertia, float ThrustRatio) {

	// Linear integrale below a y = ax + b curve with a  = ThrustVariationSpeed and  b = CurrentThrust :
	// b^2 / 2a


	FVector EngineOffset = (Engine->GetComponentLocation() - COM) / 100;

	FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
	FVector TorqueDirection = FVector::CrossProduct(EngineOffset, WorldThurstAxis);
	TorqueDirection.Normalize();


	float DeltaThurst = FMath::Abs(Engine->CurrentThrust - Engine->MaxThrust * ThrustRatio);
	return TorqueDirection * 0.5 * (DeltaThurst * DeltaThurst) / (Engine->ThrustVariationSpeed * Intertia);
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

static FVector getTotalMaxThrustInAxis(TArray<UActorComponent*>& Engines, FVector Axis, float ThurstAngleLimit)
{
	Axis.Normalize();
	FVector TotalMaxThrust = FVector::ZeroVector;
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);

		float dot = FVector::DotProduct(WorldThurstAxis, Axis);
		if (dot > ThurstAngleLimit) {
			float ratio = (dot - ThurstAngleLimit) / (1 - ThurstAngleLimit);

			TotalMaxThrust += Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis) * Engine->MaxThrust * ratio;
		}
		else if (dot < -ThurstAngleLimit) {

			float ratio = (- dot - ThurstAngleLimit) / (1 - ThurstAngleLimit);

			TotalMaxThrust -= Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis) * Engine->MinThrust * ratio;
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


static FVector computeGravity() {
	return FVector(0, 0, -13.62); // 13.62 m/s²
}


/**
Return leviration acceleration
*/
static FVector computeLevitation(FVector location) {
	return FVector(0, 0, 2.612622450e9 / FMath::Square(location.Z + 13850)); // At 13850 m gravity and levitation ar equal.
}
