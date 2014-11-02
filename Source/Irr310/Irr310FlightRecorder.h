// Copyright 2014 Frédéric Bertolus, Inc. All Rights Reserved.
#pragma once

class AIrr310Ship;

class Irr310FlightRecorder
{
public:
	Irr310FlightRecorder(AIrr310Ship* Ship);

	virtual ~Irr310FlightRecorder();

	void WriteState(float DeltaSeconds);

private:
	AIrr310Ship* Ship;
	//IFileHandle* File;
	FArchive* Archive;
	int32 TickIndex;

	void WriteLine(FString line);

};
