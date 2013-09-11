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
/*++

Module Name:

Abstract:

    IR PDD for STMicro ST202T SoC (common code).

Notes:
--*/
#include <windows.h>
#include <types.h>
#include <ceddk.h>
#include <ddkreg.h>
#include <st202t_ir.h>

#include <ircdebug.h>
#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("IRC"), {
    TEXT("Init"),TEXT("Deinit"),TEXT("Warning"),TEXT("Error"),
    TEXT("Api"),TEXT("Read"),TEXT("Open"),TEXT("Thread"),
    TEXT("Function"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined") },
    0xffff
};
#endif

//
// Default configuration
//
DWORD g_DefaultTxWatermark = DEFAULT_IRC_TX_WATERMARK;
DWORD g_DefaultRxWatermark = DEFAULT_IRC_RX_WATERMARK;
BOOL  g_DefaultWakeupOnRx = DEFAULT_IRC_WAKE_ON_RX;
DWORD g_MsgBufCount = DEFAULT_IRC_NUM_MSGBUF;
DWORD g_Priority256 = DEFAULT_IRC_PRIORITY;

RegST202T_IR::RegST202T_IR(PULONG pRegAddr)
:   m_pReg(pRegAddr)
{
    m_fIsBackedUp = FALSE;
}

BOOL   RegST202T_IR::Init()
{
    if (m_pReg)
    {
        // Clear registers
        Write_TX_PRESCALER(0);
        Write_TX_SUBCARR(0);
        Write_TX_INTEN(0);
        Write_TX_EN(0);
        Write_TX_SUBCAR_WIDTH(0);
        Write_RX_INTEN(0);
        Write_RX_EN(0);
        Write_RX_MAX_SYMPERIOD(0);
        Write_RX_NOISE_SUPP_WIDTH(0);
        Write_RC_IRDA_CTRL(0);
        Write_RX_SAMPLERATE(0);
        Write_RX_POLARITY_INV(0);
        Write_RX_CLK_SEL(0);
        return TRUE;
    }
    else
        return FALSE;
}

#if ST202T_POWERMGMT
void RegST202T_IR::Backup()
{
    m_STIR_TX_PRESCALERBackup = Read_TX_PRESCALER();
    m_STIR_TX_SUBCARRBackup = Read_TX_SUBCARR();
    m_STIR_TX_INTENBackup = Read_TX_INTEN();
    m_STIR_TX_SUBCARRWIDTHBackup = Read_TX_SUBCAR_WIDTH();
    m_STIR_RX_INTENBackup = Read_RX_INTEN();
    m_STIR_RX_MAXSYMBPERIODBackup = Read_RX_MAX_SYMPERIOD();
    m_STIR_RX_NOISESUPPWIDTHBackup = Read_RX_NOISE_SUPP_WIDTH();
    m_STIR_RX_IRDACTRLBackup = Read_RC_IRDA_CTRL();
    m_STIR_RX_POLINVBackup = Read_RX_POLARITY_INV();
    m_STIR_RX_SAMPLERATEBackup = Read_RX_SAMPLERATE();
    m_STIR_RX_CLKSELBackup = Read_RX_CLK_SEL();

    m_fIsBackedUp = TRUE;
}

void RegST202T_IR::Restore()
{
    if (m_fIsBackedUp)
    {
        Write_TX_PRESCALER(m_STIR_TX_PRESCALERBackup);
        Write_TX_SUBCARR(m_STIR_TX_SUBCARRBackup);
        Write_TX_INTEN(m_STIR_TX_INTENBackup);
        Write_TX_SUBCAR_WIDTH(m_STIR_TX_SUBCARRWIDTHBackup);
        Write_RX_INTEN(m_STIR_RX_INTENBackup);
        Write_RX_MAX_SYMPERIOD(m_STIR_RX_MAXSYMBPERIODBackup);
        Write_RX_NOISE_SUPP_WIDTH(m_STIR_RX_NOISESUPPWIDTHBackup);
        Write_RC_IRDA_CTRL(m_STIR_RX_IRDACTRLBackup);
        Write_RX_POLARITY_INV(m_STIR_RX_POLINVBackup);
        Write_RX_SAMPLERATE(m_STIR_RX_SAMPLERATEBackup);
        Write_RX_CLK_SEL(m_STIR_RX_CLKSELBackup);

        m_fIsBackedUp = FALSE;
    }
}
#endif

