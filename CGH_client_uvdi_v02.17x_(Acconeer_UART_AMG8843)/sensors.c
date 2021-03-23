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


//*****************************************************************************
//
// Values that can be passed to I2COpen as the ulMode parameter.
//
//*****************************************************************************
#define I2C_MASTER_MODE_STD     0
#define I2C_MASTER_MODE_FST     1

//*****************************************************************************
// BMA222 Device I2C address
//*****************************************************************************
#define BMA222_DEV_ADDR_          0x18

#define BMA222_CHID_ID_NUM_       0x00

//*****************************************************************************
// BMA222 Acc Data Register related macros
//*****************************************************************************
#define BMA222_ACC_DATA_X_NEW_   	(0x2)
#define BMA222_ACC_DATA_X_   		(0x3)
#define BMA222_ACC_DATA_Y_NEW_  	(0x4)
#define BMA222_ACC_DATA_Y_   		(0x5)
#define BMA222_ACC_DATA_Z_NEW_  	(0x6)
#define BMA222_ACC_DATA_Z_   		(0x7)

//*****************************************************************************
// TMP006 Device I2C address
//*****************************************************************************
#define TMP006_DEV_ADDR_         0x41 //ADDR0 = 1, ADDR0 = 0. Datasheet: 100 0001 R/W

//*****************************************************************************
// TMP006 Register offset address
//*****************************************************************************
#define TMP006_VOBJECT_REG_ADDR_        0x0
#define TMP006_TAMBIENT_REG_ADDR_       0x1
#define TMP006_CONFIG_REG_ADDR_         0x2
#define TMP006_MANUFAC_ID_REG_ADDR_     0xFE
#define TMP006_DEVICE_ID_REG_ADDR_      0xFF

//*****************************************************************************
// TMP006 Device details
//*****************************************************************************
#define TMP006_MANUFAC_ID_      0x5449
#define TMP006_DEVICE_ID_       0x0067

//*****************************************************************************
// BMA222 Data Interpretation macros
//*****************************************************************************
//#define RESOLUTION_8BIT         ((float)(1.999 / 127))  //+-2g
//#define G_VAL                   ((float)9.7798)

//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define FAILURE                 -1
#define SUCCESS                 0
#define RET_IF_ERR(Func)        {int iRetVal = (Func); \
                                 if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

extern volatile float g_accXIntervalSum;
extern volatile float g_accYIntervalSum;
extern volatile float g_accZIntervalSum;
extern volatile long long g_accSampleCount;

extern volatile float g_accTotalAvg;
extern volatile float g_accXAvg;
extern volatile float g_accYAvg;
extern volatile float g_accZAvg;

//extern unsigned long pulAdcSamples[4096];//ADC

//extern int GetRegisterValue_temp(unsigned char ucRegAddr,
//                            unsigned short *pusRegValue);
//extern double ComputeTemperature(double dVobject, double dTAmbient);

