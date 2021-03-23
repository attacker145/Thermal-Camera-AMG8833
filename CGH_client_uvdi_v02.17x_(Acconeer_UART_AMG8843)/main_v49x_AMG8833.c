/*
 *
 * Copyright (C) 2019 UVDI
 * Austhor: Roman Chak
 * Application name UVDI CGH
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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

#define cred

#define APPLICATION_VERSION "2.11x"
#define APP_NAME            "UVDI CGH Acconeer v2.19x"

//#define POST_REQUEST_URI  "/post"
//#define POST_REQUEST_URI  "/post.html"

#include "network_defines.h"

#include "button_if.h"

#define PROXY_IP            <proxy_ip>
#define PROXY_PORT          <proxy_port>

//#define READ_SIZE           1450
#define MAX_BUFF_SIZE       1460
#define SH_GPIO_0           0       //WiFi En
#define SH_GPIO_3           3       /* P58 - Light Sensor */
#define SH_GPIO_6           6       /* P61 - Light Sensor */
#define SH_GPIO_7           7       /* P62 - BLE data ready */
#define SH_GPIO_9           9       // Red LED
#define SH_GPIO_10          10      // Orange LED
#define SH_GPIO_11          11      // Green LED
#define SH_GPIO_13          13      //SW3
#define SH_GPIO_22          22      /* PIN_15 - Device Mode */
#define SH_GPIO_12          12      /* PIN_03 - AK9753A INT */
#define SH_GPIO_28          28      /* PIN_03 - AK9753A INT */
#define SH_GPIO_30          30      /* PIN_53 - I2C bus PWR EN */
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
#define BMA222_DEV_ADDR          0x18

#define BMA222_CHID_ID_NUM       0x00

//*****************************************************************************
// BMA222 Acc Data Register related macros
//*****************************************************************************
#define BMA222_ACC_DATA_X_NEW   (0x2)
#define BMA222_ACC_DATA_X       (0x3)
#define BMA222_ACC_DATA_Y_NEW   (0x4)
#define BMA222_ACC_DATA_Y       (0x5)
#define BMA222_ACC_DATA_Z_NEW   (0x6)
#define BMA222_ACC_DATA_Z       (0x7)

//*****************************************************************************
// TMP006 Device I2C address
//*****************************************************************************
#define TMP006_DEV_ADDR         0x41 //ADDR0 = 1, ADDR0 = 0 - 100 0001

//*****************************************************************************
// TMP006 Register offset address
//*****************************************************************************
#define TMP006_VOBJECT_REG_ADDR         0x0
#define TMP006_TAMBIENT_REG_ADDR        0x1
#define TMP006_CONFIG_REG_ADDR          0x2
#define TMP006_MANUFAC_ID_REG_ADDR      0xFE
#define TMP006_DEVICE_ID_REG_ADDR       0xFF

//*****************************************************************************
// TMP006 Device details
//*****************************************************************************
#define TMP006_MANUFAC_ID       0x5449
#define TMP006_DEVICE_ID        0x0067

//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define FAILURE                 -1
#define SUCCESS                 0
#define RET_IF_ERR(Func)        {int iRetVal = (Func); \
                                 if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

//*****************************************************************************
// BMA222 Data Interpretation macros
//*****************************************************************************
#define RESOLUTION_8BIT         ((float)(1.999 / 127))  //+-2g
#define G_VAL                   ((float)9.7798)

#define NO_OF_SAMPLES       128 //ADC

//#define WD_PERIOD_MS                 1000
//#define WD_PERIOD_MS                 20000 //20 secs
#define WD_PERIOD_MS                 2000
//#define WD_PERIOD_MS                 4000
#define MAP_SysCtlClockGet           80000000
#define LED_GPIO                     MCU_RED_LED_GPIO    /* RED LED */
#define MILLISECONDS_TO_TICKS(ms)    ((MAP_SysCtlClockGet / 1000) * (ms))
#define SH_GPIO_00              0

#define wrt_range 0x51

#define alarm_on  0
#define alarm_off 1

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
#define sleep
//#define WD_PERIOD_MS                 5000
#define MAP_SysCtlClockGet           80000000
#define LED_GPIO                     MCU_RED_LED_GPIO    /* RED LED */
#define MILLISECONDS_TO_TICKS(ms)    ((MAP_SysCtlClockGet / 1000) * (ms))
#define FOREVER                      1
#define array_size 51
#define mSEC_TO_LOOP(x)        ((80000/5)*x)
//#define AK975

volatile float g_accXIntervalSum = 0;
volatile float g_accYIntervalSum = 0;
volatile float g_accZIntervalSum = 0;
volatile long long g_accSampleCount = 0;

volatile float g_accTotalAvg;
volatile float g_accXAvg;
volatile float g_accYAvg;
volatile float g_accZAvg;
volatile unsigned int wdtcntr;

unsigned char pause = 0;
volatile unsigned long g_ulTimerInts = 0;
volatile unsigned long g_ulTimerInts_uart = 0;
volatile unsigned char acc_flt;
//static unsigned char g_ucDryerRunning = 0;
_u8 macAddressVal[SL_MAC_ADDR_LEN];
_u8 macAddressLen = SL_MAC_ADDR_LEN;

char Rx_buf[10];
//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern int I2C_IF_Open(unsigned long ulMode);
extern int I2C_IF_Close();
extern int I2C_IF_Write(unsigned char ucDevAddr,
             unsigned char *pucData,
             unsigned char ucLen,
             unsigned char ucStop);
extern int I2C_IF_Read(unsigned char ucDevAddr,
            unsigned char *pucData,
            unsigned char ucLen);
extern int I2C_IF_ReadFrom(unsigned char ucDevAddr,
            unsigned char *pucWrDataBuf,
            unsigned char ucWrLen,
            unsigned char *pucRdDataBuf,
            unsigned char ucRdLen);
extern void GPIO_IF_GetPortNPin(unsigned char ucPin, unsigned int *puiGPIOPort, unsigned char *pucGPIOPin);
extern void GPIO_IF_GetPortNPin(unsigned char ucPin, unsigned int *puiGPIOPort, unsigned char *pucGPIOPin);
extern unsigned char GPIO_IF_Get(unsigned char ucPin, unsigned int uiGPIOPort, unsigned char ucGPIOPin);
extern void GPIO_IF_LedOff(char ledNum);
extern int BlockRead_(unsigned char ucRegAddr, unsigned char *pucBlkData, unsigned char ucBlkDataSz);
extern int GetRegisterValue_(unsigned char ucRegAddr, unsigned char *pucRegValue);
extern int SetRegisterValue_(unsigned char ucRegAddr, unsigned char ucRegValue);
extern int BMA222Close_();
extern int BMA222Read_(signed char *pcAccX, signed char *pcAccY, signed char *pcAccZ);
extern int BMA222ReadNew_(signed char *pcAccX, signed char *pcAccY, signed char *pcAccZ);
extern void MessageUART1(const char *str);
extern void InitTermUART1();
extern int GetCmdUART1(char *pcBuffer, unsigned int uiBufLen);
extern int BMA222Open_();
//extern void AccSample_();
extern void SetAccAvg_();
extern int GetRegisterValue_temp_(unsigned char ucRegAddr, unsigned short *pusRegValue);
extern int TMP006DrvOpen_();
extern double ComputeTemperature_(double dVobject, double dTAmbient);
extern int TMP006DrvGetTemp_(float *pfCurrTemp);
//extern int BMA222Open_();
//extern unsigned short itoa(short cNum, char *cString);

