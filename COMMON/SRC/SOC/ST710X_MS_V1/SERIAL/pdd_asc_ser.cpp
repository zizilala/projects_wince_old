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

    Serial PDD for STMicro ASC UART (common code).

Notes:
--*/
#include <windows.h>
#include <types.h>
#include <ceddk.h>

#include <ddkreg.h>
#include <serhw.h>
#include <Serdbg.h>
#include <pdd_ASC_ser.h>

CRegASCSerial::CRegASCSerial(PULONG pRegAddr)
:   m_pReg(pRegAddr)
{
    m_fIsBackedUp = FALSE;
}

BOOL   CRegASCSerial::Init()
{
    if (m_pReg)
        {
        // Clear registers
        Write_ASCBAUDRATE(0);
        Write_ASCCTRL(0);
        Write_ASCGUARDTIME(0);
        Write_ASCINTEN(0);
        Write_ASCRETRIES(0);
        Write_ASCTIMEOUT(0);
        return TRUE;
        }
    else
        return FALSE;
}

void CRegASCSerial::Backup()
{
    m_fIsBackedUp = TRUE;

    m_ASCBAUDRATEBackup     = Read_ASCBAUDRATE();
    m_ASCCTRLBackup         = Read_ASCCTRL();
    m_ASCGUARDTIMEBackup    = Read_ASCGUARDTIME();
    m_ASCINTENBackup        = Read_ASCINTEN();
    m_RETRIESBackup         = Read_ASCRETRIES();
    m_ASCTIMEOUTBackup      = Read_ASCTIMEOUT();
}

void CRegASCSerial::Restore()
{
    if (m_fIsBackedUp)
        {
        Write_ASCBAUDRATE(m_ASCBAUDRATEBackup);
        Write_ASCCTRL(m_ASCCTRLBackup);
        Write_ASCGUARDTIME(m_ASCGUARDTIMEBackup);
        Write_ASCINTEN(m_ASCINTENBackup);
        Write_ASCRETRIES(m_RETRIESBackup);
        Write_ASCTIMEOUT(m_ASCTIMEOUTBackup);
        m_fIsBackedUp = FALSE;
        }
}

#ifdef DEBUG
void CRegASCSerial::DumpRegister()
{
    NKDbgPrintfW(TEXT("DumpRegister:\r\n\
                       ASCBAUDRATE=0x%x\r\n\
                       ASCCTRL=0x%x\r\n\
                       ASCGUARDTIME=0x%x\r\n\
                       ASCINTEN=0x%x\r\n\
                       ASCRETRIES=0x%x\r\n\
                       ASCSTA=0x%x\r\n\
                       ASCTIMEOUT=0x%x\r\n"),
        Read_ASCBAUDRATE(),Read_ASCCTRL(),Read_ASCGUARDTIME(),
        Read_ASCINTEN(),Read_ASCRETRIES(),Read_ASCSTA(),Read_ASCTIMEOUT());
}
#endif

CPddASCSerial::CPddASCSerial (LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj )
:   CSerialPDD(lpActivePath,pMdd, pHwObj)
,   m_ActiveReg(HKEY_LOCAL_MACHINE,lpActivePath)
,   CMiniThread (0, TRUE)
{
    m_pRegASCSerial= NULL;
    m_dwSysIntr = MAXDWORD;
    m_hISTEvent = NULL;
    m_dwDevIndex = 0;
    m_pRegVirtualAddr = NULL;
    m_XmitFlushDone =  CreateEvent(0, FALSE, FALSE, NULL);
    m_PeripheralClockFreq = 0;
    m_ParityMode = MAXDWORD;
    m_ByteSize = MAXDWORD;
}

CPddASCSerial::~CPddASCSerial()
{
    InitModem(FALSE);
    if (m_hISTEvent)
        {
        m_bTerminated=TRUE;
        ThreadStart();
        SetEvent(m_hISTEvent);
        ThreadTerminated(1000);
        InterruptDisable( m_dwSysIntr );
        CloseHandle(m_hISTEvent);
        }
    if (m_pRegASCSerial)
        {
        delete m_pRegASCSerial;
        }
    if (m_XmitFlushDone)
        {
        CloseHandle(m_XmitFlushDone);
        }
    if (m_pRegVirtualAddr != NULL)
        {
        MmUnmapIoSpace((PVOID)m_pRegVirtualAddr,0UL);
        }

}

