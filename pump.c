#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "8x8led.h"

#define REG_ROW1 0x00

#define SET_CONFIG23 "config-pin p9.23 gpio"
#define CONFIG_DIRECTION49 "echo out > /sys/class/gpio/gpio49/direction"
#define SOIL_VALUE "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define TEMP_VALUE "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define PUMP_OFF "echo 0 > /sys/class/gpio/gpio49/value"
#define PUMP_ON "echo 1 > /sys/class/gpio/gpio49/value"

double ADC0, ADC1;
int i2cFileDesc;

struct moisture_struct {
    double soil_moisture;
};

struct temp_struct {
    double temperature_reading;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Allows program to run inputted macro command 
static void runCommand(char msg[]) 
{
    FILE *pipe = popen(msg, "r");

    char buffer[1024];
    while(!feof(pipe) && !ferror(pipe)) {
        if(fgets(buffer, sizeof(buffer), pipe) == NULL)
        break;
    }

    int exitCode = WEXITSTATUS(pclose(pipe));
    if(exitCode != 0) {
        perror("Unable to execute command");
    }
}


int readFromFile(char *fileName)
{
    FILE *pFile = fopen(fileName, "r");
    if (pFile == NULL) {
    printf("ERROR: Unable to open file (%s) for read\n", fileName);
    exit(-1);
    }
    // Read string (line)
    const int MAX_LENGTH = 1024;
    char buff[MAX_LENGTH];
    fgets(buff, MAX_LENGTH, pFile);
    // Close
    fclose(pFile);
    // printf("Read: '%s'\n", buff);
    return atoi(buff);
}


// Returns the time elapsed since ~1970 in ms
static long long getTimeInMs(void) 
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    long long milliSeconds = seconds * 1000 + nanoSeconds / 1000000;
    return milliSeconds;
}


static void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

static void writeToTxt(double soilMoisture, double temperatureValue) 
{
    FILE *temperature = fopen("temperature.txt", "w");
    FILE *soil = fopen("soilMoisture.txt", "w");

    if(temperature == NULL) {
        fprintf(stderr, "Unable to write temperature to file\n");
        return;
      }
    if(soil == NULL) {
        fprintf(stderr, "Unable to write soil moisture to file\n");
        return;
    }

    fprintf(temperature, "%.2f", temperatureValue);
    fprintf(soil, "%.2f", soilMoisture);

    fclose(temperature);
    fclose(soil);
}


static void setLastWatered(char* timeLastWatered) 
{
    FILE *lastWater = fopen("timeSinceWater.txt", "w");

    if(lastWater == NULL) {
        fprintf(stderr, "Unable to write time to file\n");
        return;
    }

    fprintf(lastWater, "%s", timeLastWatered);

    fclose(lastWater);
}


char* convertTimestampToTimeString(long long time) 
{
    // Convert timestamp to time structure
    time_t seconds = time / 1000;
    struct tm *timeinfo = localtime(&seconds);

    // Format the time as HH:MM:SS
    char *formattedTime = (char*)malloc(9);
    snprintf(formattedTime, 9, "%02d:%02d:%02d", timeinfo->tm_hour + 16, timeinfo->tm_min, timeinfo->tm_sec);

    return formattedTime;
}


static void setManualControlZero() 
{
    FILE *manualControl = fopen("manualControl.txt", "w");

    if(manualControl == NULL) {
        fprintf(stderr, "Unable to write time to file\n");
        return;
    }

    fprintf(manualControl, "%d", 0);

    fclose(manualControl);

}

static char getManualControl() 
{
    FILE *manualControl = fopen("manualControl.txt", "r");

    if(manualControl == NULL) {
        fprintf(stderr, "Unable to read from manualControl.txt");
        return 2;
    }

    char value = fgetc(manualControl);

    if (value == EOF) {
        perror("Error reading file");
        fclose(manualControl);
        return 3;
    }

    fclose(manualControl);

    return value;

}

void* getSoilMoisture(void * arg) 
{
    ADC0 = readFromFile(SOIL_VALUE);
    double temp = (4095 - ADC0)/(2095);
    temp *= 100;
    struct moisture_struct *output = malloc(sizeof(*output));
    output->soil_moisture = temp;
    pthread_exit(output);
    // return temp;
}

void* getTemperature(void * arg) 
{
    ADC1 = readFromFile(TEMP_VALUE);
    double temp = ADC1/22.75;
    struct temp_struct *output = malloc(sizeof(*output));
    output->temperature_reading = temp;
    pthread_exit(output);
    // return temp;
}

