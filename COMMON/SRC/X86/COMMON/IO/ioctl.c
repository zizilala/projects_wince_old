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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//  
// -----------------------------------------------------------------------------
#include <windows.h>
#include <ceddk.h>
#include <hal.h>
#include <x86boot.h>
#include <PCIReg.h>
#include <oal.h>

extern BOOL RegisterBINFS_NAND (PX86BootInfo pX86BootInfo);
static BOOL DisableSerialDriverForDebugPort();
static BOOL DisableSerialDriverForKITLPort();
static BOOL DisableSerialDriverForPort(UCHAR port);

#define driverRegPathOverrideLength 128
#define SERIAL_KITL_PORT 2

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
ULONG
OEMGetBusDataByOffset(
                         IN BUS_DATA_TYPE BusDataType,
                         IN ULONG BusNumber,
                         IN ULONG SlotNumber,
                         IN PVOID Buffer,
                         IN ULONG Offset,
                         IN ULONG Length
                         )
{
    switch (BusDataType) {
    case PNPISAConfiguration:
        return(ISAGetBusDataByOffset(BusNumber, SlotNumber, Buffer, Offset, Length));

    case PCIConfiguration:
        return(PCIGetBusDataByOffset(BusNumber, SlotNumber, Buffer, Offset, Length));

    default:
        return(0);
    }
}



// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
ULONG
OEMSetBusDataByOffset(
                         IN BUS_DATA_TYPE BusDataType,
                         IN ULONG BusNumber,
                         IN ULONG SlotNumber,
                         IN PVOID Buffer,
                         IN ULONG Offset,
                         IN ULONG Length
                         )
{
    switch (BusDataType) {
    case PNPISAConfiguration:
        return(ISASetBusDataByOffset(BusNumber, SlotNumber, Buffer, Offset, Length));

    case PCIConfiguration:
        return(PCISetBusDataByOffset(BusNumber, SlotNumber, Buffer, Offset, Length));

    default:
        return(0);
    }
}

