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
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

Abstract:

    definitions for ST202T IR controller.

Notes:
--*/
#ifndef __ST202T_IR_H_
#define __ST202T_IR_H_

#include <CSync.h>
#include <CRegEdit.h>
#include <CMThread.h>
#include "IRC_ProtocolHandler.h"
#include "MsgQueue.h"


/////////////////////////////////////////////////////////////////////////////////////////
//// Default global configuration

extern DWORD g_DefaultTxWatermark;
extern DWORD g_DefaultRxWatermark;
extern BOOL  g_DefaultWakeupOnRx;
extern DWORD g_Priority256;
extern DWORD g_MsgBufCount;

#define DEFAULT_IRC_RX_WATERMARK  2  // valid: 1..3
#define DEFAULT_IRC_TX_WATERMARK  2  // valid: 1..3
#define DEFAULT_IRC_WAKE_ON_RX    FALSE  // valid: FALSE, TRUE. Currently not supported
#define DEFAULT_IRC_NUM_MSGBUF    128
#define DEFAULT_IRC_PRIORITY      159

/////////////////////////////////////////////////////////////////////////////////////////
//// General IR defines

#define STIR_FIFO_DEPTH                       7

/////////////////////////////////////////////////////////////////////////////////////////
// Required Registry Setting.
#define PC_REG_STIR_PROTOCOL_HANDLER_VAL_NAME TEXT("ProtocolHandler")
#define PC_REG_STIR_TX_WATERMARK_VAL_NAME TEXT("TxWaterMark")
#define PC_REG_STIR_RX_WATERMARK_VAL_NAME TEXT("RxWaterMark")
#define PC_REG_STIR_PRIORITY_VAL_NAME TEXT("Priority256")

/////////////////////////////////////////////////////////////////////////////////////////
//// IR Registers

// IR Tx register offsets from base
#define STIR_TX_PRESCALER_OFFSET                0x00
#define STIR_TX_SUBCARR_OFFSET                  0x04
#define STIR_TX_SYM_PERIOD_OFFSET               0x08
#define STIR_TX_ONTIME_OFFSET                   0x0C
#define STIR_TX_INTEN_OFFSET                    0x10
#define STIR_TX_INT_STATUS_OFFSET               0x14
#define STIR_TX_EN_OFFSET                       0x18
#define STIR_TX_CLR_OFFSET                      0x1C
#define STIR_TX_SUB_CARR_WIDTH_OFFSET           0x20
#define STIR_TX_STA_OFFSET                      0x24

// IR Rx register offsets from base
#define STIR_RX_ONTIME_OFFSET                   0x40
#define STIR_RX_SYM_PERIOD_OFFSET               0x44
#define STIR_RX_INTEN_OFFSET                    0x48
#define STIR_RX_INT_STATUS_OFFSET               0x4C
#define STIR_RX_EN_OFFSET                       0x50
#define STIR_RX_MAX_SYMBOL_PERIOD_OFFSET        0x54
#define STIR_RX_CLR_OFFSET                      0x58
#define STIR_RX_NOISE_SUPP_WIDTH_OFFSET         0x5C
#define STIR_RC_IRDA_CTRL_OFFSET                0x60
#define STIR_SAMPLE_RATE_COMM_OFFSET            0x64
#define STIR_POL_INV_OFFSET                     0x68
#define STIR_RX_STA_OFFSET                      0x6C
#define STIR_CLK_SEL_OFFSET                     0x70
#define STIR_CLK_SEL_STA_OFFSET                 0x74

// UHF Rx register offsets from base
#define STUHF_RX_ONTIME_OFFSET                  0x80
#define STUHF_RX_SYM_PERIOD_OFFSET              0x84
#define STUHF_RX_INTEN_OFFSET                   0x88
#define STUHF_RX_INT_STATUS_OFFSET              0x8C
#define STUHF_RX_EN_OFFSET                      0x90
#define STUHF_RX_MAX_SYMBOL_PERIOD_OFFSET       0x94
#define STUHF_RX_CLR_OFFSET                     0x98
#define STUHF_RX_NOISE_SUPP_WIDTH_OFFSET        0x9C
#define STUHF_RC_IRDA_CTRL_OFFSET               0xA0
#define STUHF_POL_INV_OFFSET                    0xA8
#define STUHF_RX_STA_OFFSET                     0xAC

#define STIR_TOTAL_REGISTER_SPACE               0xB0

