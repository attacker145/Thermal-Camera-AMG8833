#include <stdio.h>
#include <string.h>

// simplelink includes
#include "device.h"

// driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_hib1p2.h"
#include "timer.h"
#include "interrupt.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "utils_if.h"

// common interface includes
#include "network_if.h"
#include "gpio_if.h"
#include "common.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "utils_if.h."
#include "timer_if.h"
#include "pinmux.h"
#include "wdt_if.h"
#include "wdt.h"

#define APP_UDP_PORT            5001
#define SYS_CLK                 80000000
#define MAX_BUF                 512
#define OSI_STACK_SIZE          3000

unsigned short g_usTimerInts = 0;   // Variable used in Timer Interrupt Handler

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
void HIBUDPBroadcastTask(void *pvParameters);
void EnterHIBernate();

//*****************************************************************************
//
//! Periodic Timer Interrupt Handler
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
void TimerPeriodicIntHandler(void)
{
    unsigned long ulInts;
    //
    // Clear all pending interrupts from the timer we are
    // currently using.
    //
    ulInts = MAP_TimerIntStatus(TIMERA0_BASE, true);
    MAP_TimerIntClear(TIMERA0_BASE, ulInts);
    //
    // Increment our interrupt counter.
    //
    g_usTimerInts++;
    if(!(g_usTimerInts & 0x1))
    {
        //
        // Off Led
        //
        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    }
    else
    {
        //
        // On Led
        //
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    }
}

//****************************************************************************
//
//! Function to configure and start timer to blink the LED while device is
//! trying to connect to an AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
void LedTimerConfigNStart()
{
    //
    // Configure Timer for blinking the LED for IP acquisition
    //
    Timer_IF_Init(PRCM_TIMERA0,TIMERA0_BASE,TIMER_CFG_PERIODIC,TIMER_A,0);
    Timer_IF_IntSetup(TIMERA0_BASE,TIMER_A,TimerPeriodicIntHandler);
    Timer_IF_Start(TIMERA0_BASE,TIMER_A,100); // time value is in mSec
}

//****************************************************************************
//
//! Disable the LED blinking Timer as Device is connected to AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
void LedTimerDeinitStop()
{
    //
    // Disable the LED blinking Timer as Device is connected to AP
    //
    Timer_IF_Stop(TIMERA0_BASE,TIMER_A);
    Timer_IF_DeInit(TIMERA0_BASE,TIMER_A);

}

//****************************************************************************
//
//! Enter the HIBernate mode configuring the wakeup timer
//!
//! \param none
//!
//! This function
//!    1. Sets up the wakeup RTC timer
//!    2. Enables the RTC
//!    3. Enters into HIBernate
//!
//! \return None.
//
//****************************************************************************
/*
	sl_Stop(30);
	MAP_PRCMHibernateIntervalSet(330);
	MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
	MAP_PRCMHibernateEnter();
 */
void EnterHIBernate()
{
#define SLOW_CLK_FREQ           (32*1024) //32768
    //
    // Configure the HIB module RTC wake time
    //
    //MAP_PRCMHibernateIntervalSet(15 * SLOW_CLK_FREQ);//Wake up in 15 sec (unsigned long long ullTicks - 64 bit)
    MAP_PRCMHibernateIntervalSet(5 * SLOW_CLK_FREQ);//Wake up in 5 sec (unsigned long long ullTicks - 64 bit)

    //
    // Enable the HIB RTC
    //
    MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);

    DBG_PRINT(" HIB: Entering HIBernate...");
    MAP_UtilsDelay(80000);

    //
    // powering down SPI Flash to save power
    //
    Utils_SpiFlashDeepPowerDown();
    //
    // Enter HIBernate mode
    //
    MAP_PRCMHibernateEnter();
}
//*****************************************************************************
//
//! Application callback handler for WDT interrupt
//!
//! \param none
//!
//! This function
//!    1. Clears the WDT interrupt
//!
//! \return None.
//
//*****************************************************************************
void AppWDTCallBackHandler()
{
    MAP_WatchdogIntClear(WDT_BASE);
    DBG_PRINT("WDT Interrupt occured\n\r");
}