void CPddASCSerial::SetDefaultConfiguration()
{
    CSerialPDD::SetDefaultConfiguration();

    // Override the defaults from common PDD code

    m_CommPorp.dwProvCapabilities  =
        PCF_RTSCTS |
        PCF_SETXCHAR |
        PCF_INTTIMEOUTS |
        PCF_PARITY_CHECK |
        PCF_SPECIALCHARS |
        PCF_TOTALTIMEOUTS |
        PCF_XONXOFF;
    m_CommPorp.dwSettableBaud      =
        BAUD_075 | BAUD_110 | BAUD_150 | BAUD_300 | BAUD_600 |
        BAUD_1200 | BAUD_1800 | BAUD_2400 | BAUD_4800 |
        BAUD_7200 | BAUD_9600 | BAUD_14400 |
        BAUD_19200 | BAUD_38400 | BAUD_56K | BAUD_128K |
        BAUD_115200 | BAUD_57600 | BAUD_USER;
    m_CommPorp.dwSettableParams    =
        SP_BAUD | SP_DATABITS | SP_PARITY |
        SP_PARITY_CHECK | SP_STOPBITS;
    m_CommPorp.wSettableData       =
        DATABITS_7 | DATABITS_8;
    m_CommPorp.wSettableStopParity =
        STOPBITS_10 | STOPBITS_20 | STOPBITS_15 |
        PARITY_NONE | PARITY_ODD | PARITY_EVEN;
}

BOOL CPddASCSerial::Init()
{
    if ( CSerialPDD::Init() && IsKeyOpened() && m_XmitFlushDone!=NULL)
        {
        // IST Setup
        DDKISRINFO ddi;
        if (GetIsrInfo(&ddi)!= ERROR_SUCCESS)
            {
            return FALSE;
            }
        m_dwSysIntr = ddi.dwSysintr;
        if (m_dwSysIntr != MAXDWORD && m_dwSysIntr!=0 )
            {
            m_hISTEvent= CreateEvent(0,FALSE,FALSE,NULL);
            }

        if (m_hISTEvent!=NULL)
            {
            InterruptInitialize(m_dwSysIntr,m_hISTEvent,0,0);
            }
        else
            {
            return FALSE;
            }

        // Get Device Index.
        if (!GetRegValue(PC_REG_DEVINDEX_VAL_NAME, (PBYTE)&m_dwDevIndex, PC_REG_DEVINDEX_VAL_LEN))
            {
            m_dwDevIndex = 0;
            }
        if (!GetRegValue(PC_REG_SERASC_IST_TIMEOUTS_VAL_NAME,(PBYTE)&m_dwISTTimeout, PC_REG_SERASC_IST_TIMEOUTS_VAL_LEN))
            {
            m_dwISTTimeout = INFINITE;
            }

        if (!MapHardware() || !CreateHardwareAccess())
            {
            return FALSE;
            }

        return TRUE;
        }
    return FALSE;
}

BOOL CPddASCSerial::MapHardware()
{
    if (m_pRegVirtualAddr !=NULL)
        {
        return TRUE;
        }

    // Get IO Window From Registry
    DDKWINDOWINFO dwi;
    if ( GetWindowInfo( &dwi)!=ERROR_SUCCESS ||
            dwi.dwNumMemWindows < 1 ||
            dwi.memWindows[0].dwBase == 0 ||
            dwi.memWindows[0].dwLen < 0x2C)
        {
        return FALSE;
        }
    DWORD dwInterfaceType;
    if (m_ActiveReg.IsKeyOpened() &&
            m_ActiveReg.GetRegValue( DEVLOAD_INTERFACETYPE_VALNAME, (PBYTE)&dwInterfaceType,sizeof(DWORD)))
        {
        dwi.dwInterfaceType = dwInterfaceType;
        }

    // Translate to System Address.
    PHYSICAL_ADDRESS    ioPhysicalBase = { dwi.memWindows[0].dwBase, 0};
    ULONG               inIoSpace = 0;
    if (TranslateBusAddr(m_hParent,(INTERFACE_TYPE)dwi.dwInterfaceType,dwi.dwBusNumber, ioPhysicalBase,&inIoSpace,&ioPhysicalBase))
        {
        m_pRegVirtualAddr = MmMapIoSpace(ioPhysicalBase, dwi.memWindows[0].dwLen,FALSE);
        }

    return (m_pRegVirtualAddr!=NULL);
}

