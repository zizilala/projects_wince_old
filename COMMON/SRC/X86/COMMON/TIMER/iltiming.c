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
#include <iltiming.h>
#include <oal.h>

// ILTIMING Globals
extern BOOL   fIntrTime;
extern DWORD dwIntrTimeIsr1;
extern DWORD dwIntrTimeIsr2;
extern DWORD dwIntrTimeNumInts;
extern DWORD dwIntrTimeCountdown;
extern DWORD dwIntrTimeCountdownRef;
extern DWORD dwIntrTimeSPC;

BOOL x86IoCtllTiming (
    UINT32 code, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32 *lpBytesReturned
) {

    if (lpBytesReturned) {
        *lpBytesReturned = 0;
    }

    if ((nInBufSize == sizeof(ILTIMING_MESSAGE)) && (lpInBuf != NULL)) {
        extern DWORD PerfCountSinceTick();
        extern DWORD PerfCountFreq();

        PILTIMING_MESSAGE pITM = (PILTIMING_MESSAGE) lpInBuf;

        switch (pITM->wMsg) {
        
        case ILTIMING_MSG_ENABLE : {
                dwIntrTimeCountdownRef = pITM->dwFrequency;
                RETAILMSG (1, (TEXT("ILTiming Enable (@ every %d ticks)\r\n"), dwIntrTimeCountdownRef));
                dwIntrTimeCountdown = dwIntrTimeCountdownRef;
                dwIntrTimeNumInts = 0;
                fIntrTime = TRUE;
                break;
            }

        case ILTIMING_MSG_DISABLE : {
                RETAILMSG (1, (TEXT("ILTiming Disable\r\n")));
                fIntrTime = FALSE;
                break;
            }

        case ILTIMING_MSG_GET_TIMES : {
                pITM->dwIsrTime1 = dwIntrTimeIsr1;
                pITM->dwIsrTime2 = dwIntrTimeIsr2;
                pITM->wNumInterrupts = (WORD) dwIntrTimeNumInts;
                pITM->dwSPC = dwIntrTimeSPC;
                pITM->dwFrequency = PerfCountFreq();
                dwIntrTimeNumInts = 0;
                break;
            }

        case ILTIMING_MSG_GET_PFN : {
                pITM->pfnPerfCountSinceTick = (PVOID) PerfCountSinceTick;
                RETAILMSG (1, (TEXT("ILTiming GetPFN\r\n")));
                break;
            }

        default : {
                RETAILMSG (1, (TEXT("IOCTL_HAL_ILTIMING : BAD MESSAGE!!!\r\n")));
                NKSetLastError(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }
        }

    } else {
        RETAILMSG (1, (TEXT("IOCTL_HAL_ILTIMING : BAD PARAMETERS!!!\r\n")));
        NKSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    return(TRUE);

}