// IR Tx Interrupt Enable/Status register bits
#define STIR_TX_INT_GLOBALINT                   (1 << 0)
#define STIR_TX_INT_UNDERRUN                    (1 << 1)
#define STIR_TX_INT_EMPTY                       (1 << 2)
#define STIR_TX_INT_HALFEMPTY                   (1 << 3)
#define STIR_TX_INT_ONEWORD                     (1 << 4)

// IR Tx Enable
#define STIR_TX_DISABLE                         0x0
#define STIR_TX_ENABLE                          0x1

#define STIR_TX_STA_FIFO_LEVEL_MASK             0x700
#define STIR_TX_STA_FIFO_LEVEL_SHIFT            8

// IR Rx Interrupt Enable/Status/Clear register bits
#define STIR_RX_INT_GLOBALINT                   (1 << 0)
#define STIR_RX_INT_LAST_SYMBOL                 (1 << 1)
#define STIR_RX_INT_OVERRUN                     (1 << 2)
#define STIR_RX_INT_FULL                        (1 << 3)
#define STIR_RX_INT_HALFFULL                    (1 << 4)
#define STIR_RX_INT_ONEWORD                     (1 << 5)

#define STIR_RX_INT_ALL_SRC   ( STIR_RX_INT_LAST_SYMBOL \
                               |  STIR_RX_INT_OVERRUN \
                               |  STIR_RX_INT_FULL    \
                               |  STIR_RX_INT_HALFFULL \
                               |  STIR_RX_INT_ONEWORD )

#define STIR_RX_INT_WATERMARK_MASK              0x38
#define STIR_RX_INT_1WORD_WATERMARK             0x20
#define STIR_RX_INT_4WORD_WATERMARK             0x10
#define STIR_RX_INT_7WORD_WATERMARK             0x08

// IR Rx Enable
#define STIR_RX_DISABLE                         0x0
#define STIR_RX_ENABLE                          0x1

// IR Rx Input Selection (RC_IRDA_CTRL register)
#define STIR_RX_RC_DATA                         0x0
#define STIR_RX_IRDA_DATA                       0x1

// IR Rx Polarity Inversion (IRB_POL_INV register)
#define STIR_RX_NO_INVERT_POLARITY              0x0
#define STIR_RX_INVERT_POLARITY                 0x1

#define STIR_RX_STA_FIFO_LEVEL_MASK             0x700
#define STIR_RX_STA_FIFO_LEVEL_SHIFT            8

// IR Rx Sample Rate register
#define STIR_RX_SAMPLE_RATE_COMM_MASK           0xF

// IR Rx clock select/status registers
#define STIR_RX_CLK_SEL_SYSCLK                  0x0
#define STIR_RX_CLK_SEL_27MHZ                   0x1

// UHF Rx Enable
#define STUHF_RX_DISABLE                        0x0

// Max symbol time in microseconds
#define STIR_RX_MAX_RC_SYMBOLTIME               4000

/////////////////////////////////////////////////////////////////////////////////////////
//// IR Class definitions