int main() {

    int optimalSoilMoisture = 50;
    bool active = true;
    long long waterStart;
    char *formattedTime;
    int i = 0;
    int ledMoisture;

    struct moisture_struct *moisture = malloc(sizeof(*moisture));
    struct temp_struct *temp = malloc(sizeof(*temp));

    //initializeLed();
    i2cFileDesc = initialize_LED();

    // Only need to run once at the start of program, not needed in game/main loop

    runCommand(SET_CONFIG23);
    runCommand(CONFIG_DIRECTION49);
    runCommand(PUMP_OFF);

    while(active == true) {

        long long startLoopTime = getTimeInMs();
        long long startDataLoopTime = getTimeInMs();

        i += 1;
        printf("Loop #%d\n\n", i);

        pthread_t tid[2];

        while((getTimeInMs() - startLoopTime) < 61000) { 
            //printf("%lli\n", getTimeInMs() - startLoopTime); Used to check how many seconds have passed
            
            if((getTimeInMs() - startDataLoopTime) >= 3000) {
                sleepForMs(50);
                pthread_create(&tid[1], NULL, getSoilMoisture, NULL);
                pthread_create(&tid[0], NULL, getTemperature, NULL);

                pthread_mutex_lock(&mutex);
                pthread_join(tid[1], (void**)&moisture);
                pthread_mutex_unlock(&mutex);
                pthread_mutex_lock(&mutex);
                pthread_join(tid[0], (void**)&temp);
                pthread_mutex_unlock(&mutex);

                // printf("Soil Moisture percent: %.2f%%\n", getSoilMoisture());
                // printf("Ambient Temperature: %.2f C\n\n", getTemperature());

                printf("Soil Moisture percent: %.2f%%\n", moisture->soil_moisture);
                printf("Ambient Temperature: %.2f C\n\n", temp->temperature_reading);

                // ledMoisture = getSoilMoisture();
                ledMoisture = moisture->soil_moisture;

                writeI2cReg(i2cFileDesc, REG_ROW1, intToArray(ledMoisture));
                
                printf("%d\n", ledMoisture);
                // writeToTxt(getSoilMoisture(), getTemperature());
                writeToTxt(moisture->soil_moisture, temp->temperature_reading);
                // printf("%f\n", getTemperature());
                printf("%f\n", temp->temperature_reading);
                startDataLoopTime = getTimeInMs();
            }

            // // Only waters once a minute if soil moisture is greater than 50
            // if((getSoilMoisture() < optimalSoilMoisture) && ((getTimeInMs() - startLoopTime) >= 59000)) {
            //     printf("Pump On, Soil Moisture < 50\n");
            //     runCommand(PUMP_ON);
            //     sleepForMs(2000);
            //     runCommand(PUMP_OFF);
            //     sleepForMs(50);
            //     waterStart = getTimeInMs();
            //     formattedTime = convertTimestampToTimeString(waterStart);
            //     setLastWatered(formattedTime);
            //     printf("Pump Off, Soil Moisture == %.2f%%\n", getSoilMoisture());
            // }

            // Only waters once a minute if soil moisture is greater than 50
            if((moisture->soil_moisture < optimalSoilMoisture) && ((getTimeInMs() - startLoopTime) >= 59000)) {
                printf("Pump On, Soil Moisture < 50\n");
                runCommand(PUMP_ON);
                sleepForMs(2000);
                runCommand(PUMP_OFF);
                sleepForMs(50);
                waterStart = getTimeInMs();
                formattedTime = convertTimestampToTimeString(waterStart);
                setLastWatered(formattedTime);
                printf("Pump Off, Soil Moisture == %.2f%%\n", moisture->soil_moisture);
            }

            // If user wants to manually water the plant
            if(getManualControl() == '1') {
                printf("Manual Control On\n");
                runCommand(PUMP_ON);
                sleepForMs(2000);
                runCommand(PUMP_OFF);
                sleepForMs(500);
                setManualControlZero();
                // Update the time since last watered
                waterStart = getTimeInMs();
                formattedTime = convertTimestampToTimeString(waterStart);
                setLastWatered(formattedTime);
                printf("Manual Control Off\n");
                // Reset the 60 second counter
                startLoopTime = getTimeInMs();
            }

        }

        free(formattedTime);
        runCommand(PUMP_OFF);
    }

}