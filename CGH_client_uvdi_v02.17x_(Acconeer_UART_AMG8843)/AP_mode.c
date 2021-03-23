#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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
#include "network_if.h"
// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/common.h>
// JSON Parser
#include "jsmn.h"
#include <math.h>
#include "wdt.h"
#include "wdt_if.h"
#include "serial_wifi.h"

#include "network_defines.h"

#include "button_if.h"

extern long SmartConfigConnect();

#define AUTO_CONNECTION_TIMEOUT_COUNT   50      /* 5 Sec */
#define SH_GPIO_13          13      //SW3

static unsigned long  g_ulStatus = 0;//SimpleLink Status
static unsigned int g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
extern unsigned int uiGPIOPort;//02/17/2017
extern unsigned char pucGPIOPin;//02/17/2017
//extern long lRetVal;

//****************************************************************************
//
//! Confgiures the mode in which the device will work
//!
//! \param iMode is the current mode of the device
//!
//!
//! \return   SlWlanMode_t
//!
//
//****************************************************************************
static int ConfigureMode(int iMode)
{
    long lRetVal = -1;

    lRetVal = sl_WlanSetMode(iMode);
    ASSERT_ON_ERROR(lRetVal);

    /* Restart Network processor */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    // reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);

    return sl_Start(NULL,NULL,NULL);
}

long AP_mode (void){
    unsigned char ucPinValue;
    //Autoconfig------------------------------------------------------------------------
        GPIO_IF_GetPortNPin(SH_GPIO_13,&uiGPIOPort,&pucGPIOPin);//SW3
        ucPinValue = GPIO_IF_Get(SH_GPIO_13,uiGPIOPort,pucGPIOPin);
        if (ucPinValue == 1){
            long lRetVal = -1;
            unsigned int uiConnectTimeoutCnt =0;

            g_uiDeviceModeConfig = ROLE_AP;

            /*
             * This function initialize the communication interface,
             * set the enable pin of the device, and call to the init complete callback.
             * Return:
             * Returns the current active role (STA/AP/P2P) or an error code:
             * ROLE_STA, ROLE_AP, ROLE_P2P in case of success, otherwise in failure one of the following is return:
                SL_ERROR_ROLE_STA_ERR (Failure to load MAC/PHY in STA role)
                SL_ERROR_ROLE_AP_ERR (Failure to load MAC/PHY in AP role)
                SL_ERROR_ROLE_P2P_ERR (Failure to load MAC/PHY in P2P role)
                SL_ERROR_CALIB_FAIL (Failure of calibration)
                SL_ERROR_FS_CORRUPTED_ERR (FS is corrupted, Return to Factory Image or Program new image should be invoked (see sl_FsCtl, sl_FsProgram))
                SL_ERROR_FS_ALERT_ERR (Device is locked, Return to Factory Image or Program new image should be invoked (see sl_FsCtl, sl_FsProgram))
                SL_ERROR_RESTORE_IMAGE_COMPLETE (Return to factory image completed, perform reset)
                SL_ERROR_GENERAL_ERR (General error during init)
             */
            // staring simplelink
            lRetVal =  sl_Start(NULL,NULL,NULL);
            ASSERT_ON_ERROR( lRetVal);

            //Device is in STA Mode and Force AP Jumper is Connected
            if(ROLE_AP != lRetVal && g_uiDeviceModeConfig == ROLE_AP )
            {
                 //Switch to AP Mode
                 lRetVal = ConfigureMode(ROLE_AP);
                 ASSERT_ON_ERROR( lRetVal);
            }

            //No Mode Change Required.
            if(lRetVal == ROLE_AP)
            {
                //waiting for the AP to acquire IP address from Internal DHCP Server
                // If the device is in AP mode, we need to wait for this event
                // before doing anything
                while(!IS_IP_ACQUIRED(g_ulStatus))
                {
                //#ifndef SL_PLATFORM_MULTI_THREADED
                //    _SlNonOsMainLoopTask();
                //#endif
                }
                //Stop Internal HTTP Server
                lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
                ASSERT_ON_ERROR( lRetVal);

                //Start Internal HTTP Server
                lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
                ASSERT_ON_ERROR( lRetVal);

               char ssid[32];
               unsigned short len = 32;
               unsigned short config_opt = WLAN_AP_OPT_SSID;
               sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt , &len, (unsigned char* )ssid);//Getting WLAN configurations.
               UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r",ssid);
            }
            else
            {
                //Stop Internal HTTP Server
                lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
                ASSERT_ON_ERROR( lRetVal);

                //Start Internal HTTP Server
                lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
                ASSERT_ON_ERROR( lRetVal);

                //waiting for the device to Auto Connect
                while(uiConnectTimeoutCnt<AUTO_CONNECTION_TIMEOUT_COUNT &&
                    ((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus))))
                {
                    MAP_UtilsDelay(500000);
                    uiConnectTimeoutCnt++;
                }
                //Couldn't connect Using Auto Profile
                if(uiConnectTimeoutCnt == AUTO_CONNECTION_TIMEOUT_COUNT)
                {
                    //Blink Red LED to Indicate Connection Error
                    //GPIO_IF_LedOn(MCU_RED_LED_GPIO);

                    CLR_STATUS_BIT_ALL(g_ulStatus);

                    //Connect Using Smart Config
                    lRetVal = SmartConfigConnect();
                    ASSERT_ON_ERROR(lRetVal);

                    //Waiting for the device to Auto Connect
                    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
                    {
                        MAP_UtilsDelay(500);
                    }
            }
            //Turn RED LED Off
            //GPIO_IF_LedOff(MCU_RED_LED_GPIO);

            //g_iInternetAccess = ConnectionTest();

            }
        }
        return 0;
}