extern void on_gpio7_int(void);
extern void gpio_int_init (void);
extern void init_adc (void);
extern float adc_read (unsigned long* pulAdcSamples, unsigned int len);

extern void TimerRefIntHandler(void);
extern void TimerBaseIntHandler(void);
extern void timerA0_start(unsigned long ms);
extern void timerA0_stop(void);
extern void credentials (void);

extern void AK9753_ID (void);

extern void WDT_IF_Init(fAPPWDTDevCallbk fpAppWDTCB, unsigned int uiReloadVal);
extern void WDT_IF_DeInit(void);
extern void WatchdogIntHandler(void);
extern void PerformPRCMSleepGPTWakeup();
extern void EnterHIBernate();
extern long Network_IF_DeInitDriver(void);
void GPIO_IF_Toggle(unsigned char SH_GPIO_XX);
extern void AK975XsetMode(unsigned char mode);
extern int AK9753AReadData(unsigned char *IR1L_, unsigned char *IR1H_,
        unsigned char *IR2L_, unsigned char *IR2H_,
        unsigned char *IR3L_, unsigned char *IR3H_,
        unsigned char *IR4L_, unsigned char *IR4H_);
extern void hexdec_long( uint32_t count );
extern void setCutoffFrequency(uint8_t frequency);
extern void SoftReset(void);
extern void IntrrptSourceSet(void);
extern void ECNTL1_rst (void);
extern float getTemperature_AK9753(unsigned char *sgn);
extern int HTTPPostMethod_SQL_delete(HTTPCli_Handle httpClient);
extern void GPIOs2IntHandler();
extern void Button_IF_EnableInterrupt(unsigned char ucSwitch);
extern void Button_IF_Init(P_INT_HANDLER S2InterruptHdl,P_INT_HANDLER S3InterruptHdl );
extern void no_conn (void);
extern int write_cmmnd (unsigned char *pucData, unsigned char ucLen, unsigned char ucStop);
extern int read_range(unsigned char ucRegAddr, unsigned char *pucRegValue);
extern int ReportUART1(const char *pcFormat, ...);
extern int ts_write_cmmnd (unsigned char commnd);
extern int ts_read_range(unsigned char *pucRegValue);
extern int ts1_write_cmmnd (unsigned char commnd);
extern int ts1_read_range(unsigned char *pucRegValue);
extern void door_open(void);
extern void scan_snr (void);
extern void scan_snr1 (void);
extern void scan_IR(void);
extern void alarm (void);
extern void address (unsigned char *Databuf);
extern void auto_adjust(void);
extern double SD_SNSR1_rg(unsigned long int *mean);
extern double SD_SNSR2_rg(unsigned long int *mean);
extern void sonar_cycle_pwr (void);
extern double noise_suppr2 (double stdev, unsigned long int *mean);
extern double noise_suppr1 (double stdev, unsigned long int *mean);
//extern void data_init (void);
extern void SendUART1(const char *str, unsigned char len);
extern void credentials (void);
void read_all_pixels (float *testPixelValue);
extern bool calculateHumanPresence(float *inputPixelList, unsigned int size);
//*****************************************************************************

// Application specific status/error codes
typedef enum{
 /* Choosing this number to avoid overlap with host-driver's error codes */
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,       
    DEVICE_START_FAILED = DEVICE_NOT_IN_STATION_MODE - 1,
    INVALID_HEX_STRING = DEVICE_START_FAILED - 1,
    TCP_RECV_ERROR = INVALID_HEX_STRING - 1,
    TCP_SEND_ERROR = TCP_RECV_ERROR - 1,
    FILE_NOT_FOUND_ERROR = TCP_SEND_ERROR - 1,
    INVALID_SERVER_RESPONSE = FILE_NOT_FOUND_ERROR - 1,
    FORMAT_NOT_SUPPORTED = INVALID_SERVER_RESPONSE - 1,
    FILE_OPEN_FAILED = FORMAT_NOT_SUPPORTED - 1,
    FILE_WRITE_ERROR = FILE_OPEN_FAILED - 1,
    INVALID_FILE = FILE_WRITE_ERROR - 1,
    SERVER_CONNECTION_FAILED = INVALID_FILE - 1,
    GET_HOST_IP_FAILED = SERVER_CONNECTION_FAILED  - 1,
    
    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

//*****************************************************************************
//                          MACROS
//*****************************************************************************
#define MAX_STRING_LENGTH    80
#define CONSOLE              UARTA0_BASE
#define UartGetChar()        MAP_UARTCharGet(CONSOLE)
#define UartPutChar(c)       MAP_UARTCharPut(CONSOLE,c)
#define MAX_STRING_LENGTH    80
volatile int g_iCounter = 0;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile tBoolean g_bFeedWatchdog = true;
//volatile unsigned long g_ulWatchdogCycles;

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
unsigned long  g_ulDestinationIP; // IP address of destination server
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
unsigned char g_buff[MAX_BUFF_SIZE+1];
long bytesReceived = 0; // variable to store the file size
char buf[99];
unsigned char ucPinValue;//02/17/2017
//unsigned char Lght = 0;
//unsigned char Door = 0;
//unsigned char BLE_data_rdy = 0;
//unsigned char rng_read1;
//unsigned char rng_read2;
//unsigned char g_ucUARTRecvBuffer1[80];
unsigned int uiGPIOPort;//02/17/2017
unsigned char pucGPIOPin;//02/17/2017
unsigned char flg;
volatile unsigned char human;
unsigned char timeout;
int cx;
signed char    *g_Host = HOST_NAME;
SlDateTime g_time;

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

signed char cPassword[MAX_STRING_LENGTH+1];
signed char cSSID_NAME[MAX_STRING_LENGTH+1];

char UART1buffer[5];
signed char cCharacter;
int iStringLength = 0;

SlVersionFull ver = {0};
unsigned char ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
unsigned char ucConfigLen = sizeof(ver);

unsigned char sonar_read_cmd = 0x51;//81 - command to write to TrashSonar
unsigned char door_flg = 0;
unsigned char snr_flg=0;
unsigned char snr_flg1=0;
unsigned char T_snr [2];
unsigned int T_snr_int;
unsigned int T_snr1_set;
unsigned int T_snr2_set;
//unsigned char set_val[4];
unsigned int stl_data1[10];
unsigned int stl_data2[array_size];
unsigned int stl_data3[10];
unsigned int stl_data4[array_size];
unsigned char cred_flg=0;
//unsigned long int mean_SNSR1;
//unsigned long int mean_SNSR2;

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'-Applications
            // can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s ,"
                            " BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
//No need to sleep
#ifdef sleep
                           EnterHIBernate();//--------------------------------------------------------------sleep------
#endif
            }
            else
            {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
//No need to sleep
#ifdef sleep
                           EnterHIBernate();//--------------------------------------------------------------sleep------
#endif
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            //Gateway IP address
            g_ulGatewayIP = pEventData->gateway;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
            "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
        }
        break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    // Unused in this application
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    //
    // This application doesn't work w/ socket - Events are not expected
    //
       switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status )
            {
                case SL_ECLOSE:
                    UART_PRINT(", [SOCK ERROR] - close socket (%d) operation "
                    "failed to transmit all queued packets\n\n",
                           pSock->socketAsyncEvent.SockAsyncData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED : socket %d , reason"
                        "(%d)",
                        pSock->socketAsyncEvent.SockAsyncData.sd,
                        pSock->socketAsyncEvent.SockTxFailData.status);
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
    }
}


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************



