//*****************************************************************************
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

//*****************************************************************************
//
// Application Name     - Timer Demo
// Application Overview - This application is to showcases the usage of Timer
//                        DriverLib APIs. The objective of this application is
//                        to showcase the usage of 16 bit timers to generate
//                        interrupts which in turn toggle the state of the GPIO
//                        (driving LEDs).
//                        Two timers with different timeout value(one is twice
//                        the other) are set to toggle two different GPIOs which
//                        in turn drives two different LEDs, which will give a
//                        blinking effect.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Timer_Demo_Application
// or
// docs\examples\CC32xx_Timer_Demo_Application.pdf
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup timer_demo
//! @{
//
//*****************************************************************************

// Standard include
#include <stdio.h>

// Driverlib includes
#include "hw_types.h"
#include "interrupt.h"
#include "hw_ints.h"
#include "hw_apps_rcm.h"
#include "hw_common_reg.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "timer.h"
#include "utils.h"
#include "common.h"

// Common interface includes
#include "timer_if.h"
#include "gpio_if.h"

#include "pinmux.h"
#include "uart_if.h"



//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION        "1.1.1"
#define FOREVER                    1

//*****************************************************************************
//                      Global Variables for Vector Table
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//*****************************************************************************
//
// Globals used by the timer interrupt handler.
//
//*****************************************************************************
static volatile unsigned long g_ulSysTickValue;
static volatile unsigned long g_ulBase;
static volatile unsigned long g_ulRefBase;
static volatile unsigned long g_ulRefTimerInts = 0;
static volatile unsigned long g_ulIntClearVector;
extern volatile unsigned long g_ulTimerInts;
extern volatile unsigned long g_ulTimerInts_uart;
extern unsigned char door_flg;
extern volatile unsigned char acc_flt;

extern unsigned char BLE_timeout;
extern unsigned char timeout;
extern volatile unsigned char human;

extern void EnterHIBernate();

void GPIO_IF_Toggle(unsigned char SH_GPIO_XX);

#define SH_GPIO_00          	0

//*****************************************************************************
//
//! The interrupt handler for the second timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
void TimerRefIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    Timer_IF_InterruptClear(g_ulRefBase);

    g_ulRefTimerInts ++;
    GPIO_IF_LedToggle(MCU_RED_LED_GPIO);
}


//*****************************************************************************

//*****************************************************************************
//
//! The interrupt handler for the first timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
void TimerBaseIntHandler(void)
{

    g_ulBase = TIMERA0_BASE;            // Base address for first timer
    Timer_IF_InterruptClear(g_ulBase);  // Clear the timer interrupt.

    if ((timeout == 1) && (g_ulTimerInts>3)){//timeout is set when stall is empty
        //Report("Human is Reset");
        human = 0;//Enable ballast ON after a timeout
        g_ulTimerInts = 0;
    }
    //The timer is started in the uart_if_vXX.c. If wait for UART1 is too long reset.
    if ((acc_flt == 5) && (g_ulTimerInts_uart>5)){//ACC UART communication dead
        Report("UART timer triggered");
        EnterHIBernate();
    }

    g_ulTimerInts ++;

    if (acc_flt == 5)
        g_ulTimerInts_uart++;

    door_flg = 0;//reset door flag
}

//
//! main function demonstrates the use of the timers to generate
//! periodic interrupts.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
void timerA0_start(unsigned long ms)
{
    //
    // Base address for first timer
    //
    g_ulBase = TIMERA0_BASE;

    // Configuring the timers
    //
    Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_PERIODIC, TIMER_A, 0);

    //
    // Setup the interrupts for the timer timeouts.
    //
    Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerBaseIntHandler);

    //
    // Turn on the timers feeding values in mSec
    //
    Timer_IF_Start(g_ulBase, TIMER_A, ms);

    //Report("Timer is started \n\r");
}


void timerA0_stop(void)
{
    //
    // Base address for first timer
    //
    g_ulBase = TIMERA0_BASE;

    //
    // disable the timer
    //
    Timer_IF_Stop(g_ulBase, TIMER_A);

    //BLE_timeout = 0;
    //Report("timer STOP \n\r");
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************





