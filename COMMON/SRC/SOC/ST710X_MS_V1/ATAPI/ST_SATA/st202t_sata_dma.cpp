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

#include <satamain.h>
#include <atapipci.h>
#include <diskmain.h>
#include <ceddk.h>
#include <pm.h>
#include <ST202T_sata.h>
#include "debug.h"

// ----------------------------------------------------------------------------
// Function: WaitForDMAInterrupt
//     Wait for an interrupt from the DMA controller or an error condition.
//
// Parameters:
//     dwTimeOut -
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::WaitForDMAInterrupt(
    DWORD dwTimeOut
    )
{
    BOOL fRet = TRUE;
    DWORD dwRet;

    static CONST HANDLE lpHandles[] = {m_pPort->m_hErrorEvent, m_pPort->m_hDMAEvent};
    static DWORD dwCount = 2;

    // Wait for the DMA block transaction to complete or an error to occur.
    dwRet = WaitForMultipleObjects(dwCount, lpHandles, FALSE, dwTimeOut);
    if (dwRet == WAIT_TIMEOUT)
        {
        fRet = FALSE;
        }
    else if (dwRet == WAIT_OBJECT_0)
        {
        // SATA error condition detected
        DEBUGMSG(ZONE_IO|ZONE_ERROR, (_T(
            "Atapi!CST202T_SATA::WaitForDMAInterrupt> SError detected during DMA transaction.\r\n"
            )));

        fRet = FALSE;
        }
    else // DMA controller is interrupting
        {
        if (DMAError() || m_pPort->m_bStatus & ATA_STATUS_ERROR)
            {
            DEBUGMSG(ZONE_IO|ZONE_ERROR, (_T(
                "Atapi!CST202T_SATA::WaitForDMAInterrupt> Error detected during DMA transaction.\r\n"
                )));
            fRet = FALSE;
            }
        }

    return fRet;
}


// ----------------------------------------------------------------------------
// Function: TranslateAddress
//     Translate a system address to a bus address for the DMA controller.
//
// Parameters:
//     pdwAddr -
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::TranslateAddress(
    PDWORD pdwAddr
    )
{
    // translate a system address to a bus address for the DMA bus controller

    PHYSICAL_ADDRESS SystemLogicalAddress, TransLogicalAddress;
    DWORD dwBus;

    dwBus = m_pPort->m_pController->m_dwi.dwBusNumber;

    // translate address
    SystemLogicalAddress.HighPart = 0;
    SystemLogicalAddress.LowPart = *pdwAddr;
    if (!HalTranslateSystemAddress(Internal, dwBus, SystemLogicalAddress, &TransLogicalAddress)) {
        return FALSE;
    }

    *pdwAddr = TransLogicalAddress.LowPart;

    return TRUE;
}


// ----------------------------------------------------------------------------
// Function: SetupDMA
//     Prepare DMA transfer.  Issues the first DMA transaction and
//     enables the DMA engine.  The DMA engine must be initialized
//     before sending the DMA command to the drive.
//
// Parameters:
//     pSgBuf -
//     dwSgCount -
//     fRead -
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::SetupDMA(
    PSG_BUF pSgBuf,
    DWORD dwSgCount,
    BOOL fRead
    )
{
    BOOL bRet = FALSE;

    if(BuildLLITables(pSgBuf, dwSgCount, fRead))
        {
        // Disable DMA channel
        EnableDMAChannel(FALSE);

        // Clear all interrupt status registers for this channel
        ClearDMAStatusRegs();

        // Ensure global DMA enable bit is set.
        EnableDMAController(TRUE);

        // Populate fields for first DMA block transfer
        m_pRegDMA->dwSAR = m_LLITable[0].SAR;
        m_pRegDMA->dwDAR = m_LLITable[0].DAR;
        m_pRegDMA->dwLLP = (DWORD)m_LLITablePhys.LowPart & 0x1FFFFFFF;  // DMA controller uses P0 region
        m_pRegDMA->dwCTL_LSB = m_LLITable[0].CTL_LSB;
        m_pRegDMA->dwCTL_MSB = m_LLITable[0].CTL_MSB;

        // The following values are constant throughout the DMA transaction.
        // Set them up once here.
        if (fRead)
            {
            m_pRegDMA->dwCFG_LSB = SATA_DMA_CFG_LSB_READ;
            m_pRegDMA->dwCFG_MSB = SATA_DMA_CFG_MSB_READ;

            m_pRegHBA->dwDBTSR = SATA_HBA_DBTSR_READ;
            m_pRegHBA->dwDMACR = SATA_HBA_DMACR_READ;

            }
        else
            {
            m_pRegDMA->dwCFG_LSB = SATA_DMA_CFG_LSB_WRITE;
            m_pRegDMA->dwCFG_MSB = SATA_DMA_CFG_MSB_WRITE;

            m_pRegHBA->dwDBTSR = SATA_HBA_DBTSR_WRITE;
            m_pRegHBA->dwDMACR = SATA_HBA_DMACR_WRITE;
            }

        bRet = TRUE;
        }

    return bRet;
}