BOOL CPddASCSerial::CreateHardwareAccess()
{
    if (m_pRegASCSerial)
        {
        return TRUE;
        }
    if (m_pRegVirtualAddr!=NULL)
        {
        m_pRegASCSerial = new CRegASCSerial((PULONG)m_pRegVirtualAddr);
        if (m_pRegASCSerial && !m_pRegASCSerial->Init())
            { // FALSE.
            delete m_pRegASCSerial ;
            m_pRegASCSerial = NULL;
            }
        }
    return (m_pRegASCSerial!=NULL);
}

void CPddASCSerial::PostInit()
{
    DWORD dwCount=0;
    ULONG ulData;

    m_HardwareLock.Lock();

    // Disable all ASC interrupt sources for now
    m_pRegASCSerial->Write_ASCINTEN(0);

    // Turn on RUN bit
    ulData = SERASC_CTRL_RUN;

    // Enable FIFO mode for Rx and Tx
    ulData |= SERASC_CTRL_FIFOEN;

    // This also clears the current mode, stopbits, and parity settings, which will be set later.
    // The receiver is still disabled at this stage.
    m_pRegASCSerial->Write_ASCCTRL(ulData);

    // Flush Rx and Tx FIFOs
    m_pRegASCSerial->Write_ASCRXRST(SERASC_RX_RESET);
    m_pRegASCSerial->Write_ASCTXRST(SERASC_TX_RESET);

    m_HardwareLock.Unlock();
    CSerialPDD::PostInit();
    CeSetPriority(m_dwPriority256);

#ifdef DEBUG
    if ( ZONE_INIT )
        {
        m_pRegASCSerial->DumpRegister();
        }
#endif

    ThreadStart();  // Start IST.
}

DWORD CPddASCSerial::ThreadRun()
{
    while ( m_hISTEvent!=NULL && !IsTerminated())
        {
        if (WaitForSingleObject( m_hISTEvent,m_dwISTTimeout)==WAIT_OBJECT_0)
            {
            while (!IsTerminated() )
                {
                volatile DWORD dwStatus = m_pRegASCSerial->Read_ASCSTA();
                volatile DWORD dwMask = m_pRegASCSerial->Read_ASCINTEN();
                DEBUGMSG(ZONE_THREAD,
                        (TEXT(" CPddASCSerial::ThreadRun INT=%x, MASK =%x\r\n"),dwStatus,dwMask));
                dwMask &= dwStatus;
                if (dwMask)
                    {
                    DEBUGMSG(ZONE_THREAD,
                            (TEXT(" CPddASCSerial::ThreadRun Active INT=%x\r\n"),dwMask));

                    // The MDD handles Rx interrupts first.  This hardware clears the overrun error
                    // as soon as the Rx FIFO is read.  Thus, the overrun error may not be reported
                    // to the application layer.  Thus, we signal the event here and trigger the
                    // Rx interrupt handler.  Additionally, parity and frame errors are only cleared
                    // after the offending frame has been removed from the Rx FIFO.  To avoid
                    // interrupt saturation, we will notify the upper layers of this condition
                    // during our handling of the Rx interrupt.
                    DWORD interrupts=INTR_NONE;
                    if ((dwMask & SERASC_INT_OVERRUNERROR)!=0)
                        {
                        m_ulCommErrors |= CE_OVERRUN;
                        EventCallback(EV_ERR);
                        interrupts |= INTR_RX;
                        }
                    if ((dwMask & (SERASC_INT_RXFIFO_HALFFULL | SERASC_INT_RXBUF_NOTEMPTY))!=0)
                        {
                        interrupts |= INTR_RX;
                        }
                    if ((dwMask & (SERASC_INT_TXBUF_EMPTY | SERASC_INT_TXBUF_HALFEMPTY))!=0)
                        {
                        interrupts |= INTR_TX;
                        }
                    ASSERT(interrupts != INTR_NONE);
                    NotifyPDDInterrupt((INTERRUPT_TYPE)interrupts);
                    InterruptDone(m_dwSysIntr);
                    }
                else    // No more interrupts pending
                    {
                    break;
                    }
                }
            }
        else
            {
            // Poll for modem status change
            NotifyPDDInterrupt(INTR_MODEM);
            DEBUGMSG(ZONE_THREAD,(TEXT(" CPddASCSerial::ThreadRun programmed timeout.  Checking modem status.\r\n")));
#ifdef DEBUG
            if ( ZONE_THREAD )
                {
                m_pRegASCSerial->DumpRegister();
                }
#endif
            }
        }
    return 1;
}

