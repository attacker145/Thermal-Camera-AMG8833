/*
 * AK9753A.c
 *
 *  Created on: Jun 13, 2018
 *      Author: rchak
 */

#include <string.h>

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
//#include "serial_wifi.h" //not good
//#include "tmp006drv.h"
//#include "bma222drv.h"

#include "gpio_if.h"

// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/common.h>

// JSON Parser
#include "jsmn.h"

#include <math.h>


//extern int GetRegisterValue_(unsigned char ucRegAddr, unsigned char *pucRegValue);
//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define FAILURE                 -1
#define SUCCESS                 0
#define RET_IF_ERR(Func)        {int iRetVal = (Func); \
                                 if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

//****************************************************************************
// AK9753AE: CAD1 = 0, CAD0 = 0 Slave address = 0x64; 0xC4 - write, 0xC5 - read
//****************************************************************************
#define AK9753AE_DEV_ADDR		0x64 // - 110 0100 R/W
//Register

#define AK9753AE_S_RESET		0x1D
#define AK9753AE_Mode_FC		0x1C
#define AK9753AE_INT			0x1B
#define INT_Status				0x04
#define Status1					0x05
#define Status2					0x10
#define IR1L					0x06
#define IR1H					0x07
#define IR2L					0x08
#define IR2H					0x09
#define IR3L					0x0A
#define IR3H					0x0B
#define IR4L					0x0C
#define IR4H					0x0D
#define INT_TSL					0x0E
#define INT_TSH					0x0F

//Register addresses
#define AK975X_WIA2 			0x01 // Device ID
#define AK975X_INFO1 			0x02 // Device type
#define SENSORVERSION_AK9750 	0x00
#define SENSORVERSION_AK9753 	0x01
#define AK975X_ST1 				0x05
//#define AK975X_IR1 				0x06
//#define AK975X_IR2 				0x08
//#define AK975X_IR3 				0x0A
//#define AK975X_IR4 				0x0C
#define AK975X_TMPL 			0x0E
#define AK975X_TMPH 			0x0F
#define AK975X_ST2 				0x10 //Dummy register
#define AK975X_ETH13H 			0x11
#define AK975X_ETH13L 			0x13
#define AK975X_ETH24H 			0x15
#define AK975X_ETH24L 			0x17
#define AK975X_EHYS13 			0x19
#define AK975X_EHYS24 			0x1A
#define AK975X_EINTEN 			0x1B
#define AK975X_ECNTL1 			0x1C
#define AK975X_CNTL2 			0x19

//Valid sensor modes - Register ECNTL1
#define AK975X_MODE_STANDBY 0b000
#define AK975X_MODE_EEPROM_ACCESS 0b001
#define AK975X_MODE_SINGLE_SHOT 0b010
#define AK975X_MODE_0 0b100
#define AK975X_MODE_1 0b101
#define AK975X_MODE_2 0b110
#define AK975X_MODE_3 0b111

//Valid digital filter cutoff frequencies
#define AK975X_FREQ_0_3HZ 0b000
#define AK975X_FREQ_0_6HZ 0b001
#define AK975X_FREQ_1_1HZ 0b010
#define AK975X_FREQ_2_2HZ 0b011
#define AK975X_FREQ_4_4HZ 0b100
#define AK975X_FREQ_8_8HZ 0b101

#define SH_GPIO_9           9       // Red LED
unsigned int uiGPIOPort;//02/17/2017
unsigned char pucGPIOPin;//02/17/2017


int readRegister16_AK9753(unsigned char ucRegAddr, unsigned char *pucRegValue)
{
    //
    // Invoke the readfrom  API to get the required byte
    // ucRegAddr - Register 1 byte address
	// 1 - 1 byte address size
    if(I2C_IF_ReadFrom(AK9753AE_DEV_ADDR, &ucRegAddr, 1, pucRegValue, 2) != 0)
    {
        DBG_PRINT("I2C readfrom failed\n\r");
        return FAILURE;
    }

    return SUCCESS;
}

//****************************************************************************
//  https://www.sparkfun.com/products/14349
//! \param ucRegAddr is the start offset register address -
//! \param pucBlkData is the pointer to the data value store - array for the received data
//! \param ucBlkDataSz is the size of data to be read - number of bytes
//!
//! This function
//!    1. Returns the data values in the specified store
//!
//! \return 0: Success, < 0: Failure.
//****************************************************************************
// example: AK9753A_BlockRead (IR1L, (unsigned char *)&cIR1L, sizeof(cIR1L)))

