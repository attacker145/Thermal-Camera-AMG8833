//*****************************************************************************
// pinmux.c
//
// configure the device pins for different peripheral signals
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

// This file was automatically generated on 7/21/2014 at 3:06:20 PM
// by TI PinMux version 3.0.334
//
//*****************************************************************************

#include "pinmux.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_gpio.h"
#include "pin.h"
#include "rom.h"
#include "rom_map.h"
#include "gpio.h"
#include "prcm.h"
#include "hw_ints.h"
#include "Interrupt.h"
#include "common.h"
#include "rom_map.h"
#include "uart_if.h"
#include "utils.h"


#define SH_GPIO_6           6       /* P61 - Light Sensor */
#define SH_GPIO_9           9       // Red LED

extern unsigned int uiGPIOPort;//02/17/2017
extern unsigned char pucGPIOPin;//02/17/2017
extern unsigned char Door;
extern unsigned char door_flg;

extern void GPIO_IF_GetPortNPin(unsigned char ucPin,unsigned int *puiGPIOPort,unsigned char *pucGPIOPin);
extern unsigned char GPIO_IF_Get(unsigned char ucPin, unsigned int uiGPIOPort, unsigned char ucGPIOPin);
extern void GPIO_IF_Set(unsigned char ucPin, unsigned int uiGPIOPort, unsigned char ucGPIOPin, unsigned char ucGPIOValue);
extern void timerA0_start(unsigned long ms);
extern void door_open(void);
#define alarm_on 1