#ifdef DEBUG
void RegST202T_IR::DumpRegister()
{
    NKDbgPrintfW(TEXT("DumpRegister:\r\n\
                       TX_PRE_SCALER=0x%x\r\n\
                       TX_SUB_CARR=0x%x\r\n\
                       TX_INT_EN=0x%x\r\n\
                       TX_INT_STA=0x%x\r\n\
                       TX_EN=0x%x\r\n\
                       TX_SUB_CARR_WIDTH=0x%x\r\n\
                       TX_STA=0x%x\r\n\
                       RX_INT_EN=0x%x\r\n\
                       RX_INT_STA=0x%x\r\n\
                       RX_EN=0x%x\r\n\
                       RX_MAX_SYM_PERIOD=0x%x\r\n\
                       RX_NOISE_SUPP_WIDTH=0x%x\r\n\
                       RC_IRDA_CTRL=0x%x\r\n\
                       RX_SAMPLE_RATE_COMM=0x%x\r\n\
                       RX_POL_INV=0x%x\r\n\
                       RX_STA=0x%x\r\n\
                       RX_CLK_SEL=0x%x\r\n\
                       RX_CLK_SEL_STA=0x%x\r\n"),
        Read_TX_PRESCALER(), Read_TX_SUBCARR(), Read_TX_INTEN(), Read_TX_INTSTA(),
        Read_TX_EN(), Read_TX_SUBCAR_WIDTH(), Read_TX_STA(), Read_RX_INTEN(),
        Read_RX_INTSTA(), Read_RX_EN(), Read_RX_MAX_SYMPERIOD(), Read_RX_NOISE_SUPP_WIDTH(),
        Read_RC_IRDA_CTRL(), Read_RX_SAMPLERATE(), Read_RX_POLARITY_INV(), Read_RX_STA(),
        Read_RX_CLK_SEL(), Read_RX_CLK_SEL_STA());
}
#endif // DEBUG

ST202T_IR::ST202T_IR (LPCTSTR lpActivePath)
:   CMiniThread (0, TRUE)
{
    m_pActiveReg = new CRegistryEdit(HKEY_LOCAL_MACHINE, lpActivePath);
    m_pDeviceReg = new CRegistryEdit(lpActivePath);
    m_pRegST202T_IR= NULL;
    m_dwSysIntr = MAXDWORD;
    m_pRegVirtualAddr = NULL;
    m_hISTEvent = NULL;
#if ST202T_XMIT
    m_XmitFlushDone =  CreateEvent(0, FALSE, FALSE, NULL);
#endif
    m_PeripheralClockFreq = 0;
    m_hDevDll = NULL;
    m_bModeSet = FALSE;
    m_pRxMsg = NULL;
    m_hRxEvent = NULL;
    m_hBusAccess = CreateBusAccessHandle( lpActivePath );
    m_bCurrentlyOpen = 0;
    m_bConstructed = m_pActiveReg && m_pActiveReg->IsKeyOpened()
                     && m_pDeviceReg && m_pDeviceReg->IsKeyOpened()
#if ST202T_XMIT
                     && m_XmitFlushDone
#endif
                     && m_hBusAccess;
}

ST202T_IR::~ST202T_IR()
{
    m_bConstructed = FALSE;

    if (m_hISTEvent)
    {
        m_bTerminated=TRUE;
        ThreadStart();
        SetEvent(m_hISTEvent);
        ThreadTerminated(1000);
        InterruptDisable( m_dwSysIntr );
        CloseHandle(m_hISTEvent);
    }
    if (m_pRegST202T_IR)
    {
        delete m_pRegST202T_IR;
    }
#if ST202T_XMIT
    if (m_XmitFlushDone)
    {
        CloseHandle(m_XmitFlushDone);
    }
#endif
    if (m_pRegVirtualAddr != NULL)
    {
        MmUnmapIoSpace((PVOID)m_pRegVirtualAddr,0UL);
    }
    if (m_hBusAccess != NULL)
    {
        CloseBusAccessHandle(m_hBusAccess);
    }
    if (m_hDevDll != NULL)
    {
        FreeLibrary(m_hDevDll);
    }
    if (m_pActiveReg != NULL)
    {
        delete m_pActiveReg;
    }
    if (m_pDeviceReg != NULL)
    {
        delete m_pDeviceReg;
    }
}

