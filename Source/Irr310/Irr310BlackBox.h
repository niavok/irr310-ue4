// Copyright 2014 Fr�d�ric Bertolus, Inc. All Rights Reserved.
#pragma once

class AIrr310Ship;

class Irr310BlackBox
{
public:
	Irr310BlackBox(AIrr310Ship* Ship);

	void WriteState();

private:
	AIrr310Ship* Ship;
};