//*****************************************************************************
void
PinMuxConfig(void)
{

	/*
	 * Original pinout
	//
    // Enable Peripheral Clocks 
    //
    MAP_PRCMPeripheralClkEnable(PRCM_UARTA0, PRCM_RUN_MODE_CLK);

    //
    // Configure PIN_55 for UART0 UART0_TX
    //
    MAP_PinTypeUART(PIN_55, PIN_MODE_3);

    //
    // Configure PIN_57 for UART0 UART0_RX
    //
    MAP_PinTypeUART(PIN_57, PIN_MODE_3);
    */
	//
	// Set unused pins to PIN_MODE_0 with the exception of JTAG pins 16,17,19,20
	//

	    //
	    // Enable Peripheral Clocks
	    //
    /*
	    PRCMPeripheralClkEnable(PRCM_ADC, PRCM_RUN_MODE_CLK);
	    //PRCMPeripheralClkEnable(PRCM_TIMERA0, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_TIMERA2, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_TIMERA3, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_GPIOA0, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_GPIOA3, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_UARTA0, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
	    PRCMPeripheralClkEnable(PRCM_UARTA1, PRCM_RUN_MODE_CLK);
*/
	    //
	    // Configure PIN_04 for TimerCP4 GT_CCP04
	    //
	    //PinTypeTimer(PIN_04, PIN_MODE_12);
	    PinTypeGPIO(PIN_04, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA1_BASE, 0x20, GPIO_DIR_MODE_IN);

	    //
	    // Configure PIN_63 for TimerCP6 GT_CCP06
	    //
	    PinTypeTimer(PIN_63, PIN_MODE_12);

	    //
	    // Configure PIN_50 for GPIO Input
	    //
	    PinTypeGPIO(PIN_50, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA0_BASE, 0x1, GPIO_DIR_MODE_IN);	// WiFi EN
	    // Activate internal pull-down resistor on PIN_50
	    //MAP_PinConfigSet(PIN_50, PIN_STRENGTH_4MA, PIN_TYPE_STD_PD);
	    MAP_PinConfigSet(PIN_50, PIN_STRENGTH_4MA, PIN_TYPE_STD_PU);

	    //
	    // Configure PIN_60 for GPIO Output
	    //
	    //PinTypeGPIO(PIN_60, PIN_MODE_0, false);
	    //GPIODirModeSet(GPIOA0_BASE, 0x20, GPIO_DIR_MODE_IN);	// ADC
//#######################################################################################################################
	    //
	    // Configure PIN_61 for GPIO Input (Motion Sensor)
	    //
	    PinTypeGPIO(PIN_61, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA0_BASE, 0x40, GPIO_DIR_MODE_IN);//Changed from output 041218. Will be used to read light Sensor
	    MAP_PinConfigSet(PIN_61, PIN_STRENGTH_2MA, PIN_TYPE_STD_PU);//
/*
	    GPIOIntTypeSet(GPIOA0_BASE, GPIO_PIN_6, GPIO_RISING_EDGE);
	    IntPrioritySet(INT_GPIOA0, INT_PRIORITY_LVL_1);
	    GPIOIntRegister(GPIOA0_BASE, &door_open);
	    GPIOIntClear(GPIOA0_BASE, GPIO_PIN_6);
	    //GPIOIntEnable(GPIOA0_BASE, GPIO_PIN_6);

	    //GPIOIntClear(GPIOA0_BASE, GPIO_PIN_6);
	    IntPendClear(INT_GPIOA0);
	    IntEnable(INT_GPIOA0);
	    GPIOIntEnable(GPIOA0_BASE, GPIO_PIN_6);
*/
	    //########################################################################################################################
	    //
	    // Configure PIN_62 for GPIO7 Output
	    //
	    PinTypeGPIO(PIN_62, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA0_BASE, 0x80, GPIO_DIR_MODE_OUT);//BLE data ready
	    //GPIOIntTypeSet(GPIOA2_BASE, GPIO_PIN_7, GPIO_FALLING_EDGE);
	    //IntPrioritySet(INT_GPIOA2, INT_PRIORITY_LVL_1);
	    //GPIOIntRegister(GPIOA2_BASE, &OnButtonPress);

	    //
	    // Configure PIN_64 for GPIO9 Output Red LED
	    //
	    PinTypeGPIO(PIN_64, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA1_BASE, 0x2, GPIO_DIR_MODE_OUT);

	    //
	    // Configure PIN_03 for GPIO Input AK9753 INT
	    //
	    PinTypeGPIO(PIN_03, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA1_BASE, 0x10, GPIO_DIR_MODE_IN);

	    //
	    // Configure PIN_15 for GPIO Input SW2
	    //
	    PinTypeGPIO(PIN_15, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA2_BASE, 0x40, GPIO_DIR_MODE_IN);//SW2

	    //
	    // Configure PIN_18 for GPIO Output. GPIO28
	    //
	    PinTypeGPIO(PIN_18, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA3_BASE, 0x10, GPIO_DIR_MODE_OUT);//Sonic #2 DR

	    //
	    // Configure PIN_20 for GPIO Input
	    //
	    PinTypeGPIO(PIN_20, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA3_BASE, 0x20, GPIO_DIR_MODE_IN);

	    //
	    // Configure PIN_21 for GPIO Input
	    //
	    PinTypeGPIO(PIN_21, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA3_BASE, 0x2, GPIO_DIR_MODE_IN);

	    //
	    // Configure PIN_53 for GPIO Output
	    //
	    //PinTypeGPIO(PIN_53, PIN_MODE_0, false);
	    //GPIODirModeSet(GPIOA3_BASE, 0x40, GPIO_DIR_MODE_OUT);//I2C BUS power EN GPIO_30

	    //
	    // Configure PIN_08 for SPI0 GSPI_CS
	    //
	    PinTypeSPI(PIN_08, PIN_MODE_7);

	    //
	    // Configure PIN_05 for SPI0 GSPI_CLK
	    //
	    PinTypeSPI(PIN_05, PIN_MODE_7);

	    //
	    // Configure PIN_06 for SPI0 GSPI_MISO
	    //
	    PinTypeSPI(PIN_06, PIN_MODE_7);

	    //
	    // Configure PIN_07 for SPI0 GSPI_MOSI
	    //
	    PinTypeSPI(PIN_07, PIN_MODE_7);

	    //
	    // Configure PIN_55 for UART0 UART0_TX
	    //
	    PinTypeUART(PIN_55, PIN_MODE_3);

	    //
	    // Configure PIN_57 for UART0 UART0_RX
	    //
	    PinTypeUART(PIN_57, PIN_MODE_3);


//#################################################################################

	    //
    	// Configure PIN_58 for UART1 UART1_TX
    	//
   	   	PinTypeUART(PIN_58, PIN_MODE_6);

    	//
    	// Configure PIN_59 for UART1 UART1_RX
    	//
    	PinTypeUART(PIN_59, PIN_MODE_6);

    	/*
	    //
	    // Configure PIN_58 for GPIO Input
	    //
	    PinTypeGPIO(PIN_58, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA0_BASE, 0x8, GPIO_DIR_MODE_IN);	//Light Sensor

	    //
	    // Configure PIN_59 for ADC0 ADC_CH2
	    //
	    PinTypeADC(PIN_59, PIN_MODE_255);
	    */
//###############################################################################




	    //
	    // Configure PIN_16 for GPIO Output
	    //
	    PinTypeGPIO(PIN_16, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA2_BASE, 0x80, GPIO_DIR_MODE_OUT);

	    //
	    // Configure PIN_17 for GPIO Input
	    //
	    PinTypeGPIO(PIN_17, PIN_MODE_0, false);
	    GPIODirModeSet(GPIOA3_BASE, 0x1, GPIO_DIR_MODE_IN);


	    //
	    // Configure PIN_01 for I2C0 I2C_SCL
	    //
	    PinTypeI2C(PIN_01, PIN_MODE_1);

	    //
	    // Configure PIN_02 for I2C0 I2C_SDA
	    //
	    PinTypeI2C(PIN_02, PIN_MODE_1);


	    //
	    // Configure PIN_60 for ADC0 ADC_CH3
	    //
	    //PinTypeADC(PIN_60, PIN_MODE_255);

}
