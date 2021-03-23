/*
 * tftp.c
 *
 *  Created on: Jan 11, 2021
 *      Author: Roman.Chak
 */

//Standard includes
#include <stdlib.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"

// Standard includes
#include "simplelink.h"

// OS includes
//#include "osi.h"

//Common includes

#include "network_if.h"
#include "uart_if.h"
#include "udma_if.h"
#include "common.h"

//App includes
#include "pinmux.h"
//#include "datatypes.h"
#include "tftp.h"


//*****************************************************************************
//                      GLOBAL VARIABLES for VECTOR TABLE
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//*****************************************************************************
//                          LOCAL DEFINES
//*****************************************************************************
#define APP_NAME        "TFTP Client"
#define TFTP_IP         0xC0A8006D  /* This is the host IP ftp.cnktechlabs.com*/
#define FILE_SIZE_MAX   (20*1024)   /* Max File Size set to 20KB */
#define SSID            "cc3200demo"
#define SSID_KEY        ""
#define TFTP_READ       0       /* Set '1' to read else '0' */
#define TFTP_WRITE      1       /* Set '1' to write else '0' */

#define SLEEP_TIME              80000000
#define OSI_STACK_SIZE          2048




void TFTPTask(void *pvParameters)
{
    SlSecParams_t secParams;
    unsigned char *pucFileBuffer = NULL;  // Data read or to be written
    unsigned long uiFileSize;

    char *FileRead = "readFromServer.txt";  // File to be read using TFTP. Change string to filename
    char *FileWrite = "writeToServer.txt"; // File to be written using TFTP. Change string to filename.

    long pFileHandle;           // Pointer to file handle
    SlFsFileInfo_t pFsFileInfo;
    long lRetVal = -1;
    unsigned short uiTftpErrCode;

#ifdef conn
    // Configuring security parameters for the AP
    secParams.Key = NULL;
    secParams.KeyLen = 0;
    secParams.Type = SL_SEC_TYPE_OPEN;

    lRetVal = Network_IF_InitDriver(ROLE_STA);


    // Connecting to WLAN AP - Set with static parameters defined at the top
    // After this call we will be connected and have IP address
    lRetVal = Network_IF_ConnectAP(SSID,secParams);

    UART_PRINT("Connecting to TFTP server %d.%d.%d.%d\n\r",\
                  SL_IPV4_BYTE(TFTP_IP, 3),SL_IPV4_BYTE(TFTP_IP, 2),
                  SL_IPV4_BYTE(TFTP_IP, 1),SL_IPV4_BYTE(TFTP_IP, 0));
#endif
    if(TFTP_READ)
    {
        uiFileSize = FILE_SIZE_MAX;

        pucFileBuffer = malloc(uiFileSize);
        if(NULL == pucFileBuffer)
        {
            UART_PRINT("Can't Allocate Resources\r\n");
            LOOP_FOREVER();
        }

        memset(pucFileBuffer,'\0',uiFileSize);

        lRetVal = sl_TftpRecv(TFTP_IP, FileRead, (char *)pucFileBuffer,\
                                &uiFileSize, &uiTftpErrCode );
        if(lRetVal < 0)
        {
            free(pucFileBuffer);
            ERR_PRINT(lRetVal);
            LOOP_FOREVER();
        }

        lRetVal = sl_FsGetInfo((unsigned char *)FileRead, NULL, &pFsFileInfo);

        if(lRetVal < 0 )
            lRetVal = sl_FsOpen((unsigned char *)FileRead,\
                    FS_MODE_OPEN_CREATE(FILE_SIZE_MAX,_FS_FILE_OPEN_FLAG_COMMIT|\
                      _FS_FILE_PUBLIC_WRITE), NULL,&pFileHandle);
        else
            lRetVal = sl_FsOpen((unsigned char *)FileRead,FS_MODE_OPEN_WRITE, \
                                NULL,&pFileHandle);

        if(lRetVal < 0)
        {
            free(pucFileBuffer);
            ERR_PRINT(lRetVal);
            LOOP_FOREVER();
        }

        lRetVal = sl_FsWrite(pFileHandle,0, pucFileBuffer, uiFileSize);

        if(lRetVal < 0)
        {
            free(pucFileBuffer);
            lRetVal = sl_FsClose(pFileHandle,0,0,0);
            ERR_PRINT(lRetVal);
            LOOP_FOREVER();
        }

        UART_PRINT("TFTP Read Successful \r\n");

        lRetVal = sl_FsClose(pFileHandle,0,0,0);

    }

    if(TFTP_WRITE)
    {
        /* open same file which has been written with the server's file content */
        lRetVal = sl_FsOpen((unsigned char *)FileRead,FS_MODE_OPEN_READ, \
                                NULL,&pFileHandle);
        if(lRetVal < 0)
        {
            free(pucFileBuffer);
            ERR_PRINT(lRetVal);
            LOOP_FOREVER();
        }

        lRetVal = sl_FsGetInfo((unsigned char *)FileRead, NULL, &pFsFileInfo);
        if(lRetVal < 0)
        {
            lRetVal = sl_FsClose(pFileHandle,0,0,0);
            free(pucFileBuffer);
            ERR_PRINT(lRetVal);
            LOOP_FOREVER();
        }

        uiFileSize = (&pFsFileInfo)->FileLen;

        lRetVal = sl_FsRead(pFileHandle, 0,  pucFileBuffer, uiFileSize);
        if(lRetVal < 0)
        {
            lRetVal = sl_FsClose(pFileHandle,0,0,0);
            free(pucFileBuffer);
            ERR_PRINT(lRetVal);
            LOOP_FOREVER();
        }

        lRetVal = sl_FsClose(pFileHandle,0,0,0);
        /* write to server with different file name */
        lRetVal = sl_TftpSend(TFTP_IP, FileWrite,(char *) pucFileBuffer,\
                            &uiFileSize, &uiTftpErrCode  );
        if(lRetVal < 0)
        {
            free(pucFileBuffer);
            ERR_PRINT(lRetVal);
            LOOP_FOREVER();
        }

        UART_PRINT("TFTP Write Successful \r\n");
    }

    LOOP_FOREVER();
}
