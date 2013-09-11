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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//=======================================================================
//  COPYRIGHT (C) STMicroelectronics 2007.  ALL RIGHTS RESERVED
//
//  Use of this source code is subject to the terms of your STMicroelectronics
//  development license agreement. If you did not accept the terms of such a license,
//  you are not authorized to use this source code.
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//========================================================================

//
// Power off/Suspend routines
//

#include <windows.h>
#include <nkintr.h>
#include <oal.h>
#include "shx.h"

static DWORD dwLastWakeupSource = SYSWAKE_UNKNOWN;
BYTE fInterruptWakeup[SYSINTR_MAXIMUM];
static BYTE fInterruptWakeupMask[SYSINTR_MAXIMUM];

DWORD OEMSetWakeupSource( DWORD dwSources)
{
    if (dwSources<SYSINTR_MAXIMUM) {
        fInterruptWakeupMask[dwSources]=1;
        return 1;
    }
    return 0;
}

DWORD OEMPowerManagerInit(void)
{
    memset(fInterruptWakeup,0,SYSINTR_MAXIMUM);
    memset(fInterruptWakeupMask,0,SYSINTR_MAXIMUM);
    OEMSetWakeupSource(SYSINTR_RTC_ALARM);

    return 1;
}

DWORD OEMResetWakeupSource( DWORD dwSources)
{
    if (dwSources<SYSINTR_MAXIMUM) {
        fInterruptWakeupMask[dwSources]=0;
        return 1;
    }
    return 0;
}

DWORD OEMGetWakeupSource(void)
{
    return dwLastWakeupSource;
}

void OEMIndicateIntSource(DWORD dwSources)
{
    if (dwSources<SYSINTR_MAXIMUM ) {
        fInterruptWakeup[dwSources]=1;
    }
}

void OEMClearIntSources()
{
    memset(fInterruptWakeup,0,SYSINTR_MAXIMUM);
}

DWORD OEMFindFirstWakeSource()
{
    DWORD dwCount;

    for (dwCount=0;dwCount<SYSINTR_MAXIMUM;dwCount++) {
        if (fInterruptWakeup[dwCount]&& fInterruptWakeupMask[dwCount])
            break;
    }

    return (dwCount == SYSINTR_MAXIMUM) ? SYSWAKE_UNKNOWN : dwCount;
}

VOID OEMPowerOff (void)
{
    DWORD dwWake;
    UINT8 sleepMode;

    SH4_CPG_REGS *pCPGRegs = OALPAtoUA(SH4_REG_PA_CPG);
    sleepMode = INREG8(&pCPGRegs->STBCR);   // keep previous sleep mode
    OUTREG8(&pCPGRegs->STBCR, 0x80);    // can selectively turn off peripherials here

    dwWake = OEMFindFirstWakeSource();

    while (dwWake == SYSWAKE_UNKNOWN) {
        INTERRUPTS_ON( );

        __asm("sleep");     // go to standby mode

        INTERRUPTS_OFF( );

        dwWake = OEMFindFirstWakeSource();
    }

    OUTREG8(&pCPGRegs->STBCR, sleepMode);   // restore previous sleep mode

    dwLastWakeupSource = dwWake;
    INTERRUPTS_ON( );
}

BOOL SHxPowerIoctl (
    UINT32 code, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf,
    UINT32 nOutBufSize, UINT32 *lpBytesReturned
)
{
    switch (code) {
    case IOCTL_HAL_ENABLE_WAKE:
    case IOCTL_HAL_DISABLE_WAKE:
        if(lpBytesReturned)
        {
            *lpBytesReturned = 0;
        }
        if (lpInBuf && (nInBufSize == sizeof(DWORD)))
        {
            DWORD dwReturn = 0;
            DWORD sysintr  = (*(PDWORD)lpInBuf);

            if(sysintr < SYSINTR_DEVICES || sysintr >= SYSINTR_MAXIMUM) {
                // Invalid SYSINTR value specified
                NKSetLastError (ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            else {
                if(code == IOCTL_HAL_ENABLE_WAKE) {
                    dwReturn = OEMSetWakeupSource(sysintr);
                }
                else {// IOCTL_HAL_DISABLE_WAKE
                    dwReturn = OEMResetWakeupSource(sysintr);
                }

                return ((BOOL)dwReturn);
            }
        }
        NKSetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;

    case IOCTL_HAL_GET_WAKE_SOURCE:
        if (lpBytesReturned)
        {
            *lpBytesReturned=sizeof(DWORD);
        }
        if (!lpOutBuf && nOutBufSize > 0) {
            NKSetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        else if (!lpOutBuf || nOutBufSize < sizeof(DWORD))
        {
            NKSetLastError (ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        else
        {
            *(PDWORD)lpOutBuf=OEMGetWakeupSource();
            return TRUE;
        }

    case IOCTL_HAL_PRESUSPEND:
        OEMClearIntSources();
        return TRUE;

    default:
        break;
    }
    NKSetLastError (ERROR_INVALID_PARAMETER);
    return FALSE;
}

