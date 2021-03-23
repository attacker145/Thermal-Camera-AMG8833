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
#define NO_OF_SAMPLES       4096
//#define NO_OF_SAMPLES       3072

extern void hexdec_long( uint32_t count );
extern char Rx_buf;
extern volatile tBoolean g_bFeedWatchdog;
extern void SendUART1_UC(unsigned char *str, unsigned char len);
extern void MessageUART1(const char *str);
extern void SendUART0_UC(unsigned char *str, unsigned char len);

void init_adc (void){

    MAP_PinTypeADC(PIN_60,PIN_MODE_255);// Pinmux for the selected ADC input pin

    MAP_ADCTimerConfig(ADC_BASE,2^17);// Configure ADC timer which is used to timestamp the ADC data samples

    MAP_ADCTimerEnable(ADC_BASE);// Enable ADC timer which is used to timestamp the ADC data samples

    MAP_ADCEnable(ADC_BASE);// Enable ADC module

    MAP_ADCChannelEnable(ADC_BASE, ADC_CH_3);// Enable ADC channel
}


float adc_read (unsigned long* pulAdcSamples, unsigned int len){

	float ADCsum;
	unsigned long ulSample, temp;
	unsigned char sendbyte[1];
	unsigned int uiIndex;
	//uint64_t run;
	//unsigned long pulAdcSamples[4096];//ADC

	ulSample = MAP_ADCFIFORead(ADC_BASE, ADC_CH_3);
	while (((((float)((ulSample >> 2 ) & 0x0FFF))*1.4)/4096) < 0.6){//Wait for the spike
	    ulSample = MAP_ADCFIFORead(ADC_BASE, ADC_CH_3);
	    asm(" NOP");
	}

	//UART_PRINT("Mark");
	uiIndex = 0;
	while(uiIndex < len)
    {
        if(MAP_ADCFIFOLvlGet(ADC_BASE, ADC_CH_3))
        {
            g_bFeedWatchdog = true;
            ulSample = MAP_ADCFIFORead(ADC_BASE, ADC_CH_3); //Fixed sampling interval of 16 µs per channel
            //*pulAdcSamples = ulSample;            //2048 * 0.000016 = 0.032768 s = 32.768 ms
            //pulAdcSamples ++;
            //*(pulAdcSamples + uiIndex) = ((((float)((ulSample >> 2 ) & 0x0FFF))*1.4)/4096);
            *(pulAdcSamples + uiIndex) = ulSample;
            uiIndex++;
        }
    }

    uiIndex = 0;
    while(uiIndex < len)
    {
        g_bFeedWatchdog = true;
        temp = *(pulAdcSamples + uiIndex);
        //UART_PRINT(", V= %f",(((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4)/4096);
        //UART_PRINT(" %f \n",(((float)((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF))*1.4)/4096);

        //UART_PRINT(" %f \n",(((float)((temp >> 2 ) & 0x0FFF))*1.4)/4096);

        //UART_PRINT(" %f \n",(((float)(temp & 0x0FFF))*1.4)/16384);//X*163840/14=


        uiIndex++;
    }

    uiIndex = 0;
    while(uiIndex < len)
    {
        g_bFeedWatchdog = true;
        temp = *(pulAdcSamples + uiIndex);
        sendbyte[0] = (unsigned char) ((temp >> 8) & 0x000000000000001F);//MSB
        SendUART1_UC(sendbyte, 1);
        temp = *(pulAdcSamples + uiIndex);
        sendbyte[0] = (unsigned char) (temp & 0x00000000000000FF);//LSB little endian
        SendUART1_UC(sendbyte, 1);
        uiIndex++;
    }

    /*
	uiIndex = 0;
    while(pulAdcSamples < NO_OF_SAMPLES) // Print out ADC samples UART1
    {
        g_bFeedWatchdog = true;

        sendbyte[0] = (unsigned char) ((*pulAdcSamples >> 8) & 0x00000000000000FF);//MSB
        SendUART0_UC(sendbyte, 1);
        sendbyte[0] = (unsigned char) (*pulAdcSamples & 0x00000000000000FF);//LSB little endian
        SendUART0_UC(sendbyte, 1);

        pulAdcSamples++;
    }
*/
    //pulAdcSamples = 4095;
    ADCsum = (float)(((((*pulAdcSamples >> 2 ) & 0x0FFF))*1.4)/4096);
    return ADCsum;
}






