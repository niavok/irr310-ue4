#include "Irr310.h"
#include "Irr310FlightRecorder.h"
#include "Irr310Ship.h"
#include "Irr310ShipEngine.h"


Irr310FlightRecorder::Irr310FlightRecorder(AIrr310Ship* Ship) :
Ship(Ship),
TickIndex(0)
{
	FString Prefix = "Ship_"+ Ship->GetName();
	FString Extension = ".bbox";
	FString Path = FPaths::CreateTempFilename(*FPaths::GameLogDir(), *Prefix, *Extension);
	UE_LOG(LogTemp, Warning, TEXT("Create Black box at %s"), *Path);

	//FFileHelper::SaveStringToFile("plop", *path);

	//File = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*Path, false, false);

	Archive = IFileManager::Get().CreateFileWriter(*Path);
}

Irr310FlightRecorder::~Irr310FlightRecorder()
{
	//delete File;
	delete Archive;
}

void Irr310FlightRecorder::WriteState(float DeltaSeconds)
{
	if (TickIndex == 0) {
		// Init
		WriteLine(FString::Printf(TEXT("Ship name=%s"), *Ship->GetName()));
	}

	FVector Speed = Ship->GetLocalLinearSpeed();
	FVector WorldSpeed = Ship->GetLinearSpeed();
	FVector Location = Ship->GetLocation();
	FVector LocalLinearVelocityTarget = Ship->LocalLinearVelocityTarget;
	FVector LocalAngularVelocityTarget = Ship->LocalAngularVelocityTarget;

	FRotator WorldRotation = Ship->GetActorRotation();

	FVector LocalAngularVelocity = Ship->getLocalAngularVelocity();
	FVector WorldAngularVelocity = Ship->getWorldAngularVelocity();

	FVector LocationTarget = Ship->LocationTarget;

	WriteLine("---");
	WriteLine(FString::Printf(TEXT("id=%d time=%f delta=%f"), TickIndex, Ship->GetWorld()->TimeSeconds, DeltaSeconds));
	WriteLine(FString::Printf(TEXT("Location=%s"), *Location.ToString()));
	WriteLine(FString::Printf(TEXT("WLinearVelocity=%s"), *WorldSpeed.ToString()));

	// Dump engines
	TArray<UActorComponent*> Engines = Ship->GetComponentsByClass(UIrr310ShipEngine::StaticClass());
	for (int32 EngineIndex = 0; EngineIndex < Engines.Num(); EngineIndex++) {

		UIrr310ShipEngine* Engine = Cast<UIrr310ShipEngine>(Engines[EngineIndex]);
		WriteLine(FString::Printf(TEXT("Engine name=%s"), *Engine->GetName()));
		WriteLine(FString::Printf(TEXT("  CurrentThrust=%f"), Engine->CurrentThrust));
		WriteLine(FString::Printf(TEXT("  TargetThrust=%f"), Engine->TargetThrust));
		WriteLine(FString::Printf(TEXT("  MinThrust=%f"), Engine->MinThrust));
		WriteLine(FString::Printf(TEXT("  MaxThrust=%f"), Engine->MaxThrust));
		WriteLine(FString::Printf(TEXT("  ThrustAxis=%s"), *Engine->ThrustAxis.ToString()));
	}


	TickIndex++;
	
}

void Irr310FlightRecorder::WriteLine(FString Line)
{
	FString Text = Line + "\n";
	auto Src = StringCast<ANSICHAR>(*Text, Text.Len());
	Archive->Serialize((ANSICHAR*)Src.Get(), Src.Length() * sizeof(ANSICHAR));
	
}