int AK9753A_BlockRead (unsigned char ucRegAddr, unsigned char *Rbuffer, unsigned char ucBlkDataSz)
{

    if(I2C_IF_ReadFrom(AK9753AE_DEV_ADDR, &ucRegAddr, 1, Rbuffer, ucBlkDataSz) != 0)
    {
        DBG_PRINT("AK9753A I2C readfrom failed\n");
        return FAILURE;
    }

    return SUCCESS;
}


//****************************************************************************
//
//! Returns the value in the specified register
//!
//! \param ucRegAddr is the offset register address
//! \param pucRegValue is the pointer to the register value store
//!
//! This function
//!    1. Returns the value in the specified register
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int GetRegisterValue_AK9753(unsigned char ucRegAddr, unsigned char *pucRegValue)
{
    //
    // Invoke the readfrom  API to get the required byte
    //
    if(I2C_IF_ReadFrom(AK9753AE_DEV_ADDR, &ucRegAddr, 1,
                   pucRegValue, 1) != 0)
    {
        DBG_PRINT("I2C readfrom failed\n\r");
        GPIO_IF_GetPortNPin(SH_GPIO_9,&uiGPIOPort,&pucGPIOPin); // Computes port and pin number from the GPIO number
        GPIO_IF_Set(SH_GPIO_9,uiGPIOPort,pucGPIOPin,0);
        return FAILURE;
    }

    return SUCCESS;
}

//****************************************************************************
//
//! Sets the value in the specified register
//!
//! \param ucRegAddr is the offset register address
//! \param ucRegValue is the register value to be set
//!
//! This function
//!    1. Returns the value in the specified register
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int SetRegisterValue_AK9753(unsigned char ucRegAddr, unsigned char ucRegValue)
{
    unsigned char ucData[2];
    //
    // Select the register to be written followed by the value.
    //
    ucData[0] = ucRegAddr;
    ucData[1] = ucRegValue;
    //
    // Initiate the I2C write: Send 2 bytes - Address and Data over the I2C bus
    //
    if(I2C_IF_Write(AK9753AE_DEV_ADDR,ucData,2,1) == 0)	// DevAddr Received by I2C:
    {
        return SUCCESS;
    }
    else
    {
        DBG_PRINT("I2C write failed\n\r");
    }

    return FAILURE;
}

//Set the mode. Continuous mode 0 is favored: setMode(AK975X_MODE_0);
void AK975XsetMode(unsigned char mode)

{
	unsigned char currentSettings;

	if (mode > AK975X_MODE_3) mode = AK975X_MODE_0; //Default to mode 0
	if (mode == 0b011) mode = AK975X_MODE_0; //0x03 is prohibited

	//Read, mask set, write
	//byte currentSettings = readRegister(AK975X_ECNTL1);
	GetRegisterValue_AK9753((unsigned char) AK975X_ECNTL1, &currentSettings);
	UART_PRINT("\n\rMode reg hist: 0x%x", currentSettings);
	currentSettings &= 0b11111000; //Clear Mode bits
	currentSettings |= mode;
	//UART_PRINT("\n\rMode Data to load 0x%x", currentSettings);
	//writeRegister(AK975X_ECNTL1, currentSettings);
	SetRegisterValue_AK9753((unsigned char) AK975X_ECNTL1, currentSettings);
	GetRegisterValue_AK9753((unsigned char) AK975X_ECNTL1, &currentSettings);
	UART_PRINT("\n\rMode reg set: 0x%x", currentSettings);
}


void setCutoffFrequency(uint8_t frequency){

	unsigned char currentSettings;

	if (frequency > 0b101){
		frequency = 0; //Default to 0.3Hz
	}
	//Read, mask set, write
	//byte currentSettings = readRegister(AK975X_ECNTL1);
	GetRegisterValue_AK9753((unsigned char) AK975X_ECNTL1, &currentSettings);
	UART_PRINT("\n\rFreq reg hist: 0x%x", currentSettings);
	currentSettings &= 0b11000111; //Clear EFC bits
	currentSettings |= (frequency << 3); //Mask in
	//writeRegister(AK975X_ECNTL1, currentSettings); //Write
	SetRegisterValue_AK9753((unsigned char) AK975X_ECNTL1, currentSettings);
	GetRegisterValue_AK9753((unsigned char) AK975X_ECNTL1, &currentSettings);
	UART_PRINT("\n\rFreq reg set: 0x%x", currentSettings);
}