void printPCIConfig(PCI_COMMON_CONFIG* config)
{
    DEBUGMSG(1, (TEXT("+printPCIConfig\r\n")));
    DEBUGMSG(1, (TEXT("config.VendorID      = 0x%x\r\n"), config->VendorID));
    DEBUGMSG(1, (TEXT("config.DeviceID      = 0x%x\r\n"), config->DeviceID));
    DEBUGMSG(1, (TEXT("config.Command       = 0x%x\r\n"), config->Command));
    DEBUGMSG(1, (TEXT("config.Status        = 0x%x\r\n"), config->Status));
    DEBUGMSG(1, (TEXT("config.RevisionID    = 0x%x\r\n"), config->RevisionID));
    DEBUGMSG(1, (TEXT("config.ProgIf        = 0x%x\r\n"), config->ProgIf));
    DEBUGMSG(1, (TEXT("config.SubClass      = 0x%x\r\n"), config->SubClass));
    DEBUGMSG(1, (TEXT("config.BaseClass     = 0x%x\r\n"), config->BaseClass));
    DEBUGMSG(1, (TEXT("config.CacheLineSize = 0x%x\r\n"), config->CacheLineSize));
    DEBUGMSG(1, (TEXT("config.LatencyTimer  = 0x%x\r\n"), config->LatencyTimer));
    DEBUGMSG(1, (TEXT("config.HeaderType    = 0x%x\r\n"), config->HeaderType));
    DEBUGMSG(1, (TEXT("config.BIST          = 0x%x\r\n"), config->BIST));
    DEBUGMSG(1, (TEXT("config.BaseAddresses[0] = 0x%x\r\n"), config->u.type1.BaseAddresses[0]));
    DEBUGMSG(1, (TEXT("config.BaseAddresses[1] = 0x%x\r\n"), config->u.type1.BaseAddresses[1]));
    DEBUGMSG(1, (TEXT("config.PrimaryBusNumber = 0x%x\r\n"), config->u.type1.PrimaryBusNumber));
    DEBUGMSG(1, (TEXT("config.SecondaryBusNumber = 0x%x\r\n"), config->u.type1.SecondaryBusNumber));
    DEBUGMSG(1, (TEXT("config.SubordinateBusNumber          = 0x%x\r\n"), config->u.type1.SubordinateBusNumber));
    DEBUGMSG(1, (TEXT("config.SecondaryLatencyTimer          = 0x%x\r\n"), config->u.type1.SecondaryLatencyTimer));
    DEBUGMSG(1, (TEXT("config.IOBase          = 0x%x\r\n"), config->u.type1.IOBase));
    DEBUGMSG(1, (TEXT("config.IOLimit          = 0x%x\r\n"), config->u.type1.IOLimit));
    DEBUGMSG(1, (TEXT("config.SecondaryStatus          = 0x%x\r\n"), config->u.type1.SecondaryStatus));
    DEBUGMSG(1, (TEXT("config.MemoryBase          = 0x%x\r\n"), config->u.type1.MemoryBase));
    DEBUGMSG(1, (TEXT("config.MemoryLimit          = 0x%x\r\n"), config->u.type1.MemoryLimit));
    DEBUGMSG(1, (TEXT("config.PrefetchableMemoryBase          = 0x%x\r\n"), config->u.type1.PrefetchableMemoryBase));
    DEBUGMSG(1, (TEXT("config.PrefetchableMemoryLimit          = 0x%x\r\n"), config->u.type1.PrefetchableMemoryLimit));
    DEBUGMSG(1, (TEXT("config.PrefetchableMemoryBaseUpper32          = 0x%x\r\n"), config->u.type1.PrefetchableMemoryBaseUpper32));
    DEBUGMSG(1, (TEXT("config.PrefetchableMemoryLimitUpper32          = 0x%x\r\n"), config->u.type1.PrefetchableMemoryLimitUpper32));
    DEBUGMSG(1, (TEXT("config.IOBaseUpper          = 0x%x\r\n"), config->u.type1.IOBaseUpper));
    DEBUGMSG(1, (TEXT("config.IOLimitUpper          = 0x%x\r\n"), config->u.type1.IOLimitUpper));
    DEBUGMSG(1, (TEXT("config.Reserved2         = 0x%x\r\n"), config->u.type1.Reserved2));
    DEBUGMSG(1, (TEXT("config.ExpansionROMBase          = 0x%x\r\n"), config->u.type1.ExpansionROMBase));
    DEBUGMSG(1, (TEXT("config.InterruptLine          = 0x%x\r\n"), config->u.type1.InterruptLine));
    DEBUGMSG(1, (TEXT("config.InterruptPin          = 0x%x\r\n"), config->u.type1.InterruptPin));
    DEBUGMSG(1, (TEXT("config.BridgeControl          = 0x%x\r\n"), config->u.type1.BridgeControl));
    DEBUGMSG(1, (TEXT("-printPCIConfig\r\n")));
}