// ----------------------------------------------------------------------------
// Function: BuildLLITables
//     Populate the "linked list item" table which is native to this
//     DMA controller.
//
// Parameters:
//     pSgBuf -
//     dwSgCount -
//     fRead -
// ----------------------------------------------------------------------------
BOOL
CST202T_SATA::BuildLLITables(
    PSG_BUF pSgBuf,
    DWORD dwSgCount,
    BOOL fRead
    )
{
    BOOL bRet = FALSE;
    int TableIndex = 0;
    pSATA_DMA_LLI  LLIPhys;

    // Build generic PRD tables
    if (CPCIDisk::SetupDMA(pSgBuf, dwSgCount, fRead))
        {
        // Convert generic PRD table into an LLI table.
        PDMATable pPRD = m_pPRD;
        USHORT currentFISDataCount = 0;
        BOOL bSplitDMADetected;

        do
            {
            if (fRead)
                {
                m_LLITable[TableIndex].SAR = 0; // sata1hostc_spec_1_4.pdf, section 3.4: start address should be 0
                m_LLITable[TableIndex].DAR = pPRD->physAddr & 0x1FFFFFFF;  // DMA controller uses P0 region

                m_LLITable[TableIndex].CTL_LSB = SATA_DMA_CTL_LSB_LINKED_READ;
                }
            else    // Write
                {
                m_LLITable[TableIndex].SAR = pPRD->physAddr & 0x1FFFFFFF;  // DMA controller uses P0 region
                m_LLITable[TableIndex].DAR = 0; // sata1hostc_spec_1_4.pdf, section 3.4: start address should be 0

                m_LLITable[TableIndex].CTL_LSB = SATA_DMA_CTL_LSB_LINKED_WRITE;
                }

            //Unknown if these fields will be used by hardware
            m_LLITable[TableIndex].SSTAT = 0;
            m_LLITable[TableIndex].DSTAT = 0;


            // The maximum data transferred to the SATA controller by the drive per
            // FIS is 8192 bytes.  We must ensure that no DMA block transfer passes
            // an 8192 byte FIS boundary, otherwise read underrun errors will occur
            // before the drive has time to transfer more data to the SATA controller.
            currentFISDataCount += pPRD->size;
            if (currentFISDataCount > SATA_MAX_FIS_SIZE)
                {
                DEBUGMSG(ZONE_IO || ZONE_WARNING, (_T(
                    "Atapi!CST202T_SATA::BuildLLITables: Breaking up DMA that spanned FISes!  \r\n"
                    )));

                // Break this DMA block up so that it does not span two data FISes
                USHORT dwBytesThisTransfer = pPRD->size - (currentFISDataCount % SATA_MAX_FIS_SIZE);

                // Size is given in DWORDs
                m_LLITable[TableIndex].CTL_MSB = dwBytesThisTransfer / 4;

                // Complete this PRD in the next LLI
                pPRD->size -= dwBytesThisTransfer;
                pPRD->physAddr += dwBytesThisTransfer;

                currentFISDataCount = 0;    // We know we're on a FIS boundary; reset the counter
                bSplitDMADetected = TRUE;
                }
            else
                {
                // Size is given in DWORDs
                m_LLITable[TableIndex].CTL_MSB = pPRD->size / 4;
                currentFISDataCount %= SATA_MAX_FIS_SIZE;
                bSplitDMADetected = FALSE;
                }

            LLIPhys = (pSATA_DMA_LLI) (((DWORD)m_LLITablePhys.LowPart) & 0x1FFFFFFF);  // DMA controller uses P0 region
            m_LLITable[TableIndex].LLP = &LLIPhys[TableIndex+1];
            TableIndex++;
            } while (bSplitDMADetected || (pPRD++)->EOTpad == 0); //Increment pPRD only if split DMA was not detected

        m_LLITable[TableIndex-1].LLP = 0;
#if 0
        DEBUGMSG(ZONE_IO || ZONE_WARNING, (_T(
            "Atapi!CST202T_SATA::BuildLLITables: tableDump:  \r\n"
            )));

        int i =0;
        do
            {
                DEBUGMSG(ZONE_IO || ZONE_WARNING, (_T(
                    "table %d: SAR = 0x%x, DAR=0x%x, LLP = 0x%x, CTL_MSB = 0x%x  \r\n"
                    ), i, m_LLITable[i].SAR, m_LLITable[i].DAR, m_LLITable[i].LLP, m_LLITable[i].CTL_MSB));
            } while (m_LLITable[i].LLP && ++i);
#endif
        bRet = TRUE;
        }

    return bRet;
}