BOOL CPddASCSerial::InitialEnableInterrupt(BOOL bEnable )
{

    if (bEnable)
        {
        // We have no modem status interrups to worry about, only Rx, Tx, and
        // error interrupts.  These will be enabled after the device has been
        // opened for reading or writing.  Thus, we need not enable any
        // interrupts at this time.
        }
    else
        {
        // Clear all interrupt sources.
        m_pRegASCSerial->Write_ASCINTEN(0);
        }

    return (CSerialPDD::InitialEnableInterrupt(bEnable));
}

BOOL  CPddASCSerial::InitXmit(BOOL bInit)
{
    if (bInit)
        {
        m_HardwareLock.Lock();
        // Reset (and flush) Tx FIFO.
        m_pRegASCSerial->Write_ASCTXRST(SERASC_TX_RESET);

        m_HardwareLock.Unlock();
        }
    else
        {
        // Make Sure data has been trasmitted out.
        // We have to ensure Tx is complete because MDD will shut down
        // the device after this returns.
        DWORD dwTicks = 0;
        DWORD dwASCState;
        dwASCState = m_pRegASCSerial->Read_ASCSTA();
        while (dwTicks < 1000 && !(dwASCState & SERASC_INT_TXBUF_EMPTY))
            { // Transmitter is not yet empty
            DEBUGMSG(ZONE_THREAD|ZONE_WRITE,(TEXT("CPddASCSerial::InitXmit! Waiting for transmitter to empty. ASCSTA(TE)=%x\r\n"), dwASCState));
            Sleep(5);
            dwTicks +=5;
            dwASCState = m_pRegASCSerial->Read_ASCSTA();
            }
        }
    return TRUE;
}

DWORD   CPddASCSerial::GetWriteableSize()
{
    DWORD dwWriteSize = 0;
    DWORD dwASCState = m_pRegASCSerial->Read_ASCSTA();

    // We don't have a count of how many bytes are
    // available in the Tx FIFO.  Thus, we simply
    // check for the Tx empty or Tx half empty
    // conditions in the status register.  If the Tx
    // buffer is neither empty nor half empty, we must
    // assume that it is full.
    if ((dwASCState & SERASC_INT_TXBUF_EMPTY))
        {
        dwWriteSize = SERASC_FIFO_DEPTH;
        }
    else if (dwASCState & SERASC_INT_TXBUF_HALFEMPTY)
        {
        dwWriteSize = SERASC_FIFO_DEPTH / 2;
        }

    return dwWriteSize;
}