BOOL ST202T_IR::Init()
{
    BOOL bRet = FALSE;
    TCHAR  DevDll[DEVDLL_LEN];
    DWORD val;

    if (!IsConstructed())
    {
        goto CleanUp;
    }

    // IST Setup
    DDKISRINFO ddi;
    if (m_pDeviceReg->GetIsrInfo(&ddi)!= ERROR_SUCCESS)
    {
        goto CleanUp;
    }
    m_dwSysIntr = ddi.dwSysintr;
    if (m_dwSysIntr != MAXDWORD && m_dwSysIntr != 0 )
    {
        m_hISTEvent = CreateEvent(0,FALSE,FALSE,NULL);
    }

    if (m_hISTEvent!=NULL)
    {
        InterruptInitialize(m_dwSysIntr,m_hISTEvent,0,0);
    }
    else
    {
        goto CleanUp;
    }

    if (m_pDeviceReg->GetRegValue(PC_REG_STIR_PROTOCOL_HANDLER_VAL_NAME, (LPBYTE) DevDll, sizeof(DevDll)))
    {
        m_hDevDll = ::LoadDriver(DevDll);
    }
    if (m_hDevDll == NULL)
    {
        DEBUGMSG(ZONE_INIT | ZONE_ERROR, (TEXT( "LoadDriver(%s) failed %d\r\n"), DevDll, GetLastError()) );
        goto CleanUp;
    }

    // Protocol handler DLL successfully loaded
    // Setup protocol handler function pointers
    m_pfnDecode = (pfnRC_Decode)GetProcAddress(m_hDevDll, TEXT("RC_Decode"));
    m_pfnEncode = (pfnRC_Encode)GetProcAddress(m_hDevDll, TEXT("RC_Encode"));
    m_pfnGetCapabilities = (pfnRC_GetCapabilities)GetProcAddress(m_hDevDll, TEXT("RC_GetCapabilities"));
    m_pfnSetMode = (pfnRC_SetMode)GetProcAddress(m_hDevDll, TEXT("RC_SetMode"));
    m_pfnGetSignalProperties = (pfnRC_GetSignalProperties)GetProcAddress(m_hDevDll, TEXT("RC_GetSignalProperties"));

    // Ensure all protocol handler functions exist
    if (!(m_pfnDecode && m_pfnEncode && m_pfnGetCapabilities && m_pfnSetMode && m_pfnGetSignalProperties))
    {
        goto CleanUp;
    }

    if (m_pDeviceReg->GetRegValue(PC_REG_STIR_RX_WATERMARK_VAL_NAME,(PBYTE)&val,sizeof(DWORD))
        && IS_VALID_IRC_INTERRUPT_WATERMARK(val))
    {
        g_DefaultRxWatermark = val;
    }

    if (m_pDeviceReg->GetRegValue(PC_REG_STIR_TX_WATERMARK_VAL_NAME,(PBYTE)&val,sizeof(DWORD))
        && IS_VALID_IRC_INTERRUPT_WATERMARK(val))
    {
        g_DefaultTxWatermark = val;
    }

    if (m_pDeviceReg->GetRegValue(PC_REG_STIR_PRIORITY_VAL_NAME,(LPBYTE)&val,sizeof(DWORD)))
    {
        g_Priority256 = val;
    }

    if (!MapHardware() || !CreateHardwareAccess())
    {
        goto CleanUp;
    }

    // we'll no longer need the registry keys
    delete m_pActiveReg;
    m_pActiveReg = NULL;
    delete m_pDeviceReg;
    m_pDeviceReg = NULL;

    bRet = TRUE;

CleanUp:
    return bRet;
}

