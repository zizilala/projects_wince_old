//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

Abstract:  

Functions:


Notes: 

--*/

#undef WINCEMACRO

#include <windows.h>
#include <nkintr.h>
#include <pc.h>
#include <wdm.h>
#include <bootarg.h>
#include <x86boot.h>
#include <oal.h>
#include <x86kitl.h>
#include <pci.h>

#define LS_TSR_EMPTY        0x40
#define LS_THR_EMPTY        0x20
#define LS_RX_BREAK         0x10
#define LS_RX_FRAMING_ERR   0x08
#define LS_RX_PARITY_ERR    0x04
#define LS_RX_OVERRUN       0x02
#define LS_RX_DATA_READY    0x01

#define LS_RX_ERRORS        ( LS_RX_FRAMING_ERR | LS_RX_PARITY_ERR | LS_RX_OVERRUN )

//
// Global variables.
//

PUCHAR IoPortBase;

//   14400 = 8
//   16457 = 7 +/-
//   19200 = 6
//   23040 = 5
//   28800 = 4
//   38400 = 3
//   57600 = 2
//  115200 = 1

static X86BootInfo x86Info;
PX86BootInfo g_pX86Info = &x86Info;
UINT8 g_x86uuid[16];

extern LPCSTR  g_oalDeviceNameRoot;

void OEMWriteDebugLED (WORD wIndex, DWORD dwPattern)
{
    // no LED
}

static X86KITLINFO kitlinfo = {
    &x86Info,                           // PX86BootInfo pX86Info;
    
    (FARPROC) PCIReadBARs,              // FARPROC pfnPCIReadBARs;
    (FARPROC) PCIInitInfo,              // FARPROC pfnPCIInitInfo;
    (FARPROC) PCIReadBusData,           // FARPROC pfnPCIReadBusData;
    (FARPROC) PCIWriteBusData,          // FARPROC pfnPCIWriteBusData;
    (FARPROC) PCIGetBusDataByOffset,    // FARPROC pfnPCIGetBusDataByOffset;
    (FARPROC) PCIReg,                   // FARPROC pfnPCIReg;
};

void InitBootInfo (BOOT_ARGS *pBootArgs)
{
    if (BOOT_ARG_VERSION_SIG != pBootArgs->dwVersionSig) {
        // nothing passed from bootloader, set default
        BSPInitDfltBootInfo (&x86Info);
        
    } else {
        x86Info.dwKitlIP            = pBootArgs->EdbgAddr.dwIP;
        x86Info.dwKitlBaseAddr      = pBootArgs->dwEdbgBaseAddr;
        x86Info.dwKitlDebugZone     = pBootArgs->dwEdbgDebugZone;
        x86Info.KitlTransport       = pBootArgs->KitlTransport;
        x86Info.ucKitlAdapterType   = pBootArgs->ucEdbgAdapterType;
        x86Info.ucComPort           = pBootArgs->ucComPort;
        x86Info.ucBaudDivisor       = pBootArgs->ucBaudDivisor;
        x86Info.ucKitlIrq           = pBootArgs->ucEdbgIRQ;
        x86Info.dwRebootAddr        = (BOOTARG_SIG == pBootArgs->dwEBootFlag)? pBootArgs->dwEBootAddr : g_pNKGlobal->dwStartupAddr;
        x86Info.cxDisplayScreen     = pBootArgs->cxDisplayScreen;
        x86Info.cyDisplayScreen     = pBootArgs->cyDisplayScreen;
        x86Info.bppScreen           = pBootArgs->bppScreen;
        x86Info.ucPCIConfigType     = pBootArgs->ucPCIConfigType;
        x86Info.NANDBootFlags       = pBootArgs->NANDBootFlags;     // Boot flags related to NAND support.
        x86Info.NANDBusNumber       = pBootArgs->NANDBusNumber;     // NAND controller PCI bus number.
        x86Info.NANDSlotNumber      = pBootArgs->NANDSlotNumber;    // NAND controller PCI slot number.
        x86Info.fStaticIP           = pBootArgs->EdbgFlags & EDBG_FLAGS_STATIC_IP;
        memcpy (&x86Info.wMac[0], &pBootArgs->EdbgAddr.wMAC[0], sizeof (x86Info.wMac));
        memcpy (&x86Info.szDeviceName[0], &pBootArgs->szDeviceNameRoot[0], sizeof (x86Info.szDeviceName));
    }

    g_pOemGlobal->pKitlInfo = &kitlinfo;

    // if name root not specified, use BSP specific name root.
    // NOTE: device name can change when KITL started.
    if (!x86Info.szDeviceName[0]) {
        strncpy (x86Info.szDeviceName, g_oalDeviceNameRoot, KITL_MAX_DEV_NAMELEN);
    } else {
        g_oalDeviceNameRoot = (LPCSTR) pBootArgs->szDeviceNameRoot;
    }

    // initialize fields that are required to be non-zero
    if (!x86Info.ucBaudDivisor) {
        x86Info.ucBaudDivisor = 3;  // default to 38400
    }
    if (!x86Info.dwRebootAddr) {
        x86Info.dwRebootAddr = g_pNKGlobal->dwStartupAddr;      // no bootloader, set to startup
    }
    if (!x86Info.ucPCIConfigType) {
        x86Info.ucPCIConfigType = 1;                // default PCI config type 1
    }

    // set up our UUID based upon the MAC address of the ethernet card
    // note that this is not really universally unique, but we return it
    // for test purposes

    // Microsoft test manufacturer ID
    g_x86uuid[0] = (UCHAR)0x00;
    g_x86uuid[1] = (UCHAR)0x30;
    g_x86uuid[2] = (UCHAR)0xBD;
    g_x86uuid[3] = (UCHAR)0x2D;
    g_x86uuid[4] = (UCHAR)0x73;
    g_x86uuid[5] = (UCHAR)0x32;

    // Next 16-bits: Version/variant - Version 1.8 format (48/16/64) from Windows Mobile docs
    g_x86uuid[6] = (UCHAR)1;
    g_x86uuid[7] = (UCHAR)8;

    // Last 64-bits: Unique ID
    g_x86uuid[8]  = (UCHAR) ((x86Info.wMac[0] >> 0) & 0xFF);
    g_x86uuid[9]  = (UCHAR) ((x86Info.wMac[0] >> 8) & 0xFF);
    g_x86uuid[10] = (UCHAR) ((x86Info.wMac[1] >> 0) & 0xFF);
    g_x86uuid[11] = (UCHAR) ((x86Info.wMac[1] >> 8) & 0xFF);
    g_x86uuid[12] = (UCHAR) ((x86Info.wMac[2] >> 0) & 0xFF);
    g_x86uuid[13] = (UCHAR) ((x86Info.wMac[2] >> 8) & 0xFF);
    g_x86uuid[14] = (UCHAR) ((x86Info.wMac[3] >> 0) & 0xFF);
    g_x86uuid[15] = (UCHAR) ((x86Info.wMac[3] >> 8) & 0xFF);

}


