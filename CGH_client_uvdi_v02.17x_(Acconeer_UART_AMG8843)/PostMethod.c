/*
 * PostMethod.c
 *
 *  Created on: Sep 21, 2018
 *      Author: RChak
 */


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

#include "network_defines.h"

//*****************************************************************************
//
//! \brief HTTP POST Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int HTTPPostMethod_SQL_delete(HTTPCli_Handle httpClient)
{
    bool moreFlags = 1;
    bool lastFlag = 1;
    char tmpBuf[4];
    long lRetVal = 0;
    char buf[50];
    const char *strPtr1;

    HTTPCli_Field fields[4] = {
                                    {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                    {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                    {HTTPCli_FIELD_NAME_CONTENT_TYPE, "application/x-www-form-urlencoded"},
                                    {NULL, NULL}
                                };

    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);//Prints response from the webpage

    /* Send POST method request. */
    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
       other than setted in previous call HTTPCli_setRequestFields() at later stage.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
    */
    moreFlags = 1;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_POST, POST_DELETE_SQL, moreFlags);//POST & post.php
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send Delete SQL request header.\n\r");
        return lRetVal;
    }

    /*
     * Here we are setting lastFlag = 1 as it is last header field.
     * Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
     * Send the length of sent data
    */
    lastFlag = 1;
    lRetVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (const char *)tmpBuf, lastFlag);
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP POST request header.\n\r");
        return lRetVal;
    }

    strPtr1 = "& Erase";// POST body
    strcpy ( buf, strPtr1 );
    /* Send POST data/body */
    //lRetVal = HTTPCli_sendRequestBody(httpClient, POST_DATA, (sizeof(POST_DATA)-1));
    lRetVal = HTTPCli_sendRequestBody(httpClient, buf, (sizeof(buf)-1)); // POST data body
    if(lRetVal < 0)
    {
        UART_PRINT("Failed to send HTTP POST request body.\n\r");
        return lRetVal;
    }


    //lRetVal = readResponse(httpClient);

    return lRetVal;
}

