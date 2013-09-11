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
   This file implements the NK kernel interfaces for the real time clock.
 
Functions:


Notes: 

--*/


#include <windows.h>
#include <nkintr.h>
#include <pc.h>
#include <oal.h>

// NOTE: A problem has been found with some chipsets such that
// setting the time to 23:59:59 on the 29th or 30th day of a month which
// has less than 31 days causes the clock to roll over incorrectly.
// Uncomment the following line to fix this problem.  However, be aware 
// that the fix consists of responding to calls that set the time to 
// HH:MM:59 by instead setting the time to HH:MM:58.
//#define HARDWARE_TIME_SET_PROBLEM 1


SYSTEMTIME RTC_AlarmTime;
CRITICAL_SECTION RTC_critsect;


BOOL Bare_SetAlarmTime(LPSYSTEMTIME lpst);
BOOL Bare_SetRealTime(LPSYSTEMTIME lpst);
BOOL Bare_GetRealTime(LPSYSTEMTIME lpst);


BOOL OEMGetRealTime(LPSYSTEMTIME lpst)
{
    BOOL RetVal;

    EnterCriticalSection(&RTC_critsect);
    RetVal = Bare_GetRealTime(lpst);
    LeaveCriticalSection(&RTC_critsect);

    return RetVal;
}

BOOL OEMSetRealTime(LPSYSTEMTIME lpst)
{
    BOOL RetVal;

    EnterCriticalSection(&RTC_critsect);
    RetVal = Bare_SetRealTime(lpst);
    LeaveCriticalSection(&RTC_critsect);
    
    return RetVal;
}

BOOL OEMSetAlarmTime(LPSYSTEMTIME lpst)
{
    BOOL RetVal;

    EnterCriticalSection(&RTC_critsect);
    RetVal = Bare_SetAlarmTime(lpst);
    LeaveCriticalSection(&RTC_critsect);
    
    return RetVal;
}

BOOL x86IoCtlHalInitRTC (
    UINT32 code, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32 *lpBytesReturned
) {
    // We have a battery backed CMOS based clock.  Don't actually do anything
    // to the RTC unless it appears to have been corrupted.  I guess the best
    // way to detect corruption is by checksumming CMOS

    if (lpBytesReturned) {
        *lpBytesReturned = 0;
    }

    return TRUE;
}