//****************************************************************************
//
//! Reads a block of continuous data
//!
//! \param ucRegAddr is the start offset register address
//! \param pucBlkData is the pointer to the data value store
//! \param ucBlkDataSz is the size of data to be read
//!
//! This function
//!    1. Returns the data values in the specified store
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int BlockRead_(unsigned char ucRegAddr,
          unsigned char *pucBlkData,
          unsigned char ucBlkDataSz)
{
    //
    // Invoke the readfrom I2C API to get the required bytes
    //
    if(I2C_IF_ReadFrom(BMA222_DEV_ADDR_, &ucRegAddr, 1,
                   pucBlkData, ucBlkDataSz) != 0)
    {
        DBG_PRINT("I2C readfrom failed\n");
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
int
GetRegisterValue_(unsigned char ucRegAddr, unsigned char *pucRegValue)
{
    //
    // Invoke the readfrom  API to get the required byte
    //
    if(I2C_IF_ReadFrom(BMA222_DEV_ADDR_, &ucRegAddr, 1,
                   pucRegValue, 1) != 0)
    {
        DBG_PRINT("I2C readfrom failed\n\r");
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
int
SetRegisterValue_(unsigned char ucRegAddr, unsigned char ucRegValue)
{
    unsigned char ucData[2];
    //
    // Select the register to be written followed by the value.
    //
    ucData[0] = ucRegAddr;
    ucData[1] = ucRegValue;
    //
    // Initiate the I2C write
    //
    if(I2C_IF_Write(BMA222_DEV_ADDR_,ucData,2,1) == 0)	// DevAddr Received by I2C:
    {
        return SUCCESS;
    }
    else
    {
        DBG_PRINT("I2C write failed\n\r");
    }

    return FAILURE;
}

//****************************************************************************
//
//! Initialize the BMA222 accelerometer device with defaults
//!
//! \param None
//!
//! This function
//!    1. Reads the CHIP ID.
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int
BMA222Open_()
{
    unsigned char ucRegVal;
    //
    // Read the CHIP ID NUM
    //
    RET_IF_ERR(GetRegisterValue_(BMA222_CHID_ID_NUM_, &ucRegVal));
    DBG_PRINT("CHIP ID: 0x%x\n\r", ucRegVal);

    return SUCCESS;
}

//****************************************************************************
//
//! Place the BMA222 accelerometer device to standby
//!
//! \param None
//!
//! This function
//!    1. Sets the device to standby mode.
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int
BMA222Close_()
{
    return SUCCESS;
}

//****************************************************************************
//
//! Get the accelerometer data readings
//!
//! \param pfAccX pointer to the AccX store
//! \param pfAccY pointer to the AccY store
//! \param pfAccZ pointer to the AccZ store
//!
//! This function
//!    1. Reads the data registers over I2C.
//!    2. Applies the range conversion to the raw values
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int
BMA222Read_(signed char *pcAccX, signed char *pcAccY, signed char *pcAccZ)
{
    char cAccX = 0;
    char cAccY = 0;
    char cAccZ = 0;
    //
    // Read the acclerometer output registers LSB and MSB
    //
    RET_IF_ERR(BlockRead_(BMA222_ACC_DATA_X_, (unsigned char *)&cAccX,
                     sizeof(cAccX)));

    RET_IF_ERR(BlockRead_(BMA222_ACC_DATA_Y_, (unsigned char *)&cAccY,
             sizeof(cAccY)));

    RET_IF_ERR(BlockRead_(BMA222_ACC_DATA_Z_, (unsigned char *)&cAccZ,
             sizeof(cAccZ)));

    *pcAccX = cAccX;
    *pcAccY = cAccY;
    *pcAccZ = cAccZ;

    return SUCCESS;
}

//****************************************************************************
//
//! Get the raw accelerometer data register readings
//!
//! \param psAccX pointer to the raw AccX store
//! \param psAccY pointer to the raw AccY store
//! \param psAccZ pointer to the raw AccZ store
//!
//! This function
//!    1. Reads the data registers over I2C.
//!    2. Returns the accelerometer readings
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int BMA222ReadNew_(signed char *pcAccX, signed char *pcAccY, signed char *pcAccZ)
{
    char cAccX[6];
    //char cAccY[6];
    //char cAccZ[6];

    //
    // Read the acclerometer output registers LSB and MSB
    //
    RET_IF_ERR(BlockRead_(BMA222_ACC_DATA_X_NEW_, (unsigned char *)cAccX,6)); //6 bytes of data is X-Y-Z axis

    //
    // Read the acclerometer output registers LSB and MSB
    //
    //RET_IF_ERR(BlockRead_(BMA222_ACC_DATA_Y_NEW_, (unsigned char *)cAccY,6));

    //
    // Read the acclerometer output registers LSB and MSB
    //
    //RET_IF_ERR(BlockRead_(BMA222_ACC_DATA_Z_NEW_, (unsigned char *)cAccZ,6));

     //
    // Check whether new Sensor Data is available
    //
    if((cAccX[0] & 0x1) && (cAccX[2] & 0x1) && (cAccX[4] & 0x1))//X-Y-Z axis
    {
        *pcAccX = cAccX[1];//X-axis
        *pcAccY = cAccX[3];//Y-axis
        *pcAccZ = cAccX[5];//Z-axis
        return SUCCESS;
    }

    //New Sensor Data Not Available
    return FAILURE;

}

//*****************************************************************************
//
//! AccSample
//!
//!    @brief  Read Accelerometer Data from Sensor to Globals
//!
//!
//!     @return none
//!
//!
//
//*****************************************************************************
void AccSample_()
{
    signed char accX,accY,accZ;
    int iRet = -1;
    //unsigned long critKey;

    //critKey = osi_EnterCritical();

    iRet = BMA222ReadNew_(&accX, &accY, &accZ);
    if(iRet)
    {
        //In case of error/ No New Data return
        return;
    }


    g_accXIntervalSum += accX;
    g_accYIntervalSum += accY;
    g_accZIntervalSum += accZ;

    g_accSampleCount++;
    //osi_ExitCritical(critKey);
}

//*****************************************************************************
//
//! ReadAccSensor
//!
//!    @brief  Calculate Averages of Accelerometer Globals
//!
//!
//!     @return none
//!
//!
//
//*****************************************************************************
void SetAccAvg_()
{
	//unsigned long critKey;

    //critKey = osi_EnterCritical();
    g_accXAvg = g_accXIntervalSum / g_accSampleCount;
    g_accYAvg = g_accYIntervalSum / g_accSampleCount;
    g_accZAvg = g_accZIntervalSum / g_accSampleCount;
    g_accTotalAvg = (g_accZIntervalSum + g_accYIntervalSum + g_accXIntervalSum ) /
		(g_accSampleCount * 3);

    g_accXIntervalSum = 0;
    g_accYIntervalSum = 0;
    g_accZIntervalSum = 0;
    g_accSampleCount = 0;
    //osi_ExitCritical(critKey);

}


//****************************************************************************
//
//! Returns the value in the specified register
//!
//! \param ucRegAddr is the offset register address
//! \param pusRegValue is the pointer to the register value store
//!
//! This function
//!    1. Returns the value in the specified register
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int
GetRegisterValue_temp_(unsigned char ucRegAddr, unsigned short *pusRegValue)
{
    unsigned char ucRegData[2];
    //
    // Invoke the readfrom I2C API to get the required byte
    //
    if(I2C_IF_ReadFrom(TMP006_DEV_ADDR_, &ucRegAddr, 1,
                   &ucRegData[0], 2) != 0)
    {
        DBG_PRINT("I2C readfrom failed\n\r");
        return FAILURE;
    }

    *pusRegValue = (unsigned short)(ucRegData[0] << 8) | ucRegData[1];

    return SUCCESS;
}

//****************************************************************************
//
//! Initialize the temperature sensor
//!
//! \param None
//!
//! This function
//!    1. Get the device manufacturer and version
//!    2. Add any initialization here
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int
TMP006DrvOpen_()
{
    unsigned short usManufacID, usDevID, usConfigReg;

    //
    // Get the manufacturer ID
    //
    RET_IF_ERR(GetRegisterValue_temp_(TMP006_MANUFAC_ID_REG_ADDR_, &usManufacID));
    DBG_PRINT("Manufacturer ID: 0x%x\n\r", usManufacID);
    if(usManufacID != TMP006_MANUFAC_ID_)
    {
        DBG_PRINT("Error in Manufacturer ID\n\r");
        return FAILURE;
    }

    //
    // Get the device ID
    //
    RET_IF_ERR(GetRegisterValue_temp_(TMP006_DEVICE_ID_REG_ADDR_, &usDevID));
    DBG_PRINT("Device ID: 0x%x\n\r", usDevID);
    if(usDevID != TMP006_DEVICE_ID_)
    {
        DBG_PRINT("Error in Device ID\n");
        return FAILURE;
    }

    //
    // Get the configuration register value
    //
    RET_IF_ERR(GetRegisterValue_temp_(TMP006_CONFIG_REG_ADDR_, &usConfigReg));
    DBG_PRINT("Configuration register value: 0x%x\n\r", usConfigReg);

    return SUCCESS;
}
//****************************************************************************
//
//! Compute the temperature value from the sensor voltage and die temp.
//!
//! \param dVobject is the sensor voltage value
//! \param dTAmbient is the local die temperature
//!
//! This function
//!    1. Computes the temperature from the VObject and TAmbient values
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
double ComputeTemperature_(double dVobject, double dTAmbient)
{
    //
    // This algo is obtained from
    // http://processors.wiki.ti.com/index.php/SensorTag_User_Guide
    // #IR_Temperature_Sensor
    //
    double Tdie2 = dTAmbient + 273.15;
    const double S0 = 6.4E-14;            // Calibration factor
    const double a1 = 1.75E-3;
    const double a2 = -1.678E-5;
    const double b0 = -2.94E-5;
    const double b1 = -5.7E-7;
    const double b2 = 4.63E-9;
    const double c2 = 13.4;
    const double Tref = 298.15;
    double S = S0*(1+a1*(Tdie2 - Tref)+a2*pow((Tdie2 - Tref),2));
    double Vos = b0 + b1*(Tdie2 - Tref) + b2*pow((Tdie2 - Tref),2);
    double fObj = (dVobject - Vos) + c2*pow((dVobject - Vos),2);
    double tObj = pow(pow(Tdie2,4) + (fObj/S),.25);
    tObj = (tObj - 273.15);
    return tObj;
}

//****************************************************************************
//
//! Get the temperature value
//!
//! \param pfCurrTemp is the pointer to the temperature value store
//!
//! This function
//!    1. Get the sensor voltage reg and ambient temp reg values
//!    2. Compute the temperature from the read values
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int
TMP006DrvGetTemp_(float *pfCurrTemp)
{
    unsigned short usVObjectRaw, usTAmbientRaw;
    double dVObject, dTAmbient;
    //
    // Get the sensor voltage register value
    //
    RET_IF_ERR(GetRegisterValue_temp_(TMP006_VOBJECT_REG_ADDR_, &usVObjectRaw));
    //
    // Get the ambient temperature register value
    //
    RET_IF_ERR(GetRegisterValue_temp_(TMP006_TAMBIENT_REG_ADDR_, &usTAmbientRaw));
    //
    // Apply the format conversion
    //
    dVObject = ((short)usVObjectRaw) * 156.25e-9;
    dTAmbient = ((short)usTAmbientRaw) / 128;

    *pfCurrTemp = ComputeTemperature_(dVObject, dTAmbient);

    //
    // Convert to Farenheit
    //
    //*pfCurrTemp = ((*pfCurrTemp * 9) / 5) + 32;

    return SUCCESS;
}



