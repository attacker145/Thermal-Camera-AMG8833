/*
 * credentials.c
 *
 *  Created on: Jun 8, 2018
 *      Author: rchak
 */

#include <string.h>
#include <stdlib.h>
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
#include "wlan.h"
#include "network_if.h"

#define READ_SIZE           1450
#define MAX_BUFF_SIZE       1460
#define SH_GPIO_3           3       /* P58 - Light Sensor */
#define SH_GPIO_6           6       /* P61 - Light Sensor */
#define SH_GPIO_7           7       /* P62 - BLE data ready */
#define SH_GPIO_9           9		// Red LED
#define SH_GPIO_10          10		// Orange LED
#define SH_GPIO_11          11		// Green LED
#define SH_GPIO_22          22      /* P15 - Device Mode */
#define MAX_STRING_LENGTH    50

#define UartPutChar(c)       MAP_UARTCharPut(CONSOLE,c)
#define UartGetChar()        MAP_UARTCharGet(CONSOLE)

extern volatile unsigned long  g_ulStatus;//SimpleLink Status
extern unsigned long  g_ulDestinationIP; // IP address of destination server
extern unsigned long  g_ulGatewayIP; //Network Gateway IP address
extern unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
extern unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
extern unsigned char g_buff[MAX_BUFF_SIZE+1];
extern long bytesReceived; // variable to store the file size
extern char buf[180];
extern unsigned char g_ucUARTRecvBuffer1[80];
extern unsigned char ucPinValue;//02/17/2017
extern unsigned char Lght;
extern unsigned char BLE_data_rdy;
extern unsigned int uiGPIOPort;//02/17/2017
extern unsigned char pucGPIOPin;//02/17/2017
extern signed char cPassword[MAX_STRING_LENGTH+1];
extern signed char cSSID_NAME[MAX_STRING_LENGTH+1];
extern char UART1buffer[MAX_STRING_LENGTH+1];
extern signed char cCharacter;
extern int iStringLength;
extern volatile int g_iCounter;
extern unsigned char cred_flg;

extern long WriteFileToDevice(unsigned long *ulToken, long *lFileHandle, signed char* dat);
extern long ReadFileFromDevice(unsigned long ulToken, long lFileHandle, signed char* dat);