//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    None
//!
//! \return None
//!
//*****************************************************************************
static void InitializeAppVariables()
{
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
}


//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy to Auto and AutoSmartConfig
//!           - Deletes all the stored profiles
//!           - Enables DHCP
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode 
    if (ROLE_STA != lMode)
    {
        if (ROLE_AP == lMode)
        {
            // If the device is in AP mode, we need to wait for this event 
            // before doing anything 
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
            }
        }

        // Switch to STA role and restart 
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again 
        if (ROLE_STA != lRetVal)
        {
            // We don't want to proceed if the device is not coming up in STA-mode 
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }
    
    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt, 
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);
    
    UART_PRINT("ConfigureSimpleLinkToDefaultState - Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("ConfigureSimpleLinkToDefaultState - Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);

    // Set connection policy to Auto + SmartConfig 
    //      (Device's default connection policy)
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, 
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove all profiles
    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);

    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore 
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        // Wait
        while(IS_CONNECTED(g_ulStatus))
        {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, 
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);


    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();

    return SUCCESS;
}

//****************************************************************************
//
//! \brief Connecting to a WLAN Accesspoint
//!
//!  This function connects to the required AP (SSID_NAME) with Security
//!  parameters specified in te form of macros at the top of this file
//!
//! \param  Status value
//!
//! \return  None
//!
//! \warning    If the WLAN connection fails or we don't aquire an IP
//!            address, It will be stuck in this function forever.
//
//
//  char cPassword[MAX_STRING_LENGTH+1];
//  char cSSID_NAME[80];
//
//****************************************************************************
static long WlanConnect()
{
    SlSecParams_t secParams = {0};
    long lRetVal = 0;
    cred_flg = 1;//Get SSID name and password from memory
//#ifdef cred
    if(cred_flg == 1){
        secParams.Key = (signed char *)cPassword;
        secParams.KeyLen = strlen((const char *)cPassword);
        secParams.Type = SECURITY_TYPE;//SL_SEC_TYPE_WPA_WPA2
        secParams.Type = SL_SEC_TYPE_WPA_WPA2;//SL_SEC_TYPE_WPA_WPA2
        UART_PRINT("\n\r WlanConnect cPassword: %s", cPassword);
    }
//#else
    else{
        secParams.Key = (signed char *)SECURITY_KEY;
        secParams.KeyLen = strlen(SECURITY_KEY);
        secParams.Type = SECURITY_TYPE;
        UART_PRINT("\n\r WlanConnect SECURITY_KEY: %s", SECURITY_KEY);
    }
//#endif

//#ifdef cred
    if(cred_flg == 1){
        lRetVal = sl_WlanConnect((signed char *)cSSID_NAME, strlen((const char *)cSSID_NAME), 0, &secParams, 0);//Entered string - WiFi Name
        ASSERT_ON_ERROR(lRetVal);
        UART_PRINT("\n\r WlanConnect cSSID_NAME: %s", cSSID_NAME);
    }
//#else
    else{
        lRetVal = sl_WlanConnect((signed char *)SSID_NAME, strlen((const char *)SSID_NAME), 0, &secParams, 0);//Saved string - WiFi Name
        ASSERT_ON_ERROR(lRetVal);
        UART_PRINT("\n\r WlanConnect SSID_NAME: %s", SSID_NAME);
    }
    UART_PRINT("\n\r");
//#endif

    // Wait for WLAN Event
    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
    {
        // wait till connects to an AP
        _SlNonOsMainLoopTask();
    }

    return SUCCESS;

}


//*****************************************************************************
//
//! \brief Flush response body.
//!
//! \param[in]  httpClient - Pointer to HTTP Client instance
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int FlushHTTPResponse(HTTPCli_Handle httpClient)
{
    const char *ids[2] = {
                            HTTPCli_FIELD_NAME_CONNECTION, /* App will get connection header value. all others will skip by lib */
                            NULL
                         };
    char buf[128];
    int id;
    int len = 1;
    bool moreFlag = 0;
    char ** prevRespFilelds = NULL;


    /* Store previosly store array if any */
    prevRespFilelds = HTTPCli_setResponseFields(httpClient, ids);

    /* Read response headers */
    while ((id = HTTPCli_getResponseField(httpClient, buf, sizeof(buf), &moreFlag))
            != HTTPCli_FIELD_ID_END)
    {

        if(id == 0)
        {
            if(!strncmp(buf, "close", sizeof("close")))
            {
                UART_PRINT("Connection terminated by server\n\r");
            }
        }

    }

    /* Restore previosuly store array if any */
    HTTPCli_setResponseFields(httpClient, (const char **)prevRespFilelds);

    while(1)
    {
        /* Read response data/body */
        /* Note:
                moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
                data is available Or in other words content length > length of buffer.
                The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
                Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
                for more information.
        */
        HTTPCli_readResponseBody(httpClient, buf, sizeof(buf) - 1, &moreFlag);
        ASSERT_ON_ERROR(len);

        if ((len - 2) >= 0 && buf[len - 2] == '\r' && buf [len - 1] == '\n'){
            break;
        }

        if(!moreFlag)
        {
            /* There no more data. break the loop. */
            break;
        }
    }
    return 0;
}


//*****************************************************************************
//
//! \brief Handler for parsing JSON data
//!
//! \param[in]  ptr - Pointer to http response body data
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int ParseJSONData(char *ptr)
{
    long lRetVal = 0;
    int noOfToken;
    jsmn_parser parser;
    jsmntok_t   *tokenList;


    /* Initialize JSON PArser */
    jsmn_init(&parser);

    /* Get number of JSON token in stream as we we dont know how many tokens need to pass */
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), NULL, 10);
    if(noOfToken <= 0)
    {
        UART_PRINT("Failed to initialize JSON parser\n\r");
        return -1;

    }

    /* Allocate memory to store token */
    tokenList = (jsmntok_t *) malloc(noOfToken*sizeof(jsmntok_t));
    if(tokenList == NULL)
    {
        UART_PRINT("Failed to allocate memory\n\r");
        return -1;
    }

    /* Initialize JSON Parser again */
    jsmn_init(&parser);
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), tokenList, noOfToken);
    if(noOfToken < 0)
    {
        UART_PRINT("Failed to parse JSON tokens\n\r");
        lRetVal = noOfToken;
    }
    else
    {
        UART_PRINT("Successfully parsed %ld JSON tokens\n\r", noOfToken);
    }

    free(tokenList);

    return lRetVal;
}