void OEMInitDebugSerial(void)
{

    // Locate bootargs (this is the first opportunity the OAL has to initialize this global).
    //
    InitBootInfo ((BOOT_ARGS *) ((ULONG)(*(PBYTE *)BOOT_ARG_PTR_LOCATION) | 0x80000000));

    switch ( g_pX86Info->ucComPort ) {
    case 1:
        IoPortBase = (PUCHAR)COM1_BASE;
        break;

    case 2:
        IoPortBase = (PUCHAR)COM2_BASE;
        break;

    case 3:
        IoPortBase = (PUCHAR)COM3_BASE;
        break;

    case 4:
        IoPortBase = (PUCHAR)COM4_BASE;
        break;

    default:
        IoPortBase = 0;
        break;

    }

    if ( IoPortBase ) {
        WRITE_PORT_UCHAR(IoPortBase+comLineControl, 0x80);   // Access Baud Divisor
        WRITE_PORT_UCHAR(IoPortBase+comDivisorLow, g_pX86Info->ucBaudDivisor); 
        WRITE_PORT_UCHAR(IoPortBase+comDivisorHigh, 0x00);
        WRITE_PORT_UCHAR(IoPortBase+comFIFOControl, 0x01);   // Enable FIFO if present
        WRITE_PORT_UCHAR(IoPortBase+comLineControl, 0x03);   // 8 bit, no parity
        WRITE_PORT_UCHAR(IoPortBase+comIntEnable, 0x00);     // No interrupts, polled
        WRITE_PORT_UCHAR(IoPortBase+comModemControl, 0x03);  // Assert DTR, RTS
        OEMWriteDebugString(TEXT("Debug Serial Init\r\n"));
    }



    // Turn on ether debug zones here if you need to debug edbg.
    // KITLSetDebug(0xffff);
}


void OEMWriteDebugByte(BYTE ucChar)
{
    if ( IoPortBase ) {
        while ( !(READ_PORT_UCHAR(IoPortBase+comLineStatus) & LS_THR_EMPTY) ) {
            ;
        }

        WRITE_PORT_UCHAR(IoPortBase+comTxBuffer, ucChar);
    }
}

int OEMReadDebugByte(void)
{
    unsigned char   ucStatus;
    unsigned char   ucChar;

    if ( IoPortBase ) {
        ucStatus = READ_PORT_UCHAR(IoPortBase+comLineStatus);

        if ( ucStatus & LS_RX_DATA_READY ) {
            ucChar = READ_PORT_UCHAR(IoPortBase+comRxBuffer);

            if ( ucStatus & LS_RX_ERRORS ) {
                return (OEM_DEBUG_COM_ERROR);
            } else {
                return (ucChar);
            }

        }
    }

    return (OEM_DEBUG_READ_NODATA);
}


/*****************************************************************************
*
*
*   @func   void    |   OEMClearDebugComError | Clear a debug communications er
or
*
*/
void
OEMClearDebugCommError(
                      void
                      )
{
    if ( IoPortBase ) {
    }
}