BOOL ST202T_IR::MapHardware()
{
    if (m_pRegVirtualAddr != NULL)
    {
        return TRUE;
    }

    // Get IO Window From Registry
    DDKWINDOWINFO dwi;
    DWORD st;
    if ( (st = m_pDeviceReg->GetWindowInfo( &dwi)) != ERROR_SUCCESS ||
            dwi.dwNumMemWindows < 1 ||
            dwi.memWindows[0].dwBase == 0 ||
            dwi.memWindows[0].dwLen < STIR_TOTAL_REGISTER_SPACE)
    {
        return FALSE;
    }

    DWORD dwInterfaceType;
    if (m_pActiveReg->GetRegValue( DEVLOAD_INTERFACETYPE_VALNAME, (PBYTE)&dwInterfaceType,sizeof(DWORD)))
    {
        dwi.dwInterfaceType = dwInterfaceType;
    }

    // Translate to System Address.
    PHYSICAL_ADDRESS    ioPhysicalBase = { dwi.memWindows[0].dwBase, 0};
    ULONG               inIoSpace = 0;
    if (TranslateBusAddr(m_hBusAccess,(INTERFACE_TYPE)dwi.dwInterfaceType,dwi.dwBusNumber, ioPhysicalBase,&inIoSpace,&ioPhysicalBase))
    {
        m_pRegVirtualAddr = MmMapIoSpace(ioPhysicalBase, dwi.memWindows[0].dwLen,FALSE);
    }

    return (m_pRegVirtualAddr != NULL);
}

BOOL ST202T_IR::CreateHardwareAccess()
{
    if (m_pRegST202T_IR)
    {
        return TRUE;
    }
    if (m_pRegVirtualAddr != NULL)
    {
        m_pRegST202T_IR = new RegST202T_IR((PULONG)m_pRegVirtualAddr);
        if (m_pRegST202T_IR && !m_pRegST202T_IR->Init())
        { // FALSE.
            delete m_pRegST202T_IR ;
            m_pRegST202T_IR = NULL;
        }
    }
    return (m_pRegST202T_IR != NULL);
}

void ST202T_IR::PostInit()
{
    DWORD dwCount=0;

    m_HardwareLock.Lock();

    // Disable transmitter and receivers
    m_pRegST202T_IR->Write_TX_EN(STIR_TX_DISABLE);
    m_pRegST202T_IR->Write_RX_EN(STIR_RX_DISABLE);
    m_pRegST202T_IR->Write_UHF_EN(STUHF_RX_DISABLE);

    // Disable all IR interrupt sources for now
    m_pRegST202T_IR->Write_TX_INTEN(0);
    m_pRegST202T_IR->Write_RX_INTEN(0);
    m_pRegST202T_IR->Write_UHF_INTEN(0);

    // Determine the appropriate sampling rate clock for the receiver.
    // This won't change for the life of the driver instance.
    SetRxSamplingClockAndError();

    // Further initialization will take place when the port
    // is opened.

    m_HardwareLock.Unlock();
    CeSetPriority(g_Priority256);

#ifdef DEBUG
    if ( ZONE_INIT )
        {
        m_pRegST202T_IR->DumpRegister();
        }
#endif

    ThreadStart();  // Start IST.
}

DWORD ST202T_IR::ThreadRun()
{
    while ( m_hISTEvent!=NULL && !IsTerminated())
    {
        if (WaitForSingleObject( m_hISTEvent, INFINITE) == WAIT_OBJECT_0)
        {
            if (!IsTerminated() )
            {
                volatile DWORD stat = m_pRegST202T_IR->Read_RX_INTSTA();
                // Check for receiver interrupt
                if (stat)
                {                                    \
                    // handle the receiver interrup
                    ReceiveInterruptHandler();

                    // Clear the receiver interrupts
                    m_pRegST202T_IR->Write_RX_CLR( STIR_RX_INT_ALL_SRC );
                }

                InterruptDone(m_dwSysIntr);
            }
        }
        else
        {
            ASSERT(FALSE);
        }
    }
    return 1;
}

#if ST202T_XMIT
BOOL  ST202T_IR::InitXmit(BOOL bInit)
{
    if (bInit)
        {
        m_HardwareLock.Lock();

        // Reset (and flush) Tx FIFO.

        m_HardwareLock.Unlock();
        }
    else
        {
        // Make Sure data has been trasmitted out before returning.
        }
    return TRUE;
}

