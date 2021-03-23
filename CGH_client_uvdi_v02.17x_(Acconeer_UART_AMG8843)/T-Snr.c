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
//#define AK9753AE_DEV_ADDR       0x64 // - 110 0100 R/W
#define TSonar_DEV_ADDR         0x70 // - 111 0000 R/W
#define TSonar_DEV1_ADDR         50
//#define TSonar_rd_addr          0xE0 // - 1110 0000
//#define TSonar_wr_addr          0xE1 // - 1110 0001
#define TSonar_rd_cmmnd         0x51 //
#define TMP006_DEV_ADDR         0x41

//unsigned char ucStop = 1;
//unsigned char ucLen = 1;

extern unsigned char Lght;

int ts_write_cmmnd (unsigned char commnd){//0x51

    //****************************************************************************
    //
    //! Invokes the I2C driver APIs to write to the specified address
    //!
    //! \param ucDevAddr is the device I2C slave address
    //! \param pucData is the pointer to the data to be written
    //! \param ucLen is the length of data to be written
    //! \param ucStop determines if the transaction is followed by stop bit
    //!
    //! This function works in a polling mode,
    //!    1. Writes the device register address to be written to.
    //!    2. In a loop, writes all the bytes over I2C
    //!
    //! \return 0: Success, < 0: Failure.ucData
    //
    //****************************************************************************
    // int I2C_IF_ReadFrom(unsigned char ucDevAddr, unsigned char *pucData, unsigned char ucLen, unsigned char ucStop)
    // I2C_IF_Write (TSonar_DEV_ADDR, pucData, ucLen, ucStop);
    //****************************************************************************
    // 81, 225,
    //if(I2C_IF_Write (TSonar_DEV_ADDR, &commnd, 1, 1) !=0)
    if(I2C_IF_Write (TSonar_DEV_ADDR, &commnd, 1, 0) !=0)
    {
        DBG_PRINT("T-Snr I2C_IF_Write failed. Data Sent: %d, %d", TSonar_DEV_ADDR, commnd);
        DBG_PRINT("\n\r");
        Lght = 1;
        return FAILURE;
    }
    else{
        //DBG_PRINT("T-Snr Comm. Success. Data Sent: %d, %d", TSonar_DEV_ADDR, commnd);
    }
    return SUCCESS;
}

int ts_read_range(unsigned char *pucRegValue){
    //
    // Invoke the readfrom  API to get the required byte
    // ucRegAddr - Register 1 byte address
    // 1 - 1 byte address size
    //if(I2C_IF_ReadFrom(TSonar_DEV_ADDR, &ucRegAddr, 1, pucRegValue, 2) != 0)
    //{
    //    DBG_PRINT("I2C read from failed\n\r");
    //    return FAILURE;
    //}
    //
    // Read the specified length of data
    //
    RET_IF_ERR(I2C_IF_Read(TSonar_DEV_ADDR, pucRegValue, 2));


    return SUCCESS;
}

int ts1_write_cmmnd (unsigned char commnd){//0x51

    //****************************************************************************
    //
    //! Invokes the I2C driver APIs to write to the specified address
    //!
    //! \param ucDevAddr is the device I2C slave address
    //! \param pucData is the pointer to the data to be written
    //! \param ucLen is the length of data to be written
    //! \param ucStop determines if the transaction is followed by stop bit
    //!
    //! This function works in a polling mode,
    //!    1. Writes the device register address to be written to.
    //!    2. In a loop, writes all the bytes over I2C
    //!
    //! \return 0: Success, < 0: Failure.ucData
    //
    //****************************************************************************
    // int I2C_IF_ReadFrom(unsigned char ucDevAddr, unsigned char *pucData, unsigned char ucLen, unsigned char ucStop)
    // I2C_IF_Write (TSonar_DEV_ADDR, pucData, ucLen, ucStop);
    //****************************************************************************
    // 81, 225,
    //if(I2C_IF_Write (TSonar_DEV_ADDR, &commnd, 1, 1) !=0)
    if(I2C_IF_Write (TSonar_DEV1_ADDR, &commnd, 1, 0) !=0)
    {
        DBG_PRINT("T-Snr I2C_IF_Write failed. Data Sent: %d, %d", TSonar_DEV1_ADDR, commnd);
        DBG_PRINT("\n\r");
        Lght = 1;
        return FAILURE;
    }
    else{
        //DBG_PRINT("T-Snr Comm. Success. Data Sent: %d, %d", TSonar_DEV_ADDR, commnd);
    }
    return SUCCESS;
}

int ts1_read_range(unsigned char *pucRegValue){
    //
    // Read the specified length of data
    //
    RET_IF_ERR(I2C_IF_Read(TSonar_DEV1_ADDR, pucRegValue, 2));


    return SUCCESS;
}


#define I2C_BASE                I2CA0_BASE
void address (unsigned char *Databuf){
    I2C_IF_Write (TSonar_DEV_ADDR, Databuf, 3, 0);
}
