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
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  
//------------------------------------------------------------------------------
#include <windows.h>
#include <timer.h>
#include <pc.h>
#include <oal.h>

extern volatile ULARGE_INTEGER CurTicks;
extern DWORD  g_dwOALTimerCount;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD
PerfCountFreq()
{
    return TIMER_FREQ;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD
PerfCountSinceTick(void)
{
    WORD wCurCount;
    UCHAR ucInterrupt;

    __asm {
TRY_LATCHING_AGAIN: 
             
        pushfd                  ; Save interrupt state
        cli

        in      al,20h          ; get current interrupt state bits
        mov     ucInterrupt, al

        in      al,40h          ; clear latches, in case they are loaded
        in      al,40h          ; 
        mov     al,11000010b    ;latch status and count of counter 0
        out     43h,al
        in      al,40h          ; read status
        shl     eax, 16         ; move into high side of eax
        in      al,40h          ; read counter 0 lsb
        mov     ah,al
        in      al,40h          ; read msb
        xchg    ah, al          ; Get them in the right order
        mov     wCurCount, ax
        shr     eax, 16         ; get the status back into al
        popfd                   ; Restore interrupt state
        
        test    al, 40h         ; did the latch operation fail?
        jne     TRY_LATCHING_AGAIN     ; if so, just do it again
    }

    //
    // Note : this is a countdown timer, not count up.
    //

    if (ucInterrupt & (1 << INTR_TIMER0)) {
        return g_dwOALTimerCount + (g_dwOALTimerCount - wCurCount);
    } else {
        return g_dwOALTimerCount - wCurCount;
    }
}





//------------------------------------------------------------------------------
//
//  x86QueryPerformanceCounter
//  
//      The x86QueryPerformanceCounter function retrieves the current value of 
//      the high-resolution performance counter, if one exists. This is the defaul
//      implementation for x86. Individual BSP can implement its own if it's not 
//      suitable for it's platform.
//  
//  BOOL x86QueryPerformanceCounter (
//  
//      LARGE_INTEGER  *lpliPerformanceCount    // address of current counter value
//     );   
//  
//  Parameters
//  
//  lpliPerformanceCount
//  
//      Points to a variable that the function sets, in counts, to the current 
//      performance-counter value. If the installed hardware does not support 
//      a high-resolution performance counter, this parameter can be to zero. 
//  
//  Return Value
//  
//      If the installed hardware supports a high-resolution performance 
//      counter, the return value is TRUE.
//      If the installed hardware does not support a high-resolution 
//      performance counter, the return value is FALSE.   
//  
//
//------------------------------------------------------------------------------
BOOL 
x86QueryPerformanceCounter(
    LARGE_INTEGER *lpliPerformanceCount
    )
{
    ULARGE_INTEGER liBase;
    DWORD dwCurCount;

    // Make sure CurTicks is the same before and after read of counter to account for
    // possible rollover
    do {
        liBase = CurTicks;
        dwCurCount = PerfCountSinceTick( );
    } while  (liBase.LowPart != CurTicks.LowPart);
    
    lpliPerformanceCount->QuadPart = liBase.QuadPart + dwCurCount;

    return TRUE;
}



//------------------------------------------------------------------------------
//
//  x86QueryPerformanceFrequency
//  
//  The x86QueryPerformanceFrequency function retrieves the frequency of 
//  the high-resolution performance counter, if one exists. This is the defaul
//  implementation for x86. Individual BSP can implement its own if it's not 
//  suitable for it's platform.
//  
//  BOOL x86QueryPerformanceFrequency(
//  
//      LARGE_INTEGER  *lpliPerformanceFreq     // address of current frequency
//     );   
//  
//  Parameters
//  
//  lpliPerformanceFreq
//  
//  Points to a variable that the function sets, in counts per second, to 
//  the current performance-counter frequency. If the installed hardware 
//  does not support a high-resolution performance counter, this parameter
//  can be to zero. 
//  
//  Return Value
//  
//  If the installed hardware supports a high-resolution performance 
//  counter, the return value is TRUE.
//  If the installed hardware does not support a high-resolution 
//  performance counter, the return value is FALSE.
//
//------------------------------------------------------------------------------
BOOL 
x86QueryPerformanceFrequency(
    LARGE_INTEGER *lpliPerformanceFreq
    ) 
{
    lpliPerformanceFreq->HighPart = 0;
    lpliPerformanceFreq->LowPart  = PerfCountFreq ();
    return TRUE;
}


BOOL x86InitPerfCounter (void)
{
    //
    // setup function pointer for QPC
    //
    pQueryPerformanceCounter = x86QueryPerformanceCounter;
    pQueryPerformanceFrequency = x86QueryPerformanceFrequency;

    return TRUE;
}