void    CPddASCSerial::XmitInterruptHandler(PUCHAR pTxBuffer, ULONG *pBuffLen)
{
    PREFAST_DEBUGCHK(pBuffLen!=NULL);

    m_HardwareLock.Lock();
    if (*pBuffLen == 0)
        {
        EnableXmitInterrupt(FALSE);
        }
    else
        {
        DEBUGCHK(pTxBuffer);
        PulseEvent(m_XmitFlushDone);
        DWORD dwByteWrite;
        DWORD dwDataAvaiable = *pBuffLen;
        *pBuffLen = 0;

        // The ASC hardware does not give visibility into the status of the CTS line.
        // These checks are included to support cases where the platform code maps
        // CTS and DSR signals to PIO pins.
        if ( (m_DCB.fOutxCtsFlow && IsCTSOff()) || (m_DCB.fOutxDsrFlow && IsDSROff()) ) // We are in flow off
            {
            DEBUGMSG(ZONE_THREAD|ZONE_WRITE,(TEXT("CPddASCSerial::XmitInterruptHandler! Flow Off, Data Discard.\r\n")));
            EnableXmitInterrupt(FALSE);
            }
        else
            {
            DWORD dwWriteSize = GetWriteableSize();
            DEBUGMSG(ZONE_THREAD|ZONE_WRITE,(TEXT("CPddASCSerial::XmitInterruptHandler! WriteableSize=%x to FIFO,dwDataAvaiable=%x\r\n"),
                    dwWriteSize,dwDataAvaiable));
            for (dwByteWrite=0; ((dwByteWrite < dwWriteSize) && (dwDataAvaiable != 0)); dwByteWrite++)
                {
                m_pRegASCSerial->Write_ASCTXBUF(*pTxBuffer);
                pTxBuffer ++;
                dwDataAvaiable--;
                }
            DEBUGMSG(ZONE_THREAD|ZONE_WRITE,(TEXT("CPddASCSerial::XmitInterruptHandler! Write %d byte to FIFO\r\n"),dwByteWrite));
            *pBuffLen = dwByteWrite;
            EnableXmitInterrupt(TRUE);
            }

        }
    m_HardwareLock.Unlock();
}

void    CPddASCSerial::XmitComChar(UCHAR ComChar)
{
    // This function has to poll until the Data can be sent out.
    BOOL bDone = FALSE;
    do
        {
        m_HardwareLock.Lock();
        if ( GetWriteableSize()!=0 )
            {  // If space available
            m_pRegASCSerial->Write_ASCTXBUF(ComChar);
            bDone = TRUE;
            }
        else
            {
            EnableXmitInterrupt(TRUE);
            }
        m_HardwareLock.Unlock();
        if (!bDone)
            {
            WaitForSingleObject(m_XmitFlushDone, (ULONG)1000);
            }
        } while (!bDone);
}

BOOL    CPddASCSerial::EnableXmitInterrupt(BOOL fEnable)
{
    // We are only interested in Tx Empty or Tx half empty conditions.
    // For transmission, we initially fill the FIFO with as much data
    // as possible.  When we receive one of these interrupts, we
    // re-fill the FIFO if necessary.  See XmitInterruptHandler().
    DWORD IntMask = SERASC_INT_TXBUF_EMPTY | SERASC_INT_TXBUF_HALFEMPTY;

    m_HardwareLock.Lock();
    if (fEnable)
        {
        m_pRegASCSerial->Write_ASCINTEN(m_pRegASCSerial->Read_ASCINTEN() | IntMask);
        }
    else
        {
        m_pRegASCSerial->Write_ASCINTEN(m_pRegASCSerial->Read_ASCINTEN() & ~IntMask);
        }
    m_HardwareLock.Unlock();
    return TRUE;

}

BOOL  CPddASCSerial::CancelXmit()
{
    return InitXmit(TRUE);
}

// Receive
BOOL    CPddASCSerial::InitReceive(BOOL bInit)
{
    volatile ULONG ulData;
    m_HardwareLock.Lock();

    // Disable Rx and error interrupts
    ulData = m_pRegASCSerial->Read_ASCINTEN();
    ulData &= ~(SERASC_INT_RXFIFO_HALFFULL | SERASC_INT_RXBUF_NOTEMPTY | SERASC_INT_OVERRUNERROR);
    m_pRegASCSerial->Write_ASCINTEN(ulData);

    // Disable the receiver
    ulData = m_pRegASCSerial->Read_ASCCTRL();
    ulData &= ~SERASC_CTRL_RXEN;
    m_pRegASCSerial->Write_ASCCTRL(ulData);

    // Reset (and flush) receive FIFO.
    m_pRegASCSerial->Write_ASCRXRST(SERASC_RX_RESET);

    if (bInit)
        {
        // Enable the receiver
        ulData = m_pRegASCSerial->Read_ASCCTRL();
        ulData |= SERASC_CTRL_RXEN;
        m_pRegASCSerial->Write_ASCCTRL(ulData);

        // Enable Rx and error interrupts
        ulData = m_pRegASCSerial->Read_ASCINTEN();
        ulData |= ( SERASC_INT_RXFIFO_HALFFULL | SERASC_INT_RXBUF_NOTEMPTY | SERASC_INT_OVERRUNERROR);
        m_pRegASCSerial->Write_ASCINTEN(ulData);
        }

    m_HardwareLock.Unlock();
    return TRUE;
}

