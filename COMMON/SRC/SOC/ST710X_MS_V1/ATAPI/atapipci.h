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
    atapipci.h

Abstract:
    Base ATA/ATAPI PCI device abstraction.

--*/

#ifndef _ATAPIPCI_H_
#define _ATAPIPCI_H_

#ifdef ST202T_SATA
#include <satamain.h>
#else
#include <atamain.h>
#endif

class CPCIDisk : public CDisk {
  public:

    // member variables
    static LONG  m_lDeviceCount;

    // (DMA support)
    LPBYTE       m_pStartMemory;
    PDMATable    m_pPRD;
    PPhysTable   m_pPhysList;
    PSGCopyTable m_pSGCopy;
    PDWORD       m_pPFNs;
    DWORD        m_dwSGCount;
    PHYSICAL_ADDRESS m_pPRDPhys;
    DWORD        m_dwPhysCount;
    DWORD        m_dwPRDSize;
    LPBYTE       m_pBMCommand;

    // constructors/destructors
    CPCIDisk(HKEY hKey);
    virtual ~CPCIDisk();

    // member functions
    virtual VOID ConfigureRegisterBlock(DWORD dwStride);
    virtual BOOL Init(HKEY hActiveKey);
    virtual DWORD MainIoctl(PIOREQ pIOReq);
    // virtual void TakeCS();
    // virtual void ReleaseCS();
    virtual BOOL WaitForInterrupt(DWORD dwTimeOut);
    virtual void EnableInterrupt();
    virtual BOOL ConfigPort();
    virtual BOOL SetupDMA(PSG_BUF pSgBuf, DWORD dwSgCount, BOOL fRead);
    virtual BOOL BeginDMA(BOOL fRead);
    virtual BOOL EndDMA();
    virtual BOOL AbortDMA();
    virtual BOOL CompleteDMA(PSG_BUF pSgBuf, DWORD dwSgCount, BOOL fRead);
    void FreeDMABuffers();
    void CopyDiskInfoFromPort();
    virtual BOOL TranslateAddress(PDWORD pdwAddr);

    inline virtual void CPCIDisk::TakeCS() {
        m_pPort->TakeCS();
    }
    inline virtual void CPCIDisk::ReleaseCS() {
        m_pPort->ReleaseCS();
    }
    inline void CPCIDisk::WriteBMCommand(BYTE bCommand) {
        ATA_WRITE_BYTE(m_pBMCommand, bCommand);
    }
    inline BYTE CPCIDisk::ReadBMStatus() {
        return ATA_READ_BYTE(m_pBMCommand + 2);
    }
    inline void CPCIDisk::WriteBMTable(DWORD dwPhys) {
        ATA_WRITE_DWORD((LPDWORD)(m_pBMCommand + 4), dwPhys);
    }
    inline void CPCIDisk::WriteBMStatus(BYTE bStatus) {
        ATA_WRITE_BYTE(m_pBMCommand + 2, bStatus);
    }
    inline BOOL CPCIDisk::DoesDeviceAlreadyExist() {
        return FALSE;
    }

};

#endif //_ATAPIPCI_H_

