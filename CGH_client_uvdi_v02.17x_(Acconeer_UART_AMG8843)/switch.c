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

#define cred

#define APPLICATION_VERSION "1.1.1"
#define APP_NAME            "UVDI \n\r main_v24x.c \n\r stats_v01x.c \n\r"

//#define POST_REQUEST_URI  "/post"
//#define POST_REQUEST_URI  "/post.html"

#include "network_defines.h"

#include "button_if.h"

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
#define SH_GPIO_30          30      /* PIN_53 - I2C bus PWR EN */
#define alarm_on  0
#define alarm_off 1

extern unsigned int uiGPIOPort;//02/17/2017
extern unsigned char pucGPIOPin;//02/17/2017

void sonar_cycle_pwr (void){

    //
    // Enable Peripheral Clocks
    //
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


    //
    // Configure PIN_53 for GPIO Output
    //
    PinTypeGPIO(PIN_53, PIN_MODE_0, false);
    GPIODirModeSet(GPIOA3_BASE, 0x40, GPIO_DIR_MODE_OUT);//I2C BUS power EN GPIO_30


    MAP_UtilsDelay(10000000);
    GPIO_IF_GetPortNPin(SH_GPIO_30,&uiGPIOPort,&pucGPIOPin); // Computes port and pin number from the GPIO number
    GPIO_IF_Set(SH_GPIO_30,uiGPIOPort,pucGPIOPin,1);//I2C power ((80000000/5)*x) -> 16000 000 - 1sec
    //MAP_UtilsDelay(16000000);
    //MAP_UtilsDelay(16000000);
    //MAP_UtilsDelay(16000000);
    //MAP_UtilsDelay(16000000);
    MAP_UtilsDelay(16000000);
    GPIO_IF_GetPortNPin(SH_GPIO_30,&uiGPIOPort,&pucGPIOPin); // Computes port and pin number from the GPIO number
    GPIO_IF_Set(SH_GPIO_30,uiGPIOPort,pucGPIOPin,0);//I2C power
    MAP_UtilsDelay(16000000);
}



