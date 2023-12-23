// pump.h

#ifndef _PUMP_H_
#define _PUMP_H_

int readFromFile(char *fileName);

// Returns the time elapsed since ~1970 in ms
static long long getTimeInMs(void);

static void sleepForMs(long long delayInMs);

static void writeToTxt(double soilMoisture, double temperatureValue);

static void setLastWatered(char* timeLastWatered);

char* convertTimestampToTimeString(long long time);

static void setManualControlZero();

static char getManualControl();

static double getSoilMoisture();

static double getTemperature();

#endif