/*!
    \brief This function read respose from server and dump on console
    \param[in]      httpClient - HTTP Client object
    \return         0 on success else -ve
    \note
    \warning
*/

static int readResponse(HTTPCli_Handle httpClient)
{
    long lRetVal = 0;
    int bytesRead = 0;
    int id = 0;
    unsigned long len = 0;
    int json = 0;
    char *dataBuffer=NULL;
    bool moreFlags = 1;
    const char *ids[4] = {
                            HTTPCli_FIELD_NAME_CONTENT_LENGTH,
                            HTTPCli_FIELD_NAME_CONNECTION,
                            HTTPCli_FIELD_NAME_CONTENT_TYPE,
                            NULL
                         };

    // Read HTTP POST request status code. Server status
    lRetVal = HTTPCli_getResponseStatus(httpClient);
    if(lRetVal > 0)
    {
        switch(lRetVal)
        {
        case 200:
        {
            UART_PRINT(", HTTP Status 200");
            /*
                 Set response header fields to filter response headers. All
                  other than set by this call we be skipped by library.
             */
            HTTPCli_setResponseFields(httpClient, (const char **)ids);
            //UART_PRINT("\n\r ids : %s", ids);

            /* Read filter response header and take appropriate action. */
            /* Note:
                    1. id will be same as index of fileds in filter array setted
                    in previous HTTPCli_setResponseFields() call.
                    2. moreFlags will be set to 1 by HTTPCli_getResponseField(), if  field
                    value could not be completely read. A subsequent call to
                    HTTPCli_getResponseField() will read remaining field value and will
                    return HTTPCli_FIELD_ID_DUMMY. Please refer HTTP Client Libary API
                    documenation @ref HTTPCli_getResponseField for more information.
             */
            while((id = HTTPCli_getResponseField(httpClient, (char *)g_buff, sizeof(g_buff), &moreFlags))
                    != HTTPCli_FIELD_ID_END)
            {
                //(char *)g_buff - value Field value string.

                switch(id)
                {
                case 0: /* HTTPCli_FIELD_NAME_CONTENT_LENGTH */
                {
                    len = strtoul((char *)g_buff, NULL, 0);
                    //UART_PRINT("\n\rHTTPCli_FIELD_NAME_CONTENT_LENGTH:");
                    UART_PRINT(", Length: %s", g_buff);
                }
                break;
                case 1: /* HTTPCli_FIELD_NAME_CONNECTION */
                {
                    //UART_PRINT("\n\rHTTPCli_FIELD_NAME_CONNECTION:");
                    UART_PRINT(", Connection name: %s", g_buff);
                }
                break;
                case 2: /* HTTPCli_FIELD_NAME_CONTENT_TYPE */
                {

                    //UART_PRINT("\n\r HTTPCli_FIELD_NAME_CONTENT_TYPE\n\r");
                    UART_PRINT(", Content type : %s", g_buff);
                    if(!strncmp((const char *)g_buff, "application/json",
                            sizeof("application/json")))
                    {
                        json = 1;
                        UART_PRINT(", g_buff == application/json \n\r");
                    }
                    else
                    {
                        /* Note:
                                Developers are advised to use appropriate
                                content handler. In this example all content
                                type other than json are treated as plain text.
                         */
                        json = 0;
                    }
                }
                break;
                default:
                {
                    UART_PRINT(", Wrong filter id");
                    lRetVal = -1;
                    goto end;
                }
                }
            }
            bytesRead = 0;
            if(len > sizeof(g_buff))
            {
                dataBuffer = (char *) malloc(len);
                if(dataBuffer)
                {
                    UART_PRINT(", Failed to allocate memory");
                    lRetVal = -1;
                    goto end;
                }
            }
            else
            {
                dataBuffer = (char *)g_buff;
            }

            /* Read response data/body */
            /* Note:
                    moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
                    data is available Or in other words content length > length of buffer.
                    The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
                    Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
                    for more information
             */

            bytesRead = HTTPCli_readResponseBody(httpClient, (char *)dataBuffer, len, &moreFlags);
            //UART_PRINT("\n\n\r Received response body, POST method: \n\r %s", dataBuffer);//Print web-site response

            if(bytesRead < 0)
            {
                UART_PRINT(", Failed to received response body");
                lRetVal = bytesRead;
                goto end;
            }
            else if( bytesRead < len || moreFlags)
            {
                UART_PRINT(", Mismatch in content length and received data length ");
                goto end;
            }
            dataBuffer[bytesRead] = '\0';

            if(json)
            {
                /* Parse JSON data */
                lRetVal = ParseJSONData(dataBuffer);
                if(lRetVal < 0)
                {
                    goto end;
                }
            }
            else
            {
                /* treating data as a plain text */
            }

        }
        break;

        case 404:
            UART_PRINT("File not found. \r\n");
            /* Handle response body as per requirement.
                  Note:
                    Developers are advised to take appopriate action for HTTP
                    return status code else flush the response body.
                    In this example we are flushing response body in default
                    case for all other than 200 HTTP Status code.
             */
            break;
        default:
            /* Note:
              Need to flush received buffer explicitly as library will not do
              for next request.Apllication is responsible for reading all the
              data.
             */
            FlushHTTPResponse(httpClient);
            break;
        }
    }
    else
    {
        UART_PRINT("Failed to receive data from server.\r\n");
        goto end;
    }

    lRetVal = 0;

end:
    if(len > sizeof(g_buff) && (dataBuffer != NULL))
    {
        free(dataBuffer);
    }
    return lRetVal;
}


