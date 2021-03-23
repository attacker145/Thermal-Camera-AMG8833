/*
 * Algo_Thermal_Array.c
 *
 *  Created on: Dec 14, 2020
 *      Author: Roman
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

// SimpleLink includes
#include "simplelink.h"

// driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "utils.h"
#include "interrupt.h"
#include "adc.h"
#include "pin.h"

// common interface includes
#include "uart_if.h"
#include "uart.h"
#include "common.h"
#include "pinmux.h"
#include "i2c_if.h"
#include "hw_memmap.h"
#include "gpio.h"

#include "gpio_if.h"

// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/common.h>


#define MAXTEMP 36.0 //Human temperature, absurdly high


float MINTEMP = 0.0; //Minimum temperature of our current pixel[]
float AVTEMP = 0.0; //Average temperature of current pixel[]
float BASELINE[64]; //Stores our baseline
float STDEV = 0.0; //Stores our current standard deviation
float PIXELS[64]; //Our current pixel array
float SENSITIVITYCOEFFICIENT = 0.45; //Sensitivity coefficient, dictates deltas in avtemp,mintemp,baseline algos (see those algos)
float BASESTANDEV = 0.0; //Stores stdev of our baseline
bool updThisPer = false; //For python debugging
bool overThisPer = false; //For python debugging
unsigned char sinceLastFrame = 0; //Frames since last baseline update (see update baseline algo)
bool TRAVELING[] = {false,false,false,false,false,false}; //Will store our results as a LIFO queue
bool FP[] = {true,false,false,false,false,false}; //Array to check traveling[] against (see shiftTravelingArrayAndCheck function)
unsigned char tsize = 6; //Size of traveling array


//UTILITY/MATH FUNCS----------------

//Calculates our standard deviation given a float[] of len length
float calculateSD(float data[], unsigned int length) {
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < length; ++i) {
        sum += data[i]; //Find sum of data[]
    }
    mean = sum / length; //Find average of data[]
    for (i = 0; i < length; ++i)
        SD += pow(data[i] - mean, 2); //For each float in our list, add (float - average) ^ 2 to our SD
    return sqrt(SD / length); //Return SD calculated earlier (sigma i=0 --> L of (data[i] - average) ^ 2) divided by length, the SD formula
}

//Checks if two boolean arrays are equivalent
bool areEqual(bool arr1[], bool arr2[], int n, int m)
{
    int i;
    // If lengths of array are not equal means
    // array are not equal
    if (n != m)
        return false;


    // Linearly compare elements
    for (i = 0; i < n; i++){
        if (arr1[i] != arr2[i]){
           return false;
        }
    }



    // If all elements were same.
    return true;
}
//Used for overrides, appends new value to TRAVELING and compares to FP
bool shiftTravelingArrayAndCheck(bool result){
    unsigned char i;
    bool temp[] = {false,false,false,false,false,false}; //Temporary array
    for (i = 0; i < (tsize - 1); i++){
        temp[i+1] = TRAVELING[i]; //Makes temp equal to our traveling[] shifted over 1 unit
    }
    temp[0] = result; //The most recent result (temp[0]) is updated to our current value
    for (i = 0; i < tsize; i++){
        TRAVELING[i] = temp[i]; //Set traveling array equal to our temporary array. Effectively, these operations drop the last value of our traveling array, shift remaining values over, and
        //add the new value
    }
    if (areEqual(TRAVELING,FP,6,6)){//Checks if Traveling == {t,f,f,f,f,f} where t = true, f = false. Thus, it checks for first frame to be detected as human in a series of frames
        overThisPer = true;
        return true;
    }
    overThisPer = false;
    return false;
    //Return statements tell us if the conditions were met for an override
}

double average(float *list, unsigned int size){ //Simple func to find average of a float[]
    if (size > 0){
        double averageCounter = 0;
        unsigned int i;
        for (i = 0; i < size; i++){
            averageCounter+=list[i];
        }
        return averageCounter/size;
    }
    return 0;

}
//Simple function to update sensitivity coefficient based on temperature. Used for Mintemp/Avtemp algos, which can likely be deprecated but if it aint broke don't fix it
void updateSensitivityCoefficient(){
    SENSITIVITYCOEFFICIENT = 0.45 + 0.05 * (AVTEMP - 22.0);
    if (SENSITIVITYCOEFFICIENT < 0.45){//Floor of 0.45
        SENSITIVITYCOEFFICIENT = 0.45;
    }
}

//Updates our MINTEMP value
void updateMinimumTemperature(float *listToDo, unsigned int size){
    MINTEMP = 100; //Set to some exorbitant temperature, alternatively set to listToDo[0]
    unsigned int i;
    for (i = 0; i < size; i++){
        if (listToDo[i] < MINTEMP){ //If this pixel is smaller than MINTEMP, MINTEMP = this pixel
            MINTEMP = listToDo[i];
        }
    }
}

//Converts t/f into 1/0 for math purposes
unsigned int boolToInt(bool a){
    if (a){
        return 1;
    }
    return 0;
}

//Our function to update baseline, called STOverweight since it solely relies on SD algorithm. Part of effort to deprecate Mintemp/Avtemp
void updateBaselineSTOverweight(bool r1, float *inputPixelList, unsigned int size){
    if (!r1){//If our standard deviation returns False
        unsigned int i;
        sinceLastFrame += 1;//Checks how many frames have passed since last baseline update
        if (sinceLastFrame >= 10){//If more than 10 frames passed
            for (i = 0; i < size; i++){
               BASELINE[i] = inputPixelList[i];//Update baseline to current pixels
            }
        }
        float temp = calculateSD(BASELINE,64); //Gets the standard deviation of our baseline
        if (BASESTANDEV == 0){//If our Baseline Standard Deviation was not yet calculated, set our BaseSD to our current SD
            BASESTANDEV = temp;
            updThisPer = true;
        }
        //Otherwise, if 10 frames have passed and new calculated standard deviation is within 0.05 degrees of the old SD
        //And the new standard deviation is within a range of [0.3,0.7]
        //Or 50 frames have passed since last standard deviation update
        else if ((temp >= (BASESTANDEV - 0.05) && (temp <= BASESTANDEV + 0.05) && sinceLastFrame >=10 && temp <= 0.7 && temp >= 0.3) || sinceLastFrame >= 50){
            updThisPer = true;
            BASESTANDEV = temp;//Update standard deviation temperature
            sinceLastFrame = 0;//Reset frame count
        } else {
            updThisPer = false;
        }
    } else {
        updThisPer = false;
    }
}

//-----------------------------------

//ALGOS------------------------------

bool medianTemperatureAlgo(float *inputPixelList, unsigned int size){
    float delta = (MAXTEMP - AVTEMP)/7.0 * SENSITIVITYCOEFFICIENT; //Calculate a dynamic delta
    unsigned int count = 0;
    unsigned int i;
    for (i = 0; i<size; i++){
        if (inputPixelList[i] - delta > AVTEMP){ //For each value in our current pixel array, if the value is greater than our Avtemp + our delta, increment count by 1
            count+=1;
        }
    }
    if (count > 5){ //If more than 5 pixels are present with a temp greater than AV + Delta --> Assume human
        return true;
    }
    return false;
}

bool standardDeviationAlgo(float stdev){
    if (BASESTANDEV != 0){ //If our baseline standard deviation has been created
        //If our current SD is >= our standard deviation of our baseline + 0.15 and is greater than 0.65 or is greater than 0.85, return true
        if ((stdev >= (BASESTANDEV + 0.1) && stdev >= 0.6) || stdev >= 0.8){
            return true;
        }
    } else { //Otherwise, check for SD > 0.8
        if (stdev >= 0.75){
            return true;
        }

        //ORIG: +0.15, 0.65, 0.85, 0.8
    }
    return false;
}

bool minimumTemperatureAlgo(float *inputPixelList, unsigned int size){
    float delta = (MAXTEMP - MINTEMP)/2.5 * SENSITIVITYCOEFFICIENT; //Same idea as Avg Temperature, but with Mintemp of all pixels instead of Avtemp
    unsigned int count = 0;
    unsigned int i;
    for (i = 0; i < size; i++){
        if (inputPixelList[i] - MINTEMP > delta){
            count+=1;
        }
    }
    if (count > 6){
        return true;
    }
    return false;
}

bool checkAgainstBaseline(float *inputPixelList, unsigned int size){
    if (BASELINE[0] != 0){ //Basically checks if the baseline was created, otherwise b[0] = 0
        float averageOfBaseline = average(BASELINE, 64);
        float delta = (MAXTEMP - averageOfBaseline)/8.0 * SENSITIVITYCOEFFICIENT; //Same idea as Avtemp/Mintemp but looking at avg temp of baseline and current pixels
        unsigned int count = 0;
        unsigned int i;
        for (i = 0; i < size; i++){
            if (BASELINE[i] < inputPixelList[i] - delta){
                count+=1;
            }
        }
        if (count > 6){
            return true;
        }
        return false;
    }
    return true; //Until baseline is built, return true (if baseline is not built this has 0 impact)
}

//-----------------------------------------------
//DECISION FUNCTIONS------

bool noBaselineDecision(bool medTempRes, bool minTempRes, bool stdevres){

    if (boolToInt(medTempRes) || boolToInt(minTempRes) || boolToInt(stdevres)){ //If any algos return true, assume person
        return true;
    }
    return false;

}
//If 2/4 algos return true, assume human (note this is only run if standev returns false, so realistically 2/3 of the other algos have to return true)
bool baselineWeightedDecision(bool r1, bool r2, bool r3, bool r4){
   if ((boolToInt(r4) + boolToInt(r1)  + boolToInt(r2)  + boolToInt(r3)) >= 2){
        return true;
    }
    return false;
}

//---------
//OVERARCH

bool calculateHumanPresence(float *inputPixelList, unsigned int size){

    //----

    //Update our constants

    AVTEMP = average(inputPixelList,size);
    updateSensitivityCoefficient();
    STDEV = calculateSD(inputPixelList,size);

    updateMinimumTemperature(inputPixelList,size);

    //-----

    //Get Mintemp, Avtemp, Stdev algo results (baseline is checked later if needed)

    //Note: resultOne = mediantemp, resultTwo = minimumtemp, resultFive = standev. These indices were used earlier and i don't want to make a mistake changing them. Just remember.
    bool resultOne = medianTemperatureAlgo(inputPixelList,size);
    bool resultTwo = minimumTemperatureAlgo(inputPixelList,size);
    bool resultFive = standardDeviationAlgo(STDEV);

    //----

    updateBaselineSTOverweight(resultFive,inputPixelList,size);//Update baseline, see function

    if (resultFive || AVTEMP >= 29.5){//If the standard deviation algo returns true or our temperature > 29.5 degs (what the camera sees human as in testing)
        if (!shiftTravelingArrayAndCheck(resultFive)){ //Check if no override
            DBG_PRINT("%f,%f,%f,%d,%d,",AVTEMP,SENSITIVITYCOEFFICIENT,STDEV,boolToInt(updThisPer),boolToInt(overThisPer)); //Print for my python program
            return true;
        }
        DBG_PRINT("%f,%f,%f,%d,%d,",AVTEMP,SENSITIVITYCOEFFICIENT,STDEV,boolToInt(updThisPer),boolToInt(overThisPer)); //If override, return false. Print (see above)
        return false;
    } else {
        if (BASELINE[0] != 0){ //If baseline exists
            bool resultFour = checkAgainstBaseline(inputPixelList,size); //Get baseline result
            bool finres = baselineWeightedDecision(resultOne,resultTwo,resultFour,resultFive); //See function
            if (shiftTravelingArrayAndCheck(finres)){ //If override, return no human
                finres = false;
            }
            DBG_PRINT("%f,%f,%f,%d,%d,",AVTEMP,SENSITIVITYCOEFFICIENT,STDEV,boolToInt(updThisPer),boolToInt(overThisPer)); //Print4Python (remove in final version)

            return finres;
        }
        else{

            bool finres = noBaselineDecision(resultOne,resultTwo,resultFive); //See above
            if (shiftTravelingArrayAndCheck(finres)){ //If override, no human
                finres = false;
            }

            return finres;
        }
    }

}