// ----------------------------------------------------------------------------
// Function: StartDMATransaction
//     Program the DMA controller and initiate a block transaction.
//
// Parameters:
//     pSgBuf -
//     dwSgCount -
//     fRead -
// ----------------------------------------------------------------------------
void CST202T_SATA::StartDMATransaction(
    BOOL fRead, const pSATA_DMA_LLI pLLITable
    )
{
    CacheSync(CACHE_SYNC_DISCARD);

    // Enable Block Transfer done interrupt for this channel
    EnableDMACTfrInt(TRUE);

    // Initiate transfer
    EnableDMAChannel(TRUE);

    return;
}

// ----------------------------------------------------------------------------
// Function: ProcessDMA
//     This function processes the entire DMA transaction, which possibly
//     involves multiple DMA block transactions.
//
// Parameters:
//     fRead -
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::ProcessDMA(
    BOOL fRead
    )
{
    BOOL bRet = TRUE;

    // Wait for transfer to complete.
    if ( !WaitForDMAInterrupt(m_dwDiskIoTimeOut) )
        {
        DEBUGMSG(ZONE_IO || ZONE_WARNING, (_T(
            "Atapi!CST202T_SATA::ProcessDMA> Failed to wait for interrupt or error received\r\n"
            )));
        bRet = FALSE;
        }

    // Wait for AHB bus to go idle (per instructions from ST)
    WaitForAHBIdle();

    // DMA transfer is complete
    EnableDMACTfrInt(FALSE);

    return bRet;

}

// ----------------------------------------------------------------------------
// Function: EndDMA
//     End DMA transfer
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::EndDMA(
    )
{

    DWORD ErrorStatus;
    BOOL bRet = FALSE;

    // Wait for AHB bus to go idle (per instructions from ST)
    WaitForAHBIdle();

    ErrorStatus = GetDMAErrorStatus();

    if (ErrorStatus != 0)
        {
        DEBUGMSG(ZONE_ERROR|ZONE_DMA, (_T(
            "Atapi!CST202T_SATA::EndDMA> Error\r\n"
            )));
        }
    else
        {
        bRet = TRUE;
        }

    m_pRegHBA->dwDMACR = 0;
    EnableDMAChannel(FALSE);

    return bRet;

}

// ----------------------------------------------------------------------------
// Function: AbortDMA
//     Abort DMA transfer
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::AbortDMA(
    )
{
    DWORD i;

    // Reset DMA controller's state
    m_pRegHBA->dwDMACR = 0;
    EnableDMACTfrInt(FALSE);
    EnableDMAChannel(FALSE);
    ClearDMAStatusRegs();

    for (i = 0; i < m_dwSGCount; i++) {
        if (!m_pSGCopy[i].pDstAddress) {
            UnlockPages(m_pSGCopy[i].pSrcAddress, m_pSGCopy[i].dwSize);
        }
    }

    // free all but the first @MIN_PHYS_PAGES pages; these are fixed
    for (i = MIN_PHYS_PAGES; i < m_dwPhysCount; i++) {
        FreePhysMem(m_pPhysList[i].pVirtualAddress);
    }

    return FALSE;
}

// ----------------------------------------------------------------------------
// Function: ReadWriteDiskDMA
//     This function reads from/writes to an ATA device
//
// Parameters:
//     pIOReq -
//     fRead -
// ----------------------------------------------------------------------------