DWORD   ST202T_IR::GetWriteableSize()
{
    DWORD dwWriteSize = 0;
    DWORD dwTxState = m_pRegST202T_IR->Read_TX_STA();

    // First, determine the number of Tx FIFO pairs which are full.
    DWORD dwTxFullFifos = (dwTxState & STIR_TX_STA_FIFO_LEVEL_MASK) >> STIR_TX_STA_FIFO_LEVEL_SHIFT;

    return (STIR_FIFO_DEPTH - dwTxFullFifos);
}

void    ST202T_IR::XmitInterruptHandler(PUCHAR pTxBuffer, ULONG *pBuffLen)
{
    PREFAST_DEBUGCHK(pBuffLen!=NULL);

    m_HardwareLock.Lock();
    if (*pBuffLen == 0)
        {
        EnableXmitInterrupt(FALSE);
        }
    else
        {
        // Encode and transmit data
        }
    m_HardwareLock.Unlock();
}


BOOL    ST202T_IR::EnableXmitInterrupt(BOOL fEnable)
{
    m_HardwareLock.Lock();
    if (fEnable)
        {
        // Enable desired Tx interrupts
        }
    else
        {
        // Disable Tx interrupts
        }
    m_HardwareLock.Unlock();

    return TRUE;
}

BOOL  ST202T_IR::CancelXmit()
{
    return InitXmit(TRUE);
}
#endif // ST202T_XMIT

// Receive
BOOL    ST202T_IR::InitReceive(BOOL bInit)
{
    BOOL    bRet = FALSE;
    m_HardwareLock.Lock();

    // Disable Rx and error interrupts
    m_pRegST202T_IR->Write_RX_INTEN(0);

    // Disable the receiver
    m_pRegST202T_IR->Write_RX_EN(STIR_RX_DISABLE);

    // Reset the decoder's state
    m_pfnDecode(NULL, NULL, NULL, NULL, TRUE, 0);

    // Flush Rx FIFOs
    FlushRxFIFOs();

    if (bInit && m_bModeSet) // Setup the receiver only if the decoding mode has been set
    {
        ULONG ulData = 0;

        // Set the timeout for end-symbol identification
        m_pRegST202T_IR->Write_RX_MAX_SYMPERIOD(STIR_RX_MAX_RC_SYMBOLTIME);

        // Get Rx trigger level
        ulData |= WatermarkBits[m_dwRxWaterMark];

        // Generate interrupt on detection of last symbol
        ulData |= STIR_RX_INT_LAST_SYMBOL;

        // Generate interrupt on overrun
        ulData |= STIR_RX_INT_OVERRUN;

        // Enable Rx interrupt sources
        ulData |= STIR_RX_INT_GLOBALINT;
        m_pRegST202T_IR->Write_RX_INTEN(ulData);

        // Finally, enable the receiver itself
        m_pRegST202T_IR->Write_RX_EN(STIR_RX_ENABLE);
    }

    m_HardwareLock.Unlock();
    return bRet;
}

void ST202T_IR::SetRxSamplingClockAndError()
{
    // IRB_SAMPLE_RATE_COMM is a 4-bit divisor that must
    // be programmed to satisfy the following:
    //
    //    System Clock
    // -------------------- = 10 MHz
    // IRB_SAMPLE_RATE_COMM
    //
    // Thus,
    //
    //                         System Clock
    // IRB_SAMPLE_RATE_COMM = ---------------
    //                            10 MHz
    //
    // Since IRB_SAMPLE_RATE_COMM is a 4-bit register and
    // represents an integer divisor, and the system clock
    // is not necessariely a multiple of 10MHz, we must round
    // the result of the above division to the nearset integer.
    // This produces an error factor in our sampling rate
    // clock frequency.  The error is calculated as follows:
    //
    // --                    --
    // |    System Clock      |
    // | -------------------- |
    // | IRB_SAMPLE_RATE_COMM |
    // --                    --
    // ------------------------- = Error factor
    //        10 MHz
    //
    // Thus, this function also sets m_ErrorFactor to this value.
    // The Rx interrupt handler uses this error factor to fix up
    // the received signals using the following calculation:
    //
    //                      Sampled SignalTime
    // ActualSymbolTime = ----------------------
    //                         Error Factor

    ULONG IrbSampleRateComm;

    IrbSampleRateComm = m_PeripheralClockFreq / 10000000; // Integer math trucates fractional part
    // Round up if necessary
    if ((m_PeripheralClockFreq % 10000000) > 5000000)
    {
        IrbSampleRateComm++;
    }

    // Program the IRB_SAMPLE_RATE_COMM register with the result
    m_pRegST202T_IR->Write_RX_SAMPLERATE(IrbSampleRateComm);

    m_ErrorFactor = (m_PeripheralClockFreq / IrbSampleRateComm) / 100000;

    return;
}