ULONG   CPddASCSerial::ReceiveInterruptHandler(PUCHAR pRxBuffer,ULONG *pBufflen)
{
    DEBUGMSG(ZONE_THREAD|ZONE_READ,(TEXT("+CPddASCSerial::ReceiveInterruptHandler pRxBuffer=%x,*pBufflen=%x\r\n"),
        pRxBuffer,pBufflen!=NULL?*pBufflen:0));

    DWORD dwBytesDropped = 0;

    if (pRxBuffer && pBufflen)
        {
        DWORD dwBytesStored = 0 ;
        DWORD dwRoomLeft = *pBufflen;
        m_bReceivedCanceled = FALSE;
        m_HardwareLock.Lock();

        while (dwRoomLeft && !m_bReceivedCanceled)
            {
            volatile ULONG ulASCStat = m_pRegASCSerial->Read_ASCSTA();
            if (ulASCStat & SERASC_INT_RXBUF_NOTEMPTY) // Data is available
                {
                bool bParityErrorDetected = FALSE;
                bool bFramingErrorDetected = FALSE;
                UCHAR uData;
                SERASC_RXBUF RxData;
                // Read the data and extract any error condition.  We
                // do not use the status register's frame or parity error
                // indicators to gather error information, as those bits
                // are set any time any frame in the Rx FIFO has an error.
                // We inspect each frame here for an error condition and
                // act accordingly.
                RxData.ul = m_pRegASCSerial->Read_ASCRXBUF();
                if (m_ByteSize == 7)
                    {
                    uData = (UCHAR)RxData.Rx7BitDataWithParity.RD0_6;
                    bParityErrorDetected = RxData.Rx7BitDataWithParity.ParityError;
                    bFramingErrorDetected = RxData.Rx7BitDataWithParity.FrameError;
                    }
                else if (m_ByteSize == 8)
                    {
                    if (m_ParityMode == NOPARITY)
                        {
                        uData = (UCHAR)RxData.Rx8BitData.RD0_7;
                        bFramingErrorDetected = RxData.Rx8BitData.FrameError;
                        }
                    else
                        {
                        uData = (UCHAR)RxData.Rx8BitDataWithParity.RD0_7;
                        bParityErrorDetected = RxData.Rx8BitDataWithParity.ParityError;
                        bFramingErrorDetected = RxData.Rx8BitDataWithParity.FrameError;
                        }
                    }
                else
                    {
                    DEBUGMSG(ZONE_ERROR,(TEXT("+CPddASCSerial::ReceiveInterruptHandler invalid mode, aborting!\r\n")));
                    ASSERT(FALSE);
                    }

                if (bParityErrorDetected || bFramingErrorDetected)
                    {
                    m_ulCommErrors |= (bParityErrorDetected && m_DCB.fParity) ? CE_RXPARITY : 0;
                    m_ulCommErrors |= bFramingErrorDetected ? CE_FRAME : 0;
                    EventCallback(EV_ERR);
                    }

                if (DataReplaced(&uData, (bParityErrorDetected && m_DCB.fParity)))
                    {
                    *pRxBuffer++ = uData;
                    dwRoomLeft--;
                    dwBytesStored++;
                    }
                }
            else  // No more data
                {
                break;
                }
            }
        if (m_bReceivedCanceled)
            {
            dwBytesStored = 0;
            }

        m_HardwareLock.Unlock();
        *pBufflen = dwBytesStored;
        }
    else
        {
        ASSERT(FALSE);
        }
    DEBUGMSG(ZONE_THREAD|ZONE_READ,(TEXT("-CPddASCSerial::ReceiveInterruptHandler pRxBuffer=%x,*pBufflen=%x,dwBytesDropped=%x\r\n"),
        pRxBuffer,pBufflen!=NULL?*pBufflen:0,dwBytesDropped));

    return dwBytesDropped;
}