BOOL x86IoCtlHalDdkCall (
    UINT32 code, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32 *lpBytesReturned
) {
    DWORD dwErr = 0;

    if (lpBytesReturned) {
        *lpBytesReturned = 0;
    }

    // For now, the only ddk ioctls in CEPC are SetBusData, GetBusData,
    // ReadBusData, WriteBusData, and TransBusAddress.
    // Eventually x86 PCI code should use platform\common
    // more extensively and use the common implementation of the DDK ioctl.
    if (!lpInBuf) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else if (nInBufSize == sizeof(BUSDATA_PARMS)) {
        PBUSDATA_PARMS pbd = (PBUSDATA_PARMS)lpInBuf;
        
        switch (*(DWORD *)lpInBuf) {
        case IOCTL_HAL_SETBUSDATA:
            pbd->ReturnCode = OEMSetBusDataByOffset(
                                                 ((PBUSDATA_PARMS)lpInBuf)->BusDataType,
                                                 ((PBUSDATA_PARMS)lpInBuf)->BusNumber,
                                                 ((PBUSDATA_PARMS)lpInBuf)->SlotNumber,
                                                 ((PBUSDATA_PARMS)lpInBuf)->Buffer,
                                                 ((PBUSDATA_PARMS)lpInBuf)->Offset,
                                                 ((PBUSDATA_PARMS)lpInBuf)->Length
                                                 );
            break;
        case IOCTL_HAL_GETBUSDATA:
            pbd->ReturnCode = OEMGetBusDataByOffset(
                                                 ((PBUSDATA_PARMS)lpInBuf)->BusDataType,
                                                 ((PBUSDATA_PARMS)lpInBuf)->BusNumber,
                                                 ((PBUSDATA_PARMS)lpInBuf)->SlotNumber,
                                                 ((PBUSDATA_PARMS)lpInBuf)->Buffer,
                                                 ((PBUSDATA_PARMS)lpInBuf)->Offset,
                                                 ((PBUSDATA_PARMS)lpInBuf)->Length
                                                 );
            break;
        default:
            dwErr = ERROR_INVALID_PARAMETER;
        }
    } else if (nInBufSize == sizeof(OAL_DDK_PARAMS)) {
        OAL_DDK_PARAMS* pParams = (OAL_DDK_PARAMS*)lpInBuf;
        ULONG device, function, bus, size, offset;

        bus = (pParams->busData.devLoc.LogicalLoc >> 16) & 0xFF;   // Bus Number
        device = (pParams->busData.devLoc.LogicalLoc >> 8) & 0xFF; // Device Number
        function = (pParams->busData.devLoc.LogicalLoc) & 0xFF;    // Function Number.
        size = pParams->busData.length;
        offset = pParams->busData.offset;

        switch (*(DWORD *)lpInBuf) {
        case IOCTL_OAL_READBUSDATA:
            pParams->rc = PCIReadBusData (bus, device, function, pParams->busData.pBuffer, offset, size);
            break;
        case IOCTL_OAL_WRITEBUSDATA:
            pParams->rc = PCIWriteBusData (bus, device, function, pParams->busData.pBuffer, offset, size);
            break;
        case IOCTL_OAL_TRANSBUSADDRESS:
            // No special translation necessary for x86, just return success.
            pParams->rc = 1; // success
            break;
        default:
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    } else {
        // didn't match any valid input sizes
        dwErr = ERROR_INVALID_PARAMETER;
    }

    if (dwErr)
        NKSetLastError (dwErr);

    return !dwErr;
}


BOOL x86IoCtlHalInitRegistry (
    UINT32 code, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32 *lpBytesReturned
) 
{
    // If we don't have any NAND we don't care about the results of RegisterBINFS_NAND,
    // and if we don't have a serial port, we don't care about the results of 
    // DisableSerialDriverForDebugPort.  Therefore, this function is considered
    // successful regardless of the return value of these two functions.
    BOOL rc = TRUE;

    if (lpBytesReturned) {
        *lpBytesReturned = 0;
    }

    RegisterBINFS_NAND(g_pX86Info);

    DisableSerialDriverForDebugPort();
    DisableSerialDriverForKITLPort();
    return rc;
}


static BOOL DisableSerialDriverForDebugPort()
{
    BOOL rc = TRUE;
    if(g_pX86Info->ucComPort == 0) {
        // We're not using serial debug output so we don't have to disabled anything
        OALMSG(OAL_INFO&&OAL_IO,(TEXT("Override Serial Driver: No COM port selected for serial debug output, no override necessary.\r\n")));
        RETAILMSG (1,(TEXT("Override Serial Driver: No COM port selected for serial debug output, no override necessary.\r\n")));
    } else {
        if(rc = DisableSerialDriverForPort(g_pX86Info->ucComPort)) {
            RETAILMSG (1,(TEXT("Override Serial Driver: serial debug output selected on COM port %d, disabling serial driver for that port.\r\n"),g_pX86Info->ucComPort));
        }
    }
    return rc;
}

static BOOL DisableSerialDriverForKITLPort()
{
    BOOL rc = TRUE;
    if(g_pX86Info->KitlTransport != KTS_SERIAL) {
        // We're not using serial debug output so we don't have to disabled anything
        OALMSG(OAL_INFO&&OAL_IO,(TEXT("Override Serial Driver: No COM port selected for serial KITL transport, no override necessary.\r\n")));
        RETAILMSG (1,(TEXT("Override Serial Driver: No COM port selected for serial KITL transport, no override necessary.\r\n")));
    } else {
        if(rc = DisableSerialDriverForPort(SERIAL_KITL_PORT)) {
            RETAILMSG (1,(TEXT("Override Serial Driver: serial KITL transport selected on COM port %d, disabling serial driver for that port.\r\n"),SERIAL_KITL_PORT));
        }
    }
    return rc;
}

static BOOL DisableSerialDriverForPort(UCHAR port)
{
    // If we're using serial debug output, don't load the serial driver for its COM port
    BOOL retVal = FALSE;
    DWORD dwStatus;
    DWORD noLoadFlag = 0x00000004;
    LPCWSTR szSerialBootArgsPath = L"Drivers\\BootArg";
    
    // "SerialDbg " - the final blank (0x0020) character will be filled in later in this function
    WCHAR szSerialSubKeyName[11] = L"SerialDbg ";

    HKEY hkSerialBootArgs = NULL;
    HKEY hkSerialBuiltInEntry = NULL;
    DWORD driverToDisableBytes = driverRegPathOverrideLength*(sizeof(WCHAR)/sizeof(BYTE));
    WCHAR szDriverToDisable[driverRegPathOverrideLength]; // large enough to hold a regpath string to a builtin serial driver

    szSerialSubKeyName[9] = (WCHAR)(port + 0x30); // convert the COM port to a character
    if(!(dwStatus = NKRegOpenKeyEx(HKEY_LOCAL_MACHINE, szSerialBootArgsPath, 0, 0, &hkSerialBootArgs))) {
        // Opened key successfully, try to get the regpath to the driver to disable
        if(!(dwStatus = NKRegQueryValueEx(hkSerialBootArgs, szSerialSubKeyName, NULL, NULL, (LPBYTE) szDriverToDisable, &driverToDisableBytes))) {
            // Read regpath of driver to disable, open the key and write the Device:NoLoad Flag to that regpath
            DWORD dwDisp;
            if(!(dwStatus = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, szDriverToDisable, 0, NULL, 0, 0, NULL, &hkSerialBuiltInEntry, &dwDisp))) {
                if(!(dwStatus = NKRegSetValueEx(hkSerialBuiltInEntry, TEXT("Flags"), 0, REG_DWORD, (BYTE*)(&noLoadFlag), sizeof(DWORD)))) {
                    OALMSG (OAL_INFO&&OAL_IO,(TEXT("Disabling serial driver for port %d.\r\n"), port));
                    retVal = TRUE;
                }
                else {
                    OALMSG (OAL_WARN&&OAL_IO,(TEXT("Override Serial Driver: failed to write disable flag to builtin driver, dwStatus = %d\r\n"),dwStatus));
                }
                NKRegCloseKey(hkSerialBootArgs);
            }
            else {
                OALMSG (OAL_WARN&&OAL_IO,(TEXT("Override Serial Driver: failed to open key of builtin driver key, dwStatus = %d\r\n"),dwStatus));
            }            
        }
        else {
            OALMSG (OAL_WARN&&OAL_IO,(TEXT("Override Serial Driver: failed to query value for regpath of builtin driver, dwStatus = %d\r\n"),dwStatus));
        }
        NKRegCloseKey(hkSerialBootArgs);
    }
    else {
        OALMSG (OAL_WARN&&OAL_IO,(TEXT("Override Serial Driver: failed to open bootarg key, dwStatus = %d\r\n"),dwStatus));
    }

    return retVal;
}