VOID ST202T_IR::ReceiveInterruptHandler()
{
    DEBUGMSG(ZONE_THREAD|ZONE_READ,(TEXT("+ST202T_IR::ReceiveInterruptHandler\r\n")));

    BYTE OneMsg[4];  // the longest IR message

    m_bReceivedCanceled = FALSE;

    m_HardwareLock.Lock();

    volatile ULONG ulRxStat = 0;
    while (!m_pRxMsg->IsFull() && !m_bReceivedCanceled)
    {
        ulRxStat = m_pRegST202T_IR->Read_RX_STA();
        DWORD dwOnTime;
        DWORD dwSymbolPeriod;
        if (ulRxStat & STIR_RX_STA_FIFO_LEVEL_MASK) // Data is available
        {
            // Read a symbol descriptor pair
            dwSymbolPeriod = m_pRegST202T_IR->Read_RX_SYMPERIOD();
            dwOnTime = m_pRegST202T_IR->Read_RX_ONTIME();

            // We must apply an error correction factor to the received
            // signal, to account for the case where IRB_SAMPLE_RATE_COMM
            // is not exactly 10MHz.  See comments in SetRxSamplingClockAndError
            // above.

            dwOnTime = dwOnTime * 100 / m_ErrorFactor;
            dwSymbolPeriod = dwSymbolPeriod * 100 / m_ErrorFactor;

            DWORD DecoderBufferLen = sizeof(OneMsg);
            if (m_pfnDecode(dwOnTime, dwSymbolPeriod, OneMsg, &DecoderBufferLen, FALSE, 0))
            {
                // The decoder returns TRUE if it was able to decode the symbol passed
                // to it and the symbol was valid for the decoder's current state.
                // However, the decoder does not actually return any data until
                // an entire message has been received.
                if (DecoderBufferLen != 0)
                {
                    // Sanity check
                    ASSERT(DecoderBufferLen <= m_dwBytesPerMessage);

                    // The decoder returned data, so an entire message was correctly
                    // received.
                    m_pRxMsg->AddMsg(OneMsg);
                    SetEvent(m_hRxEvent);
                }
            }
            else
            {
                // There is a decoder error. Either the data did not match the decoder state or
                // was invalid. This path will repeat until all symbol related data is
                // extracted from the FIFO

                DEBUGMSG(ZONE_WARNING|ZONE_READ,
                        (TEXT("ST202T_IR::ReceiveInterruptHandler - decoding error\r\n")));

                m_dwDecodingErrorCount++;
            }
        }
        else  // No more data
        {
            // We have emptied the FIFO, no check for an overrun condition
            if (ulRxStat & STIR_RX_INT_OVERRUN)
            {
                // If an overrun has occurred, we have to assume that any
                // messages currently in the decode process are no longer
                // valid.  Signal the decoder to reset its internal state.
                // Increment count of overrun errors.
                m_pfnDecode(NULL, NULL, NULL, NULL, TRUE, 0);
                m_dwOverrunErrorCount++;
            }

            break;
        }
    }

    m_HardwareLock.Unlock();

    DEBUGMSG(ZONE_THREAD|ZONE_READ,(TEXT("-ST202T_IR::ReceiveInterruptHandler\r\n")));
}

ULONG   ST202T_IR::CancelReceive()
{
    m_bReceivedCanceled = TRUE;
    InitReceive(TRUE);
    return 0;
}

VOID   ST202T_IR::FlushRxFIFOs()
{
    while (m_pRegST202T_IR->Read_RX_STA() & STIR_RX_STA_FIFO_LEVEL_MASK)
    {
        m_pRegST202T_IR->Read_RX_SYMPERIOD();
        m_pRegST202T_IR->Read_RX_ONTIME();
    }
}