ULONG   CPddASCSerial::CancelReceive()
{
    m_bReceivedCanceled = TRUE;
    InitReceive(TRUE);
    return 0;
}

BOOL    CPddASCSerial::InitModem(BOOL bInit)
{
    // None of the 16550-esque modem status/control bits are
    // supported by this hardware generically.
    // Note that some lines may be simulated via
    // PIO pins.  Thus, this function may be overridden
    // by platform code.

    return TRUE;
}

ULONG   CPddASCSerial::GetModemStatus()
{
    // None of the 16550-esque modem status bits are
    // supported by this hardware generically.
    // Note that some lines may be simulated via
    // PIO pins.  Thus, this function may be overridden
    // by platform code.

    return 0;
}

void    CPddASCSerial::SetRTS(BOOL bSet)
{
    // We have no control over the RTS line on the ASC.
    // Note, platform code may override this function to
    // control RTS via a PIO pin.

    return;
}

void    CPddASCSerial::SetDTR(BOOL bSet)
{
    // We have no control over the RTS line on the ASC.
    // Note, platform code may override this function to
    // control RTS via a PIO pin.

    // We have no DTR lines on this hardware.

    return;
}

BOOL CPddASCSerial::InitLine(BOOL bInit)
{
    // Nothing to do here.  The receiver is turned off until InitReceive(TRUE)
    // is called.  Parity and framing errors are checked in the Rx interrupt
    // handler during the receive operation.
    return TRUE;
}

VOID CPddASCSerial::GetLineStatus()
{
    m_HardwareLock.Lock();
    ULONG ulData = m_pRegASCSerial->Read_ASCSTA();
    m_HardwareLock.Unlock();
    ULONG ulError = 0;
    if (ulData & SERASC_INT_OVERRUNERROR)
        {
        ulError |= CE_OVERRUN;
        }
    if (ulData & SERASC_INT_PARITYERROR)
        {
        ulError |= CE_RXPARITY;
        }
    if (ulData & SERASC_INT_FRAMINGERROR)
        {
        ulError |=  CE_FRAME;
        }
    if (ulError)
        {
        SetReceiveError(ulError);
        }

    return;

}

void    CPddASCSerial::SetBreak(BOOL bSet)
{
    // Unsupported on this hardware.
    return;
}

BOOL    CPddASCSerial::SetBaudRate(ULONG BaudRate,BOOL /*bIrModule*/)
{
    ULONG ulReloadVal;

    double Divisor;
    if (BaudRate < 19200)
        {
        // Mode 0 calculation (baud rates of less than 19200)
        //
        //            Peripheral clock freq
        // Divisor = ------------------------
        //               16 * BaudRate
        //

        Divisor = (double)m_PeripheralClockFreq/(16*BaudRate);
        }
    else
        {
        // Mode 1 calculation (baud rates greater than or equal to 19200)
        //
        //            16 * (2^16) * BaudRate
        // Divisor = ------------------------
        //            Peripheral clock freq
        //

        Divisor = (double)(16*65535)/(double)m_PeripheralClockFreq;
        Divisor *= BaudRate;
        }

    ulReloadVal = (ULONG)Divisor;

    m_HardwareLock.Lock();

    // Use mode 0 for baud rates below 19200, and mode 1 for
    // baud rates above or equal to 19200.
    ULONG ulData = m_pRegASCSerial->Read_ASCCTRL() & ~SERASC_CTRL_BAUDMODE_1;
    ulData |= (BaudRate < 19200)? 0 : SERASC_CTRL_BAUDMODE_1;
    m_pRegASCSerial->Write_ASCCTRL(ulData);

    m_pRegASCSerial->Write_ASCBAUDRATE(ulReloadVal);

    m_HardwareLock.Unlock();

    return TRUE;
}