//*****************************************************************************
//
//! \brief HTTP POST Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPPostMethod_data(HTTPCli_Handle httpClient)
{
    bool moreFlags = 1;
    bool lastFlag = 1;
    char tmpBuf[4];
    long lRetVal = 0;


    HTTPCli_Field fields[4] = {
                                    {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                    {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                    {HTTPCli_FIELD_NAME_CONTENT_TYPE, "application/x-www-form-urlencoded"},
                                    {NULL, NULL}
                                };

    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send POST method request. */
    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
       other than setted in previous call HTTPCli_setRequestFields() at later stage.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
    */
    moreFlags = 1;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_POST, POST_REQUEST_URI, moreFlags);//POST & custom_post.php
    if(lRetVal < 0)
    {
        UART_PRINT(", Failed to send HTTP POST request header");
        return lRetVal;
    }

    //acc=26 & accX=13 & accY=-1 & accZ=67 & sensortemp=23.85
    sprintf((char *)tmpBuf, "%d", (sizeof(buf)-14));  // Print Size of POST data body 99
    //sprintf((char *)tmpBuf, "%d", (sizeof(POST_DATA)-1));  // Size of POST data body

    /*
     * Here we are setting lastFlag = 1 as it is last header field.
     * Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
     * Send the length of sent data
    */
    lastFlag = 1;
    lRetVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (const char *)tmpBuf, lastFlag);
    if(lRetVal < 0)
    {
        UART_PRINT(", Failed to send HTTP POST request header");
        return lRetVal;
    }


    /* Send POST data/body */
    //lRetVal = HTTPCli_sendRequestBody(httpClient, POST_DATA, (sizeof(POST_DATA)-1));
    lRetVal = HTTPCli_sendRequestBody(httpClient, buf, (sizeof(buf)-14)); // POST data body
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP POST request body");
        return lRetVal;
    }


    lRetVal = readResponse(httpClient);

    return lRetVal;
}
#ifdef test
//*****************************************************************************
//
//! \brief HTTP DELETE Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPDeleteMethod(HTTPCli_Handle httpClient)
{
  
    long lRetVal = 0;
    HTTPCli_Field fields[3] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {NULL, NULL}
                            };
    bool moreFlags;


    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send DELETE method request. */
    /* Here we are setting moreFlags = 0 as there are no more header fields need to send
       at later stage. Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest
       for more information.
    */
    moreFlags = 0;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_DELETE, DELETE_REQUEST_URI, moreFlags);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP DELETE request header.\n\r");
        return lRetVal;
    }

    lRetVal = readResponse(httpClient);

    return lRetVal;
}

//*****************************************************************************
//
//! \brief HTTP PUT Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPPutMethod(HTTPCli_Handle httpClient)
{
  
    long lRetVal = 0;
    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {HTTPCli_FIELD_NAME_CONTENT_TYPE, "text/html"},
                                {NULL, NULL}
                            };
    bool        moreFlags = 1;
    bool        lastFlag = 1;
    char        tmpBuf[4];
    
    
    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send PUT method request. */
    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
       other than setted in previous call HTTPCli_setRequestFields() at later stage.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
    */
    moreFlags = 1;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_PUT, PUT_REQUEST_URI, moreFlags);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP PUT request header.\n\r");
        return lRetVal;
    }

    sprintf((char *)tmpBuf, "%d", (sizeof(PUT_DATA)-1));

    /* Here we are setting lastFlag = 1 as it is last header field.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
    */
    lastFlag = 1;
    lRetVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (char *)tmpBuf, lastFlag);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP PUT request header.\n\r");
        return lRetVal;
    }

    /* Send PUT data/body */
    lRetVal = HTTPCli_sendRequestBody(httpClient, PUT_DATA, (sizeof(PUT_DATA)-1));
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP PUT request body.\n\r");
        return lRetVal;
    }

    lRetVal = readResponse(httpClient);

    return lRetVal;
}

