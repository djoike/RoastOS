const int tempIntervalArrayLength = 3;
const int temperatureIntervals[tempIntervalArrayLength] = {1,5,999};  // Could possibly later be defined from external source
const double temperaturePaces[tempIntervalArrayLength] = {0.025,0.05,0.25};  // Could possibly later be defined from external source
const int tempArrayLength = 5;
double temperatures[tempArrayLength];
int outOfPaceTicks = 0;
const int maxRoastingEffect = 100;

String debugLine = "";

// Enum of different pace cases 
// 1: currentPace > targetPace(+)
// 2: currentPace < targetPace(+)
// 3: currentPace > targetPace(-)
// 4: currentPace < targetPace(-)
int currentPaceCase = 2; 

void tc_temperatureAdjustmentCycle()
{
	// Serial.println("temperatureAdjustmentCycle");
	// This will handle all work regarding the temperature approximation algorithm

	tc_collectTemperature();
	currentPace = tc_getCurrentPace();
	currentTargetPace = tc_getCurrentTargetPace();
	debugLine = doubleToString(currentPace);
	debugLine += " - " + doubleToString(currentTargetPace) + " - ";
	for( int i = 0; i<=4; i++)
	{
		if (i > 0)
		{
			debugLine += ";";
		}
		debugLine += doubleToString(temperatures[i]);
	}
	if (currentTargetPace > 0.0)
	{
		if (currentPace > currentTargetPace)
		{
			tc_setOutOfPaceTicks(1);
			// Serial.println("lamp -");
		}
		else
		{
			tc_setOutOfPaceTicks(2);
			// Serial.println("lamp +");
		}
	}
	else
	{
		if (currentPace > currentTargetPace)
		{
			tc_setOutOfPaceTicks(3);
			// Serial.println("lamp -");
		}
		else
		{
			tc_setOutOfPaceTicks(4);
			// Serial.println("lamp +");
		}
	}

	int adjustBySteps = tc_getRoastingEffectAdjustment();
	tc_adjustRoastingEffect(adjustBySteps);
	//Serial.println(debugLine);
}

void tc_adjustRoastingEffect(int adjustmentSteps)
{
	// Serial.println("adjustRoastingEffect");
	if (currentRoastingEffect + adjustmentSteps < 0)
	{
		currentRoastingEffect = 0;
	}
	else if (currentRoastingEffect + adjustmentSteps > maxRoastingEffect)
	{
		currentRoastingEffect = maxRoastingEffect;
	}
	else
	{
		currentRoastingEffect += adjustmentSteps;
	}
	//Serial.print("Current roasting effect: ");
	debugLine += " - " + String(currentRoastingEffect);
	dcSetLevel(currentRoastingEffect);
}

int tc_getRoastingEffectAdjustment()
{
	// Serial.println("getRoastingEffectAdjustment");
	int steps = outOfPaceTicks;
	
	if (steps > 5)
	{
		steps = 5;
	}
	
	if (currentPaceCase == 1 or currentPaceCase == 3)
	{
		steps = -steps * 1; //Changed from 2 to 1 to allow for slower cooldown
	}
	return steps;
}

void tc_setOutOfPaceTicks(int paceCase)
{
	// Serial.println("setOutOfPaceTicks");
	if (paceCase != currentPaceCase)
	{
		currentPaceCase = paceCase;
		outOfPaceTicks = 1;
	}
	else 
	{
		outOfPaceTicks++;
	}
}

void tc_collectTemperature()
{
	// Serial.println("collectTemperature");
	// Get the latest temperature and update the array of temperatures
	double tmpTemp = tc_getCurrentTemperature();
	//Serial.println(tmpTemp);
	for( int i = 0; i<tempArrayLength-1; i++)
	{
		temperatures[i] = temperatures[i+1];
		//Serial.println(i);
	}
	temperatures[tempArrayLength-1] = tmpTemp;
	//Serial.println(tmpTemp);
}

void tc_resetTemperatures()
{
	// Serial.println("resetTemperatures");
	// Replace all data in the temperature array with the current temperature

	double tmpTemp = tc_getCurrentTemperature();
	for( int i = 0; i<tempArrayLength; i++)
	{
		temperatures[i] = tmpTemp;
	}
}

double tc_getCurrentPace()
{
	// Serial.println("getCurrentPace");
	return (temperatures[tempArrayLength-1] - temperatures[0]) / (tempArrayLength-1);		
}

double tc_getCurrentTargetPace()
{
	// Serial.println("getCurrentTargetPace");
	// Find the correct pace for the difference between the target temperature and the current temperature.
	// Returns signed float AKA signed double

	double tmpPace = 0;
	double difference = tc_getCurrentTargetTemp() - tc_getCurrentTemperature();

	for( int i = 0; i<tempIntervalArrayLength; i++)
	{
		if(fabs(difference) < temperatureIntervals[i])
		{
			if(difference > 0)
			{
				tmpPace = temperaturePaces[i];
			}
			else
			{
				tmpPace = -temperaturePaces[i];
			}
			break;
		}
	}
	return tmpPace;

}

double tc_getCurrentTemperature()
{
	double tempSensorValue = analogRead(tempSensorPin);
	double knownResistance = 1000.0;
	double measuredResistance = 0.0;
	double returnTemp = 0.0;
	double coefA = 0.0008087537565;
	double coefB = 0.0002010885617;
	double coefC = 0.0000001508150106;

	//Goodbye ye olde faithful formulae! Many were the good times we had together.
	//returnTemp = 0.000000000002235240435*pow(tempSensorValue, 5.0) - 0.000000005008836496*pow(tempSensorValue, 4.0) + 0.000004523905046*pow(tempSensorValue, 3.0) - 0.002014934182*pow(tempSensorValue, 2.0) + 0.5996981289*tempSensorValue + 26.81100308;

	measuredResistance = ((1024.0 * knownResistance) / tempSensorValue) - knownResistance;

	returnTemp = pow(coefA + (coefB * log(measuredResistance)) + (coefC * pow(log(measuredResistance),3.0)),-1.0) - 273.15;

	return returnTemp;
}

double tc_getLastReadTemperature()
{
	return temperatures[tempArrayLength-1];
}

double tc_getCurrentTargetTemp()
{
	if (statusCode == 130)
	{
		return prof_GetCurrentTargetTemp();
	}
	if (statusCode == 220)
	{
		return manualRoastTargetTemperature;
	}
	return 0;
}