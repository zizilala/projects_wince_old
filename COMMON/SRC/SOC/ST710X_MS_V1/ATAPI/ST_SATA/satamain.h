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
    satamain.h

Abstract:
    SATA device driver definitions.

--*/

#ifndef _SATAMAIN_H_
#define _SATAMAIN_H_

#include <atamain.h>

// SATA-specific additions to atamain.h
#define REG_VAL_IDE_RETRIESONERROR        (_T("NumRetriesOnSATAError")) // number of retries when transient error is detected

// forward declaration
class CSATABUS;

// SATA channel abstraction
class CSATAPort {
  public:
    // member variables
    CSATABUS         *m_pController;   // parent
    CRITICAL_SECTION  m_csPort;        // protect access to I/O ports
    DWORD             m_fInitialized;  // whether port has been initialized by SATA driver
    DWORD             m_dwFlag;        // m_dwFlag
    DWORD             m_dwRegBase;     // base virtual address of command I/O port
    DWORD             m_dwRegAlt;      // base virtual address of status I/O port
    DWORD             m_dwBMR;         // base virtual address of bus master I/O port
    PDSKREG           m_pDskReg[2];    // DSK_ registry value - 2 entries for compatibility with existing ATAPI driver
    HANDLE            m_hIRQEvent;     // IRQ event handle
    HANDLE            m_hSATAEvent;    // SATA event (ATA interrupt)
    HANDLE            m_hDMAEvent;     // DMA event
    HANDLE            m_hErrorEvent;   // SATA error event
    DWORD             m_dwSysIntr;     // SysIntr associated with ATA channel (for compatibility with existing ATAPI driver)
    BYTE              m_bStatus;       // ATA Status register; the IST populates this
    // constructors/destructors
    CSATAPort(CSATABUS *pParent);
    ~CSATAPort();
    // member functions
    void TakeCS();
    void ReleaseCS();
    void PrintInfo();
};

#define MAX_SATA_DEVICES_PER_CONTROLLER 1

class CSATABUS {
  public:
    // member variables
    HANDLE         m_hDevice[MAX_SATA_DEVICES_PER_CONTROLLER];  // device activation handles
    LPWSTR         m_szDevice[MAX_SATA_DEVICES_PER_CONTROLLER]; // device key paths
    DDKWINDOWINFO  m_dwi;                                       // resource information for controller
    PIDEREG        m_pIdeReg;                                   // IDE_ registry value set
    DWORD          m_dwControllerBase;                          // Controller's base address (contains wrapper and DMA registers)
    DWORD          m_dwControllerRegSize;                       // Size of controller's I/O window
    CSATAPort     *m_pPort[MAX_SATA_DEVICES_PER_CONTROLLER];    // I/O port of channels
    DWORD          m_dwIrq;                                     // IRQ associated with SATA controller
    DWORD          m_dwSysIntr;                                 // SYSINTR associated with SATA interrupt
    // constructors/destructors
    CSATABUS();
    ~CSATABUS();
};

#endif _SATAMAIN_H_