DWORD
CST202T_SATA::ReadWriteDiskDMA(
    PIOREQ pIOReq,
    BOOL fRead
    )
{
    DWORD dwError;
    BOOL bRetry;

    // Certain SATA errors are non-recoverable by the hardware but are transient in nature.
    // Fetch the number of times we should retry a command when an error is detected.
    DWORD dwRetriesRemaining = m_pPort->m_pController->m_pIdeReg->dwNumRetriesOnSATAError;

Begin_DMA:
    bRetry = FALSE;

    // Perform DMA Transaction
    dwError = DoReadWriteDiskDMA(pIOReq, fRead);

    // We must check for SATA error conditions here to ensure that
    // we haven't read or written invalid data.
    switch (GetSATAError())
        {
        case SATA_ERROR_NO_ERROR:
            // Check that the command completed successfully
            if ( (ERROR_READ_FAULT == dwError) || (ERROR_WRITE_FAULT == dwError) )
                {
                m_dwErrorCount++;
                bRetry = TRUE;
                }
            break;
        case SATA_ERROR_PERSISTENT:
            RETAILMSG(ZONE_IO|ZONE_ERROR, (_T(
                "Atapi!CST202T_SATA::ReadWriteDiskDMA> Unrecoverable error detected.  ABORT!\r\n"
                )));

            m_dwErrorCount++;
            dwError = ERROR_GEN_FAILURE;
            break;
        case SATA_ERROR_TRANSIENT:
            m_dwErrorCount++;
            bRetry = TRUE;
            break;
        }

    if (bRetry)
        {
        if (dwRetriesRemaining--)
            {
            DEBUGMSG(ZONE_IO|ZONE_ERROR, (_T(
                "Atapi!CST202T_SATA::ReadWriteDiskDMA> Error detected. Resetting controller and retrying command. Error Count = %d\r\n"
                ), m_dwErrorCount));

            ResetController(FALSE);

            goto Begin_DMA;
            }
        else
            {
            RETAILMSG(ZONE_IO|ZONE_ERROR, (_T(
                "Atapi!CST202T_SATA::ReadWriteDiskDMA> Error detected. %d retries failed. ABORT! Error Count = %d\r\n"
                ), m_pPort->m_pController->m_pIdeReg->dwNumRetriesOnSATAError, m_dwErrorCount));

            dwError = ERROR_GEN_FAILURE;
            }
        }

    return dwError;

}

// ----------------------------------------------------------------------------
// Function: DoReadWriteDiskDMA
//     This function reads from/writes to a SATA device.  This is largely
//     taken from CDisk::ReadWriteDiskDMA.  Some modifications have been made
//     for the DMA controller on this platform.
//
// Parameters:
//     pIOReq -
//     fRead -
// ----------------------------------------------------------------------------

