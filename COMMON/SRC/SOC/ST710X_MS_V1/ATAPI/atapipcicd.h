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
    atapipcicd.h

Abstract:
    Base ATA/ATAPI PCI CD-ROM/DVD device support.

--*/

#pragma once

#include <atapipci.h>

class CPCIDiskAndCD : public CPCIDisk {
    PCDROM_READ m_pSterileCdRomReadRequest;
    DWORD m_cbSterileCdRomReadRequest;
  public:
    static BOOL SterilizeCdRomReadRequest(
                PCDROM_READ* ppSafe,
                LPDWORD      lpcbSafe,
                PCDROM_READ  pUnsafe,
                DWORD        cbUnsafe,
                DWORD        dwArgType,
                OUT PUCHAR * saveoldptrs
                );
    CPCIDiskAndCD(HKEY hKey);
    virtual ~CPCIDiskAndCD();
    virtual DWORD MainIoctl(PIOREQ pIOReq);
    DWORD AtapiIoctl(PIOREQ pIOReq);
    DWORD ReadCdRom(CDROM_READ *pReadInfo, PDWORD pBytesReturned);
    DWORD SetupCdRomRead(BOOL bRawMode, DWORD dwLBAAddr, DWORD dwTransferLength, PATAPI_COMMAND_PACKET pCmdPkt);
    virtual DWORD ReadCdRomDMA(DWORD dwLBAAddr, DWORD dwTransferLength, WORD wSectorSize, DWORD dwSgCount, SGX_BUF *pSgBuf);
    BOOL AtapiSendCommand(PATAPI_COMMAND_PACKET pCmdPkt, WORD wCount = 0, BOOL fDMA = FALSE);
    BOOL AtapiReceiveData(PSGX_BUF pSgBuf, DWORD dwSgCount,LPDWORD pdwBytesRead);
    BOOL AtapiSendData(PSGX_BUF pSgBuf, DWORD dwSgCount,LPDWORD pdwBytesWritten);
    BOOL AtapiIsUnitReady(PIOREQ pIOReq = NULL);
    BOOL AtapiIsUnitReadyEx();
    BOOL AtapiGetSenseInfo(CD_SENSE_DATA *pSenseData);
    BOOL AtapiIssueInquiry(INQUIRY_DATA *pInqData);
    BOOL AtapiGetToc(CDROM_TOC *pTOC);
    DWORD AtapiGetDiscInfo(PIOREQ pIOReq);
    DWORD AtapiReadQChannel(PIOREQ pIOReq);
    DWORD AtapiLoadMedia(BOOL bEject=FALSE);
    DWORD AtapiStartDisc();
    BOOL AtapiDetectDVD();
    void AtapiDumpSenseData();
    DWORD ControlAudio(PIOREQ pIOReq);
    DWORD DVDReadKey(PIOREQ pIOReq);
    DWORD DVDGetRegion(PIOREQ pIOReq);
    DWORD DVDSendKey(PIOREQ pIOReq);
    DWORD DVDSetRegion(PIOREQ pIOReq);
    BOOL DVDGetCopySystem(LPBYTE pbCopySystem, LPBYTE pbRegionManagement);
};
