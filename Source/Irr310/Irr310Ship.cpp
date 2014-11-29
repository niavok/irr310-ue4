// Fill out your copyright notice in the Description page of Project Settings.

#include "Irr310.h"
#include "Irr310Ship.h"
#include "Irr310ShipModule.h"
#include "Irr310ShipEngine.h"

#include "Irr310PhysicHelper.h"

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
	
	// Fix center of mass
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector CurrentCOM = c->GetBodyInstance()->GetCOMPosition();
	COM = c->GetComponentToWorld().TransformPosition(LocalCOM);
	FVector COMShift = COM - CurrentCOM;
	if (COMShift.SizeSquared() > 1) {
		c->GetBodyInstance()->COMNudge += COMShift;
		c->GetBodyInstance()->UpdateMassProperties();
	}


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
	InputComponent->BindAction("Mode+", IE_Pressed, this, &AIrr310Ship::OnIncrementMode);
	InputComponent->BindAction("Mode-", IE_Pressed, this, &AIrr310Ship::OnDecrementMode);

}

void AIrr310Ship::AddForceAtLocation(FVector force, FVector applicationPoint)
{
	TickSumForce += force;

	

	// TODO extract com
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector COM = c->GetComponentToWorld().TransformPosition(LocalCOM);

	FVector ApplicationOffset = (applicationPoint - COM) / 100; // TODO divise by 100 in parameter

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
static FVector getDeltaVToEnginesRatio(TArray<UActorComponent*>& Engines, float Mass, float ThrustRatio, FVector Axis, float ThurstAngleLimit);
static FVector getDeltaVToEngineRatio(UIrr310ShipEngine* Engine, float Mass, float ThrustRatio);
FVector getDeltaAngularVelocityToEnginesRatio(TArray<UActorComponent*>& Engines, FVector COM, float Intertia, float ThrustRatio);
FVector getAngularDeltaVToEngineRatio(UIrr310ShipEngine* Engine, FVector COM, float Intertia, float ThrustRatio);
static float getEnginesToRatioDuration(TArray<UActorComponent*>& Engines, float ThrustRatio);
static float getEngineToRatioDuration(UIrr310ShipEngine* Engines, float ThrustRatio);
static FVector getTotalMaxThrust(TArray<UActorComponent*>& Engines);
static FVector getTotalMaxLocalThrust(UPrimitiveComponent* root, TArray<UActorComponent*>& Engines);
static FVector getTotalMaxThrustInAxis(TArray<UActorComponent*>& Engines, FVector Axis, float ThurstAngleLimit);
static FVector getTotalMaxTorqueInAxis(TArray<UActorComponent*>& Engines, FVector TorqueDirection, FVector COM, float ThurstAngleLimit);

// Tools
static float computeSoftAngularCommand(float measure, float mMaxAngularSpeed, float mSoftModeAngularLimit, float command);

void AIrr310Ship::PhysicSubTick(float deltaTime)
{

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	//UE_LOG(LogTemp, Warning, TEXT("01 - Acceleration: %s"), *Acceleration.ToString());

	// Gravity
	FVector Gravity = Irr310PhysicHelper::ComputeGravity();

	//UE_LOG(LogTemp, Warning, TEXT("02 - Acceleration: %s"), *Acceleration.ToString());

	// Levitation
	FVector Levitation = Irr310PhysicHelper::ComputeLevitation(GetActorLocation() / 100); // At 13850 m gravity and levitation ar equal.


	// Air resistance
	float Ro = 1.6550; // Air density
	float A = 1 * 0.5;
	FVector AirResistance = Irr310PhysicHelper::ComputeLinearAirResistance(c->GetPhysicsLinearVelocity() / 100, Ro, A);

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

	FVector AngularAirResistance = Irr310PhysicHelper::ComputeAngularAirResistance(c->GetPhysicsAngularVelocity(), Ro);

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

void AIrr310Ship::AutoPilotSubTick(float DeltaSeconds)
{
	// Basic auto pilot

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	TArray<UActorComponent*> Engines = GetComponentsByClass(UIrr310ShipEngine::StaticClass());


	TArray<float*> EngineCommands;


	if (ControlMode == 0) {
		// Manual control
		FVector linearTarget = c->GetComponentToWorld().GetRotation().RotateVector(LocalLinearVelocityTarget);
		FVector angularTarget = c->GetComponentToWorld().GetRotation().RotateVector(LocalAngularVelocityTarget);
		EngineCommands.Add(ComputeLinearVelocityStabilisation(DeltaSeconds, Engines, linearTarget, 0.0));
		//EngineCommands.Add(ComputeLinearVelocityStabilisationWithRotation(DeltaSeconds, Engines, linearTarget));
		EngineCommands.Add(ComputeAngularVelocityStabilisation(DeltaSeconds, Engines, angularTarget));
		// Keep head up
		//EngineCommands.Add(ComputeAngularControl(Engines, FVector(0, 0, 1), FVector(0, 0, 1)));
		//EngineCommands.Add(ComputeAngularControl(Engines, FVector(1, 0, 0), FrontAxisTarget));
	} else if (ControlMode == 2) {
		EngineCommands.Add(ComputeLinearVelocityStabilisationWithRotation(DeltaSeconds, Engines, FVector(LocalLinearVelocityTarget.Size(), 0, 0)));
	}
	else if (ControlMode == 3) {
		EngineCommands.Add(ComputeLinearVelocityStabilisationWithRotation(DeltaSeconds, Engines, FVector(-LocalLinearVelocityTarget.Size(), 0, 0)));
	}
	else if (ControlMode == 4) {
		EngineCommands.Add(ComputeLinearVelocityStabilisationWithRotation(DeltaSeconds, Engines, FVector(0, LocalLinearVelocityTarget.Size(), 0)));
	}
	else if (ControlMode == -1) {
		UCameraComponent* Camera = Cast<UCameraComponent>(GetComponentByClass(UCameraComponent::StaticClass()));

		FVector CameraAxis = Camera->GetComponentToWorld().GetRotation().RotateVector(FVector(1, 0, 0));
		FrontAxisTarget = CameraAxis;

		FVector linearTarget = c->GetComponentToWorld().GetRotation().RotateVector(LocalLinearVelocityTarget);

		EngineCommands.Add(ComputeLinearVelocityStabilisation(DeltaSeconds, Engines, linearTarget, 0.0));
		EngineCommands.Add(ComputeAngularControl(DeltaSeconds, Engines, FVector(1, 0, 0), FrontAxisTarget));

	} else {
	

		// Keep head up
		//EngineCommands.Add(ComputeAngularControl(DeltaSeconds, Engines, FVector(0, 0, 1), FVector(0, 0, 1)));
	
		//EngineCommands.Add(ComputeAngularControl(DeltaSeconds, Engines, FVector(1, 0, 0), FVector(1, 0, 0)));
	
		// Go to target
		//EngineCommands.Add(ComputePositionWithRotationControl(DeltaSeconds, Engines, LocationTarget, LocalLinearVelocityTarget.Size()));
		EngineCommands.Add(ComputePositionWithoutRotationControl(DeltaSeconds, Engines, LocationTarget, FVector(0, 0, 0), FVector(0, 0, 0), LocalLinearVelocityTarget.Size()));
	
		//EngineCommands.Add(ComputeLinearVelocityStabilisation(Engines, FVector(40,-5,0.5), 0));

		//EngineCommands.Add(ComputeLinearVelocityStabilisation(Engines, LocalLinearVelocityTarget));
	
	
		//EngineCommands.Add(ComputeAngularVelocityStabilisation(Engines, LocalAngularVelocityTarget));


		
	}

	for (int32 EngineIndex = 0; EngineIndex < Engines.Num(); EngineIndex++) {
		float ThrustRatio = 0;
		for (int32 CommandIndex = 0; CommandIndex < EngineCommands.Num(); CommandIndex++) {
			float newThustRatio =EngineCommands[CommandIndex][EngineIndex];
			if (newThustRatio * ThrustRatio < 0) {
				// Opposite order
				ThrustRatio = ThrustRatio * 0.5;
			}
			ThrustRatio = ThrustRatio + newThustRatio;
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

float* AIrr310Ship::ComputeLinearVelocityStabilisation(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector WorldTargetSpeed, float ThrustAngleLimit)
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector WorldVelocity = GetLinearSpeed();
	
	// Compute the equilibrium Thrust: Target Air Resistance = Total Thurst
	float Ro = 1.6550; // Air density
	float A = 1 * 0.5;
	FVector TargetAirResistance = Irr310PhysicHelper::ComputeLinearAirResistance(WorldTargetSpeed, Ro, A);
	FVector CurrentAirResistance = Irr310PhysicHelper::ComputeLinearAirResistance(WorldVelocity, Ro, A);

	FVector GravityForce = Irr310PhysicHelper::ComputeGravity() * Mass;
	FVector LevitationForce = Irr310PhysicHelper::ComputeLevitation(GetActorLocation() / 100) * Mass;

	// Compute final thrust
	// Its the thrust to keep the WorldTargetSpeed constant if the ship is at WorldTargetSpeed
	// The final thrust is the negative off all forces : air resistance, gravity and levitation
	FVector FinalThurst = -(TargetAirResistance + GravityForce + LevitationForce);
	
	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
		float LocalTargetVelocity = FVector::DotProduct(WorldThurstAxis, WorldTargetSpeed);

		float TotalMaxThrustInAxis = FVector::DotProduct(WorldThurstAxis, getTotalMaxThrustInAxis(Engines, WorldThurstAxis, ThrustAngleLimit));
		float FinalThurstInAxis = FVector::DotProduct(WorldThurstAxis, FinalThurst);
		float FinalThurstRatio;
		if (FMath::IsNearlyZero(FinalThurstInAxis) || FMath::IsNearlyZero(TotalMaxThrustInAxis)) {
			// No need to thrust compensation
			FinalThurstRatio = 0;
		}
		else 
		{
			// Find total thurst in limit in the axis of FinalThrust
			FinalThurstRatio = FMath::Clamp(FinalThurstInAxis / TotalMaxThrustInAxis, -1.f, 1.f);
		}

		
		// Compute delta to stop
		float WorldVelocityToEnginesStop = FVector::DotProduct(WorldThurstAxis, WorldVelocity);
		//WorldVelocityToEnginesStop += FVector::DotProduct(WorldThurstAxis, getDeltaVToEnginesRatio(Engines, Mass, FinalThurstRatio, WorldThurstAxis, ThrustAngleLimit));

		// Check if air resistant won't make the estimation optimist.
		//WorldVelocityToEnginesStop += FVector::DotProduct(WorldThurstAxis, (getEngineToRatioDuration(Engine, FinalThurstRatio) * (-FinalThurst) / Mass)); // Assusme the air resistance will be almost constant during all the process. It's wrong, but it's better than noting

		float DeltaVelocityToStop = (LocalTargetVelocity - WorldVelocityToEnginesStop);

		float ThrustAjustement = DeltaVelocityToStop * Mass / (1.5*DeltaSeconds);
		float ThrustCommand = FMath::Clamp((FinalThurstRatio + ThrustAjustement / TotalMaxThrustInAxis), -1.0f, 1.0f);
	

		if (FMath::IsNearlyZero(ThrustCommand)) {
			ThrustCommand = 0;
		}
		command[i] = ThrustCommand;
	}
	return command;
}

float* AIrr310Ship::ComputeLinearVelocityStabilisationWithRotation(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector WorldTargetSpeed) {

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;



	// Time to reach speed without rotation
	FVector DeltaVelocity = WorldTargetSpeed - GetLinearSpeed();

	FVector DeltaVelocityAxis = DeltaVelocity;
	DeltaVelocityAxis.Normalize();

	FVector MaxThrustDirection = getTotalMaxThrust(Engines);
	MaxThrustDirection.Normalize();
	FVector LocalMaxThrustDirection = getTotalMaxLocalThrust(c, Engines);
	LocalMaxThrustDirection.Normalize();

	// Find Usefull thrust
	FVector MaxThrustInAxis = getTotalMaxThrustInAxis(Engines, DeltaVelocity, 0);
	float UsefullThrust = FVector::DotProduct(DeltaVelocityAxis, MaxThrustInAxis);

	float Ro = 1.6550; // Air density
	float A = 1 * 0.5;
	FVector TargetAirResistance = Irr310PhysicHelper::ComputeLinearAirResistance(WorldTargetSpeed, Ro, A);

	FVector GravityForce = Irr310PhysicHelper::ComputeGravity() * Mass;
	FVector LevitationForce = Irr310PhysicHelper::ComputeLevitation(GetActorLocation() / 100) * Mass;

	// Compute final thrust
	// Its the thrust to keep the WorldTargetSpeed constant if the ship is at WorldTargetSpeed
	// The final thrust is the negative off all forces : air resistance, gravity and levitation
	// TODO Check disable gravity and levitation 
	FVector FinalThurst = (TargetAirResistance + GravityForce*0 + LevitationForce*0);
	FVector FinalThurstAxis = FinalThurst;
	FinalThurstAxis.Normalize();


	float MalusThurst = FVector::DotProduct(DeltaVelocityAxis, FinalThurst);
	MalusThurst = FMath::Min(MalusThurst, 0.f);

	// External thrust usure
	UsefullThrust += MalusThurst;
	UsefullThrust = FMath::Max(0.f, UsefullThrust);
	// TODO air resistance
	float TimeToFinalVelocity;

	if (FMath::IsNearlyZero(DeltaVelocity.SizeSquared()))
	{
		TimeToFinalVelocity = 0;
	}
	else {
		TimeToFinalVelocity = Mass * DeltaVelocity.Size() / UsefullThrust;
	}

	// Time to rotate
	FVector COM = c->GetComponentToWorld().TransformPosition(LocalCOM);
	FVector MaxTorqueInAxis = getTotalMaxTorqueInAxis(Engines, DeltaVelocityAxis, COM, 0);
	float UsefullTorque = FVector::DotProduct(DeltaVelocityAxis, MaxTorqueInAxis);

	float dot = FVector::DotProduct(MaxThrustDirection, DeltaVelocityAxis);
	float angle = FMath::RadiansToDegrees(FMath::Acos(dot));

	float TimeToRotate= 2 * angle * InertiaTensor / UsefullTorque;
	

	//Time with rotation
	FVector MaxThrustInMaxAxis = getTotalMaxThrustInAxis(Engines, MaxThrustDirection, 0);
	float MaxUsefullThrust = FVector::DotProduct(MaxThrustDirection, MaxThrustInMaxAxis);
	// External thrust usure
	MaxUsefullThrust += MalusThurst;
	MaxUsefullThrust = FMath::Max(0.f, MaxUsefullThrust);


	// During the rotation, we assume the thruster work at 50% as the current value
	float DeltaVDuringRotation = TimeToRotate * (UsefullThrust / Mass) * 0.1;
	float TimeToFinalVelocityWithRotation = TimeToRotate + Mass * (DeltaVelocity.Size() - DeltaVDuringRotation) / MaxUsefullThrust;

	
	//Faster to rotate
	float* angularCommand;
	float* positionCommand;
	//FinalThurst.Size() > 1 ||

	float ratio = 1 / FMath::Max(c->GetPhysicsAngularVelocity().Size(), 1.f);

	if (!FMath::IsFinite(TimeToFinalVelocity) || TimeToFinalVelocityWithRotation < TimeToFinalVelocity) {
		//Faster to rotate

		FVector Orientation = -0.5 * TargetAirResistance / Mass + DeltaVelocity;
		Orientation.Normalize();

		angularCommand = ComputeAngularControl(DeltaSeconds, Engines, LocalMaxThrustDirection, Orientation);
	}
	else {
		angularCommand = ComputeAngularVelocityStabilisation(DeltaSeconds, Engines, FVector::ZeroVector);
		ratio = 1;
	}

	
	
	FVector targetSpeed = WorldTargetSpeed * ratio + GetLinearSpeed() * (1 - ratio) *0.5;

	positionCommand = ComputeLinearVelocityStabilisation(DeltaSeconds, Engines, targetSpeed, 0.0);


	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {

		/*if (angularCommand[i] * positionCommand[i] < 0) {
			angularCommand[i] *= 0.2;
		}*/
		command[i] = angularCommand[i] +  positionCommand[i];
	}

	delete[] angularCommand;
	delete[] positionCommand;

	return command;

}

float* AIrr310Ship::ComputeAngularVelocityStabilisation(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector WorldTargetSpeed)
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector COM = c->GetComponentToWorld().TransformPosition(LocalCOM);
	FVector AngularVelocity = c->GetPhysicsAngularVelocity();

	float Ro = 1.6550; // Air density
	FVector TargetAirResistance = Irr310PhysicHelper::ComputeAngularAirResistance(WorldTargetSpeed, Ro);
	FVector CurrentAirResistance = Irr310PhysicHelper::ComputeAngularAirResistance(AngularVelocity, Ro);

	
	// Compute final thrust
	// Its the thrust to keep the WorldTargetSpeed constant if the ship is at WorldTargetSpeed
	// The final thrust is the negative off all forces : air resistance
	FVector FinalTorque = -TargetAirResistance;

	float* command = new float[Engines.Num()];
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		FVector EngineOffset = (Engine->GetComponentLocation() - COM) / 100;

		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
		FVector TorqueDirection = FVector::CrossProduct(EngineOffset, WorldThurstAxis); 
		if (TorqueDirection.Size() < 0.001) {
			command[i] = 0;
			continue;
		}
		TorqueDirection.Normalize();

		float LocalTargetVelocity = FVector::DotProduct(TorqueDirection, WorldTargetSpeed);

		float TotalMaxTorqueInAxis = FVector::DotProduct(TorqueDirection, getTotalMaxTorqueInAxis(Engines, TorqueDirection, COM, 0));
		float FinalTorqueInAxis = FVector::DotProduct(TorqueDirection, FinalTorque);
		float FinalThurstRatio;
		if (FMath::IsNearlyZero(FinalTorqueInAxis) || FMath::IsNearlyZero(TotalMaxTorqueInAxis)) {
			// No need to thrust compensation
			FinalThurstRatio = 0;
		}
		else
		{
			// Find total thurst in limit in the axis of FinalThrust
			FinalThurstRatio = FMath::Clamp(FinalTorqueInAxis / TotalMaxTorqueInAxis, -1.f, 1.f);
		}

		// Compute delta to stop
		float WorldVelocityToEnginesStop = FVector::DotProduct(TorqueDirection, AngularVelocity);
		//WorldVelocityToEnginesStop += FVector::DotProduct(TorqueDirection, getDeltaAngularVelocityToEnginesRatio(Engines, COM, InertiaTensor, FinalThurstRatio)); // TODO inertia

		// Check if air resistant won't make the estimation optimist.
		//WorldVelocityToEnginesStop += FVector::DotProduct(TorqueDirection, (getEngineToRatioDuration(Engine, FinalThurstRatio) * (-FinalTorque) / InertiaTensor)); // Assusme the air resistance will be almost constant during all the process. It's wrong, but it's better than noting

		float DeltaVelocityToStop = (LocalTargetVelocity - WorldVelocityToEnginesStop);

		float ThrustAjustement = DeltaVelocityToStop * InertiaTensor / (1.5*DeltaSeconds);
		float ThrustCommand = FMath::Clamp((FinalThurstRatio + ThrustAjustement / TotalMaxTorqueInAxis), -1.0f, 1.0f);

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
float* AIrr310Ship::ComputeAngularControl(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector LocalShipAxis, FVector TargetAxis)
{
	/*UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector WorldShipAxis = c->GetComponentToWorld().GetRotation().RotateVector(LocalShipAxis);

	WorldShipAxis.Normalize();
	TargetAxis.Normalize();


	// Accelerate as fast as possible and decelerate when the integrale of the remaining angular movement if breaking now is higther than the target
	FVector RotationDirection = FVector::CrossProduct(WorldShipAxis, TargetAxis);


	float dot = FVector::DotProduct(WorldShipAxis, TargetAxis);
	
	
	//Angle vary with rotation direction and negative dot

	if (dot < 0) {
		//Invert and maximize
		if (RotationDirection.Size() == 0) {
			RotationDirection = FVector(1, 1, 1) - TargetAxis;
			
		}
		//RotationDirection *= -1;
		RotationDirection.Normalize();
	}
	*/
	/*
	// Soften the end only
	RotationDirection.X = FMath::Pow(RotationDirection.X,3);
	RotationDirection.Y = FMath::Pow(RotationDirection.Y, 3);
	RotationDirection.Z = FMath::Pow(RotationDirection.Z, 3);
	*/
	/*
	RotationDirection *= 180;

	RotationDirection.ClampMaxSize(360);*/

	
	//TODO Improve


	// Check what is the faster angle is 

	// TODO Evaluate if and intermediate rotation help

	// TODO pass ComputeAngularVelocityStabilisation in world space
	//FVector WorldRotationDirection = c->GetComponentToWorld().GetRotation().Inverse().RotateVector(RotationDirection);

	////////////////////////////////////


	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector COM = c->GetComponentToWorld().TransformPosition(LocalCOM);
	FVector AngularVelocity = c->GetPhysicsAngularVelocity();
	FVector WorldShipAxis = c->GetComponentToWorld().GetRotation().RotateVector(LocalShipAxis);
	
	WorldShipAxis.Normalize();
	TargetAxis.Normalize();

	FVector RotationDirection = FVector::CrossProduct(WorldShipAxis, TargetAxis);
	RotationDirection.Normalize();
	float dot = FVector::DotProduct(WorldShipAxis, TargetAxis);
	float angle = FMath::RadiansToDegrees(FMath::Acos(dot));

	FVector DeltaVelocity = -AngularVelocity;
	FVector DeltaVelocityAxis = DeltaVelocity;
	DeltaVelocityAxis.Normalize();

	// Time to reach Target velocity

	// To reach target velocity 

	// The profil to reach the target velocity is the following :
	// - a minimum DeltaV, induce by the time to the engine to reach their final trust.
	// - 2 thrust variation profil to accelerate more then less
	// - a constant thrust profil : at max thrust

	// Each engine as it's own final thust ratio and intertia so the minimun deltaV 
	// The sum of all engine give the minium delta V

	// For all engine that finish to reach there delta V the deltaV generated during all this time must be added.

	// Then compute the distance done during this time. If the distance is higther 


	// Find Usefull thrust
	FVector MaxTorqueInAxis = getTotalMaxTorqueInAxis(Engines, DeltaVelocityAxis, COM, 0);
	float UsefullTorque = FVector::DotProduct(DeltaVelocityAxis, MaxTorqueInAxis);

	// TODO air resistance
	float TimeToFinalVelocity;

	if (FMath::IsNearlyZero(DeltaVelocity.SizeSquared()))
	{
		TimeToFinalVelocity = 0;
	}
	else {
		TimeToFinalVelocity = InertiaTensor * DeltaVelocity.Size() / UsefullTorque;
	}

	float AngleToStop = (DeltaVelocity.Size() / 2) * (TimeToFinalVelocity + DeltaSeconds);

	/*UE_LOG(LogTemp, Warning, TEXT("===="));
	UE_LOG(LogTemp, Warning, TEXT("3 - MaxThrustInAxis: %s"), *MaxThrustInAxis.ToString());
	UE_LOG(LogTemp, Warning, TEXT("3 - UsefullThrust: %f"), UsefullThrust);
	UE_LOG(LogTemp, Warning, TEXT("3 - TimeToFinalVelocity: %f"), TimeToFinalVelocity);
	UE_LOG(LogTemp, Warning, TEXT("3 - Distance: %f"), Distance);
	UE_LOG(LogTemp, Warning, TEXT("3 - DeltaSeconds: %f"), DeltaSeconds);
	UE_LOG(LogTemp, Warning, TEXT("3 - DeltaLocation.Size(): %f"), DeltaLocation.Size());*/

	FVector RelativeResultSpeed;

	if (AngleToStop > angle) {
		RelativeResultSpeed = FVector::ZeroVector;
	}
	else {

		float MaxPreciseSpeed = FMath::Min((angle - AngleToStop) / DeltaSeconds, 180.0f);

		//UE_LOG(LogTemp, Warning, TEXT("3 - MaxPreciseSpeed: %f"), MaxPreciseSpeed);

		RelativeResultSpeed = RotationDirection;
		RelativeResultSpeed *= MaxPreciseSpeed;
	}

	return ComputeAngularVelocityStabilisation(DeltaSeconds, Engines, RelativeResultSpeed);
}

float* AIrr310Ship::ComputePositionWithoutRotationControl(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector TargetLocation, FVector SpeedAtTarget, FVector TargetSpeed, float maxSpeed) {

	// We want to go from the current location to the Target location.
	// But we want to be at the final location at the target speed.

	// The most important is to be at the target speed because so if from the current position, if during the time to reach the target speed the distance is higther than the distance to the target, we must
	// decelerate immediatly.
	// We will add one current tick time to the time to get the time to decelerate juste before the target and not just after.

	// If it's ok, we can accelerate et reach the target faster.

	// However, we can be in a zone from were we will not be able to reach the target at the right speed. There is a volume from where we cannot reach the target in the right condition.
	// For exemple if we have to reach 0,0  at 10,0 speed and the ship is at 100, 10 it cannot pass 0,0 with the right speed. It mus prepare its manoeuver and go for exemple to -50,0 to go to the target from
	// the right position.

	// There 2 hard task : find these bad zone. The find the best place in the good zone to begin the final move

	// If the Speed at target is zero, the is no bad zone.
	// If the Speed at target is not zere, the baz zone is a half space separated by a plan. The plan is normal to the back of the target at a distance of a target equal to the min distance to accelerate
	// to the target speed from the current speed

	// To target location is the point on this plane where its normal it the ship.
	// As secondary objectives, try to go to the point of the plane no

	// Then, once we are on the rigth zone, go to the objective point but only if the time to reach the other axis in inferio to the main axis

	// Compute all in targetSpeed reference. The target will be considered as static
	FVector RelativeVelocity = GetLinearSpeed() - TargetSpeed;
	FVector RelativeVelocityAtTarget = SpeedAtTarget - TargetSpeed;

	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector DeltaLocation = TargetLocation - c->GetComponentLocation() / 100;
	FVector DeltaLocationAxis = DeltaLocation;
	DeltaLocationAxis.Normalize();


	bool IsInBadZone = false;

	if (!FMath::IsNearlyZero(SpeedAtTarget.SizeSquared())) {
		// Has bad zone

		// TODO

	}

	FVector RelativeResultSpeed(0, 0, 0);

	if (IsInBadZone)
	{
		// TODO
	}
	else {
		FVector DeltaVelocity = RelativeVelocityAtTarget - RelativeVelocity;
		FVector DeltaVelocityAxis = DeltaVelocity;
		DeltaVelocityAxis.Normalize();

		// Time to reach Target velocity

		// To reach target velocity 
		
		// The profil to reach the target velocity is the following :
		// - a minimum DeltaV, induce by the time to the engine to reach their final trust.
		// - 2 thrust variation profil to accelerate more then less
		// - a constant thrust profil : at max thrust

		// Each engine as it's own final thust ratio and intertia so the minimun deltaV 
		// The sum of all engine give the minium delta V

		// For all engine that finish to reach there delta V the deltaV generated during all this time must be added.

		// Then compute the distance done during this time. If the distance is higther 


		// Find Usefull thrust
		FVector MaxThrustInAxis = getTotalMaxThrustInAxis(Engines, DeltaVelocity, 0);
		float UsefullThrust = FVector::DotProduct(DeltaVelocityAxis, MaxThrustInAxis);
		
		float Ro = 1.6550; // Air density
		float A = 1 * 0.5;
		FVector TargetAirResistance = Irr310PhysicHelper::ComputeLinearAirResistance(SpeedAtTarget, Ro, A);
		
		FVector GravityForce = Irr310PhysicHelper::ComputeGravity() * Mass;
		FVector LevitationForce = Irr310PhysicHelper::ComputeLevitation(GetActorLocation() / 100) * Mass;

		// Compute final thrust
		// Its the thrust to keep the WorldTargetSpeed constant if the ship is at WorldTargetSpeed
		// The final thrust is the negative off all forces : air resistance, gravity and levitation
		FVector FinalThurst = (TargetAirResistance + GravityForce + LevitationForce);

		float MalusThurst = FVector::DotProduct(DeltaVelocityAxis, FinalThurst);
		MalusThurst = FMath::Min(MalusThurst, 0.f);

		// External thrust usure
		UsefullThrust += MalusThurst;

		// TODO air resistance
		float TimeToFinalVelocity;
		
		if (FMath::IsNearlyZero(DeltaVelocity.SizeSquared())) 
		{
			TimeToFinalVelocity  = 0;
		}
		else {
			TimeToFinalVelocity = Mass * DeltaVelocity.Size() / UsefullThrust;
		}
		

		
		


		float RelativeVelocityAtTargetInDeltaLocationAxis = FVector::DotProduct(DeltaLocationAxis, RelativeVelocityAtTarget);
		float DeltaVelocityInDeltaLocationAxis = FVector::DotProduct(DeltaLocationAxis, DeltaVelocity);


		float Distance = (DeltaVelocity.Size() / 2 + RelativeVelocityAtTargetInDeltaLocationAxis) * (TimeToFinalVelocity + DeltaSeconds);

		/*UE_LOG(LogTemp, Warning, TEXT("===="));
		UE_LOG(LogTemp, Warning, TEXT("3 - MaxThrustInAxis: %s"), *MaxThrustInAxis.ToString());
		UE_LOG(LogTemp, Warning, TEXT("3 - UsefullThrust: %f"), UsefullThrust);
		UE_LOG(LogTemp, Warning, TEXT("3 - TimeToFinalVelocity: %f"), TimeToFinalVelocity);
		UE_LOG(LogTemp, Warning, TEXT("3 - Distance: %f"), Distance);
		UE_LOG(LogTemp, Warning, TEXT("3 - DeltaSeconds: %f"), DeltaSeconds);
		UE_LOG(LogTemp, Warning, TEXT("3 - DeltaLocation.Size(): %f"), DeltaLocation.Size());*/

		
		if (Distance > DeltaLocation.Size()) {
			RelativeResultSpeed = RelativeVelocityAtTarget;
		}
		else {
			
			float MaxPreciseSpeed = FMath::Min((DeltaLocation.Size() - Distance) / DeltaSeconds, maxSpeed);
			

			float LateralDeltaVelocity = DeltaVelocity.Size() - FMath::Abs(DeltaVelocityInDeltaLocationAxis);
			MaxPreciseSpeed *= 1 - LateralDeltaVelocity / DeltaLocation.Size();

			MaxPreciseSpeed = FMath::Max(0.f, MaxPreciseSpeed);

			//UE_LOG(LogTemp, Warning, TEXT("3 - MaxPreciseSpeed: %f"), MaxPreciseSpeed);
			//UE_LOG(LogTemp, Warning, TEXT("3 - LateralDeltaVelocity: %f"), LateralDeltaVelocity);

			RelativeResultSpeed = DeltaLocation;
			RelativeResultSpeed.Normalize();
			RelativeResultSpeed *= MaxPreciseSpeed;
		}
		//UE_LOG(LogTemp, Warning, TEXT("3 - RelativeResultSpeed: %s"), *RelativeResultSpeed.ToString());
		



	}


	
	// Restore reference
	FVector ResultSpeed = RelativeResultSpeed + TargetSpeed;
	//return ComputeLinearVelocityStabilisation(DeltaSeconds, Engines, RelativeResultSpeed, 0);
	return ComputeLinearVelocityStabilisationWithRotation(DeltaSeconds, Engines, RelativeResultSpeed);
}

float* AIrr310Ship::ComputePositionWithoutRotationControlOld(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector TargetLocation, float speed) {
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;
	FVector DeltaPosition = TargetLocation - c->GetComponentLocation() / 100;
	FVector Speed = DeltaPosition;
	Speed.Normalize();
	//TODO find max speed
	
	


	Speed *= speed;

	return ComputeLinearVelocityStabilisation(DeltaSeconds, Engines, Speed, 0);
}

float* AIrr310Ship::ComputePositionWithRotationControl(float DeltaSeconds, TArray<UActorComponent*>& Engines, FVector TargetLocation, float speed) {
	
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
	return ComputeLinearVelocityStabilisationWithRotation(DeltaSeconds, Engines, TargetSpeed);

	

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

void  AIrr310Ship::OnIncrementMode()
{
	ControlMode++;
}

void  AIrr310Ship::OnDecrementMode()
{
	ControlMode--;
}

void AIrr310Ship::OnPichtCommand(float Val)
{
	LocalAngularVelocityTarget.Y = Val * 180;
}

void AIrr310Ship::OnYawCommand(float Val)
{
	LocalAngularVelocityTarget.Z = - Val * 180;
}

void AIrr310Ship::OnRollCommand(float Val)
{
	LocalAngularVelocityTarget.X = -Val * 180;
}

void AIrr310Ship::OnThrustCommand(float Val)
{
	//LocalLinearVelocityTarget.X = Val * 100;
}


void AIrr310Ship::OnRandomizeRotationSpeed()
{
	UPrimitiveComponent* c = (UPrimitiveComponent*)RootComponent;

	FVector randomAngularVelocity = FVector(FMath::FRandRange(-360, 360), FMath::FRandRange(-360, 360), FMath::FRandRange(-360, 360));
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
FVector getDeltaVToEnginesRatio(TArray<UActorComponent*>& Engines, float Mass, float ThrustRatio, FVector Axis, float ThurstAngleLimit) {

	FVector DeltaVToEnginesStop = FVector::ZeroVector;

	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);

		float dot = FVector::DotProduct(WorldThurstAxis, Axis);
		if (dot > ThurstAngleLimit) {
			float ratio = (dot - ThurstAngleLimit) / (1 - ThurstAngleLimit);
			DeltaVToEnginesStop += getDeltaVToEngineRatio(Engine, Mass, ThrustRatio * ratio);
		}
	}

	return DeltaVToEnginesStop;
}



FVector getDeltaVToEngineRatio(UIrr310ShipEngine* Engine, float Mass, float ThrustRatio) {
	
	// Linear integrale below a y = ax + b curve with a  = ThrustVariationSpeed and  b = CurrentThrust - target thurst :
	// b^2 / 2a

	// But here with have to add the speed of the target speed during all the fligh : target * duration;

	FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
	WorldThurstAxis.Normalize();
	float DeltaThurst = Engine->CurrentThrust - Engine->MaxThrust * ThrustRatio;

	float VariationPart = 0.5 * (DeltaThurst * DeltaThurst) / (Engine->ThrustVariationSpeed * Mass);

	float StaticPart = 0;

	
	float VariationDuration = DeltaThurst / Engine->ThrustVariationSpeed;

	StaticPart = VariationDuration * FMath::Min(Engine->MaxThrust * ThrustRatio, Engine->CurrentThrust) / Mass;

	

	return WorldThurstAxis * (StaticPart + VariationPart);
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
	return FVector::CrossProduct(EngineOffset, WorldThurstAxis * 0.5 * (DeltaThurst * DeltaThurst) / (Engine->ThrustVariationSpeed * Intertia));
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

			TotalMaxThrust += WorldThurstAxis * Engine->MaxThrust * ratio;
		}
		else if (dot < -ThurstAngleLimit) {

			float ratio = (- dot - ThurstAngleLimit) / (1 - ThurstAngleLimit);

			TotalMaxThrust -= WorldThurstAxis * Engine->MinThrust * ratio;
		}
	}

	return TotalMaxThrust;
}


static FVector getTotalMaxTorqueInAxis(TArray<UActorComponent*>& Engines, FVector TorqueAxis, FVector COM, float ThurstAngleLimit)
{
	TorqueAxis.Normalize();
	FVector TotalMaxTorque = FVector::ZeroVector;
	for (int32 i = 0; i < Engines.Num(); i++) {
		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[i]);

		FVector EngineOffset = (Engine->GetComponentLocation() - COM) / 100;
		
		FVector WorldThurstAxis = Engine->GetComponentToWorld().GetRotation().RotateVector(Engine->ThrustAxis);
		FVector TorqueDirection = FVector::CrossProduct(EngineOffset, WorldThurstAxis);

		float dot = FVector::DotProduct(TorqueAxis, TorqueDirection);
		if (dot > ThurstAngleLimit) {
			float ratio = (dot - ThurstAngleLimit) / (1 - ThurstAngleLimit);

			TotalMaxTorque += FVector::CrossProduct(EngineOffset, WorldThurstAxis * Engine->MaxThrust * ratio);
		}
		else if (dot < -ThurstAngleLimit) {

			float ratio = (-dot - ThurstAngleLimit) / (1 - ThurstAngleLimit);

			TotalMaxTorque -= FVector::CrossProduct(EngineOffset, WorldThurstAxis * Engine->MinThrust * ratio);
		}
	}

	return TotalMaxTorque;


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