//*****************************************************************************
//
//! \brief HTTP GET Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPGetMethod(HTTPCli_Handle httpClient)
{
  
    long lRetVal = 0;
    /*
     * GET /get.php HTTP/1.1
     * This line specifies an HTTP command, called a method, followed by the address of a
     * document and the version of the HTTP protocol being used. In this case, the request
     * is using the GET method to ask for the index.html document using HTTP 1.1. After
     * this initial line, the request can contain optional header information that gives the server
     * additional data about the request. For example:
     * User-Agent: Mozilla/5.0 (Windows 2000; U) Opera 6.0 [en]
     *
     */

    //GET /get.php HTTP/1.1
    // optional header information that gives the server additional data about the request.
    //Accept: image/gif, image/jpeg, text/*, */* <---- "*/*" indicating all media types

    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"}, //Accept: image/gif, image/jpeg, text/*, */*
                                {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
                                {NULL, NULL}
                            };
    bool        moreFlags;
    
    
    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send GET method request. */
    /* Here we are setting moreFlags = 0 as there are no more header fields need to send
       at later stage. Please refer HTTP Library API documentaion @ HTTPCli_sendRequest
       for more information.
    */
    moreFlags = 0;

    /*
     * Here we are setting moreFlags = 1 as there are some more header fields need to send
     * other than setted in previous call HTTPCli_setRequestFields() at later stage.
     * Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
     */
    //moreFlags = 1;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_REQUEST_URI, moreFlags);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP GET request.\n\r");
        return lRetVal;
    }

     lRetVal = readResponse(httpClient);    //Currently returns all the text from get.html with text formatting (prints data to a serial port)
     if(lRetVal < 0)
     {
         UART_PRINT("Failed to send HTTP GET request.\n\r");
         //return lRetVal;
     }
    return lRetVal;
}
#endif
/*!
    \brief This function read respose from server and dump on console
    \param[in]      httpClient - HTTP Client object
    \return         0 on success else -ve
    \note
    \warning
    Called from HTTPGetPageMethod method
*/
static int readPageResponse(HTTPCli_Handle httpClient)//Read data.txt page
{

    long lRetVal = 0;
    int bytesRead = 0;
    int id = 0;
    unsigned long len = 0;
    int json = 0;
    char *dataBuffer=NULL;
    bool moreFlags = 1;
    char ID[256];
    uint64_t ID_val;
    unsigned char ID_flg = 0;
    //char * str1;
    //char * str2;
    const char *ids[4] = {
                            HTTPCli_FIELD_NAME_CONTENT_LENGTH,
                            HTTPCli_FIELD_NAME_CONNECTION,
                            HTTPCli_FIELD_NAME_CONTENT_TYPE,
                            NULL
                         };

    /* Read HTTP POST request status code. Server status */
    lRetVal = HTTPCli_getResponseStatus(httpClient);
    if(lRetVal > 0)
    {
        //UART_PRINT("\n\r --------------Read Page Response------------\n\r");
        switch(lRetVal)
        {
        case 200:
        {
            UART_PRINT(", HTTP Status 200");

            HTTPCli_setResponseFields(httpClient, (const char **)ids);

            while((id = HTTPCli_getResponseField(httpClient, (char *)g_buff, sizeof(g_buff), &moreFlags))
                    != HTTPCli_FIELD_ID_END)
            {
                //(char *)g_buff - value Field value string.

                switch(id)
                {
                case 0: /* HTTPCli_FIELD_NAME_CONTENT_LENGTH, case 0 server sent number of bytes in the response*/
                {
                    len = strtoul((char *)g_buff, NULL, 0);
                    //UART_PRINT("\n\rHTTPCli_FIELD_NAME_CONTENT_LENGTH\n\r");
                    UART_PRINT(", Number of characters received (xstart.php): %s", len);// Print number of characters that will be sent by the server
                }
                break;
                case 1: /* HTTPCli_FIELD_NAME_CONNECTION */
                {
                    //UART_PRINT("\n\rHTTPCli_FIELD_NAME_CONNECTION\n\r");
                    UART_PRINT(", Connection name: %s", g_buff);
                }
                break;
                case 2: /* HTTPCli_FIELD_NAME_CONTENT_TYPE. Content of the response */
                {

                    //UART_PRINT("\n\r HTTPCli_FIELD_NAME_CONTENT_TYPE\n\r");
                    UART_PRINT(", Content type : %s", g_buff);// Print Content of the response
                    if(!strncmp((const char *)g_buff, "application/json",
                            sizeof("application/json")))
                    {
                        json = 1;
                        UART_PRINT(", g_buff == application/json");
                    }
                    else
                    {
                        json = 0;
                    }
                }
                break;
                default:
                {
                    UART_PRINT(", Wrong filter id");
                    lRetVal = -1;
                    goto end;
                }
                }
            }
            bytesRead = 0;
            if(len > sizeof(g_buff))
            {
                dataBuffer = (char *) malloc(len);
                if(dataBuffer)
                {
                    UART_PRINT("\n\r Failed to allocate memory \n\r");
                    lRetVal = -1;
                    goto end;
                }
            }
            else
            {
                dataBuffer = (char *)g_buff;
            }

            bytesRead = HTTPCli_readResponseBody(httpClient, (char *)dataBuffer, len, &moreFlags);
            if(bytesRead < 0)
            {
                UART_PRINT("\n\r Failed to received response body\n\r");
                lRetVal = bytesRead;
                goto end;
            }
            else if( bytesRead < len || moreFlags)
            {
                UART_PRINT("\n\r Mismatch in content length and received data length\n\r");
                goto end;
            }
            dataBuffer[bytesRead] = '\0';
            UART_PRINT(", Text from /webapp/CC3200command.php ");
            char *output = NULL;

            UART_PRINT(", ");
            UART_PRINT(dataBuffer);
            UART_PRINT(", ");
            ID_val = (*(uint64_t *) macAddressVal) & 0x0000FFFFFFFFFFFF;
            snprintf(ID, sizeof(ID)-1, "%llu", (unsigned long long) ID_val);
            output = strstr (dataBuffer,ID);//Find current ID value (string) in the received buffer
            if (output){
                UART_PRINT("\nID is correct  ");
                ID_flg = 1;
            }else{
                UART_PRINT("\nID is not correct  ");
                ID_flg = 0;
            }
            *output = NULL;
            output = strstr (dataBuffer,"redledon");
            if (output && (ID_flg == 1)){
                GPIO_IF_GetPortNPin(SH_GPIO_9,&uiGPIOPort,&pucGPIOPin); // Turn On Red LED
                GPIO_IF_Set(SH_GPIO_9,uiGPIOPort,pucGPIOPin,alarm_off);
                UART_PRINT("\nUV ON \n");
            }
            else{
                GPIO_IF_GetPortNPin(SH_GPIO_9,&uiGPIOPort,&pucGPIOPin); // Turn Off Red LED
                GPIO_IF_Set(SH_GPIO_9,uiGPIOPort,pucGPIOPin,alarm_on);
                UART_PRINT("\nUV OFF \n");
            }

            if(json)
            {
                /* Parse JSON data */
                lRetVal = ParseJSONData(dataBuffer);
                if(lRetVal < 0)
                {
                    goto end;
                }
            }
            else
            {
                /* treating data as a plain text */
            }

        }
        break;

        case 404:
            UART_PRINT("File not found. \r\n");
            /* Handle response body as per requirement.
                  Note:
                    Developers are advised to take appropriate action for HTTP
                    return status code else flush the response body.
                    In this example we are flushing response body in default
                    case for all other than 200 HTTP Status code.
             */

            break;
        default:
            /* Note:
              Need to flush received buffer explicitly as library will not do
              for next request.Apllication is responsible for reading all the
              data.
             */
            FlushHTTPResponse(httpClient);
            break;
        }
    }
    else
    {
        UART_PRINT("Failed to receive data from server.\r\n");
        goto end;
    }

    lRetVal = 0;

end:
    if(len > sizeof(g_buff) && (dataBuffer != NULL))
    {
        free(dataBuffer);
    }
    return lRetVal;
}

static int HTTPGetPageMethod(HTTPCli_Handle httpClient)//Read xstart.php page
{

    long lRetVal = 0;

    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"}, //Accept: image/gif, image/jpeg, text/*, */*
                                {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
                                {NULL, NULL}
                            };
    bool        moreFlags;

    /* Set request header fields to be send for HTTP request. GET_REQUEST_URI*/
    HTTPCli_setRequestFields(httpClient, fields);

    moreFlags = 0;

    //lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, data_txt, moreFlags);//data_txt = data.txt
    //lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_action_page, moreFlags);
    //lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_REQUEST_URI, moreFlags);//data.html
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, message_brd, moreFlags);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP GET request.\n\r");
        return lRetVal;
    }

     lRetVal = readPageResponse(httpClient);    //Currently returns all the text from data.html with text formatting (prints data to a serial port)
     //lRetVal = readResponse(httpClient);
    return lRetVal;
}

//*****************************************************************************
//
//! Function to connect to AP
//!
//! \param  none
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************
static long ConnectToAP()
{
    long lRetVal = -1;
    
    //
    // Following function configure the device to default state by cleaning
    // the persistent settings stored in NVMEM (viz. connection profiles &
    // policies, power policy etc)
    //
    // Applications may choose to skip this step if the developer is sure
    // that the device is in its desired state at start of applicaton
    //
    // Note that all profiles and persistent settings that were done on the
    // device will be lost
    //
    lRetVal = ConfigureSimpleLinkToDefaultState();
    if(lRetVal < 0)
    {
        if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
        {
            UART_PRINT("Failed to configure the device in its default state, "
                            "Error-code: %d\n\r", DEVICE_NOT_IN_STATION_MODE);
        }

        return -1;
    }

    UART_PRINT("ConnectAP: Device is configured in default state \n\r");

    //
    // Assumption is that the device is configured in station mode already
    // and it is in its default state
    //
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0 || ROLE_STA != lRetVal)
    {
        ASSERT_ON_ERROR(DEVICE_START_FAILED);
    }

    UART_PRINT("Device started as STATION \n\r");

    // Connecting to WLAN AP - Set with static parameters defined at the top
    // After this call we will be connected and have IP address
    lRetVal = WlanConnect();
    if(lRetVal == SUCCESS){
        UART_PRINT("\n\rConnected to the AP: %s\r\n", cSSID_NAME);
    }
    else{
        UART_PRINT("\n\rFailed to connect to the AP: %s\r\n", SSID_NAME);
    }
    return 0;
}

