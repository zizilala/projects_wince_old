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
//------------------------------------------------------------------------------
//
//  File:  kitlioctl.c
//
#include <windows.h>
#include <oal.h>
#include <x86kitl.h>

extern BOOL OALIoCtlVBridge(
    UINT32 code, VOID *pInBuffer, UINT32 inSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize);
void RegisterKITL (void);
static BOOL ReserveKitlIRQ();

BOOL OEMKitlIoctl (DWORD code, VOID * pInBuffer, DWORD inSize, VOID * pOutBuffer, DWORD outSize, DWORD * pOutSize)
{
    BOOL fRet = FALSE;
    switch (code)
    {
        case IOCTL_HAL_INITREGISTRY:
            RegisterKITL();
            ReserveKitlIRQ();
            NKSetLastError (ERROR_NOT_SUPPORTED);
            // return FALSE, and set last error to Not-supported so the IOCTL got routed to OEMIoControl
            break;

        default:
            fRet = OALIoCtlVBridge (code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize);
    }

    return fRet;
}

static BOOL ReserveKitlIRQ()
{
    // If we're using an interrupt for KITL, make sure nothing else can share its IRQ
    // by reserving it in the resource manager

    BOOL retVal = FALSE;
    UINT32 kitlFlags;

    if(!OALKitlGetFlags(&kitlFlags))
    {
        KITL_RETAILMSG(ZONE_WARNING,("WARN: Reserve KITL IRQ: Could not get KITL flags, bailing out\r\n"));
        return FALSE;
    }

    // Are we using active, interrupt-based KITL?
    if((g_pNKGlobal->pfnKITLIoctl) &&
      (kitlFlags & OAL_KITL_FLAGS_ENABLED) &&
      !(kitlFlags & OAL_KITL_FLAGS_PASSIVE) &&
      !(kitlFlags & OAL_KITL_FLAGS_POLL))
    {
        DWORD dwStatus, dwDisp, dwEnableKITLSharedIRQ;
        DWORD dwSize = sizeof(DWORD);
        HKEY hkResourcesReserved;
        HKEY hkNoReserve;
        DWORD kitlIRQ = g_pX86KitlInfo->pX86Info->ucKitlIrq;
        const WCHAR szReservedIRQPath[] = L"Drivers\\Resources\\IRQ\\Reserved";
        const WCHAR szNoReservePath[] = L"Platform";

        // Check the registry to see if we've added a platform-specific key telling us disable IRQ sharing with the KITL NIC
        if(!(dwStatus = NKRegOpenKeyEx(HKEY_LOCAL_MACHINE, szNoReservePath, 0, 0, &hkNoReserve)))
        {
            if(!(dwStatus = NKRegQueryValueEx(hkNoReserve, TEXT("DisableKITLSharedIRQ"), NULL, NULL, (BYTE*)(&dwEnableKITLSharedIRQ), &dwSize)))
            {
                if(dwEnableKITLSharedIRQ == 1)
                {
                    // Registry has requested that disable IRQ sharing for KITL (this can help verify debugger hangs
                    // that occur due to IRQ-sharing hangs)
 
                    // If we got here then go ahead and reserve the IRQ
                    if(!(dwStatus = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, szReservedIRQPath, 0, NULL, 0, 0, NULL, &hkResourcesReserved, &dwDisp)))
                    {
                        if(!(dwStatus = NKRegSetValueEx(hkResourcesReserved, TEXT("KitlIRQ"), 0, REG_DWORD, (BYTE*)(&kitlIRQ), sizeof(DWORD))))
                        {
                            KITL_RETAILMSG (ZONE_INIT,("Reserve KITL IRQ: Reserved IRQ %d.  Other device drivers will not load on this IRQ.\r\n",kitlIRQ));
                            retVal = TRUE;
                        }
                        else
                        {
                            KITL_RETAILMSG(ZONE_INIT, ("Reserve KITL IRQ: Error creating reserved value\r\n"));
                        }
                    }
                    else
                    {
                        KITL_RETAILMSG(ZONE_INIT, ("Reserve KITL IRQ: Error creating reserved key\r\n"));
                    }
                }
            }
        }
    }
    else
    {
        // Not using active interrupt-based KITL, don't need to reserve an IRQ
        retVal = TRUE;
    }

    if(!retVal)
    {        
        KITL_RETAILMSG(ZONE_INIT, ("Reserve KITL IRQ: No IRQ reserved, KITL NIC IRQ may be shared with other devices.\r\n"));
    }

    return retVal;
}