class RegST202T_IR {
public:
    RegST202T_IR(PULONG pRegAddr);
    ~RegST202T_IR() { }
    BOOL    Init() ;
#pragma optimize( "", off )
    void    Write_TX_PRESCALER (ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_TX_PRESCALER_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_TX_PRESCALER() { return READ_REGISTER_ULONG(m_pReg + STIR_TX_PRESCALER_OFFSET/sizeof(ULONG)); }
    void    Write_TX_SUBCARR (ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_TX_SUBCARR_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_TX_SUBCARR() { return READ_REGISTER_ULONG(m_pReg + STIR_TX_SUBCARR_OFFSET/sizeof(ULONG)); }
    void    Write_TX_SYMPERIOD(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_TX_SYM_PERIOD_OFFSET/sizeof(ULONG), uData); }
    void    Write_TX_ONTIME(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_TX_ONTIME_OFFSET/sizeof(ULONG), uData); }
    void    Write_TX_INTEN(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_TX_INTEN_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_TX_INTEN() { return READ_REGISTER_ULONG(m_pReg + STIR_TX_INTEN_OFFSET/sizeof(ULONG)); }
    ULONG   Read_TX_INTSTA() { return READ_REGISTER_ULONG(m_pReg + STIR_TX_INT_STATUS_OFFSET/sizeof(ULONG)); }
    void    Write_TX_EN(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_TX_EN_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_TX_EN() { return READ_REGISTER_ULONG(m_pReg + STIR_TX_EN_OFFSET/sizeof(ULONG)); }
    void    Write_TX_CLR(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_RX_CLR_OFFSET/sizeof(ULONG), uData); }
    void    Write_TX_SUBCAR_WIDTH(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_TX_SUB_CARR_WIDTH_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_TX_SUBCAR_WIDTH() { return READ_REGISTER_ULONG(m_pReg + STIR_TX_SUB_CARR_WIDTH_OFFSET/sizeof(ULONG)); }
    ULONG   Read_TX_STA() { return READ_REGISTER_ULONG(m_pReg + STIR_TX_STA_OFFSET/sizeof(ULONG)); }

    ULONG   Read_RX_ONTIME() { return READ_REGISTER_ULONG(m_pReg + STIR_RX_ONTIME_OFFSET/sizeof(ULONG)); }
    ULONG   Read_RX_SYMPERIOD() { return READ_REGISTER_ULONG(m_pReg + STIR_RX_SYM_PERIOD_OFFSET/sizeof(ULONG)); }
    void    Write_RX_INTEN(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_RX_INTEN_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_RX_INTEN() { return READ_REGISTER_ULONG(m_pReg + STIR_RX_INTEN_OFFSET/sizeof(ULONG)); }
    ULONG   Read_RX_INTSTA() { return READ_REGISTER_ULONG(m_pReg + STIR_RX_INT_STATUS_OFFSET/sizeof(ULONG)); }
    void    Write_RX_EN(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_RX_EN_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_RX_EN() { return READ_REGISTER_ULONG(m_pReg + STIR_RX_EN_OFFSET/sizeof(ULONG)); }
    void    Write_RX_MAX_SYMPERIOD(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_RX_MAX_SYMBOL_PERIOD_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_RX_MAX_SYMPERIOD() { return READ_REGISTER_ULONG(m_pReg + STIR_RX_MAX_SYMBOL_PERIOD_OFFSET/sizeof(ULONG)); }
    void    Write_RX_CLR(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_RX_CLR_OFFSET/sizeof(ULONG), uData); }
    void    Write_RX_NOISE_SUPP_WIDTH(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_RX_NOISE_SUPP_WIDTH_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_RX_NOISE_SUPP_WIDTH() { return READ_REGISTER_ULONG(m_pReg + STIR_RX_NOISE_SUPP_WIDTH_OFFSET/sizeof(ULONG)); }
    void    Write_RC_IRDA_CTRL(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_RC_IRDA_CTRL_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_RC_IRDA_CTRL() { return READ_REGISTER_ULONG(m_pReg + STIR_RC_IRDA_CTRL_OFFSET/sizeof(ULONG)); }
    void    Write_RX_SAMPLERATE(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_SAMPLE_RATE_COMM_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_RX_SAMPLERATE() { return READ_REGISTER_ULONG(m_pReg + STIR_SAMPLE_RATE_COMM_OFFSET/sizeof(ULONG)); }
    void    Write_RX_POLARITY_INV(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_POL_INV_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_RX_POLARITY_INV() { return READ_REGISTER_ULONG(m_pReg + STIR_POL_INV_OFFSET/sizeof(ULONG)); }
    ULONG   Read_RX_STA() { return READ_REGISTER_ULONG(m_pReg + STIR_RX_STA_OFFSET/sizeof(ULONG)); }
    void    Write_RX_STA(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_RX_STA_OFFSET/sizeof(ULONG), uData); }
    void    Write_RX_CLK_SEL(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STIR_CLK_SEL_OFFSET/sizeof(ULONG), uData); }
    ULONG   Read_RX_CLK_SEL() { return READ_REGISTER_ULONG(m_pReg + STIR_CLK_SEL_OFFSET/sizeof(ULONG)); }
    ULONG   Read_RX_CLK_SEL_STA() { return READ_REGISTER_ULONG(m_pReg + STIR_CLK_SEL_STA_OFFSET/sizeof(ULONG)); }

    void    Write_UHF_INTEN(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STUHF_RX_INTEN_OFFSET/sizeof(ULONG), uData); }
    void    Write_UHF_EN(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + STUHF_RX_EN_OFFSET/sizeof(ULONG), uData); }
#pragma optimize( "", on )

    PULONG  GetRegisterVirtualAddr() { return m_pReg; };
#if ST202T_POWERMGMT
    void    Backup();
    void    Restore();
#endif
#ifdef DEBUG
    void    DumpRegister();
#endif
protected:
    volatile PULONG    m_pReg;
    BOOL               m_fIsBackedUp;

private:
    ULONG   m_STIR_TX_PRESCALERBackup;
    ULONG   m_STIR_TX_SUBCARRBackup;
    ULONG   m_STIR_TX_INTENBackup;
    ULONG   m_STIR_TX_SUBCARRWIDTHBackup;
    ULONG   m_STIR_RX_INTENBackup;
    ULONG   m_STIR_RX_MAXSYMBPERIODBackup;
    ULONG   m_STIR_RX_NOISESUPPWIDTHBackup;
    ULONG   m_STIR_RX_IRDACTRLBackup;
    ULONG   m_STIR_RX_POLINVBackup;
    ULONG   m_STIR_RX_SAMPLERATEBackup;
    ULONG   m_STIR_RX_CLKSELBackup;
};

const UCHAR WatermarkBits[] = {0,  // range of indecies used is 1..3
                                STIR_RX_INT_1WORD_WATERMARK,
                                STIR_RX_INT_4WORD_WATERMARK,
                                STIR_RX_INT_7WORD_WATERMARK};

class ST202T_IR : public CMiniThread {

// init, teardown
public:
    ST202T_IR (LPCTSTR lpActivePath);
    ~ST202T_IR();
    BOOL Init();
    VOID PostInit();

// driver API
public:
    BOOL Open();
    BOOL Close();
    DWORD IOControl(
        DWORD dwCode,
        PBYTE pBufIn,
        DWORD dwLenIn,
        PBYTE pBufOut,
        DWORD dwLenOut,
        PDWORD pdwActualOut);
    DWORD Read(
        PBYTE pBuffer,
        DWORD Count,
        PDWORD pActualOut);

// internal / utility
private:
    BOOL    MapHardware();
    BOOL    CreateHardwareAccess();

    BOOL    m_bConstructed;     // to indicate the constructor completed successfully
    BOOL    m_bModeSet;         // indicate whether the protocol mode/submode has been configured
    LONG    m_bCurrentlyOpen;   // count of open handles. currently can only be 0 or 1

// internal data accessors
public:
    HANDLE GetOpenHandle()
    {
        return m_bCurrentlyOpen ? (HANDLE)&m_bCurrentlyOpen : NULL;
    }

    BOOL IsConstructed() { return m_bConstructed; }

#if ST202T_POWERMGMT
//  Power Manager Required Function.
    void    RegisterBackup() { m_pRegST202T_IR->Backup(); };
    void    RegisterRestore() { m_pRegST202T_IR->Restore(); };
#endif

// configuration and members accessible by derived class
protected:
    RegST202T_IR  * m_pRegST202T_IR;

    HANDLE          m_hBusAccess;
    PVOID           m_pRegVirtualAddr;

    CRegistryEdit   *m_pActiveReg;
    CRegistryEdit   *m_pDeviceReg;

    ULONG           m_PeripheralClockFreq;

private:
    DWORD   m_dwRxWaterMark;
    DWORD   m_dwTxWaterMark;
    BOOL    m_bWakeupOnRemote;

#if ST202T_XMIT
//  Tx Function.
private:
    BOOL    InitXmit(BOOL bInit);
    void    XmitInterruptHandler(PUCHAR pTxBuffer, ULONG *pBuffLen);
    BOOL    EnableXmitInterrupt(BOOL bEnable);
    BOOL    CancelXmit();
    DWORD   GetWriteableSize();

    HANDLE  m_XmitFlushDone;
#endif

//  Rx Function.
private:
    BOOL    InitReceive(BOOL bInit);
    VOID    ReceiveInterruptHandler();
    ULONG   CancelReceive();
    VOID    SetRxSamplingClockAndError();
    VOID    FlushRxFIFOs();

    BOOL            m_bReceivedCanceled;
    DWORD           m_dwBytesPerMessage;
    DWORD           m_ErrorFactor;
    CLockObject     m_HardwareLock;

    HANDLE          m_hRxEvent;
    CMsgQueue     * m_pRxMsg;

//  Interrupt Handler
private:
    DWORD   ThreadRun();   // IST
    DWORD       m_dwSysIntr;
    HANDLE      m_hISTEvent;


// Protocol Handler Function Pointers
private:
    HMODULE          m_hDevDll;
    pfnRC_Decode                m_pfnDecode;
    pfnRC_Encode                m_pfnEncode;
    pfnRC_GetCapabilities       m_pfnGetCapabilities;
    pfnRC_SetMode               m_pfnSetMode;
    pfnRC_GetSignalProperties   m_pfnGetSignalProperties;

// Error counters
private:
    DWORD   m_dwOverrunErrorCount;
    DWORD   m_dwUnderrunErrorCount;
    DWORD   m_dwDecodingErrorCount;
};

#endif // __ST202T_IR_H_