//*****************************************************************************
//
//! Function to connect to HTTP server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************
static int ConnectToHTTPServer(HTTPCli_Handle httpClient)
{
    long lRetVal = -1;
    struct sockaddr_in addr;
  

#ifdef USE_PROXY
    struct sockaddr_in paddr;
    paddr.sin_family = AF_INET;
    paddr.sin_port = htons(PROXY_PORT);
    paddr.sin_addr.s_addr = sl_Htonl(PROXY_IP);
    HTTPCli_setProxy((struct sockaddr *)&paddr);
#endif
    
    /* Resolve HOST NAME/IP SL_AF_INET6, SL_AF_INET*/
    lRetVal = sl_NetAppDnsGetHostByName((signed char *)HOST_NAME,
                                          strlen((const char *)HOST_NAME),
                                          &g_ulDestinationIP,SL_AF_INET);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to get IP address of the destination server. Error(%d)\n\r", GET_HOST_IP_FAILED);
        ASSERT_ON_ERROR(GET_HOST_IP_FAILED);//Failed to get IP address of the destination server
    }

    /* Set up the input parameters for HTTP Connection */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HOST_PORT);
    addr.sin_addr.s_addr = sl_Htonl(g_ulDestinationIP);

    /* Testing HTTPCli open call: handle, address params only */
    HTTPCli_construct(httpClient);
    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, 0, NULL);
    if (lRetVal < 0)
    {
        UART_PRINT("Connection to server failed. error(%d)\n\r", SERVER_CONNECTION_FAILED);
        ASSERT_ON_ERROR(SERVER_CONNECTION_FAILED);
    }    
    else
    {
        UART_PRINT("Connection to server created successfully: ");
        UART_PRINT(HOST_NAME);
        UART_PRINT("\n\r");
    }

    return 0;
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{
    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\t\t      CC3200 %s Application       \n\r", AppName);
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\n\n\n\r");
}


//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

// Temperature
//###################################################################################

//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//****************************************************************************
//static int GetRegisterValue_temp(unsigned char ucRegAddr,
//                            unsigned short *pusRegValue);
//static double ComputeTemperature(double dVobject, double dTAmbient);

extern int GetRegisterValue_temp_(unsigned char ucRegAddr,
                            unsigned short *pusRegValue);
extern double ComputeTemperature_(double dVobject, double dTAmbient);

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
double ComputeTemperature(double dVobject, double dTAmbient)
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