DWORD
CST202T_SATA::DoReadWriteDiskDMA(
    PIOREQ pIOReq,
    BOOL fRead
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSG_REQ pSgReq = (PSG_REQ)pIOReq->pInBuf;
    DWORD dwSectorsToTransfer;
    SG_BUF CurBuffer[MAX_SG_BUF];
    BYTE bCmd;
    BOOL bErrorDetected = FALSE;

    // pIOReq->pInBuf is a sterile copy of the caller's SG_REQ

    if ((pSgReq == NULL) || (pIOReq->dwInBufSize < sizeof(SG_REQ)))
        {
        return ERROR_INVALID_PARAMETER;
        }
    if ((pSgReq->sr_num_sec == 0) || (pSgReq->sr_num_sg == 0))
        {
        return  ERROR_INVALID_PARAMETER;
        }
    if ((pSgReq->sr_start + pSgReq->sr_num_sec) > m_DiskInfo.di_total_sectors)
        {
        return ERROR_SECTOR_NOT_FOUND;
        }

    DEBUGMSG(ZONE_IO, (_T(
        "Atapi!CST202T_SATA::DoReadWriteDiskDMA> sr_start(%ld), sr_num_sec(%ld), sr_num_sg(%ld)\r\n"
        ), pSgReq->sr_start, pSgReq->sr_num_sec, pSgReq->sr_num_sg));

    // We are not interested in the ATA interrupt during DMA transactions.
    // It will normally fire at the end of a DMA transaction, but we
    // rely on the DMA controller's block transfer done interrupt to
    // inform us that the transfer is complete.  Disable the ATA interrupt
    // here, and re-enable it at the end of the DMA transaction.
    WriteAltDriveController(ATA_CTRL_DISABLE_INTR);

    // clear pending interrupts
    GetBaseStatus();
    ClearSError();
    ResetEvent(m_pPort->m_hSATAEvent);
    ResetEvent(m_pPort->m_hDMAEvent);
    ResetEvent(m_pPort->m_hErrorEvent);



    DWORD dwStartBufferNum = 0, dwEndBufferNum = 0, dwEndBufferOffset = 0;
    DWORD dwNumSectors = pSgReq->sr_num_sec;
    DWORD dwStartSector = pSgReq->sr_start;

    // process scatter/gather buffers in groups of MAX_SEC_PER_COMMAND sectors
    // each DMA request handles a new SG_BUF array which is a subset of the
    // original request, and may start/stop in the middle of the original buffer

    while (dwNumSectors)
        {

        // determine number of sectors to transfer
        dwSectorsToTransfer = (dwNumSectors > MAX_SECT_PER_COMMAND) ? MAX_SECT_PER_COMMAND : dwNumSectors;

        // determine size (in bytes) of transfer
        DWORD dwBufferLeft = dwSectorsToTransfer * BYTES_PER_SECTOR;

        DWORD dwNumSg = 0;

        // while the transfer is not complete
        while (dwBufferLeft)
            {
            // determine the size of the current scatter/gather buffer
            DWORD dwCurBufferLen = pSgReq->sr_sglist[dwEndBufferNum].sb_len - dwEndBufferOffset;

            if (dwBufferLeft < dwCurBufferLen)
                {
                CurBuffer[dwEndBufferNum - dwStartBufferNum].sb_buf = pSgReq->sr_sglist[dwEndBufferNum].sb_buf + dwEndBufferOffset;
                CurBuffer[dwEndBufferNum - dwStartBufferNum].sb_len = dwBufferLeft;
                dwEndBufferOffset += dwBufferLeft;
                dwBufferLeft = 0;
                }
            else
                {
                CurBuffer[dwEndBufferNum - dwStartBufferNum].sb_buf = pSgReq->sr_sglist[dwEndBufferNum].sb_buf + dwEndBufferOffset;
                CurBuffer[dwEndBufferNum - dwStartBufferNum].sb_len = dwCurBufferLen;
                dwEndBufferOffset = 0;
                dwEndBufferNum++;
                dwBufferLeft -= dwCurBufferLen;
                }
            dwNumSg++;
            }

        bCmd = fRead ? m_bDMAReadCommand : m_bDMAWriteCommand;

        // setup the DMA transfer
        if (!SetupDMA(CurBuffer, dwNumSg, fRead))
            {
            dwError = fRead ? ERROR_READ_FAULT : ERROR_WRITE_FAULT;
            // We do not set bErrorDetected to true here, as the DMA
            // transfer was not initialized, and thus shouldn't be aborted.
            goto Exit;
            }

        // Wait for AHB bus to go idle (per instructions from ST)
        WaitForAHBIdle();

        // Begin the DMA transfer
        StartDMATransaction(fRead, m_LLITable);

        // write the read/write command
        if (!SendIOCommand(dwStartSector, dwSectorsToTransfer, bCmd))
            {
            bErrorDetected = TRUE;
            goto Exit;
            }

        // start the DMA transfer
        if (ProcessDMA(fRead))
            {
            if (CheckIntrState() == ATA_INTR_ERROR)
                {
                    DEBUGMSG(ZONE_IO || ZONE_WARNING, (_T(
                        "Atapi!CST202T_SATA::DoReadWriteDiskDMA> Failed to wait for interrupt; device(%d)\r\n"
                        ), m_dwDeviceId));
                    bErrorDetected = TRUE;
                    goto Exit;
                }
            // stop the DMA transfer
            if (EndDMA())
                {
                if (WaitOnBusy(FALSE) != 0)
                    {
                    DEBUGMSG(ZONE_IO || ZONE_WARNING, (_T(
                        "Atapi!CST202T_SATA::DoReadWriteDiskDMA> Drive won't come out of busy state!\r\n"
                        )));
                    bErrorDetected = TRUE;
                    goto Exit;
                    }
                else
                    {
                    CompleteDMA(CurBuffer, pSgReq->sr_num_sg, fRead);
                    }
                }
            else // EndDMA returned an error condition
                {
                bErrorDetected = TRUE;
                goto Exit;
                }
            }
        else // ProcessDMA returned an error condition
            {
            bErrorDetected = TRUE;
            goto Exit;
            }

        // update transfer
        dwStartSector += dwSectorsToTransfer;
        dwStartBufferNum = dwEndBufferNum;
        dwNumSectors -= dwSectorsToTransfer;
        }

Exit:
    if (bErrorDetected)
        {
        dwError = fRead ? ERROR_READ_FAULT : ERROR_WRITE_FAULT;
        AbortDMA();
        }

    // ATA interrupt was disabled for the duration of the DMA
    // tranaction.  Re-enable it now.
    GetBaseStatus(); // Clear any pending interrupt.
    WriteAltDriveController(ATA_CTRL_ENABLE_INTR);

    pSgReq->sr_status = dwError;
    return dwError;
}

