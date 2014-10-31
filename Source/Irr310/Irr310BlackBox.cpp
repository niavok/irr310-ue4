#include "Irr310.h"
#include "Irr310BlackBox.h"
#include "Irr310Ship.h"


Irr310BlackBox::Irr310BlackBox(AIrr310Ship* Ship) :
Ship(Ship)
{
	FString Prefix = "Ship_";
	FString Extension = ".bbox";
	FString path = FPaths::CreateTempFilename(*FPaths::GameLogDir(), *Prefix, *Extension);
	UE_LOG(LogTemp, Warning, TEXT("Create Black box at %s"), *path);
	FFileHelper::SaveStringToFile("plop", *path);
}

void Irr310BlackBox::WriteState()
{

	
}