void credentials (void){
    unsigned int i;
	ucPinValue = 0;
    long lRetVal;
    unsigned char policyVal;
    long lFileHandle;
    unsigned long ulToken;
    signed char ssidpass[100];
    signed char read_ssidpass[100];
	//Turn off RED LED
	//GPIO_IF_GetPortNPin(SH_GPIO_9,&uiGPIOPort,&pucGPIOPin);	// Computes port and pin number from the GPIO number
	//GPIO_IF_Set(SH_GPIO_9,uiGPIOPort,pucGPIOPin,1);//Turn O red LED 08/18/2017
	//#################GET PASSWORD FROM CONSOLE############################
    GPIO_IF_GetPortNPin(SH_GPIO_22,&uiGPIOPort,&pucGPIOPin);    //Read SW2 state. Computes port and pin number from the GPIO number
    ucPinValue = GPIO_IF_Get(SH_GPIO_22,uiGPIOPort,pucGPIOPin); //Read GPIO22: SW2

    if(ucPinValue == 1){                                        //If SW2 is pressed
        cred_flg = 1;
        iStringLength = 0;
        UART_PRINT("\nEnter SSID name: ");
        cCharacter = UartGetChar();
        while(cCharacter != '\r' && cCharacter != '\n' && (iStringLength <= MAX_STRING_LENGTH -1)){
            g_iCounter++;
            if(cCharacter == '\r' || cCharacter == '\n' || (iStringLength >= MAX_STRING_LENGTH -1))//Enter is hit
            {
                if(iStringLength >= MAX_STRING_LENGTH - 1)//Password is too long
                {
                    UartPutChar(cCharacter);
                    cSSID_NAME[iStringLength] = cCharacter; //password
                    iStringLength++;
                }
                iStringLength++;
                cSSID_NAME[iStringLength] = '\0';
                iStringLength = 0;
                Report("\n\r SSID: \n\r %s", cSSID_NAME);// Echoes the input string
                UART_PRINT("\n\r");
            }
            else
            {//User still enters the SSID name
                UartPutChar(cCharacter);
                cSSID_NAME[iStringLength] = cCharacter;
                iStringLength++;
            }
            cCharacter = UartGetChar();
        }

        iStringLength = 0;
        UART_PRINT("\nEnter SSID password: ");
        cCharacter = UartGetChar(); // Get the first character
        while(cCharacter != '\r' && cCharacter != '\n' && (iStringLength <= MAX_STRING_LENGTH -1)){
            g_iCounter++;
            if(cCharacter == '\r' || cCharacter == '\n' || (iStringLength >= MAX_STRING_LENGTH -1))
            {
                if(iStringLength >= MAX_STRING_LENGTH - 1)
                {
                    UartPutChar(cCharacter);
                    cPassword[iStringLength] = cCharacter; //password
                    iStringLength++;
                }
                iStringLength++;
                cPassword[iStringLength] = '\0';
                iStringLength = 0;
                Report("\n\rPassword: %s", cPassword);// Echoes the input string
                UART_PRINT("\n\r");
            }
            else
            {//User still enters the PASSWORD
                UartPutChar(cCharacter);
                cPassword[iStringLength] = cCharacter;
                iStringLength++;
            }
            cCharacter = UartGetChar();
        }

        signed char* total = (signed char*) malloc((2 * MAX_STRING_LENGTH) * sizeof(signed char)); // array to hold the result
        memcpy(total, cSSID_NAME, MAX_STRING_LENGTH * sizeof(signed char)); //Load SSID name
        memcpy(total + MAX_STRING_LENGTH, cPassword, MAX_STRING_LENGTH * sizeof(signed char)); //Load PASSWORD
        Report("\n\rssidpass array: ");
        for (i = 0; i < (2 * MAX_STRING_LENGTH); i++){
            ssidpass[i] = *(total+i);
            UartPutChar(ssidpass[i]);
        }
/*        Report("\nssidpass array HEX: ");
        for (i = 0; i < (2 * MAX_STRING_LENGTH); i++){
            Report("%02X:", ssidpass[i]);
        }
        Report("\n");*/
        free(total);
        lRetVal = sl_Start(NULL, NULL, NULL);// Initializing the CC3200 networking layers
        if(lRetVal < 0)
        {
            UART_PRINT("\nsl_Start failed");
            LOOP_FOREVER();
        }
        // reset all network policies
        lRetVal = sl_WlanPolicySet(  SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0,0,0,0,0), &policyVal, 1 /*PolicyValLen*/);
        if(lRetVal < 0)
        {
            UART_PRINT("\nsl_WlanPolicySet failed");
            //LOOP_FOREVER();
        }
        if(WriteFileToDevice(&ulToken, &lFileHandle, ssidpass) < 0)
        {
            //LOOP_FOREVER();
        }
        if(ReadFileFromDevice(ulToken, lFileHandle, read_ssidpass) < 0)
        {
            //LOOP_FOREVER();
        }
        Report("\nMemory Record: %s \n\r", read_ssidpass);
    }
    else{
        lRetVal = sl_Start(NULL, NULL, NULL);// Initializing the CC3200 networking layers
        if(lRetVal < 0)
        {
            UART_PRINT("\n sl_Start failed");
            LOOP_FOREVER();
        }
        // reset all network policies
        lRetVal = sl_WlanPolicySet(  SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0,0,0,0,0), &policyVal, 1 /*PolicyValLen*/);
        if(lRetVal < 0)
        {
            UART_PRINT("\n sl_WlanPolicySet failed");
            //LOOP_FOREVER();
        }
        if(ReadFileFromDevice(ulToken, lFileHandle, read_ssidpass) < 0)
        {
            //LOOP_FOREVER();
        }
        Report("Memory Record: %s", read_ssidpass);
        i = 0;
        while (read_ssidpass[i] != 0){
            cSSID_NAME[i] = read_ssidpass[i];
            i++;
        }
        Report("\n\rSSID: %s", cSSID_NAME);
        i = MAX_STRING_LENGTH;
        while (read_ssidpass[i] != 0){
            cPassword[(i - MAX_STRING_LENGTH)] = read_ssidpass[i];
            i++;
        }
        Report("\n\rPassword: %s \n\r", cPassword);
    }
}