void ECNTL1_rst (void){

	unsigned char currentSettings = 0;

	SetRegisterValue_AK9753((unsigned char) AK975X_ECNTL1, currentSettings);
	GetRegisterValue_AK9753((unsigned char) AK975X_ECNTL1, &currentSettings);
	UART_PRINT("\n\rMode and Fc reg: 0x%x", currentSettings);
}



void SoftReset(void){

	unsigned char data;
	data = 0x01;
	SetRegisterValue_AK9753((unsigned char)0x1D, data);
}

void IntrrptSourceSet(void){

	unsigned char data;
	unsigned char currentSettings;

	data = 0x01;

	//Read, mask set, write
	//byte currentSettings = readRegister(AK975X_ECNTL1);
	GetRegisterValue_AK9753((unsigned char) 0x1B, &currentSettings);
	//UART_PRINT("\n\rINT reg hist: 0x%x", currentSettings);
	currentSettings &= 0b11100000; //Clear IR13HI, IR13LI, IR24HI, IR24LI, DRI
	currentSettings |= data; // Set DRI
	data = currentSettings; //save currentSettings to data


	SetRegisterValue_AK9753((unsigned char)0x1B, currentSettings);
	GetRegisterValue_AK9753((unsigned char)0x1B, &currentSettings);

	if (currentSettings == data){
		UART_PRINT("\n\rIntrrpt Source set: 0x%x", currentSettings);
	}
	else{
		UART_PRINT("\n\rIntrrpt Source err 0x%x", currentSettings);
	}
}

/*
 * Function call:
 * AK9753AReadData((unsigned char) &IR1L_, (unsigned char) &IR1H_,
		(unsigned char) &IR2L_, (unsigned char) &IR2H_,
		(unsigned char) &IR3L_, (unsigned char) &IR3H_,
		(unsigned char) &IR4L_, (unsigned char) &IR4H_)
 */
