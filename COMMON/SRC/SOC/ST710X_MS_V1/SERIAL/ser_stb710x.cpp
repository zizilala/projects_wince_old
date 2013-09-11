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
/*++

Module Name:

Abstract:

    ASC serial PDD for ST Micro 710x development board.

Notes:
--*/
#include <windows.h>
#include <types.h>
#include <ceddk.h>
#include <ddkreg.h>
#include <serhw.h>
#include <Serdbg.h>

#include <ser_stb710x.h>
#include "sysfreq.h"

#define PIO5_CTS         0x4
#define PIO5_RTS         0x8


CPdd710xSerial::~CPdd710xSerial()
{
    if (m_pIOPregs != NULL)
        {
        MmUnmapIoSpace((PVOID)m_pIOPregs, sizeof(STB7100_IOPORT_REG));
        }
}

BOOL CPdd710xSerial::Init()
{
    PHYSICAL_ADDRESS    ioPhysicalBase = { STB7100_PIO5_REGS_BASE, 0 };  // UART3 is configured via PIO5 pins
    ULONG               inIoSpace = 0;

    if (TranslateBusAddr(m_hParent,Internal,0, ioPhysicalBase,&inIoSpace,&ioPhysicalBase))
        {
        // Map it to our process
        m_pIOPregs = (STB7100_IOPORT_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(STB7100_IOPORT_REG),FALSE);
        }
    if (m_pIOPregs)
        {
        DDKISRINFO ddi;
        // Get SYSINTR value for this ASC IRQ line
        if (GetIsrInfo(&ddi)== ERROR_SUCCESS &&
                KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &ddi.dwIrq, sizeof(UINT32), &ddi.dwSysintr, sizeof(UINT32), NULL))
            {
            RegSetValueEx(DEVLOAD_SYSINTR_VALNAME,REG_DWORD,(PBYTE)&ddi.dwSysintr, sizeof(UINT32));
            }
        else
            {
            return FALSE;
            }

            // Route UART3_TXD to alternative function output.
            m_pIOPregs->CLR_PnC0 = 0x1;
            m_pIOPregs->SET_PnC1 = 0x1;
            m_pIOPregs->SET_PnC2 = 0x1;

            // Route UART3_RXD to PIO Input
            m_pIOPregs->CLR_PnC0 = 0x2;
            m_pIOPregs->CLR_PnC1 = 0x2;
            m_pIOPregs->CLR_PnC2 = 0x2;

            // We handle RTS and CTS signals manually in the driver.
            // Route UART3_CTS to PIO input
            m_pIOPregs->CLR_PnC0 = 0x4;
            m_pIOPregs->CLR_PnC1 = 0x4;
            m_pIOPregs->CLR_PnC2 = 0x4;

            // Route UART3_RTS to PIO output
            m_pIOPregs->CLR_PnC0 = 0x8;
            m_pIOPregs->SET_PnC1 = 0x8;
            m_pIOPregs->CLR_PnC2 = 0x8;
        }
    else
        {
        return FALSE;
        }

    // The ASC driver needs the peripheral clock frequency so that it
    // can accurately configure the baudrate generation register.
    STB710X_SYSFREQS SysFreqs;
    if (!KernelIoControl(IOCTL_HAL_GET_SYS_FREQ, NULL, 0, &SysFreqs, sizeof(SysFreqs), NULL))
        {
         DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("serial_stb710x!CPdd710xSerial::Init> Unable to determine peripheral clock frequency!\r\n")));
         return FALSE;
        }
    else
        {
        m_PeripheralClockFreq = SysFreqs.EMI_BUS;
        DEBUGMSG (ZONE_INIT,(TEXT("serial_stb710x!CPdd710xSerial::Init> Using peripheral clock frequency of %d Hz\r\n"), m_PeripheralClockFreq));
        }

    return CPddASCSerial::Init();
}

void  CPdd710xSerial::SetDefaultConfiguration()
{
    CPddASCSerial::SetDefaultConfiguration();

    // The platform code implements RTS/CTS via PIO signals.  Thus,
    // they may be dynamically configured.
    m_CommPorp.dwSettableParams |= SP_HANDSHAKING;
}

BOOL CPdd710xSerial::InitModem(BOOL bInit)
{
    // Set RTS active by default.
    SetRTS(TRUE);

    // The generic ASC hardware does not give the driver
    // visibility into the CTS line state.  We configure
    // PIO lines for RTS/CTS.  (See Init() above.)
    // Save current state of the CTS line.  ThreadRun() will
    // periodically awaken to call GetModemStatus, which will
    // check for a change in this line.
    m_ulCTSStatus = m_pIOPregs->PnIN & PIO5_CTS;

    return TRUE;
}

void CPdd710xSerial::SetRTS(BOOL bSet)
{
    m_HardwareLock.Lock();
    if (bSet)
        {
        // Bit 3 is wired to RTS pin.  See section 3.2.6 of 7100A.pdf.
        // For 16550 compatibility, this signal is active low.
        m_pIOPregs->PnOUT &= ~PIO5_RTS;
        }
    else
        {
        m_pIOPregs->PnOUT |= PIO5_RTS;
        }
    m_HardwareLock.Unlock();
    return;
}

ULONG CPdd710xSerial::GetModemStatus()
{
    ULONG ulReturn = CPddASCSerial::GetModemStatus();
    ULONG ulEvent = 0;

    // Inspect PnIN for the state of the CTS line
    // Bit 2 is wired to the CTS pin.  See secton 3.2.6 of 7100A.pdf.
    // For 16550 compatibility, this signal is active low.
    ULONG ulCTSStatus = m_pIOPregs->PnIN & PIO5_CTS;

    if (ulCTSStatus != m_ulCTSStatus)
        {
        m_ulCTSStatus = ulCTSStatus;
        ulEvent |= EV_CTS;
        }

    if (ulEvent != 0)
        {
        EventCallback(ulEvent);
        }
    ulReturn |= (ulCTSStatus == 0)?MS_CTS_ON:0;  // Active low signalling

    return (ulReturn);
}