//*****************************************************************************
//
//! Application callback handler for GPT Timer A0 interrupt
//!
//! \param none
//!
//! This function
//!    1. Clears the WDT interrupt
//!
//! \return None.
//
//*****************************************************************************
void AppGPTCallBackHandler()
{
    MAP_TimerIntClear(TIMERA0_BASE,TIMER_TIMB_TIMEOUT|TIMER_TIMA_TIMEOUT);
    DBG_PRINT("GPT TimerA0 Interrupt occured\n\r");
}

//****************************************************************************
//
//! Implements Sleep followed by wakeup using WDT timeout
//!
//! \param none
//!
//! This function
//!    1. Implements Sleep followed by wakeup using WDT
//!
//! \return None.
//
//****************************************************************************
void PerformPRCMSleepWDTWakeup()
{
    //
    // Initialize the WDT
    //
    WDT_IF_Init(AppWDTCallBackHandler, (4 * SYS_CLK));

    //
    // Enable the Sleep Clock
    //
    MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_SLP_MODE_CLK);

    //
    // Enter SLEEP...WaitForInterrupt ARM intrinsic
    //
    DBG_PRINT("WDT_SLEEP: Entering Sleep\n\r");
    MAP_UtilsDelay(80000);
    MAP_PRCMSleepEnter();
    DBG_PRINT("WDT_SLEEP: Exiting Sleep\n\r");

    //
    // Disable the Sleep Clock
    //
    MAP_PRCMPeripheralClkDisable(PRCM_WDT, PRCM_SLP_MODE_CLK);

    //
    // Deinitialize the WDT
    //
    WDT_IF_DeInit();

    //
    // PowerOff WDT
    //
    MAP_PRCMPeripheralClkDisable(PRCM_WDT, PRCM_RUN_MODE_CLK);
}

//****************************************************************************
//
//! Implements Sleep followed by wakeup using GPT timeout
//!
//! \param none
//!
//! This function
//!    1. Implements Sleep followed by wakeup using GPT
//!
//! \return None.
//
//****************************************************************************
void PerformPRCMSleepGPTWakeup()
{
    //
    // Power On the GPT along with sleep clock
    //
    MAP_PRCMPeripheralClkEnable(PRCM_TIMERA0, PRCM_RUN_MODE_CLK);

    //
    // Initialize the GPT as One Shot timer
    //
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_ONE_SHOT, TIMER_BOTH, 0);
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_BOTH, AppGPTCallBackHandler);

	//
	// Start timer with value in mSec
	//
	Timer_IF_Start(TIMERA0_BASE, TIMER_BOTH, 4000);

    //
    // Enable the Sleep Clock
    //
    MAP_PRCMPeripheralClkEnable(PRCM_TIMERA0, PRCM_SLP_MODE_CLK);

    //
    // Enter SLEEP...WaitForInterrupt ARM intrinsic
    //
    DBG_PRINT("GPT_SLEEP: Entering Sleep\n\r");
    MAP_UtilsDelay(80000);
    MAP_PRCMSleepEnter();
    DBG_PRINT("GPT_SLEEP: Exiting Sleep\n\r");

    //
    // Disable the Sleep Clock
    //
    MAP_PRCMPeripheralClkDisable(PRCM_TIMERA0, PRCM_SLP_MODE_CLK);

    //
    // Deinitialize the GPT
    //
    Timer_IF_Stop(TIMERA0_BASE, TIMER_BOTH);
    Timer_IF_DeInit(TIMERA0_BASE, TIMER_BOTH);

    //
    // PowerOff GPT
    //
    MAP_PRCMPeripheralClkDisable(PRCM_TIMERA0, PRCM_RUN_MODE_CLK);
}
