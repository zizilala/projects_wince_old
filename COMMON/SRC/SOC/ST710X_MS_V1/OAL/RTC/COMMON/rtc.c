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
//------------------------------------------------------------------------------
//
//  File:  rtc.c
//
#include <windows.h>
#include <oal.h>
#include <nkintr.h>
#include "shx.h"

#define MAX_RTC_YEAR    9999    //The RTC year counter has range from 0 to 9999.

//------------------------------------------------------------------------------
// Utility routines for converting RTC register values into decimal values
//
UINT16  RegValToDecimal(UINT8);
UINT16  RegValShortToDecimal(UINT16);
UINT8   DecimalToRegVal(UINT16);
UINT16  DecimalToRegValShort(UINT16);

//------------------------------------------------------------------------------
//
//  Function:  OEMGetRealTime
//
//  This function is called by the kernel to retrieve the time from
//  the real-time clock.
//
BOOL OEMGetRealTime(LPSYSTEMTIME lpst)
{
    SH4_RTC_REGS *pRTCRegs = OALPAtoUA(SH4_REG_PA_RTC);

    OALMSG(OAL_FUNC & OAL_RTC, (L"+OEMGetRealTime\r\n"));

    // Clear CIE flag
    OUTREG8(&pRTCRegs->RCR1, (INREG8(&pRTCRegs->RCR1) & (~RTC_RCR1_CIE)));

    do
    {
        // Clear carry flag
        OUTREG8(&pRTCRegs->RCR1, (INREG8(&pRTCRegs->RCR1) & (~RTC_RCR1_CF)));

        // Convert and populate SYSTEMTIME values
        lpst->wMilliseconds = 0;
        lpst->wSecond       = RegValToDecimal(INREG8(&pRTCRegs->RSECCNT));
        lpst->wMinute       = RegValToDecimal(INREG8(&pRTCRegs->RMINCNT));
        lpst->wHour         = RegValToDecimal(INREG8(&pRTCRegs->RHRCNT));
        lpst->wDayOfWeek    = INREG8(&pRTCRegs->RWKCNT);
        lpst->wDay          = RegValToDecimal(INREG8(&pRTCRegs->RDAYCNT));
        lpst->wMonth        = RegValToDecimal(INREG8(&pRTCRegs->RMONCNT));
        lpst->wYear         = RegValShortToDecimal(INREG16(&pRTCRegs->RYRCNT));
    } while (INREG8(&pRTCRegs->RCR1) & RTC_RCR1_CF);

    OALMSG(OAL_FUNC & OAL_RTC, (L"-OEMGetRealTime\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMSetRealTime
//
//  This function is called by the kernel to set the real-time clock.
//
BOOL OEMSetRealTime(LPSYSTEMTIME lpst)
{
    BOOL       retVal  = FALSE;
    SH4_RTC_REGS *pRTCRegs = OALPAtoUA(SH4_REG_PA_RTC);

    OALMSG(OAL_FUNC & OAL_RTC, (L"+OEMSetRealTime()\r\n"));

    if(lpst->wYear > MAX_RTC_YEAR)
    {
        goto cleanUp;
    }

    // First stop RTC
    OUTREG8(&pRTCRegs->RCR2, (INREG8(&pRTCRegs->RCR2) & (~RTC_RCR2_START)));

    // Now reset RTC
    OUTREG8(&pRTCRegs->RCR2, (INREG8(&pRTCRegs->RCR2) | RTC_RCR2_RESET));

    // Convert and set SYSTEMTIME into RTC registers
    OUTREG8(&pRTCRegs->RSECCNT, DecimalToRegVal(lpst->wSecond));
    OUTREG8(&pRTCRegs->RMINCNT, DecimalToRegVal(lpst->wMinute));
    OUTREG8(&pRTCRegs->RHRCNT,  DecimalToRegVal(lpst->wHour));
    OUTREG8(&pRTCRegs->RWKCNT,  (UINT8)lpst->wDayOfWeek);
    OUTREG8(&pRTCRegs->RDAYCNT, DecimalToRegVal(lpst->wDay));
    OUTREG8(&pRTCRegs->RMONCNT, DecimalToRegVal(lpst->wMonth));
    OUTREG16(&pRTCRegs->RYRCNT, DecimalToRegValShort(lpst->wYear));

    // Finally, start RTC
    OUTREG8(&pRTCRegs->RCR2, INREG8(&pRTCRegs->RCR2) | RTC_RCR2_START);
    retVal  = TRUE;

cleanUp:
    OALMSG(OAL_FUNC & OAL_RTC, (L"-OEMSetRealTime(rc = %d)\r\n", retVal));
    return retVal;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalInitRTC
//
//  This function is called by WinCE OS to initialize the time after boot.
//  Input buffer contains SYSTEMTIME structure with default time value. If
//  hardware has persistent real time clock it will ignore this value
//  (or all call).
//
BOOL OALIoCtlHalInitRTC(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize
) {
    SYSTEMTIME *lpst   = NULL;
    BOOL       retVal  = FALSE;

    OALMSG(OAL_FUNC & OAL_RTC, (L"+OALIoCtlHalInitRTC()\r\n"));

    if(pOutSize)
    {
        *pOutSize = 0;
    }

    // Validity checks
    if((code       != IOCTL_HAL_INIT_RTC) ||
       (pInpBuffer == NULL)               ||
       (inpSize    != sizeof(SYSTEMTIME)))
    {
        OALMSG(OAL_ERROR, (L"ERROR: Invalid calling parameters...returning\r\n"));
        retVal = FALSE;
        goto initExit;
    }

    // Call OEMSetRealTime
    lpst    = (SYSTEMTIME *)pInpBuffer;
    retVal  = OEMSetRealTime(lpst);

initExit:
    OALMSG(OAL_FUNC & OAL_RTC, (L"-OALIoCtlHalInitRTC(rc = %d)\r\n", retVal));
    return retVal;
}

//------------------------------------------------------------------------------
//
//  Function:   RegValtoDecimal
//
UINT16 RegValToDecimal(UINT8 regVal)
{
    UINT16 tensVal = 0;
    UINT16 onesVal = 0;

    onesVal = regVal & 0x0F;
    tensVal = ((regVal >> 4) & 0x0F) * 10;

    return (tensVal + onesVal);
}

UINT16 RegValShortToDecimal(UINT16 regVal)
{
    UINT16 thousandsVal = 0;
    UINT16 hundredsVal  = 0;

    hundredsVal  = ((regVal >> 8) & 0x0F) * 100;
    thousandsVal = ((regVal >> 12) & 0x0F) * 1000;

    return (thousandsVal + hundredsVal + RegValToDecimal((UINT8)(regVal & 0x00FF)));
}

//------------------------------------------------------------------------------
//
//  Function:   DecimalToRegVal
//
UINT8 DecimalToRegVal(UINT16 decimal)
{
    UINT8 tensVal = 0;
    UINT8 onesVal = 0;

    tensVal = (decimal / 10) << 4;
    onesVal = (decimal % 10);

    return (tensVal | onesVal);
}

UINT16 DecimalToRegValShort(UINT16 decimal)
{
    UINT16 thousandsVal = 0;
    UINT16 hundredsVal  = 0;

    thousandsVal = (decimal / 1000) << 12;
    hundredsVal  = ((decimal % 1000) / 100) << 8;

    return (thousandsVal | hundredsVal | DecimalToRegVal((UINT8)(decimal%100)));
}

//------------------------------------------------------------------------------
