// Standard includes
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Driverlib includes
#include "utils.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_adc.h"
#include "hw_ints.h"
#include "hw_gprcm.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "uart.h"
#include "pinmux.h"
#include "pin.h"
#include "adc.h"
#include "common.h"

//#include "adc_userinput.h"
#include "uart_if.h"
#include "gpio.h"

//#define NO_OF_SAMPLES 		128
//#define NO_OF_SAMPLES       1
#define NO_OF_SAMPLES       128
unsigned long pulAdcSamples[4096];

void adc (void){

	unsigned long ulSample;
	unsigned int  uiChannel;
	unsigned int  uiIndex=0;

	while(uiIndex < NO_OF_SAMPLES + 4)
	{
		if(MAP_ADCFIFOLvlGet(ADC_BASE, uiChannel))
	    {
			ulSample = MAP_ADCFIFORead(ADC_BASE, uiChannel);

	        pulAdcSamples[uiIndex++] = ulSample;			// Fill data sample buffer
	    }
	}

}


void adcPrint (void){
	unsigned int  uiIndex=0;
	//
	// Print out ADC samples
	//
	//float sum = 0;

	while(uiIndex < NO_OF_SAMPLES)
	{
		//UART_PRINT("\n\rVoltage is %f\n\r",(((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4*2.279)/4096);
	    //uiIndex++;
	    //sum += (((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4*2.279)/4096;
	}


	//UART_PRINT("\n\rVoltage is %f\n\r",((pulAdcSamples[4] >> 2 ) & 0x0FFF)*1.4/4096);
	                //UART_PRINT("\n\r");
	                //UART_PRINT("\n\rAverage Voltage Level %f\n\r", (float)(sum/NO_OF_SAMPLES));
}



void init_adc (void){
    //
    // Pinmux for the selected ADC input pin
    //
    MAP_PinTypeADC(PIN_60,PIN_MODE_255);

    //
    // Configure ADC timer which is used to timestamp the ADC data samples
    //
    MAP_ADCTimerConfig(ADC_BASE,2^17);

    //
    // Enable ADC timer which is used to timestamp the ADC data samples
    //
    MAP_ADCTimerEnable(ADC_BASE);

    //
    // Enable ADC module
    //
    MAP_ADCEnable(ADC_BASE);

    //
    // Enable ADC channel
    //

    MAP_ADCChannelEnable(ADC_BASE, ADC_CH_3);
}

float adc_read (void){

	unsigned int  uiIndex=0;
	float ADCsum;
	unsigned long ulSample;
	//unsigned long pulAdcSamples[4096];//ADC

    //
    // Pinmux for the selected ADC input pin
    //
	MAP_PinTypeADC(PIN_60,PIN_MODE_255);

	//
	// Configure ADC timer which is used to timestamp the ADC data samples
	//
	MAP_ADCTimerConfig(ADC_BASE,2^17);

	//
	// Enable ADC timer which is used to timestamp the ADC data samples
	//
	MAP_ADCTimerEnable(ADC_BASE);

	//
	// Enable ADC module
	//
	MAP_ADCEnable(ADC_BASE);

    //
    // Enable ADC channel
    //

    MAP_ADCChannelEnable(ADC_BASE, ADC_CH_3);

    while(uiIndex < NO_OF_SAMPLES + 4)
    {
        if(MAP_ADCFIFOLvlGet(ADC_BASE, ADC_CH_3))
        {
            ulSample = MAP_ADCFIFORead(ADC_BASE, ADC_CH_3);
            pulAdcSamples[uiIndex++] = ulSample;
            //MAP_UtilsDelay(16000);//delay 0.001s 11/6/2019
        }


    }

   	MAP_ADCChannelDisable(ADC_BASE, ADC_CH_3);


    uiIndex = 0;

    //UART_PRINT("\n\rTotal no of 32 bit ADC data printed :4096 \n\r");
    //UART_PRINT("\n\rADC data format:\n\r");
    //UART_PRINT("\n\rbits[13:2] : ADC sample\n\r");
    //UART_PRINT("\n\rbits[31:14]: Time stamp of ADC sample \n\r");

    //
    // Print out ADC samples
    //

    while(uiIndex < NO_OF_SAMPLES)
    {
        //UART_PRINT(", V= %f",(((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4)/4096);
        UART_PRINT(" %f",(((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4)/4096);
        UART_PRINT("\n\r");
        uiIndex++;
    }

    ADCsum = (float)(((((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4)/4096);
    return ADCsum;
    //return pulAdcSamples[5];
}






