/*
 * AMG88xx.c
 *
 *  Created on: Oct 26, 2020
 *      Author: Roman.Chak
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

#include "gpio_if.h"

// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/common.h>


//extern int GetRegisterValue_(unsigned char ucRegAddr, unsigned char *pucRegValue);
//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define FAILURE                 -1
#define SUCCESS                 0
#define RET_IF_ERR(Func)        {int iRetVal = (Func); \
                                 if (SUCCESS != iRetVal) \
                                     return  iRetVal;}


//extern int GetRegisterValue_(unsigned char ucRegAddr, unsigned char *pucRegValue);
//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define FAILURE                 -1
#define SUCCESS                 0
#define RET_IF_ERR(Func)        {int iRetVal = (Func); \
                                 if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

#define SH_GPIO_9           9       // Red LED
unsigned int uiGPIOPort;//02/17/2017
unsigned char pucGPIOPin;//02/17/2017

//Registers
#define POWER_CONTROL_REGISTER        0x00
#define RESET_REGISTER                0x01
#define FRAMERATE_REGISTER            0x02
#define INT_CONTROL_REGISTER          0x03
#define STATUS_REGISTER               0x04
#define STATUS_CLEAR_REGISTER         0x05
#define AVERAGE_REGISTER              0x07
#define INT_LEVEL_REGISTER_UPPER_LSB  0x08
#define INT_LEVEL_REGISTER_UPPER_MSB  0x09
#define INT_LEVEL_REGISTER_LOWER_LSB  0x0A
#define INT_LEVEL_REGISTER_LOWER_MSB  0x0B
#define INT_LEVEL_REGISTER_HYST_LSB   0x0C
#define INT_LEVEL_REGISTER_HYST_MSB   0x0D
#define THERMISTOR_REGISTER_LSB       0x0E
#define THERMISTOR_REGISTER_MSB       0x0F
#define INT_TABLE_REGISTER_INT0       0x10
#define RESERVED_AVERAGE_REGISTER     0x1F
#define TEMPERATURE_REGISTER_START    0x80

/*=========================================================================
    I2C ADDRESS/BITS
    -----------------------------------------------------------------------*/
#define AMG88xx_ADDRESS (0x69)

#define AMG88xx_PIXEL_ARRAY_SIZE 64
#define AMG88xx_PIXEL_TEMP_CONVERSION .25
#define AMG88xx_THERMISTOR_CONVERSION .0625

int readRegister16_AMG88xx(unsigned char ucRegAddr, unsigned char *pucRegValue)
{
    //
    // Invoke the readfrom  API to get the required byte
    // ucRegAddr - Register 1 byte address
    // 1 - 1 byte address size
    if(I2C_IF_ReadFrom(AMG88xx_ADDRESS, &ucRegAddr, 1, pucRegValue, 2) != 0)
    {
        DBG_PRINT("I2C readfrom failed\n");
        return FAILURE;
    }

    return SUCCESS;
}

//****************************************************************************
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

int AMG88xx_BlockRead (unsigned char ucRegAddr, unsigned char *Rbuffer, unsigned char ucBlkDataSz)
{

    if(I2C_IF_ReadFrom(AMG88xx_ADDRESS, &ucRegAddr, 1, Rbuffer, ucBlkDataSz) != 0)
    {
        DBG_PRINT("AMG88xx I2C readfrom failed\n");
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
int GetRegisterValue_AMG88xx(unsigned char ucRegAddr, unsigned char *pucRegValue)
{
    //
    // Invoke the readfrom  API to get the required byte
    //
    if(I2C_IF_ReadFrom(AMG88xx_ADDRESS, &ucRegAddr, 1, pucRegValue, 1) != 0)
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
int SetRegisterValue_AMG88xx(unsigned char ucRegAddr, unsigned char ucRegValue)
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
    if(I2C_IF_Write(AMG88xx_ADDRESS,ucData,2,1) == 0) // DevAddr Received by I2C:
    {
        return SUCCESS;
    }
    else
    {
        DBG_PRINT("I2C write failed\n\r");
    }

    return FAILURE;
}

float getPixelTemperature(unsigned char pixelAddr)
{

  // Temperature registers are numbered 128-255
  // Each pixel has a lower and higher register
  unsigned char pixelLowRegister = TEMPERATURE_REGISTER_START + (2 * pixelAddr);
  unsigned char temperature[2];
  int16_t temperature_16bit = 0;
  readRegister16_AMG88xx(pixelLowRegister, temperature);//readRegister16_AMG88xx will send read request to AMG88xx. pixelLowRegister is the address of a pixel to read.     int16_t temperature = getRegister(pixelLowRegister, 2);
  temperature_16bit = temperature[1];
  temperature_16bit = temperature_16bit << 8;
  temperature_16bit = temperature_16bit + temperature[0];
  // temperature is reported as 12-bit twos complement
  // check if temperature is negative
  if(temperature_16bit & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and
    // make the float negative
    temperature_16bit &= ~(1 << 11);
    temperature_16bit = temperature_16bit * -1;
  }

  float DegreesC = temperature_16bit * 0.25;

  return DegreesC;

}


void read_all_pixels (float *testPixelValue){
    unsigned char i;
    unsigned char toNewline = 0;
    //float testPixelValue = 0;
    DBG_PRINT("\n");
    for(i = 0; i < 64; i++){
      testPixelValue[i] = getPixelTemperature(i);
      //DBG_PRINT("%.2f ", testPixelValue[i]);
      //toNewline++;
      //if(toNewline == 8){
        //  toNewline = 0;
        //  DBG_PRINT(" ");
      //}
    }

}