BOOL    CPddASCSerial::SetByteSize(ULONG ByteSize)
{
    m_ByteSize = ByteSize;

    // HW requires parity to be enabled for 7-bit data lengths.  Thus,
    // to cover all cases, we assume even parity here if m_ParityMode
    // has not yet been set.  This can be overridden for 8-bit data lengths
    // in the call to SetParity.
    return SetMode(ByteSize, (m_ParityMode==MAXDWORD?EVENPARITY:m_ParityMode));
}

BOOL    CPddASCSerial::SetParity(ULONG Parity)
{
    m_ParityMode = Parity;

    // HW requires parity to be enabled for 7-bit data lengths.  Thus,
    // to cover all cases, we assume an 8-bit ByteSize if m_ByteSize
    // has not yet been set.  This can be overridden in the call to
    // SetByteSize.
    return SetMode( (m_ByteSize==MAXDWORD?8:m_ByteSize), Parity);
}

BOOL    CPddASCSerial::SetMode(ULONG ByteSize, ULONG Parity)
{
    BOOL bRet = TRUE;
    ULONG ulMode = 0;

    // First, set the ASC_CTRL mode bits.  Abort
    // if we're asked to set an invalid mode.
    switch (ByteSize)
        {
        case 8:
            if (Parity == NOPARITY)
                {
                ulMode = SERASC_8BITDATA;
                }
            else
                {
                ulMode = SERASC_8BITDATA_PARITY;
                }
            break;
        case 7:
            if (Parity == NOPARITY)
                {
                bRet = FALSE;
                }
            else
                {
                ulMode = SERASC_7BITDATA_PARITY;
                }
            break;
        default:
            bRet = FALSE;
            break;
        }

    // Now set the ASC_CTRL parity mode bit (even/odd).
    switch (Parity)
        {
        case ODDPARITY:
            ulMode |= SERASC_CTRL_PARITY_ODD;
            break;
        case EVENPARITY:
            ulMode |= SERASC_CTRL_PARITY_EVEN;
            break;
        case NOPARITY:
            break;
        default:
            bRet = FALSE;
            break;
        }

    if (bRet)
        {
        m_HardwareLock.Lock();

        ULONG ulData = m_pRegASCSerial->Read_ASCCTRL() & ~SERASC_CTRL_MODE_MASK & ~SERASC_CTRL_PARITY_MASK;
        ulData |= ulMode;
        m_pRegASCSerial->Write_ASCCTRL(ulData);

        m_HardwareLock.Unlock();
        }
    else
        {
        m_ulCommErrors |= CE_MODE;
        EventCallback(EV_ERR);
        DEBUGMSG(ZONE_ERROR,(TEXT("+CPddASCSerial::SetParity - invalid mode, aborting!\r\n")));
        }

    return bRet;

}

BOOL    CPddASCSerial::SetStopBits(ULONG StopBits)
{
    BOOL bRet = TRUE;
    ULONG ulStopBits = 0;

    switch ( StopBits )
        {
        case ONESTOPBIT:
            ulStopBits = SERASC_1_STOPBIT;
            break;
        case ONE5STOPBITS:
            ulStopBits = SERASC_1_5_STOPBITS;
        case TWOSTOPBITS:
            ulStopBits = SERASC_2_STOPBITS;
            break;
        default:
            bRet = FALSE;
            break;
        }
    if (bRet)
        {
        m_HardwareLock.Lock();

        ULONG ulData = m_pRegASCSerial->Read_ASCCTRL() & ~SERASC_CTRL_STOPBITS_MASK;
        ulData |= ulStopBits;
        m_pRegASCSerial->Write_ASCCTRL(ulData);

        m_HardwareLock.Unlock();
        }
    else
        {
        DEBUGMSG(ZONE_ERROR,(TEXT("+CPddASCSerial::SetStopBits - invalid mode, aborting!\r\n")));
        }

    return bRet;
}
void    CPddASCSerial::SetOutputMode(BOOL UseIR, BOOL Use9Pin)
{
    // IR is not supported.

    return;
}