//Declared data vars are outputs
int AK9753AReadData(unsigned char *IR1L_, unsigned char *IR1H_,
		unsigned char *IR2L_, unsigned char *IR2H_,
		unsigned char *IR3L_, unsigned char *IR3H_,
		unsigned char *IR4L_, unsigned char *IR4H_)
{
	unsigned char cIR1L;
	unsigned char cIR1H;
	unsigned char cIR2L;
	unsigned char cIR2H;
	unsigned char cIR3L;
	unsigned char cIR3H;
	unsigned char cIR4L;
	unsigned char cIR4H;
	unsigned char INTstat;
	unsigned char stat;

	GetRegisterValue_AK9753((unsigned char)0x04, &INTstat);//INT Status
	//UART_PRINT("\n\rINT status: 0x%x", INTstat);
	GetRegisterValue_AK9753((unsigned char)0x05, &stat);
	//UART_PRINT("\n\rStatus1: 0x%x", stat);
	stat &= 0b00000001;
	if (stat == 0x01){
		//UART_PRINT("\n\rIR Data is ready");
	    UART_PRINT(", IR");
	}else{
		//UART_PRINT("\n\rIR Data is not ready");
	    UART_PRINT(", NIR");
	}

	//                           (Address of reg to read, var to store data, size of var)
    RET_IF_ERR(AK9753A_BlockRead (IR1L, &cIR1L, sizeof(cIR1L)));
	RET_IF_ERR(AK9753A_BlockRead (IR1H, &cIR1H, sizeof(cIR1H)));
    *IR1L_ = cIR1L; // get value of cIR1L. *IR1L has current data
    *IR1H_ = cIR1H; // get value of cIR1H

    RET_IF_ERR(AK9753A_BlockRead (IR2L, &cIR2L, sizeof(cIR2L)));
	RET_IF_ERR(AK9753A_BlockRead (IR2H, &cIR2H, sizeof(cIR2H)));
    *IR2L_ = cIR2L; // get value of cIR2L
    *IR2H_ = cIR2H; // get value of cIR2H

    RET_IF_ERR(AK9753A_BlockRead (IR3L, &cIR3L, sizeof(cIR3L)));
	RET_IF_ERR(AK9753A_BlockRead (IR3H, &cIR3H, sizeof(cIR3H)));
    *IR3L_ = cIR3L; // get value of cIR3L
    *IR3H_ = cIR3H; // get value of cIR3H

    RET_IF_ERR(AK9753A_BlockRead (IR4L, &cIR4L, sizeof(cIR4L)));
	RET_IF_ERR(AK9753A_BlockRead (IR4H, &cIR4H, sizeof(cIR4H)));
    *IR4L_ = cIR4L; // get value of cIR4L
    *IR4H_ = cIR4H; // get value of cIR4H
    unsigned int IR1 = 0;
	IR1 = cIR1H;																			//*
	IR1 = IR1 << 8;																			//*
	IR1 = IR1 + (unsigned int)cIR1L;
    //UART_PRINT("\n\rIR1L: %d", cIR1L);
    //UART_PRINT("\n\rIR1H: %d", cIR1H);
	//UART_PRINT("\n\rIR1: %d", IR1);

	IR1 = cIR2H;																			//*
	IR1 = IR1 << 8;																			//*
	IR1 = IR1 + (unsigned int)cIR2L;
    //UART_PRINT("\n\rIR2L: %d", cIR2L);
    //UART_PRINT("\n\rIR2H: %d", cIR2H);
	//UART_PRINT("\n\rIR2: %d", IR1);

	IR1 = cIR3H;																			//*
	IR1 = IR1 << 8;																			//*
	IR1 = IR1 + (unsigned int)cIR3L;
    //UART_PRINT("\n\rIR3L: %d", cIR3L);
    //UART_PRINT("\n\rIR3H: %d", cIR3H);
	//UART_PRINT("\n\rIR3: %d", IR1);

	IR1 = cIR4H;																			//*
	IR1 = IR1 << 8;																			//*
	IR1 = IR1 + (unsigned int)cIR4L;
    //UART_PRINT("\n\rIR4L: %d", cIR4L);
    //UART_PRINT("\n\rIR4H: %d", cIR4H);
	//UART_PRINT("\n\rIR4: %d", IR1);

    //UART_PRINT("\n\r");
    GetRegisterValue_AK9753((unsigned char)0x10, &INTstat);
    return SUCCESS;
}

void AK9753_ID (void){

	unsigned char deviceID;
	unsigned char sensorType;

	GetRegisterValue_AK9753((unsigned char) AK975X_WIA2, &deviceID);
	//AK9753A_BlockRead (WIA2, (unsigned char *)&cID, sizeof(cID));
	UART_PRINT("\n\rAK9753 ID: %d", deviceID);
	GetRegisterValue_AK9753((unsigned char) AK975X_INFO1, &sensorType);

	if (deviceID != 0x13) //Device ID should be 0x13
		UART_PRINT("\n\rAK9753 ID invalid");

	  if (sensorType == SENSORVERSION_AK9750)
		  UART_PRINT("\n\rAK9750 Online!");
	  if (sensorType == SENSORVERSION_AK9753) {
		  UART_PRINT("\n\rAK9753 Online!");
	  }

}


float getTemperature_AK9753(unsigned char *sgn){
	unsigned char cTL;
	unsigned char cTH;
	unsigned char INTstat;
	unsigned char stat;

	GetRegisterValue_AK9753((unsigned char)0x04, &INTstat);//INT Status
	//UART_PRINT("\n\rINT status: 0x%x", INTstat);
	GetRegisterValue_AK9753((unsigned char)0x05, &stat);
	//UART_PRINT("\n\rStatus1: 0x%x", stat);

	//unsigned char sgn = '+';
	int value;
   	RET_IF_ERR(AK9753A_BlockRead (AK975X_TMPL, &cTL, sizeof(cTL)));
	RET_IF_ERR(AK9753A_BlockRead (AK975X_TMPH, &cTH, sizeof(cTH)));
	value = cTH;
	value = value << 8;
	if ((value & 0x80) == 0x80){
		*sgn = '-';
	}
	else{
		*sgn = '+';
	}
	value = value + (int)cTL;
	value >>= 6;
	float temperature = 26.75 + (value * 0.125);// 0 = 26.75C
	GetRegisterValue_AK9753((unsigned char)0x10, &INTstat);
	return (temperature);
}