void door_open(void){
    asm(" NOP");
    //GPIOPinWrite(GPIOA1_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_3);

    GPIOIntDisable(GPIOA0_BASE,GPIO_PIN_6);
    GPIOIntClear(GPIOA0_BASE,GPIO_PIN_6);
    IntDisable(INT_GPIOA0);
    //GPIO_IF_GetPortNPin(SH_GPIO_6,&uiGPIOPort,&pucGPIOPin);
    //Door = GPIO_IF_Get(SH_GPIO_6,uiGPIOPort,pucGPIOPin);
    //if (Door == 1){
    //GPIO_IF_GetPortNPin(SH_GPIO_9,&uiGPIOPort,&pucGPIOPin); // Computes port and pin number from the GPIO number
    //GPIO_IF_Set(SH_GPIO_9,uiGPIOPort,pucGPIOPin,alarm_on);
    UART_PRINT("\n\rDoor swing");
    MAP_UtilsDelay(10000000);
    door_flg = 1;
    //}
    IntPendClear(INT_GPIOA0);
    IntEnable(INT_GPIOA0);
    GPIOIntEnable(GPIOA0_BASE, GPIO_PIN_6);
    timerA0_start((unsigned long)5000);
}
//const char plc_test[] = {0x11, 0x05,   0x03,0xE8,   0xFF,0x01,    0xBA,0x49};
//######################################################################################
int main()
{

    long lRetVal = -1;
    float testPixelValue[64];
    //unsigned char i;
    //float sensorTemp;
    //tBoolean bRetcode;

//#define cloud
#ifdef cloud
    HTTPCli_Struct httpClient;
    unsigned char cnn_fail = 0;
    _u8 macAddressVal[SL_MAC_ADDR_LEN];
    _u8 macAddressLen = SL_MAC_ADDR_LEN;
    unsigned int f_cntr = 0;
    unsigned int t_cntr = 0;
#endif
    //char uart1Buff[75];
    //unsigned int uiBufLen;
    //char *statobj = NULL;
    //char *movobj = NULL;
    //char *SnsrFlt = NULL;

#ifdef AK975
    unsigned char IR1L_ = 0;
    unsigned char IR1H_ = 0;
    unsigned char IR2L_ = 0;
    unsigned char IR2H_ = 0;
    unsigned char IR3L_ = 0;
    unsigned char IR3H_ = 0;
    unsigned char IR4L_ = 0;
    unsigned char IR4H_ = 0;
    unsigned int IR1 = 0;
    //unsigned char sgn;
    //float roomtemp;
    //tBoolean bRetcode;

#endif

    BoardInit();// Board Initialization

    sonar_cycle_pwr();// 5sec off pins start to initialize here

    PinMuxConfig();// Configure the pinmux settings for the peripherals exercised

    InitTerm();// Serial Configuring UART0
    InitTermUART1();// Serial Configuring UART1

    GPIO_IF_GetPortNPin(SH_GPIO_9,&uiGPIOPort,&pucGPIOPin); // Computes port and pin number from the GPIO number
    GPIO_IF_Set(SH_GPIO_9,uiGPIOPort,pucGPIOPin,alarm_on);//Turn OFF red LED 08/18/2017

    DisplayBanner(APP_NAME); // Display banner

    //WDT_IF_Init(WatchdogIntHandler, MILLISECONDS_TO_TICKS(WD_PERIOD_MS));// Set up the watchdog interrupt handler.

    //bRetcode = MAP_WatchdogRunning(WDT_BASE);
    //if(!bRetcode)
    //{
    //   WDT_IF_DeInit();
    //}
#ifdef cloud
    //SET MAC address
    GPIO_IF_GetPortNPin(SH_GPIO_13,&uiGPIOPort,&pucGPIOPin);//Read SW3. If button is pressed set MAC address
    ucPinValue = GPIO_IF_Get(SH_GPIO_13,uiGPIOPort,pucGPIOPin);
    if (ucPinValue == 1){
        sl_Start(NULL,NULL,NULL);
        _u8 MAC_Address[6];
        MAC_Address[0] = 0x8;
        MAC_Address[1] = 0x0;
        MAC_Address[2] = 0x28;
        MAC_Address[3] = 0x22;
        MAC_Address[4] = 0x69;
        MAC_Address[5] = 0x31;
        sl_NetCfgSet(SL_MAC_ADDRESS_SET,1,SL_MAC_ADDR_LEN,(_u8 *)MAC_Address);
        sl_Stop(0);
        UART_PRINT("\n MAC address is set to: %02x:%02x:%02x:%02x:%02x:%02x \n",
                   MAC_Address[0],
                   MAC_Address[1],
                   MAC_Address[2],
                   MAC_Address[3],
                   MAC_Address[4],
                   MAC_Address[5]);
    }
#endif

   //lRetVal = I2C_IF_Open(I2C_MASTER_MODE_FST);// I2C Init
   lRetVal = I2C_IF_Open(I2C_MASTER_MODE_STD);// I2C Init

   if(lRetVal < 0)
   {
      ERR_PRINT(lRetVal);
      UART_PRINT("I2C failed");
      LOOP_FOREVER();
   }

//*********************************************************************************************
   //AK975X
#ifdef AK975
   MAP_UtilsDelay(3);                                                                       //*
   SoftReset();                                                                             //*
   IntrrptSourceSet();                                                                      //*
   ECNTL1_rst ();//ECNTL1 reset to 0                                                        //*
   setCutoffFrequency(AK975X_FREQ_0_6HZ);   //AK975X_FREQ_0_3HZ = 000                       //*
   AK975XsetMode(AK975X_MODE_0); //Set to continuous read   1000                            //*
   AK9753_ID ();                                                                            //*
#endif
//*********************************************************************************************

     init_adc ();
     UART_PRINT("\n\rADC is initialized.");
#define temp
#ifdef temp
     //lRetVal = TMP006DrvOpen_();
     //if(lRetVal < 0)
     //{
        //ERR_PRINT(lRetVal);
         //LOOP_FOREVER();
       // while (1){
         //   g_bFeedWatchdog = true;
        //}
     //}

/*     lRetVal = BMA222Open_();
     if(lRetVal < 0)
     {
        ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
        while (1){
            g_bFeedWatchdog = true;
        }
     }

    AccSample_(); // Just do a single reading for now. TODO: Make Async.
    SetAccAvg_(); // g_accXAvg, g_accYAvg, g_accZAvg, g_accTotalAvg
*/
    //TMP006DrvGetTemp_(&sensorTemp);
#endif

#ifdef cloud
    //LTE connect
    GPIO_IF_GetPortNPin(SH_GPIO_0,&uiGPIOPort,&pucGPIOPin);    // Computes port and pin number from the GPIO number
    ucPinValue = GPIO_IF_Get(SH_GPIO_0,uiGPIOPort,pucGPIOPin); // Read pin status of GPIO0 -WiFi (PIN 50)
    if(ucPinValue == 0){
    credentials ();
    //UART_PRINT("\n\rCredentials are stored.");
    InitializeAppVariables();
    lRetVal = ConnectToAP();
    if(lRetVal < 0)//Could not connect to AP
    {
        UART_PRINT("\n\rCouldn't connect to AP.\n\r");//
        cnn_fail = 1;
        //EnterHIBernate();
    }else{
        lRetVal = ConnectToHTTPServer(&httpClient);
        if(lRetVal < 0){
            UART_PRINT("\n\rFailed to connect to a remote server.\n\r");
            cnn_fail = 1;
           //EnterHIBernate();
        }
    }

    sl_NetCfgGet(SL_MAC_ADDRESS_GET,NULL,&macAddressLen,(_u8 *)macAddressVal);
    UART_PRINT("\n MAC address: %02X:%02X:%02X:%02X:%02X:%02X \n",
               macAddressVal[0],
               macAddressVal[1],
               macAddressVal[2],
               macAddressVal[3],
               macAddressVal[4],
               macAddressVal[5]);
    }
#endif
    //timerA0_start(1000);
    //uiBufLen = 85;
    //cred_flg = 0;
    while(1){
        g_bFeedWatchdog = true;

        read_all_pixels (testPixelValue);
        //for(i = 0; i < 64; i++)
            //DBG_PRINT("%.2f \n", testPixelValue[i]);
        if (calculateHumanPresence(testPixelValue, 64)){
            UART_PRINT("HUMAN");
            GPIO_IF_Set(SH_GPIO_28,GPIOA3_BASE, 0x10,1);//Turn On
            GPIO_IF_Set(SH_GPIO_9,uiGPIOPort,pucGPIOPin,1);//Turn On
        } else {
            UART_PRINT("NO HUMAN");
            GPIO_IF_Set(SH_GPIO_28,GPIOA3_BASE, 0x10,0);//Turn Off
            GPIO_IF_Set(SH_GPIO_9,uiGPIOPort,pucGPIOPin,0);//Turn On
        }

        MAP_UtilsDelay(mSEC_TO_LOOP(80));//80ms delay. Clock tick at 80 Mhz
        MAP_UtilsDelay(mSEC_TO_LOOP(80));//80ms delay. Clock tick at 80 Mhz
        MAP_UtilsDelay(mSEC_TO_LOOP(80));//80ms delay. Clock tick at 80 Mhz

#ifdef cloud
        GPIO_IF_GetPortNPin(SH_GPIO_0,&uiGPIOPort,&pucGPIOPin);    // Computes port and pin number from the GPIO number
        ucPinValue = GPIO_IF_Get(SH_GPIO_0,uiGPIOPort,pucGPIOPin); // Read pin status of GPIO0 -WiFi (PIN 50)
        if((t_cntr > 100) && (ucPinValue == 0) && (cnn_fail == 0)){
            GPIO_IF_GetPortNPin(SH_GPIO_9,&uiGPIOPort,&pucGPIOPin);
            ucPinValue = GPIO_IF_Get(SH_GPIO_9,uiGPIOPort,pucGPIOPin); //Read LED ON/OFF status
            if(human == 1){//Human Detected
                if (ucPinValue == 1){//GPIO9 Red LED ON
                    cx = snprintf(buf, 99, "IR1=Human Detected & ms=%llX & TName=CGH & BLE=ON",
                                  (*(uint64_t *) macAddressVal) & 0x0000FFFFFFFFFFFF);
                }else{//GPIO9 Red LED OFF
                    cx = snprintf(buf, 99, "IR1=Human Detected & ms=%llX & TName=CGH & BLE=OFF",
                                  (*(uint64_t *) macAddressVal) & 0x0000FFFFFFFFFFFF);
                }
            }
            else{//No Human
                if (ucPinValue == 1){//GPIO9 Red LED ON
                    cx = snprintf(buf, 99, "IR1=Empty Stall & ms=%llX & TName=CGH & BLE=ON",
                                  (*(uint64_t *) macAddressVal) & 0x0000FFFFFFFFFFFF);
                }else{
                    cx = snprintf(buf, 99, "IR1=Empty Stall & ms=%llX & TName=CGH & BLE=OFF",
                                  (*(uint64_t *) macAddressVal) & 0x0000FFFFFFFFFFFF);
                }
            }
            UART_PRINT("\n buf:%s \n", buf);
            lRetVal = HTTPPostMethod_data(&httpClient);//POST data
            if(lRetVal < 0){// If failed post data to the web-page
                f_cntr++;
                UART_PRINT(" Failed Post Method");
                //    EnterHIBernate();
            }
            HTTPGetPageMethod(&httpClient);//Get Command
            if(lRetVal < 0){
                UART_PRINT(" Failed Get Method");
                //f_cntr++;
                //EnterHIBernate();
            }
            t_cntr = 0;
        }
        t_cntr++;
        if (f_cntr > 3){
            UART_PRINT(" Looks like a connection problem. Resetting...  ");
            EnterHIBernate();
        }
#endif
    }

}
