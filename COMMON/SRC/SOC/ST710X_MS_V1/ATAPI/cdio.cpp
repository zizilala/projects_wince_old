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
#ifdef ST202T_SATA
#include <satamain.h>
#else
#include <atamain.h>
#endif

#include <atapipcicd.h>

// pIOReq->pBytesReturned is safe (supplied by DSK_IOControl).  Exceptions are
// caught in CDisk::PerformIoctl.  And, we can map pInBuf and pOutBuf in-place,
// since they are copies setup by DSK_IOControl.
DWORD CPCIDiskAndCD::AtapiIoctl(PIOREQ pIOReq)
{
    DWORD dwError = ERROR_SUCCESS;
    DEBUGMSG(ZONE_IOCTL, (TEXT("ATAPI:PerformIoctl: %x DeviceId: %x \r\n"), pIOReq->dwCode, m_dwDeviceId));

    switch (pIOReq->dwCode) {
        //
        // ATAPI
        //
        case IOCTL_CDROM_READ_SG:
            PUCHAR  savedoldptrs[MAX_SG_BUF];   // This will hold a copy of the user mode pointers that get overwritten
                                                // ValidateSg
            SG_BUF  mappedbufs[MAX_SG_BUF];     // Temporary dummy array to convert SGX_BUF to SG_BUF in the call to UnmapSG

            // Sterilize buffer.  Note that it's correct to pass the address of
            // of m_pSterileCdRomReadRequest, as it may be reallocated.
            if (FALSE == SterilizeCdRomReadRequest(
                &m_pSterileCdRomReadRequest,
                &m_cbSterileCdRomReadRequest,
                (PCDROM_READ)pIOReq->pInBuf,
                pIOReq->dwInBufSize,
                ARG_O_PTR,
                savedoldptrs)) {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }
            // Replace buffer with sterile container.
            VERIFY(pIOReq->pInBuf = (LPBYTE)m_pSterileCdRomReadRequest);
            if (FALSE == AtapiIsUnitReadyEx()) {
                dwError = ERROR_NOT_READY;
                break;
            }
            // Execute IOCTL.
            dwError = ReadCdRom((CDROM_READ*)pIOReq->pInBuf, pIOReq->pBytesReturned);

            // Cleanup pointers mapped in SterilizeCdRomReadRequest

            ASSERT(m_pSterileCdRomReadRequest->sgcount <= MAX_SG_BUF);
            for(DWORD i = 0; i < m_pSterileCdRomReadRequest->sgcount; i++) {
                mappedbufs[i].sb_buf = m_pSterileCdRomReadRequest->sglist[i].sb_buf;
                mappedbufs[i].sb_len = m_pSterileCdRomReadRequest->sglist[i].sb_len;
            }

            if (FAILED(UnmapSg(
                        mappedbufs,
                        savedoldptrs,
                        m_pSterileCdRomReadRequest->sgcount,
                        ARG_O_PTR)))

            {
                ASSERT(!"Cleanup call to CeCloseCallerBuffer failed unexpectedly");
                dwError = ERROR_GEN_FAILURE;
            }
            break;
        case IOCTL_CDROM_RAW_READ:
            {
                // Technically, this IOCTL isn't documented, but it's defined
                // in cdioctl.h.
                CDROM_READ     CdRomRead;
                PRAW_READ_INFO pRawReadInfo = (PRAW_READ_INFO)pIOReq->pInBuf;
                // Validate buffers.
                if (NULL == pIOReq->pInBuf) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                if (NULL == pIOReq->pOutBuf) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Validate size.
                if (sizeof(RAW_READ_INFO) > pIOReq->dwInBufSize) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Execute IOCTL.
                if (FALSE == AtapiIsUnitReadyEx()) {
                    dwError = ERROR_NOT_READY;
                    break;
                }
                CdRomRead.StartAddr.Mode = CDROM_ADDR_LBA;
                CdRomRead.bRawMode = TRUE;
                CdRomRead.sgcount = 1;
                CdRomRead.TrackMode = CDDA;
                CdRomRead.StartAddr.Address.lba = pRawReadInfo->DiskOffset.LowPart;
                CdRomRead.TransferLength = (DWORD)(pRawReadInfo->SectorCount & 0xffffffff);
                if (FAILED(CeOpenCallerBuffer(
                    (PVOID *)&CdRomRead.sglist[0].sb_buf,
                    pIOReq->pOutBuf,
                    pIOReq->dwOutBufSize,
                    ARG_O_PTR,
                    FALSE))) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                CdRomRead.sglist[0].sb_len = pIOReq->dwOutBufSize;
                dwError = ReadCdRom(&CdRomRead, pIOReq->pBytesReturned);
                if (dwError != ERROR_SUCCESS) break;
                if (FAILED(CeCloseCallerBuffer(
                    (PVOID)CdRomRead.sglist[0].sb_buf,
                    pIOReq->pOutBuf,
                    pIOReq->dwOutBufSize,
                    ARG_O_PTR))) {
                    dwError = ERROR_GEN_FAILURE;
                }
            }
            break;
        case IOCTL_CDROM_TEST_UNIT_READY:
            {
                CDROM_TESTUNITREADY CdRomTestUnitReady;
                LPBYTE              pUnsafe = NULL;
                DWORD               cbUnsafe = 0;
                // Validate buffer and swap unsafe buffer and safe copy.
                if (pIOReq->pInBuf) {
                    pUnsafe = pIOReq->pInBuf;
                    pIOReq->pInBuf = (LPBYTE)&CdRomTestUnitReady;
                    cbUnsafe = pIOReq->dwInBufSize;
                }
                else if(pIOReq->pOutBuf) {
                    pUnsafe = pIOReq->pOutBuf;
                    pIOReq->pOutBuf = (LPBYTE)&CdRomTestUnitReady;
                    cbUnsafe = pIOReq->dwOutBufSize;
                }
                else {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Validate size.
                if (sizeof(CDROM_TESTUNITREADY) != cbUnsafe) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Execute IOCTL.
                if (FALSE == AtapiIsUnitReady(pIOReq)) {
                    dwError = ERROR_NOT_READY;
                    break;
                }
                // Return safe copy to unsafe buffer.
                if (0 == CeSafeCopyMemory((LPVOID)pUnsafe, (LPVOID)&CdRomTestUnitReady, cbUnsafe)) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
            break;
        case IOCTL_CDROM_DISC_INFO:
            {
                CDROM_DISCINFO CdRomDiscInfo;
                LPBYTE         pUnsafe = NULL;
                // Validate buffer.
                if (NULL == pIOReq->pOutBuf) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Validate size.
                if (sizeof(CDROM_DISCINFO) > pIOReq->dwOutBufSize) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Swap unsafe buffer and safe copy.
                pUnsafe = pIOReq->pOutBuf;
                pIOReq->pOutBuf = (LPBYTE)&CdRomDiscInfo;
                // Execute IOCTL.
                if (FALSE == AtapiIsUnitReadyEx()) {
                    dwError = ERROR_NOT_READY;
                    break;
                }
                dwError = AtapiGetDiscInfo(pIOReq);
                if (dwError != ERROR_SUCCESS) {
                    break;
                }
                // Return safe copy to unsafe buffer.
                if (0 == CeSafeCopyMemory((LPVOID)pUnsafe, (LPVOID)&CdRomDiscInfo, pIOReq->dwOutBufSize)) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
            break;
        case IOCTL_CDROM_EJECT_MEDIA:
            dwError = AtapiLoadMedia(TRUE);
            break;
        case IOCTL_CDROM_LOAD_MEDIA:
            dwError = AtapiLoadMedia(FALSE);
            break;
        case IOCTL_CDROM_GET_SENSE_DATA:
            {
                CD_SENSE_DATA CdSenseData;
                // Validate buffer.
                if (NULL == pIOReq->pOutBuf) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Validate size.
                if (sizeof(CD_SENSE_DATA) > pIOReq->dwOutBufSize) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Execute IOCTL.
                if (FALSE == AtapiGetSenseInfo(&CdSenseData)) {
                    dwError = ERROR_GEN_FAILURE;
                    break;
                }
                // Return safe copy to unsafe buffer.
                if (0 == CeSafeCopyMemory((LPVOID)pIOReq->pOutBuf, (LPVOID)&CdSenseData, pIOReq->dwOutBufSize)) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                dwError = ERROR_SUCCESS;
            }
            break;
        case IOCTL_CDROM_ISSUE_INQUIRY:
            {
                INQUIRY_DATA InquiryData;
                // Validate buffer.
                if (NULL == pIOReq->pOutBuf) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Validate size.
                if (sizeof(INQUIRY_DATA) > pIOReq->dwOutBufSize) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Execute IOCTL.
                if (FALSE == AtapiIssueInquiry(&InquiryData)) {
                    dwError = ERROR_GEN_FAILURE;
                    break;
                }
                // Return safe copy to unsafe buffer.
                if (0 == CeSafeCopyMemory((LPVOID)pIOReq->pOutBuf, (LPVOID)&InquiryData, pIOReq->dwOutBufSize)) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                dwError = ERROR_SUCCESS;
            }
            break;
        case IOCTL_CDROM_READ_TOC:
            {
                CDROM_TOC CdRomToc;
                LPBYTE    pUnsafe = NULL;
                DWORD     cbUnsafe = 0;
                // Validate buffer and swap unsafe buffer and safe copy.
                if (NULL != pIOReq->pInBuf) {
                    pUnsafe = pIOReq->pInBuf;
                    pIOReq->pInBuf = (LPBYTE)&CdRomToc;
                    cbUnsafe = pIOReq->dwInBufSize;
                }
                else if (NULL != pIOReq->pOutBuf) {
                    pUnsafe = pIOReq->pOutBuf;
                    pIOReq->pOutBuf = (LPBYTE)&CdRomToc;
                    cbUnsafe = pIOReq->dwOutBufSize;
                }
                else {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Validate size.
                if (sizeof(CDROM_TOC) > cbUnsafe) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                // Execute IOCTL.
                if (!AtapiIsUnitReadyEx()) {
                    dwError = ERROR_NOT_READY;
                    break;
                }
                if (FALSE == AtapiGetToc(&CdRomToc)) {
                    dwError = ERROR_GEN_FAILURE;
                    break;
                }
                // Return safe copy to unsafe buffer.
                if (0 == CeSafeCopyMemory((LPVOID)pUnsafe, (LPVOID)&CdRomToc, cbUnsafe)) {
                    dwError = ERROR_INVALID_PARAMETER;
                    break;
                }
                dwError = ERROR_SUCCESS;
            }
            break;
        //
        // DVD
        //
        case IOCTL_DVD_START_SESSION:
        case IOCTL_DVD_READ_KEY:
            // DVDReadKey validates args.
            if (FALSE == AtapiIsUnitReadyEx()) {
                dwError = ERROR_NOT_READY;
                break;
            }
            dwError = DVDReadKey(pIOReq);
            break;
        case IOCTL_DVD_END_SESSION:
        case IOCTL_DVD_SEND_KEY:
            // DVDSendKey validates args.
            if (FALSE == AtapiIsUnitReadyEx()) {
                dwError = ERROR_NOT_READY;
                break;
            }
            dwError = DVDSendKey(pIOReq);
            break;
        case IOCTL_DVD_GET_REGION:
            // DVDGetRegion validates args.
            dwError = DVDGetRegion(pIOReq);
            break;
        case IOCTL_DVD_SET_REGION:
            // DVDSetRegion validates args.
            dwError = DVDSetRegion(pIOReq);
            break;
        //
        // CDDA
        //
        case IOCTL_CDROM_READ_Q_CHANNEL:
            // AtapiReadQChannel validates args.
            if (FALSE == AtapiIsUnitReadyEx()) {
                dwError = ERROR_NOT_READY;
                break;
            }
            dwError = AtapiReadQChannel(pIOReq);
            break;
        case IOCTL_CDROM_PLAY_AUDIO_MSF:
        case IOCTL_CDROM_SEEK_AUDIO_MSF:
        case IOCTL_CDROM_RESUME_AUDIO:
        case IOCTL_CDROM_STOP_AUDIO:
        case IOCTL_CDROM_PAUSE_AUDIO:
        case IOCTL_CDROM_SCAN_AUDIO:
            if (FALSE == AtapiIsUnitReadyEx()) {
                dwError = ERROR_NOT_READY;
                break;
            }
            // ControlAudio validates args.
            dwError = ControlAudio(pIOReq);
            break;
        default:
            dwError = ERROR_NOT_SUPPORTED;
            break;
    }
    return dwError;
}


BOOL CPCIDiskAndCD::AtapiIsUnitReadyEx()
{
    DWORD dwCount;
    for (dwCount = 0; dwCount < 5; dwCount++) {
        if (AtapiIsUnitReady()) {
            m_dwLastCheckTime = GetTickCount();
            break;
        }
        StallExecution(100);
    }
    if (dwCount == 5)
        return FALSE;
    return TRUE;
}

DWORD CPCIDiskAndCD::SetupCdRomRead(BOOL bRawMode, DWORD dwLBAAddr, DWORD dwTransferLength, PATAPI_COMMAND_PACKET pCmdPkt)
{
    BOOL fIsDVD = (m_dwDeviceFlags & DFLAGS_DEVICE_ISDVD);

    memset( pCmdPkt, 0, sizeof(ATAPI_COMMAND_PACKET));


    /**** Atapi Packet *****
    Byte 0 - Cmd/OpCode
    Byte 1 - N/A
    Byte 2 - Logical Block (MSB)
    Byte 2 - Logical Block
    Byte 2 - Logical Block
    Byte 2 - Logical Block (LSB)
    Byte 6 - Reserved
    Byte 7 - DataLength (MSB)
    Byte 8 - DataLength (LSB)
    Byte 9 - Control Byte
    ****** Atapi Packet ****/
    pCmdPkt->Byte_1 = 0x00;
    pCmdPkt->Byte_2 = LBA_MSB(&dwLBAAddr);
    pCmdPkt->Byte_3 = LBA_3rdLSB(&dwLBAAddr);
    pCmdPkt->Byte_4 = LBA_2ndLSB(&dwLBAAddr);
    pCmdPkt->Byte_5 = LBA_LSB(&dwLBAAddr);
    if (fIsDVD && !bRawMode) {
        pCmdPkt->Opcode = ATAPI_PACKET_CMD_READ_12;
        pCmdPkt->Byte_6 = (BYTE)( dwTransferLength >> 24);
        pCmdPkt->Byte_7 = (BYTE)( (dwTransferLength & 0x00ff0000) >> 16);
        pCmdPkt->Byte_8 = (BYTE)( (dwTransferLength & 0x0000ff00) >> 8);
        pCmdPkt->Byte_9 = (BYTE)( dwTransferLength);
    } else {
        pCmdPkt->Opcode = bRawMode ? ATAPI_PACKET_CMD_READ_CD : ATAPI_PACKET_CMD_READ;
        pCmdPkt->Byte_6 = 0x00; // Reserved
        pCmdPkt->Byte_7 = (BYTE)( (dwTransferLength & 0x0000ff00) >> 8);
        pCmdPkt->Byte_8 = (BYTE)( dwTransferLength);
        pCmdPkt->Byte_9 = bRawMode ? 0x10 : 0x00;
    }

    pCmdPkt->Byte_10 = 0;
    return ERROR_SUCCESS;
}

DWORD CPCIDiskAndCD::ReadCdRom(CDROM_READ *pReadInfo, PDWORD pBytesReturned)
{
    ATAPI_COMMAND_PACKET    CmdPkt;
    CDROM_ADDR              CurAddr;
    WORD                    wSectorSize;
    DWORD                   dwError=ERROR_SUCCESS;
    PSGX_BUF                pSgBuf;

    GetBaseStatus(); // Clear Interrupt if it is already set

    CurAddr = pReadInfo->StartAddr;

    // The request must either be in MSF format or LBA format
    DEBUGCHK(pReadInfo->StartAddr.Mode == CDROM_ADDR_MSF || pReadInfo->StartAddr.Mode == CDROM_ADDR_LBA);

    DEBUGMSG( ZONE_IO | ZONE_CDROM, (TEXT("ATAPI:ReadCdRom Address=%ld Mode=%02X Length=%ld TrackMode=%02X\r\n"), CurAddr.Address, CurAddr.Mode, pReadInfo->TransferLength, pReadInfo->TrackMode));

    // If in MSF format then convert it to LBA
    if (CurAddr.Mode == CDROM_ADDR_MSF) {
        CDROM_MSF_TO_LBA(&CurAddr);
    }
    // Verify that the transfer count is not 0
    if ((pReadInfo->TransferLength == 0) ||
       (pReadInfo->sgcount == 0)) {
       return ERROR_INVALID_PARAMETER;
    }

    if( pReadInfo->bRawMode) {
        wSectorSize = CDROM_RAW_SECTOR_SIZE;
    } else {
        wSectorSize = CDROM_SECTOR_SIZE;
    }

    pSgBuf = &(pReadInfo->sglist[0]);

    if (IsDMASupported()) {
        dwError = ReadCdRomDMA(CurAddr.Address.lba, pReadInfo->TransferLength, wSectorSize, pReadInfo->sgcount, pSgBuf);
        if (dwError == ERROR_SUCCESS) {
            *(pBytesReturned) = pReadInfo->TransferLength * wSectorSize;
        }
    } else {
        SetupCdRomRead(pReadInfo->bRawMode, CurAddr.Address.lba, pReadInfo->TransferLength, &CmdPkt);

        if (AtapiSendCommand(&CmdPkt, wSectorSize, IsDMASupported())) {
            if (!AtapiReceiveData(pSgBuf, pReadInfo->sgcount,pBytesReturned)) {
                dwError = ERROR_READ_FAULT;
            }
        } else {
             dwError = ERROR_READ_FAULT;
        }
    }
    return dwError;
}

DWORD CPCIDiskAndCD::ReadCdRomDMA(DWORD dwLBAAddr, DWORD dwTransferLength, WORD wSectorSize, DWORD dwSgCount, SGX_BUF *pSgBuf)
{
    ATAPI_COMMAND_PACKET    CmdPkt;
    DWORD                   dwError=ERROR_SUCCESS;
    DWORD               dwSectorsToTransfer;
    SG_BUF              CurBuffer[MAX_SG_BUF];
    WORD                wCount;

    DWORD dwStartBufferNum = 0, dwEndBufferNum = 0, dwEndBufferOffset = 0;
    DWORD dwNumSectors = dwTransferLength;
    DWORD dwStartSector = dwLBAAddr;

    // Process the SG buffers in blocks of MAX_CD_SECT_PER_COMMAND.  Each DMA request will have a new SG_BUF array
    // which will be a subset of the original request, and may start/stop in the middle of the original buffer.
    while (dwNumSectors) {

        dwSectorsToTransfer = (dwNumSectors > MAX_CD_SECT_PER_COMMAND) ? MAX_CD_SECT_PER_COMMAND : dwNumSectors;

        DWORD dwBufferLeft = dwSectorsToTransfer * wSectorSize;
        DWORD dwNumSg = 0;

        while (dwBufferLeft) {
            DWORD dwCurBufferLen = pSgBuf[dwEndBufferNum].sb_len - dwEndBufferOffset;

            if (dwBufferLeft < dwCurBufferLen) {
                // The buffer left for this block is less than the current SG buffer length
                CurBuffer[dwEndBufferNum - dwStartBufferNum].sb_buf = pSgBuf[dwEndBufferNum].sb_buf + dwEndBufferOffset;
                CurBuffer[dwEndBufferNum - dwStartBufferNum].sb_len = dwBufferLeft;
                dwEndBufferOffset += dwBufferLeft;
                dwBufferLeft = 0;
            } else {
                // The buffer left for this block is greater than or equal to the current SG buffer length.  Move on to the next SG buffer.
                CurBuffer[dwEndBufferNum - dwStartBufferNum].sb_buf = pSgBuf[dwEndBufferNum].sb_buf + dwEndBufferOffset;
                CurBuffer[dwEndBufferNum - dwStartBufferNum].sb_len = dwCurBufferLen;
                dwEndBufferOffset = 0;
                dwEndBufferNum++;
                dwBufferLeft -= dwCurBufferLen;
            }
            dwNumSg++;
        }

        if (!SetupDMA(CurBuffer, dwNumSg, TRUE)) {
            dwError = ERROR_READ_FAULT;
            goto ExitFailure;
        }

        BeginDMA(TRUE);

        SetupCdRomRead( wSectorSize == CDROM_RAW_SECTOR_SIZE ? TRUE : FALSE, dwStartSector, dwSectorsToTransfer, &CmdPkt);

        wCount = (SHORT)((dwSectorsToTransfer * wSectorSize) >> 1);

        if (AtapiSendCommand(&CmdPkt, wCount, IsDMASupported())) {
            if (m_fInterruptSupported) {
                if (!WaitForInterrupt(m_dwDiskIoTimeOut)) {
                    DEBUGMSG( ZONE_IO, (TEXT("ATAPI:ReadCdRom- WaitforInterrupt failed (DevId %x) \r\n"),m_dwDeviceId));
                    dwError = ERROR_READ_FAULT;
                    goto ExitFailure;
                }
            }
            if (EndDMA()) {
                WaitOnBusy(FALSE);
                CompleteDMA( (PSG_BUF)pSgBuf, dwSgCount, TRUE);
            } else {
                dwError = ERROR_READ_FAULT;
                goto ExitFailure;
            }
        }


        dwStartSector += dwSectorsToTransfer;
        dwStartBufferNum = dwEndBufferNum;
        dwNumSectors -= dwSectorsToTransfer;
    }


ExitFailure:
    if (dwError != ERROR_SUCCESS) {
        AbortDMA();
    }
    return dwError;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//---------------------------------------------------------------------------
//
//  Send command to ATA Device.
//
//---------------------------------------------------------------------------

BOOL CPCIDiskAndCD::AtapiSendCommand(PATAPI_COMMAND_PACKET pCmdPkt, WORD wCount, BOOL fDMA)
{
    // Set the Drive/Head registers
    SelectDevice();
    GetBaseStatus();

    SelectDevice();
     if (WaitOnBusy(FALSE))
     {
         if (GetError() & ATA_STATUS_ERROR)
         {
            return FALSE;
         }
     }
#if 1
//    for( DWORD dwCount = 0; dwCount < 10; dwCount++)   {
//        if( WaitForInterrupt(0)) {
//            break;
//        }
//    }
#endif

    WriteSectorCount(0);

    WriteSectorNumber(0);


    // Set the byte tranfer count
    if (wCount) {
        WriteLowCount((BYTE)(0xff & wCount));
        WriteHighCount((BYTE)(0xff & (wCount >> 8)));
    } else {
        WriteLowCount(0xFE);
        WriteHighCount(0xFF);
    }


    //
    // Set PIO or DMA Mode as specified in bFlags. 0 = PIO, 1 = DMA
    //
    WriteFeature(fDMA ? 0x1 : 0x0);

    WaitForDisc( WAIT_TYPE_NOT_BUSY, 20);

    // Write ATAPI into  command register

    SelectDevice();

    WriteCommand(ATAPI_CMD_COMMAND);

    WaitForDisc( WAIT_TYPE_NOT_BUSY, 20);
    //
    // Check how device is reporting CPU attention: DRQ or INTRQ?
    // INTRQ within 10 ms!!!
    //
    if (m_fInterruptSupported && IsDRQTypeIRQ())
    {
        //  ATA_INTR_CMD is expected
        //
        if (!WaitForInterrupt(m_dwDiskIoTimeOut))
        {
            DEBUGMSG( ZONE_IO | ZONE_CDROM, (TEXT("ATAPI:AtapiSendCommand - Wait for ATA_INTR_CMD Interrupt (DevId %x) \r\n"), m_dwDeviceId));
            return FALSE;
        }
    }
    //
    // Device will assert DRQ  within (50us or 3ms) if no interrupt id used
    // Wait for not BSY and DRQ


    if (!WaitForDRQ())
    {
        DEBUGMSG( ZONE_IO | ZONE_CDROM, (TEXT("ATAPIPCI:AtapiSendCommand 1 - ATAWaitForDisc failed with: %x (DevId %x)\r\n"), GetError(), m_dwDeviceId));
        return FALSE;

    }

    // Write the ATAPI Command Packet.

    WriteWordBuffer( (LPWORD)pCmdPkt,GetPacketSize()/sizeof(WORD));

    return TRUE;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

BOOL CPCIDiskAndCD::AtapiReceiveData(PSGX_BUF pSgBuf, const DWORD dwSgCount,LPDWORD pdwBytesRead)
{
    DWORD       dwSgLeft = dwSgCount;
    DWORD       dwTransferCount = 0;
    PSGX_BUF    pCurrentSegment = NULL;
    DWORD       dwReadCount = 0;
    DWORD       dwThisCount = 0;
    BYTE        *pCurrentBuffer = NULL;
    DWORD       dwLen = 0;

    DEBUGMSG( ZONE_IO | ZONE_CDROM, (TEXT("ATAPI:AtapiReceiveData - Entered SgCount=%ld.\r\n"), dwSgCount));

    if (ERROR_SUCCESS != WaitForDisc( WAIT_TYPE_READY, 5000, 100))
        return FALSE;

    pCurrentSegment = pSgBuf;

    // Illegal arguments
    if (!pCurrentSegment && dwSgCount > 0) {
        DEBUGMSG(ZONE_ERROR, (TEXT(
            "Atapi!CPCIDiskAndCD::AtapiReceiveData> pSgBuf null\r\n"
            )));
        return ERROR_INVALID_PARAMETER;
    }

    // The TEST UNIT READY command processor will call this with a null scatter/gather
    // buffer and a null scatter/gather buffer count--which is valid.  We only
    // want to map the caller buffer if the buffer and count are non-null.

    if (pCurrentSegment && dwSgCount > 0) {
        pCurrentBuffer = (LPBYTE)pCurrentSegment->sb_buf;                   
        dwLen = pCurrentSegment->sb_len;
    }

    m_wNextByte = 0xFFFF; // There is no byte left from the previous transaction.

    for(;;) {
        if (m_fInterruptSupported) {
            //
            //  Waiting for ATA_INTR_READ or ATA_INTR_WRITE  or ATA_INTR_READY
            //
            if (!WaitForInterrupt(m_dwDiskIoTimeOut)) {
                DEBUGMSG(ZONE_CDROM, (TEXT("ATAPI:AtapiReceiveData - Wait for ATA_INTR_READ failed (DevId %x) \r\n"), m_dwDeviceId));
                return FALSE;
            }
            WORD wState = CheckIntrState();
            //
            // Return Error if not IO Interrupt
            //
            if ((wState ==  ATA_INTR_ERROR) || (GetAltStatus() & ATA_STATUS_ERROR))
            {
                DEBUGMSG( ZONE_IO | ZONE_CDROM, (TEXT("ATAPI:AtapiReceiveData - Wait for ATA_INTR_READ failed (DevId %x) \r\n"), m_dwDeviceId));
                return FALSE;

            }
            if (wState ==  ATA_INTR_READY)
            {
                DEBUGMSG( ZONE_IO | ZONE_CDROM, (TEXT("ATAPI:AtapiReceiveData - Exiting with Interrupt Ready signal Device=%ld\r\n"), m_dwDeviceId));
                return TRUE;
            }


        };
        //
        // Wait until device is ready for  data transfer.
        //
        if (!WaitForDRQ())
        {
            DEBUGMSG( ZONE_IO | ZONE_CDROM, (TEXT("ATAPI:AtapiReceiveData Failed at WaitForDRQ Status=%02X Error=%02X Deivce=%ld\r\n"), GetAltStatus(), GetError(), m_dwDeviceId));
            return(FALSE);
        }

        //
        //  Read Transfer Counter set by Device.
        //
        dwTransferCount = GetCount();

        DEBUGMSG (ZONE_CDROM | ZONE_IO, (TEXT(">>>Read Transfer Count : %x  SG=%x \r\n"),dwTransferCount,dwSgLeft));

        while ((dwSgLeft>0) && (dwTransferCount>0))
        {
            dwThisCount = min(dwTransferCount, dwLen);

            if (pCurrentBuffer) {
                ReadBuffer(pCurrentBuffer,dwThisCount);
                dwTransferCount -= dwThisCount;
                dwReadCount += dwThisCount;
            }
            pCurrentBuffer += dwThisCount;
            dwLen -= dwThisCount;

            if (dwLen == 0) {
                // Go to the next SG
                dwSgLeft--;
                pCurrentSegment++;
                if (dwSgLeft && pCurrentSegment) {
                    dwLen = pCurrentSegment->sb_len;
                    pCurrentBuffer = (LPBYTE)pCurrentSegment->sb_buf;
                }
            }

        } // End of while loop

        // Discard the rest of data if left.

        while (dwTransferCount > 0)
        {
            (void) ReadWord();
            dwTransferCount-=2 ;
        }
        if (pdwBytesRead)
            *pdwBytesRead = dwReadCount;
        if (!dwSgLeft)
            break;
    }

    return TRUE;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
BOOL CPCIDiskAndCD::AtapiSendData(PSGX_BUF pSgBuf, DWORD dwSgCount,LPDWORD pdwBytesWrite)
{
    DWORD       dwSgLeft = dwSgCount;
    DWORD       dwTransferCount;
    PSGX_BUF     pCurrentSegment;
    DWORD       dwWriteCount = 0;
    DWORD       dwThisCount;
    BYTE        *pCurrentBuffer=NULL;

    DEBUGMSG( ZONE_IO | ZONE_CDROM, (TEXT("ATAPI:AtapiReceiveData - Entered SgCount=%ld.\r\n"), dwSgCount));

    pCurrentSegment = pSgBuf;

    m_wNextByte = 0xFFFF; // There is no byte left from the previous transaction.

    for(;;) {
        if (m_fInterruptSupported) {
            //
            //  Waiting for ATA_INTR_READ or ATA_INTR_WRITE  or ATA_INTR_READY
            //
            if (!WaitForInterrupt(m_dwDiskIoTimeOut)) {
                DEBUGMSG( ZONE_CDROM | ZONE_ERROR, (TEXT("ATAPI:AtapiReceiveData - Wait for ATA_INTR_READ failed (DevId %x) \r\n"), m_dwDeviceId));
                return FALSE;
            }
            WORD wState = CheckIntrState();
            //
            // Return Error if not IO Interrupt
            //
            if (wState ==  ATA_INTR_ERROR)
            {
                DEBUGMSG( ZONE_IO | ZONE_ERROR | ZONE_CDROM, (TEXT("ATAPI:AtapiReceiveData - Wait for ATA_INTR_READ failed (DevId %x) \r\n"), m_dwDeviceId));
                return FALSE;

            }
            if (wState ==  ATA_INTR_READY)
            {
                DEBUGMSG( ZONE_IO | ZONE_CDROM, (TEXT("ATAPI:AtapiReceiveData - Exiting with Interrupt Ready signal Device=%ld\r\n"), m_dwDeviceId));
                return TRUE;
            }
        };
        //
        // Wait until device is ready for  data transfer.
        //
        if (!WaitForDRQ())
        {
            DEBUGMSG( ZONE_IO | ZONE_ERROR | ZONE_CDROM, (TEXT("ATAPI:AtapiReceiveData Failed at WaitForDRQ Status=%02X Error=%02X Deivce=%ld\r\n"), GetAltStatus(), GetError(), m_dwDeviceId));
            return(FALSE);
        }

        //
        //  Read Transfer Counter set by Device.
        //
        dwTransferCount = GetCount();

        DEBUGMSG (ZONE_CDROM | ZONE_IO, (TEXT(">>>Read Transfer Count : %x  SG=%x \r\n"),dwTransferCount,dwSgLeft));

        while ((dwSgLeft>0) && (dwTransferCount>0))
        {
            ASSERT(pCurrentSegment);
            dwThisCount = min(dwTransferCount, pCurrentSegment->sb_len);

            if (pCurrentSegment->sb_buf) {
                pCurrentBuffer = (LPBYTE)pCurrentSegment->sb_buf;
            }

            if (pCurrentBuffer)
            {
                WriteBuffer(pCurrentBuffer,dwThisCount);
                dwTransferCount -= dwThisCount;
                dwWriteCount += dwThisCount;
            }
            pCurrentSegment->sb_len -=dwThisCount;
            pCurrentSegment->sb_buf +=dwThisCount;

            if (pCurrentSegment->sb_len == 0)
            {
                // Go to the next SG
                dwSgLeft--;
                pCurrentSegment++;
            }

        } // End of while loop

        if (pdwBytesWrite)
            *pdwBytesWrite = dwWriteCount;
        if (!dwSgLeft)
            break;
    }
    return TRUE;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

BOOL CPCIDiskAndCD::AtapiIsUnitReady(PIOREQ pIOReq)
{
    ATAPI_COMMAND_PACKET    CmdPkt;
    DWORD dwRet;
    BOOL fRet = TRUE;
    if (!IsRemoveableDevice())
        return(TRUE);
    memset(&CmdPkt, 0, sizeof(ATAPI_COMMAND_PACKET));
    CmdPkt.Opcode = ATAPI_PACKET_CMD_TEST_READY;
    if (AtapiSendCommand(&CmdPkt)) {
        if (!AtapiReceiveData( NULL, 0, &dwRet)) {
            fRet = FALSE;
        }
    } else {
         fRet = FALSE;
    }
    if ( pIOReq && pIOReq->pInBuf)
    {
        ((CDROM_TESTUNITREADY *)pIOReq->pInBuf)->bUnitReady = fRet;
    }
    if ( pIOReq && pIOReq->pOutBuf)
    {
        ((CDROM_TESTUNITREADY *)pIOReq->pOutBuf)->bUnitReady = fRet;
    }
    if (!fRet) {
        DEBUGMSG(ZONE_CDROM, (TEXT("ATAPI:AtapiIsUnitReady Status=%02X Error=%02X Reason=%02X\r\n"), GetAltStatus(), GetError(), GetReason()));
    }

    return fRet;
}

//---------------------------------------------------------------------------
//
//      function: AtaGetSenseInfo
//
//      synopsis: Issue a request sense command to get additional error data
//
//              ENTRY
//
//              EXIT
//
//                      Failure
//                              Returns an extended error code.
//
//-----------------------------------------------------------------------------

BOOL CPCIDiskAndCD::AtapiGetSenseInfo(CD_SENSE_DATA *pSenseData)
{
    ATAPI_COMMAND_PACKET    CmdPkt;
    SGX_BUF SgBuf;
    DWORD dwRet;

    memset(&CmdPkt, 0, sizeof(ATAPI_COMMAND_PACKET));
    CmdPkt.Opcode = ATAPI_PACKET_CMD_REQUEST_SENSE;
    CmdPkt.Byte_4 = (BYTE)sizeof(CD_SENSE_DATA);

    SgBuf.sb_len = sizeof(CD_SENSE_DATA);
    SgBuf.sb_buf = (PBYTE) pSenseData;

    if (AtapiSendCommand(&CmdPkt)) {
        if (!AtapiReceiveData(&SgBuf, 1, &dwRet)) {
            DEBUGMSG( ZONE_ERROR|ZONE_CDROM, (TEXT("AtaGetSenseInfo Failed!!!\r\n")));
            return FALSE;
        }
    } else {
         return FALSE;
    }
    // Issue the request sense command

    DEBUGMSG( ZONE_CDROM | ZONE_IO, (TEXT("Sense Info\r\n")));
    DEBUGMSG( ZONE_CDROM | ZONE_IO, (TEXT("   Error Code = 0x%2.2X, Segment Number = %d, Sense Key = 0x%2.2X\r\n"), pSenseData->sd_ErrCode, pSenseData->sd_SegNum, pSenseData->sd_ILI_Key));
    DEBUGMSG( ZONE_CDROM | ZONE_IO, (TEXT("   Information = 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\r\n"), pSenseData->sd_Info[0], pSenseData->sd_Info[1], pSenseData->sd_Info[2], pSenseData->sd_Info[3]));
    DEBUGMSG( ZONE_CDROM | ZONE_IO, (TEXT("   Additional Sense Length = %d\r\n"), pSenseData->sd_Length));
    DEBUGMSG( ZONE_CDROM | ZONE_IO, (TEXT("   Command Specific Information = 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\r\n"),pSenseData->sd_CmdInfo[0], pSenseData->sd_CmdInfo[1], pSenseData->sd_CmdInfo[2], pSenseData->sd_CmdInfo[3]));
    DEBUGMSG( ZONE_CDROM | ZONE_IO, (TEXT("   ASC = 0x%2.2X, ASCQ = 0x%2.2X, FRUC = 0x%2.2X\r\n"), pSenseData->sd_SenseCode, pSenseData->sd_Qualifier, pSenseData->sd_UnitCode));
    DEBUGMSG( ZONE_CDROM | ZONE_IO, (TEXT("   Sense Key Specfic = 0x%2.2X 0x%2.2X 0x%2.2X\r\n"), pSenseData->sd_Key1, pSenseData->sd_Key2, pSenseData->sd_Key3));

    return TRUE;
}


//
//  This function send INQUIRY command to Atapi Device and
//  process reply  corresponding reply.
//

BOOL CPCIDiskAndCD::AtapiIssueInquiry(INQUIRY_DATA *pInqData)
{
    ATAPI_COMMAND_PACKET    CmdPkt;
    SGX_BUF SgBuf;
    DWORD dwRet;

    memset((void *)&CmdPkt, 0, sizeof(CmdPkt));
    CmdPkt.Opcode = ATAPI_PACKET_CMD_INQUIRY;
    CmdPkt.Byte_4 = sizeof(INQUIRY_DATA);

    SgBuf.sb_len = sizeof(INQUIRY_DATA);
    SgBuf.sb_buf = (PBYTE) pInqData;

    if (AtapiSendCommand(&CmdPkt)) {
        if (!AtapiReceiveData(&SgBuf, 1, &dwRet)) {
            DEBUGMSG( ZONE_ERROR|ZONE_CDROM, (TEXT("AtapiIssueInquriy Failed\r\n")));
            return FALSE;
        }
    } else {
       return FALSE;
    }
    return TRUE;
 }


//---------------------------------------------------------------------------
//
//  Function: AtapiLoadMedia
//
//  synopsis: Process a IOCTL_CDROM_LOAD_MEDIA/IOCTL_CDROM_EJECT_MEDIA request
//
//  ENTRY
//      Nothing
//  EXIT
//      Extended error set on failure
//
//-----------------------------------------------------------------------------


DWORD CPCIDiskAndCD::AtapiLoadMedia(BOOL bEject)
{
    ATAPI_COMMAND_PACKET    CmdPkt;
    SGX_BUF SgBuf;
    DWORD dwRet;
    DWORD dwError = ERROR_SUCCESS;

    DEBUGMSG(ZONE_IOCTL, (TEXT("ATAPI:ATAPILoadMedia - Entered. Load=%s\r\n"), bEject ? L"TRUE": L"FALSE"));
    memset((void *)&CmdPkt, 0, sizeof(CmdPkt));
    CmdPkt.Opcode = ATAPI_PACKET_CMD_START_STOP;
    CmdPkt.Byte_4 = bEject ? 2 : 3;

    SgBuf.sb_len = 0;
    SgBuf.sb_buf = NULL;

    if (AtapiSendCommand(&CmdPkt)) {
        if (!AtapiReceiveData(&SgBuf, 0, &dwRet)) {
            DEBUGMSG( ZONE_ERROR|ZONE_CDROM, (TEXT("ATAPI::LoadMedia failed on receive\r\n")));
            dwError = ERROR_READ_FAULT;
        }
        if (!bEject) {
            Sleep(5000);
            AtapiIsUnitReadyEx();
        }
    } else {
       return ERROR_GEN_FAILURE;
    }
    return dwError;
}

DWORD CPCIDiskAndCD::AtapiStartDisc()
{
    ATAPI_COMMAND_PACKET    CmdPkt;
    SGX_BUF SgBuf;
    DWORD dwRet;
    DWORD dwError = ERROR_SUCCESS;

    DEBUGMSG(ZONE_CDROM, (TEXT("ATAPI:AtapiStartDisc - Entered.\r\n")));
    memset((void *)&CmdPkt, 0, sizeof(CmdPkt));
    CmdPkt.Opcode = ATAPI_PACKET_CMD_START_STOP;
    CmdPkt.Byte_4 = 1;

    SgBuf.sb_len = 0;
    SgBuf.sb_buf = NULL;

    if (AtapiSendCommand(&CmdPkt)) {
        if (!AtapiReceiveData(&SgBuf, 1, &dwRet)) {
            DEBUGMSG( ZONE_ERROR|ZONE_CDROM, (TEXT("ATAPI::LoadMedia failed on receive\r\n")));
            dwError = ERROR_READ_FAULT;
        }
    } else {
       return ERROR_GEN_FAILURE;
    }
    return dwError;
}

DWORD CPCIDiskAndCD::AtapiReadQChannel(PIOREQ pIOReq)
{
    ATAPI_COMMAND_PACKET    CmdPkt;
    SGX_BUF SgBuf;
    DWORD dwRet;
    DWORD dwError = ERROR_SUCCESS;
    CDROM_SUB_Q_DATA_FORMAT *pcsqdf = (CDROM_SUB_Q_DATA_FORMAT *)pIOReq->pInBuf;
    SUB_Q_CHANNEL_DATA sqcd;
    DWORD dwDataSize = 0;

    if (!pcsqdf || !pIOReq->pOutBuf || (sizeof(CDROM_SUB_Q_DATA_FORMAT) != pIOReq->dwInBufSize)) {
        return ERROR_BAD_ARGUMENTS;
    }
    switch( pcsqdf->Format) {
        case IOCTL_CDROM_CURRENT_POSITION:
            dwDataSize = sizeof(SUB_Q_CURRENT_POSITION);
            break;
        case IOCTL_CDROM_MEDIA_CATALOG:
            dwDataSize = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
            break;
        case IOCTL_CDROM_TRACK_ISRC:
            dwDataSize = sizeof(SUB_Q_TRACK_ISRC);
            break;
        default:
            dwError = ERROR_BAD_FORMAT;
    }
    if (pIOReq->dwOutBufSize < dwDataSize) {
        dwError = ERROR_BAD_ARGUMENTS;
    }

    if (dwError == ERROR_SUCCESS) {
        memset((void *)&CmdPkt, 0, sizeof(CmdPkt));
        CmdPkt.Opcode = ATAPI_PACKET_CMD_READ_SUB_CHAN;
            CmdPkt.Byte_1 = 0x00;
            CmdPkt.Byte_2 = 0x40;               // Header + data required
            CmdPkt.Byte_3 = (BYTE)(pcsqdf->Format);
            CmdPkt.Byte_8 = (BYTE)dwDataSize;

        SgBuf.sb_len = dwDataSize;
        SgBuf.sb_buf = (PBYTE) &sqcd;

        if (AtapiSendCommand(&CmdPkt)) {
            if (!AtapiReceiveData(&SgBuf, 1, &dwRet)) {
                DEBUGMSG( ZONE_ERROR|ZONE_CDROM, (TEXT("ATAPI::ReadQChannel failed on receive\r\n")));
                dwError = ERROR_READ_FAULT;
            } else {
                memcpy( pIOReq->pOutBuf, (LPBYTE)&sqcd, dwDataSize);
            }
        } else {
            DEBUGMSG( ZONE_ERROR|ZONE_CDROM, (TEXT("ATAPI::ReadQChannel failed on SendCommand\r\n")));
            dwError = ERROR_GEN_FAILURE;
        }
    }

    return dwError;
}

//---------------------------------------------------------------------------
//
//  Function: AtapiGetToc
//
//  synopsis: Gets the table of contents from the media
//
//  ENTRY
//      Nothing
//  EXIT
//      Extended error set on failure
//
//-----------------------------------------------------------------------------
BOOL CPCIDiskAndCD::AtapiGetToc(CDROM_TOC * pTOC)
{
    ATAPI_COMMAND_PACKET    CmdPkt;
    SGX_BUF SgBuf;
    DWORD dwRet;
    BOOL fRet = TRUE;

    memset((void *)&CmdPkt, 0, sizeof(CmdPkt));
    CmdPkt.Opcode = ATAPI_PACKET_CMD_READ_TOC;
    CmdPkt.Byte_1 = 0x02;           // Use MSF Address Format
    CmdPkt.Byte_7 = (BYTE)((sizeof(CDROM_TOC)>> 8) &0x0FF);    //3
    CmdPkt.Byte_8 = (BYTE)(sizeof(CDROM_TOC) &0x0FF);          // 24

    SgBuf.sb_len = sizeof(CDROM_TOC);
    SgBuf.sb_buf = (PBYTE) pTOC;

    if (AtapiSendCommand(&CmdPkt)) {
        if (!AtapiReceiveData(&SgBuf, 1, &dwRet)) {
            DEBUGMSG( ZONE_ERROR|ZONE_CDROM, (TEXT("ATAPI::GetToc failed on receive\r\n")));
            fRet = FALSE;
        } else {
            WORD wTOCDataLen = (MAKEWORD(pTOC->Length[1], pTOC->Length[0]) - 2);

            //
            //      Minus 4 for the the entire header
            //

            if (wTOCDataLen != (dwRet - 4))
            {
                DEBUGMSG(ZONE_ERROR, (TEXT("ATAPI:AtapiGetToc - Bad Length in TOC Header = %d, Expecting = %d\r\n"), wTOCDataLen, dwRet - 4));
                fRet = FALSE;
            }

            if ((wTOCDataLen % sizeof(TRACK_DATA)) != 0)
            {
                DEBUGMSG( ZONE_ERROR, (TEXT("ATAPI:AtapiGetToc - Data length  = %d which is not a multiple of CDREADTOC_MSFINFO size\r\n"), wTOCDataLen));
                fRet = FALSE;
            }
        }
    } else {
         DEBUGMSG( ZONE_ERROR|ZONE_CDROM, (TEXT("ATAPI::GetToc failed on SendCommand\r\n")));
        fRet = FALSE;
    }

    return fRet;
}

//---------------------------------------------------------------------------
//
//      Function: AtapiGetDiscInfo
//
//      Synopsis: implements CDROM_IOCTL_DISC_INFO - Get Disc Information
//
//              This function returns the TOC (table of contents) information
//              from the Q-channel in the lead-in track of the disc, which
//              indicates the starting and ending tracks and the start of
//              the lead-out track.
//
//              The first and last track number are binary values, not BCD.
//
//              It is recommended that the information from the TOC be read
//              in and cached on drive initialization so that when this function
//              is called, there is not need to interrupt drive play to get this
//              information.  Note that the first and last track numbers do not
//              include the lead-in and lead-out tracks of the session.
//
//              The SessionIndex is used to get TOC information from disks
//              with more than one recorded session.  Sessions are the same
//              as OrangeBook Volumes.  The first session has a SessionIndex
//              of zero, and the second session has a SessionIndex of one.
//              A SessionIndex of DISC_INFO_LAST_SESSION (-1) requests the disc
//              info for the last session recorded on the disc.
//
//              The LogicStartAddr is the logical sector address of the first
//              data sector of the first track in this session.
//
//              For standard Redbook and Yellowbook CD-ROMs, zero (0) is the
//              only valid SessionIndex value.  In this case, LogicStartAddr
//              should be returned as zero (0).
//
//              Note: The LogicStartAddr is normally used to locate the first
//              sector of the Volume Recognition Sequence for the indicated
//              session. The Volume Recognition Sequence for the session is
//              expected to start at Sector (LogicStartAddr + 16).
//
//      ENTRY
//              IOCTL Packet format specified by CDROM_DISCINFO structure.
//              Reserved - Reserved Zero
//              SessionIndex - Set to desired session number to query.
//              SessionIndex = 0 indicates a query for the first session
//              SessionIndex = 0xFFFFFFFF indicates a query for the last
//              session on the disc
//
//      EXIT
//              Success
//                      Returns NO_ERROR
//                      CDROM_DISCINFO structure filled in for indicated session
//
//                      If SessionIndex was passed in as 0xFFFFFFFF, it may be
//                      modified to contain the index of the last session on the
//                      disc.  Because not all device drivers must scan through
//                      all the sessions on the disc, this update may or may not
//                      actually take place.
//
//              Failure
//                      Returns an extended error code.
//                      ERROR_UNKNOWN_COMMAND - The command is not supported
//                      ERROR_INVALID_PARAMETER - The session number is invalid
//                      Other Extended Errors may also be returned
//-----------------------------------------------------------------------------

DWORD CPCIDiskAndCD::AtapiGetDiscInfo(PIOREQ pIOReq)
{
// TODO: Support multiple sessions
    CDROM_DISCINFO *pDiscInfo = (CDROM_DISCINFO *)pIOReq->pOutBuf;
    CDROM_TOC Toc;
    DWORD dwError = ERROR_SUCCESS;

    return ERROR_SUCCESS; // Bypass for now !!!

    if ((pDiscInfo == NULL) ||
        (pIOReq->dwOutBufSize< sizeof(CDROM_DISCINFO))) {
        return(ERROR_INVALID_PARAMETER);
    }
    if (AtapiGetToc(&Toc)) {

        TRACK_DATA *pTrack = &(Toc.TrackData[0]);

        WORD wStartTrack = 0, wLastTrack = 0;
        // Find the first & last track of the requested session using the LeadOutTrack to detect
        // the session changes
        WORD wTrackIndex;

        //
        //      The size or the array is the indicated size minus two bytes. One byte
        //      for the first track number and one for the last
        //

        WORD wTOCDataLen = (MAKEWORD(Toc.Length[1], Toc.Length[0]) - 2);

        // If there is already a TOC buffer alloacted then see if it is big enough to hold the
        // newly read TOC, else allocate a buffer to cache the TOC track info here

        WORD wTrackCount = wTOCDataLen/sizeof(TRACK_DATA);

        for (wTrackIndex = 0; wTrackIndex < wTrackCount; wTrackIndex++) {
            if (pTrack[wTrackIndex].TrackNumber == 0xaa) { // CDATAPI_SESSION_LEADOUT_TRACK = 0xaa
                break;
            } else {
                if( pTrack[wTrackIndex].TrackNumber < wStartTrack)
                {
                    wStartTrack = pTrack[wTrackIndex].TrackNumber;
                    wStartTrack = wTrackIndex;
                }
                if( pTrack[wTrackIndex].TrackNumber > wLastTrack)
                {
                    wLastTrack = pTrack[wTrackIndex].TrackNumber;
                }
            }
        }

        if (wTrackIndex == wTrackCount)  {
            dwError = ERROR_IO_DEVICE;
            return FALSE;
        }

        pDiscInfo->FirstTrack = pTrack[wStartTrack].TrackNumber;
        pDiscInfo->LastTrack =  pTrack[wLastTrack].TrackNumber;

        pDiscInfo->LogicStartAddr = CDROM_MSFCOMP_TO_LBA(pTrack[wStartTrack].Address[1], pTrack[wStartTrack].Address[2], pTrack[wStartTrack].Address[3]);

        pDiscInfo->LeadOutTrackAddr.msf_Frame = pTrack[wTrackIndex].Address[3];
        pDiscInfo->LeadOutTrackAddr.msf_Second = pTrack[wTrackIndex].Address[2];
        pDiscInfo->LeadOutTrackAddr.msf_Minute = pTrack[wTrackIndex].Address[1];
        pDiscInfo->LeadOutTrackAddr.msf_Filler = 0;

        pDiscInfo->FirstSession = 1;
        pDiscInfo->LastSession = 1;
        pDiscInfo->RetSession = 1;

    }
    return dwError;
}


BOOL CPCIDiskAndCD::AtapiDetectDVD()
{
    CDMODE_SENSE_INFO SenseInfo;
    ATAPI_COMMAND_PACKET    CmdPkt;
    SGX_BUF SgBuf;
    DWORD dwRet;


    memset(&CmdPkt, 0, sizeof(ATAPI_COMMAND_PACKET));
    CmdPkt.Opcode = ATAPI_PACKET_CMD_MODE_SENSE;
    CmdPkt.Byte_2 = 0x2a;                      // Get the mech status page
    CmdPkt.Byte_7 = 2;                      // Say 512 bytes, expect only 18

    SgBuf.sb_len = sizeof(CDMODE_SENSE_INFO);
    SgBuf.sb_buf = (PBYTE) &SenseInfo;

    if (AtapiSendCommand(&CmdPkt)) {
        if (!AtapiReceiveData(&SgBuf, 1, &dwRet)) {
            DEBUGMSG( ZONE_ERROR|ZONE_CDROM, (TEXT("AtaGetSenseInfo Failed!!!\r\n")));
            return FALSE;
        }
    } else {
         return FALSE;
    }
    return (SenseInfo.cap.cdvdc_readbits & READCAPS_DVD);
}

void CPCIDiskAndCD::AtapiDumpSenseData()
{
    CD_SENSE_DATA SenseData;
    if (AtapiGetSenseInfo(&SenseData)) {
        DEBUGMSG( ZONE_CDROM | ZONE_ERROR, (TEXT("Sense Info\r\n")));
        DEBUGMSG( ZONE_CDROM | ZONE_ERROR, (TEXT("   Error Code = 0x%2.2X, Segment Number = %d, Sense Key = 0x%2.2X\r\n"), SenseData.sd_ErrCode, SenseData.sd_SegNum, SenseData.sd_ILI_Key));
        DEBUGMSG( ZONE_CDROM | ZONE_ERROR, (TEXT("   Information = 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\r\n"), SenseData.sd_Info[0], SenseData.sd_Info[1], SenseData.sd_Info[2], SenseData.sd_Info[3]));
        DEBUGMSG( ZONE_CDROM | ZONE_ERROR, (TEXT("   Additional Sense Length = %d\r\n"), SenseData.sd_Length));
        DEBUGMSG( ZONE_CDROM | ZONE_ERROR, (TEXT("   Command Specific Information = 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\r\n"),SenseData.sd_CmdInfo[0], SenseData.sd_CmdInfo[1], SenseData.sd_CmdInfo[2], SenseData.sd_CmdInfo[3]));
        DEBUGMSG( ZONE_CDROM | ZONE_ERROR, (TEXT("   ASC = 0x%2.2X, ASCQ = 0x%2.2X, FRUC = 0x%2.2X\r\n"), SenseData.sd_SenseCode, SenseData.sd_Qualifier, SenseData.sd_UnitCode));
        DEBUGMSG( ZONE_CDROM | ZONE_ERROR, (TEXT("   Sense Key Specfic = 0x%2.2X 0x%2.2X 0x%2.2X\r\n"), SenseData.sd_Key1, SenseData.sd_Key2, SenseData.sd_Key3));
    } else {
        DEBUGMSG( ZONE_CDROM | ZONE_ERROR, (TEXT(" Unable to get CD Sense info\r\n")));
    }
}

