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

EXTERN_C
CDisk *
CreateST202TSata(HKEY hDevKey)
{
    DEBUGMSG(ZONE_INIT,(TEXT("Atapi!CreateST202TSata (built %s %s)\r\n"),TEXT(__DATE__),TEXT(__TIME__) ));
    return new CST202T_SATA(hDevKey);
}

// ----------------------------------------------------------------------------
// Function: CST202T_SATA
//     Constructor
//
// Parameters:
//     hKey -
// ----------------------------------------------------------------------------
CST202T_SATA::CST202T_SATA(HKEY hKey) : CPCIDiskAndCD(hKey)
{
    m_dwErrorCount = 0;
    m_fUseLBA48 = FALSE;

    m_LLITable = NULL;
    m_LLITablePhys.QuadPart = NULL;

    m_pRegDMA = NULL;
    m_pRegSATA = NULL;
    m_pRegHBA = NULL;
}

// ----------------------------------------------------------------------------
// Function: CST202T_SATA
//     Destructor
//
// Parameters:
//     hKey -
// ----------------------------------------------------------------------------

CST202T_SATA::~CST202T_SATA(
    )
{
    // Disable the SATA error interrupt.
    EnableSErrorInt(FALSE);

    // Disable DMA interrupt
    EnableDMACTfrInt(FALSE);

    // Disable ATA interrupt
    WriteAltDriveController(ATA_CTRL_DISABLE_INTR);

    // Disable DMA channel
    EnableDMAChannel(FALSE);

    // Disable DMA controller
    EnableDMAController(FALSE);

    if (m_LLITable) {
        HalFreeCommonBuffer(NULL, NULL, m_LLITablePhys, m_LLITable, NULL);
        m_LLITable = NULL;
        m_LLITablePhys.QuadPart = NULL;
    }
}

// ----------------------------------------------------------------------------
// Function: InterruptThreadStub
 //     SATA IST stub (needed so that InterruptThread may be made static)
//
// Parameters:
//     lParam -
// ----------------------------------------------------------------------------

DWORD
CST202T_SATA::InterruptThreadStub(
    IN PVOID lParam
    )
{
    return ((CST202T_SATA *)lParam)->InterruptThread(lParam);
}

// ----------------------------------------------------------------------------
// Function: InterruptThread
 //     SATA IST
//
// Parameters:
//     lParam -
// ----------------------------------------------------------------------------

DWORD CST202T_SATA::InterruptThread(
    IN PVOID lParam
    )
{
    CST202T_SATA        *pDisk = (CST202T_SATA *)lParam;

    while(TRUE) {
        WaitForSingleObject(pDisk->m_pPort->m_hIRQEvent, INFINITE);

        // We have three interrupts of interest: the ATA interrupt, the
        // DMA block transfer complete interrupt, and the SATA error
        // interrupt.
        //
        // NOTE: We have no indicatior of the ATA interrupt being active.
        //       Thus, we must assume the ATA interrupt is the interrupt
        //       being triggered if none of the other interrupts is
        //       detected.  Note that some versions of the SATA host
        //       document provided by ST describe bit 31 of INTPR
        //       as the "IPF" bit, which would indicate the state
        //       of the ATA interrupt.  However, this bit is not
        //       present in this part.
        ////////////////////////////////////////////////////////////////


        // First, determine if a SATA error condition has been detected.
        if (IsSErrorIntActive())
            {
            // Simply acknowledge the interrupt here to unblock any pending
            // read or write operation.  The GetSATAError function will
            // handle the SATA error at the appropriate time and re-enable
            // the interrupt.
            EnableSErrorInt(FALSE);

            // Get ATA status and clear pending ATA interrupt condition.
            pDisk->m_pPort->m_bStatus = GetBaseStatus();

//            DEBUGMSG(ZONE_IO, (_T(
//                "Atapi!CST202T_SATA::InterruptThread> Got SError Interrupt!\r\n"
//                )));

            SetEvent(pDisk->m_pPort->m_hErrorEvent);
            }
        else if (IsDMACTfrIntActive())
            {
                // If we're operating in DMA mode, the DMA controller may have
                // generated the interrupt. The only DMA interrupt we are interested
                // in is the transfer complete interrupt.  ACK it here.
                ClearDMACTfrInt();

                // Get ATA status and clear pending ATA interrupt condition.
                pDisk->m_pPort->m_bStatus = GetBaseStatus();

//                DEBUGMSG(ZONE_IO, (_T(
//                    "Atapi!CST202T_SATA::InterruptThread> Got DMA Interrupt!\r\n"
//                    )));

                SetEvent(pDisk->m_pPort->m_hDMAEvent);
            }
        else // ATA interrupt
            {
            // SATA interrupt condition.
            pDisk->m_pPort->m_bStatus = GetBaseStatus();

//            DEBUGMSG(ZONE_IO, (_T(
//                "Atapi!CST202T_SATA::InterruptThread> Got SATA Interrupt!\r\n"
//                )));

            SetEvent(pDisk->m_pPort->m_hSATAEvent);
            }

        InterruptDone(pDisk->m_pPort->m_dwSysIntr);
    }

    return(0);
}

// ----------------------------------------------------------------------------
// Function: WaitForInterrupt
//     Wait for a SATA interrupt or error condition.
//
// Parameters:
//     dwTimeOut -
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::WaitForInterrupt(
    DWORD dwTimeOut
    )
{
    BOOL fRet = TRUE;
    DWORD dwRet;

    static CONST HANDLE lpHandles[] = {m_pPort->m_hErrorEvent, m_pPort->m_hSATAEvent};
    static DWORD dwCount = 2;

    // Wait for the ATA interrupt or an error to occur.
    dwRet = WaitForMultipleObjects(dwCount, lpHandles, FALSE, dwTimeOut);
    if (dwRet == WAIT_FAILED)
        {
        if (!WaitForDisc(WAIT_TYPE_DRQ, dwTimeOut, 10))
            {
            fRet = FALSE;
            }
        }
    else if (dwRet == WAIT_TIMEOUT)
        {
        fRet = FALSE;
        }
    else if (dwRet == WAIT_OBJECT_0)
        {
        // SATA error condition detected
        DEBUGMSG(ZONE_IO|ZONE_ERROR, (_T(
            "Atapi!CST202T_SATA::WaitForInterrupt> SError detected.\r\n"
            )));

        fRet = FALSE;
        }
    else // ATA interrupt detected
        {
        if (m_pPort->m_bStatus & ATA_STATUS_ERROR)
            {
            DEBUGMSG(ZONE_IO|ZONE_ERROR, (_T(
                "Atapi!CST202T_SATA::WaitForInterrupt> Error detected.  Error = 0x%x\r\n"
                ), GetError()));

            fRet = FALSE;
            }
        }

    return fRet;
}

// ----------------------------------------------------------------------------
// Function: Init
//     Initialize channel
//
// Parameters:
//     hActiveKey -
// ----------------------------------------------------------------------------
BOOL CST202T_SATA::Init(HKEY hActiveKey)
{
    BOOL bRet = FALSE;

    DEBUGMSG(ZONE_INIT,(TEXT("+Atapi!CST202T_SATA::Init\r\n")));
    DEBUGMSG(ZONE_WARNING,(TEXT("+Atapi!CST202T_SATA::Init> Compiled for STB7109 CPU cut 2 or later (16-bit IO on CDR0)\r\n")));

    // Save SYSINTR value for Init
    m_pPort->m_dwSysIntr = m_pPort->m_pController->m_dwSysIntr;

    if(CPCIDisk::Init(hActiveKey))
        {
        ConfigLBA48();  // Check if the drive supports 48-bit LBA and configure the port appropriately
        DEBUGMSG(ZONE_INIT,(TEXT("Atapi!CST202T_SATA::Init> Device %d sucessfully initialized.  \r\n"), m_dwDeviceId));

        bRet = TRUE;
        }
    else
        {
        RETAILMSG(ZONE_INIT,(TEXT("Atapi!CST202T_SATA::Init> Device %d does not exist or did not come out of reset properly.  Skipping.  \r\n"), m_dwDeviceId));
        }

    DEBUGMSG(ZONE_INIT,(TEXT("-Atapi!CST202T_SATA::Init\r\n")));
    return bRet;
}

// ----------------------------------------------------------------------------
// Function: SetWriteCacheMode
//     Issue SET FEATURES enable write cache command.  We override the
//     CDisk version of this function since the additional calls to
//     SelectDevice() after issuing the command were causing errors on this
//     platform (we can't write to CDR6, which is the Device/Head register,
//     while the device is still busy).
//
// Parameters:
//     fEnable -
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::SetWriteCacheMode(
    BOOL fEnable
    )
{
    BYTE bError, bStatus;

    // select device
    SelectDevice();

    // wait for device to acknowledge selection
    WaitForDisc(WAIT_TYPE_NOT_BUSY, 100);
    WaitForDisc(WAIT_TYPE_READY, 1000);
    WaitOnBusy(TRUE);

    // write command
    WriteFeature(fEnable ? ATA_ENABLE_WRITECACHE : ATA_DISABLE_WRITECACHE);
    WriteCommand(ATAPI_CMD_SET_FEATURES);

    // wait for device to respond to command
    WaitOnBusy(TRUE);
    WaitForDisc(WAIT_TYPE_NOT_BUSY, 200);

    // check response
    bStatus = GetBaseStatus();
    bError = GetError();
    if ((bStatus & ATA_STATUS_ERROR) && (bError & ATA_ERROR_ABORTED)) {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::SetWriteCacheMode> Failed to enable write cache; status(%02X), error(%02X)\r\n"
            ), bStatus, bError));
        ResetController(FALSE);
        return FALSE;
    }

    return TRUE;
}

// ----------------------------------------------------------------------------
// Function: SetLookAhead
//     Issue SET FEATURES enable read look-ahead command.  We override the
//     CDisk version of this function since the additional calls to
//     SelectDevice() after issuing the command were causing errors on this
//     platform (we can't write to CDR6, which is the Device/Head register,
//     while the device is still busy).
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::SetLookAhead(
    )
{
    BYTE bError = 0;
    BYTE bStatus = 0;

    // select device
    SelectDevice();

    // wait for device to acknowledge selection
    WaitForDisc(WAIT_TYPE_NOT_BUSY, 100);
    WaitForDisc(WAIT_TYPE_READY, 1000);
    WaitOnBusy(TRUE);

    // write command
    WriteFeature(ATA_ENABLE_LOOKAHEAD);
    WriteCommand(ATAPI_CMD_SET_FEATURES);

    // wait for device to respond to command
    WaitOnBusy(TRUE);
    WaitForDisc(WAIT_TYPE_NOT_BUSY, 200);

    // check response
    bStatus = GetBaseStatus();
    bError = GetError();
    if ((bStatus & ATA_STATUS_ERROR) && (bError & ATA_ERROR_ABORTED)) {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::SetLookAhead> Failed to enable read look-ahead; status(%02X), error(%02X)\r\n"
            ), bStatus, bError));
        ResetController(FALSE);
        return FALSE;
    }

    return TRUE;
}


// ----------------------------------------------------------------------------
// Function: ConfigureRegisterBlock
//     This function is called by DSK_Init before any other CDisk function to
//     set up the register block.
//
// Parameters:
//     dwStride -
// ----------------------------------------------------------------------------

VOID
CST202T_SATA::ConfigureRegisterBlock(
    DWORD dwStride
    )
{
    // Standard ATA registers
    m_dwStride = dwStride;  // Unused; this is 0 in this driver
    m_dwDataDrvCtrlOffset = SATA_REG_DATA;
    m_dwFeatureErrorOffset = SATA_REG_ERROR;
    m_dwSectCntReasonOffset = SATA_REG_SECT_CNT;
    m_dwSectNumOffset = SATA_REG_SECT_NUM;
    m_dwDrvHeadOffset = SATA_REG_DEV_HEAD;
    m_dwCommandStatusOffset = SATA_REG_COMMAND;
    m_dwByteCountLowOffset = SATA_REG_CYL_LOW;
    m_dwByteCountHighOffset = SATA_REG_CYL_HIGH;
    m_dwAltStatusOffset =  SATA_REG_ALT_STATUS;
    m_dwAltDrvCtrl = SATA_REG_ALT_STATUS;

    // Configure pointers to register regions
    m_pPort->m_dwBMR = m_pPort->m_pController->m_dwControllerBase + SATA_DMA_BASE_OFFSET;
    m_pPort->m_dwRegBase = m_pPort->m_pController->m_dwControllerBase + SATA_HBA_BASE_OFFSET;
    m_pPort->m_dwRegAlt = m_pPort->m_dwRegBase + SATA_REG_ALT_STATUS;

    // Register access macros use this
    m_pATAReg = (PBYTE)m_pPort->m_dwRegBase;
    // SATA registers
    m_pRegSATA = (pSATA_Regs)(m_pPort->m_dwRegBase + SATA_REG_SSTATUS);
    // DMA control registers for this channel
    m_pRegDMA = (pSATA_DMAChannel)m_pPort->m_dwBMR;
    // Host control registers for this channel
    m_pRegHBA = (pSATA_HostController)(m_pPort->m_dwRegBase + SATA_REG_FPTAGR);
    // ARM Host Bus <->ST Bus Protocol Converter registers
    m_pRegAHB2STB = (pSATA_AHB2STB)(m_pPort->m_pController->m_dwControllerBase + SATA_AHB2STB_BASE_OFFSET);

}

// ----------------------------------------------------------------------------
// Function: ConfigPort
//     Setup SATA interrupt and do initial channel configuration.
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::ConfigPort(
    )
{
    BOOL bRet = FALSE;

    DMA_ADAPTER_OBJECT Adapter;

    Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
    Adapter.InterfaceType = (INTERFACE_TYPE)m_pPort->m_pController->m_dwi.dwInterfaceType;
    Adapter.BusNumber = m_pPort->m_pController->m_dwi.dwBusNumber;

    if (!m_LLITable)
    {
        m_LLITable = (pSATA_DMA_LLI)HalAllocateCommonBuffer(&Adapter, UserKInfo[KINX_PAGESIZE], &m_LLITablePhys, FALSE);
    }

    // Setup the interrupt here and create the controller's IST.
    if (m_LLITable && CPCIDisk::ConfigPort())
        {
        HANDLE  hThread;

        // Create IST
        hThread = ::CreateThread(NULL, 0, InterruptThreadStub, this, 0, NULL);

        // Set IST thread priority if it is specified in registry
        DWORD dwThreadPri;
        if (AtaGetRegistryValue (m_hDevKey, L"IstPriority256", &dwThreadPri))
            {
            ::CeSetThreadPriority (hThread, dwThreadPri);
            }

        CloseHandle(hThread);

        bRet = TRUE;
        }

    return bRet;
}

// ----------------------------------------------------------------------------
// Function: ConfigLBA48
//     This is a helper function which is called after the IDENTIFY_DEVICE
//     command has been successfully executed.  It parses the results
//     of the IDENTIFY_DEVICE command to determine if 48-bit LBA is supported
//     by the device.
//
// Parameters:
//     None
// ----------------------------------------------------------------------------
void CST202T_SATA::ConfigLBA48(void)
{
    PIDENTIFY_DATA6 pId = (PIDENTIFY_DATA6)&m_Id;

    // Word 87 (CommandSetFeatureDefault):
    //         bit 14 is set and bit 15 is cleared if config data
    //         in word 86 (CommandSetFeatureEnabled2) is valid.
    // Note that this is only valid for non-ATAPI devices
    if ( !IsAtapiDevice() &&
         (pId->CommandSetFeatureDefault & (1 << 14)) &&
         !(pId->CommandSetFeatureDefault & (1 << 15)) &&
         (pId->CommandSetFeatureEnabled2 & (1 << 10)) )
        {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CST202T_SATA::ConfigLBA48> Device supports 48-bit LBA\r\n")));
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CST202T_SATA::ConfigLBA48> Max LBA Address = 0x%08x%08x"), pId->lMaxLBAAddress[1], pId->lMaxLBAAddress[0]));

        m_fUseLBA48 = TRUE;

        // The CE file system currently supports a maximum of 32-bit sector addresses,
        // so we only use the lower DWORD of lMaxLBAAdress.
        m_DiskInfo.di_total_sectors = pId->lMaxLBAAddress[0];

        // CDisk::Identify has determined whether or not the device supports multi-sector transfers
        // Update read/write command to use [READ|WRITE] [SECTORS|MULTIPLE] EXT
        if (m_bReadCommand == ATA_CMD_READ)
            {
            m_bReadCommand = ATA_CMD_READ_SECTOR_EXT;
            m_bWriteCommand = ATA_CMD_WRITE_SECTOR_EXT;
            }
        else // CDisk::Identify has determined that the devce supports multi-sector transfers
            {
            m_bReadCommand = ATA_CMD_READ_MULTIPLE_EXT;
            m_bWriteCommand = ATA_CMD_WRITE_MULTIPLE_EXT;
            }

        m_bDMAReadCommand = ATA_CMD_READ_DMA_EXT;
        m_bDMAWriteCommand = ATA_CMD_WRITE_DMA_EXT;
        }
}

// ----------------------------------------------------------------------------
// Function: ToggleOOB
//     This function triggers a hardware reset of the SATA port.
//
// ----------------------------------------------------------------------------
VOID CST202T_SATA::ToggleOOB(DWORD dwSpeedLimit)
{
    // Write 0,1,0 to SControl's DET field on this port to trigger a hard reset of the device
    m_pRegSATA->dwSControl = SATA_SCONTROL_DET_DEASSERT_INITCOM | dwSpeedLimit;
    Sleep(2);
    m_pRegSATA->dwSControl = SATA_SCONTROL_DET_ASSERT_INITCOM | dwSpeedLimit;
    Sleep(2);
    m_pRegSATA->dwSControl = SATA_SCONTROL_DET_DEASSERT_INITCOM | dwSpeedLimit;

    // Wait at least 10ms for the PHY to detect presence of an attached device. (SATA 1.0A, section 5.2)
    Sleep(10);

}

// ----------------------------------------------------------------------------
// Function: ResetController
//     Implement ATA/ATAPI-6 R3B 9.2 (Software reset protocol).  We override
//     the common implementation of this function in its entirety due to
//     differences in the way error detection is done after SRST has been
//     asserted.
//
// Parameters:
//     bSoftReset -
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::ResetController(
    BOOL bSoftReset // ignore
    )
{
    DWORD dwAttempts = 0;
    DWORD dwStatus = 0;
    BYTE bStatus = 0;
    BOOL fRet = FALSE;

    ToggleOOB(SATA_SCONTROL_SPD_1_5_SPEEDLIMIT);  // This part supports generation 1 speeds only.

    // SError will reflect several transient error conditions resulting from
    // the reset sequence.  Clear them now.
    m_pRegSATA->dwSError = m_pRegSATA->dwSError;

    // we have to negate the RESET signal for 5 microseconds before we assert it

    WriteAltDriveController(0x00);
    Sleep(2);
    ::StallExecution(25);

    // Set_SRST
    // --------
    // to enter Set_SRST state, set SRST in the Device Control register to one;
    // this will assert the RESET signal and reset both devices on the current
    // channel

    WriteAltDriveController(0x04); // 0x04 == SRST

    // remain in this state for at least 5 microseconds; i.e., assert RESET signal
    // for at least 5 microseconds
    // if this is a hardware reset, then assert RESET signal for at least 25
    // microseconds
    Sleep(2);
    ::StallExecution(25); // this should be CEDDK implementation

    // Clear_wait
    // ----------
    // clear SRST in the Device Control register, i.e., negate RESET signal

    WriteAltDriveController(0x00);

    // remain in this state for at least 2 milliseconds

    Sleep(5);

HSR2_Check_status:;

    // Check_status
    // ------------
    // read the Status or Alternate Status register
    // if BSY is set to one, then re-enter this state
    // if BSY is cleared to zero, check the ending status in the Error register
    // and the signature (9.12) and transition to Host_Idle

    bStatus = GetAltStatus(); // read Status register
    if (bStatus & 0x80) {
        // BSY is set to one, re-enter this state
        if ( !((m_pPort->m_pController->m_pIdeReg->dwSoftResetTimeout - dwAttempts) % 100) )
            {
            DEBUGMSG(ZONE_INIT, (TEXT(
                "Atapi!CDisk::ResetController> Device is busy; %u seconds remaining\r\n"
                ), ((m_pPort->m_pController->m_pIdeReg->dwSoftResetTimeout - dwAttempts)/100)));
            }

        Sleep(10);
        dwAttempts += 1;
        // a device has at most 31 seconds to complete a software reset; we'll use 3 seconds
        if (dwAttempts == m_pPort->m_pController->m_pIdeReg->dwSoftResetTimeout) {
            DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::ResetController> Timeout\r\n")));
            goto exit;
        }
        goto HSR2_Check_status;
    }
    DEBUGMSG(ZONE_INIT, (TEXT(
        "Atapi!CDisk::ResetController> Device is ready\r\n"
        )));

    // BSY is cleared to zero, check the ending status in the Error register
    // and the signature
    // TODO: Check the signature (9.12)

    // From SATA 1.0A, seciton 9.1, 9.3, and 9.4:
    // Bit 0 of Error register is set to 1 if the reset succeeded.
    BYTE bError;
    bError = GetError();        // read Error register
    bStatus = GetAltStatus();   // read Status register
    if (!(bError & 0x01)) {
        DEBUGMSG(ZONE_INIT, (TEXT(
            "Atapi!CDisk::ResetController> SRST failed.  Error = 0x%x, Status = 0x%x\r\n"
            ), bError, bStatus));
        // TODO: Recover from error
        goto exit;
    }

    fRet = TRUE;

exit:;
    // SError will reflect several transient error conditions resulting from
    // the reset sequence.  Clear them now.
    m_pRegSATA->dwSError = m_pRegSATA->dwSError;

    // Clear any pending interrupt
    bStatus = GetBaseStatus();

    // Enable the SATA error interrupt (this is cleared by the COMRESET sequence).
    EnableSErrorInt(TRUE);

    return fRet;
}

// ----------------------------------------------------------------------------
// Function: SendIOCommand
//     Issue I/O command.  This function supports 48-bit LBA.
//
// Parameters:
//     pId -
//     dwNumberOfSectors -
//     bCmd -
// ----------------------------------------------------------------------------

BOOL
CST202T_SATA::SendIOCommand(
    DWORD dwStartSector,
    DWORD dwNumberOfSectors,
    BYTE bCmd
    )
{
    DEBUGMSG(ZONE_IO, (TEXT(
        "Atapi!CST202T_SATA::SendIOCommand> sector(%d), sectors left(%x), command(%x)\r\n"
        ), dwStartSector,dwNumberOfSectors,bCmd));

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_IOCOMMAND, &bCmd, sizeof(bCmd), 0, CELZONE_ALWAYSON, 0, FALSE);

    SelectDevice();

    if (WaitOnBusy(FALSE)) {
        DEBUGMSG(ZONE_IO, (TEXT(
            "Atapi!CST202T_SATA::SendIOCommand> Failed to send command; status(%x), error(%x)\r\n"
            ), GetAltStatus(),GetError()));
        return FALSE;
    }

    if (m_fUseLBA48)
        {
        // to transfer 65536 sectors, set number of sectors to 0
        if (dwNumberOfSectors == MAX_SECT_PER_EXT_COMMAND)
            {
            dwNumberOfSectors = 0;
            }

        WriteSectorCount((BYTE)(dwNumberOfSectors >> 8));   // Sector Count 15:8
        WriteSectorCount((BYTE)(dwNumberOfSectors));        // Sector Count 7:0

        // CE supports 32-bit LBA.  Therefore, clear the upper 16 bits of LBA.
        WriteLBAHigh(0);    // LBA 47:40
        WriteLBAMid(0);     // LBA 39:32
        WriteLBALow((BYTE)(dwStartSector >> 24));   // LBA 31:24
        WriteLBAHigh((BYTE)(dwStartSector >> 16));  // LBA 23:16
        WriteLBAMid((BYTE)(dwStartSector >> 8));    // LBA 15:8
        WriteLBALow((BYTE)dwStartSector);           // LBA 7:0

        WriteDriveHeadReg( ATA_HEAD_LBA_MODE | ((m_dwDevice == 0 ) ? ATA_HEAD_DRIVE_1 : ATA_HEAD_DRIVE_2));
        }
    else  // 28-bit LBA
        {
        // to transfer 256 sectors, set number of sectors to 0
        if (dwNumberOfSectors == MAX_SECT_PER_COMMAND)
            {
            dwNumberOfSectors = 0;
            }

        WriteSectorCount((BYTE)dwNumberOfSectors);
        if (m_fLBAMode == TRUE)
            {
            WriteSectorNumber( (BYTE)dwStartSector);
            WriteLowCount((BYTE)(dwStartSector >> 8));
            WriteHighCount((BYTE)(dwStartSector >> 16));
            WriteDriveHeadReg((BYTE)((dwStartSector >> 24) | ATA_HEAD_LBA_MODE) | ((m_dwDevice == 0 ) ? ATA_HEAD_DRIVE_1 : ATA_HEAD_DRIVE_2));
            }
        else
            {
            DWORD dwSectors = m_DiskInfo.di_sectors;
            DWORD dwHeads = m_DiskInfo.di_heads;
            WriteSectorNumber((BYTE)((dwStartSector % dwSectors) + 1));
            WriteLowCount((BYTE) (dwStartSector /(dwSectors*dwHeads)));
            WriteHighCount((BYTE)((dwStartSector /(dwSectors*dwHeads)) >> 8));
            WriteDriveHeadReg((BYTE)(((dwStartSector/dwSectors)% dwHeads) | ((m_dwDevice == 0 ) ? ATA_HEAD_DRIVE_1 : ATA_HEAD_DRIVE_2)));
            }
        }

    WriteCommand(bCmd);

    return (TRUE);
}

// ----------------------------------------------------------------------------
// Function: ReadDisk
//     Perform PIO read.
//
// Parameters:
//     pId -
//     dwNumberOfSectors -
//     bCmd -
// ----------------------------------------------------------------------------

DWORD CST202T_SATA::ReadDisk(PIOREQ pIoReq)
{
    DWORD dwError;
    BOOL bRetry;

    // Certain SATA errors are non-recoverable by the hardware but are transient in nature.
    // Fetch the number of times we should retry a command when an error is detected.
    DWORD dwRetriesRemaining = m_pPort->m_pController->m_pIdeReg->dwNumRetriesOnSATAError;

Begin_Read:
    bRetry = FALSE;

    // clear pending interrupts
    GetBaseStatus();
    ClearSError();
    ResetEvent(m_pPort->m_hSATAEvent);
    ResetEvent(m_pPort->m_hErrorEvent);

    // Perform Read
    dwError = CDisk::ReadDisk(pIoReq);

    // We must check for SATA error conditions here to ensure that
    // we haven't read invalid data.
    switch (GetSATAError())
        {
        case SATA_ERROR_NO_ERROR:
            // Check that the command completed successfully
            if (dwError != ERROR_SUCCESS)
                {
                m_dwErrorCount++;
                bRetry = TRUE;
                }
            break;
        case SATA_ERROR_PERSISTENT:
            RETAILMSG(ZONE_IO|ZONE_ERROR, (_T(
                "Atapi!CST202T_SATA::ReadDisk> Unrecoverable error detected.  ABORT!\r\n"
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
                "Atapi!CST202T_SATA::ReadDisk> Error detected. Resetting controller and retrying command. Error Count = %d\r\n"
                ), m_dwErrorCount));

            ResetController(FALSE);

            goto Begin_Read;
            }
        else
            {
            RETAILMSG(ZONE_IO|ZONE_ERROR, (_T(
                "Atapi!CST202T_SATA::ReadDisk> Error detected. %d retries failed. ABORT! Error Count = %d\r\n"
                ), m_pPort->m_pController->m_pIdeReg->dwNumRetriesOnSATAError, m_dwErrorCount));

            dwError = ERROR_GEN_FAILURE;
            }
        }

    return dwError;
}

// ----------------------------------------------------------------------------
// Function: WriteDisk
//     Perform PIO write.
//
// Parameters:
//     pId -
//     dwNumberOfSectors -
//     bCmd -
// ----------------------------------------------------------------------------

DWORD CST202T_SATA::WriteDisk(PIOREQ pIoReq)
{
    DWORD dwError;
    BOOL bRetry;

    // Certain SATA errors are non-recoverable by the hardware but are transient in nature.
    // Fetch the number of times we should retry a command when an error is detected.
    DWORD dwRetriesRemaining = m_pPort->m_pController->m_pIdeReg->dwNumRetriesOnSATAError;

Begin_Write:
    bRetry = FALSE;

    // clear pending interrupts
    GetBaseStatus();
    ClearSError();
    ResetEvent(m_pPort->m_hSATAEvent);
    ResetEvent(m_pPort->m_hErrorEvent);

    // Perform Write
    dwError = CDisk::WriteDisk(pIoReq);

    // We must check for SATA error conditions here to ensure that
    // we haven't written invalid data.
    switch (GetSATAError())
        {
        case SATA_ERROR_NO_ERROR:
            // Check that the command completed successfully
            if (dwError != ERROR_SUCCESS)
                {
                m_dwErrorCount++;
                bRetry = TRUE;
                }
            break;
        case SATA_ERROR_PERSISTENT:
            RETAILMSG(ZONE_IO|ZONE_ERROR, (_T(
                "Atapi!CST202T_SATA::WriteDisk> Unrecoverable error detected.  ABORT!\r\n"
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
                "Atapi!CST202T_SATA::WriteDisk> Error detected. Resetting controller and retrying command. Error Count = %d\r\n"
                ), m_dwErrorCount));

            ResetController(FALSE);

            goto Begin_Write;
            }
        else
            {
            RETAILMSG(ZONE_IO|ZONE_ERROR, (_T(
                "Atapi!CST202T_SATA::WriteDisk> Error detected. %d retries failed. ABORT! Error Count = %d\r\n"
                ), m_pPort->m_pController->m_pIdeReg->dwNumRetriesOnSATAError, m_dwErrorCount));

            dwError = ERROR_GEN_FAILURE;
            }
        }

    return dwError;
}

// ----------------------------------------------------------------------------
// Function: GetSATAError
//     This function inspects the current state of the SError register and
//     informs the caller of the class of the error condition (none,
//     persistent, or transient).
//
// Parameters:
//     pId -
//     dwNumberOfSectors -
//     bCmd -
// ----------------------------------------------------------------------------

SError_Type CST202T_SATA::GetSATAError(void)
{
    DWORD dwSError;

    dwSError = m_pRegSATA->dwSError;            // read SError register

//    DEBUGMSG(ZONE_IO, (_T(
//        "ATAPI!CST202T_SATA::GetSATAError> SError(1) = 0x%x\r\n"
//        ), dwSError));

    // On some hardware, I've seen the error arrive "late."  Double-check the value.
    if (dwSError != m_pRegSATA->dwSError)
        {
        dwSError |= m_pRegSATA->dwSError;            // read SError register
        DEBUGMSG(ZONE_IO, (_T(
            "ATAPI!CST202T_SATA::GetSATAError> Change in SError detected!  SError(2) = 0x%x\r\n"
            ), dwSError));
        }

    if (dwSError != m_pRegSATA->dwSError)
        {
        dwSError |= m_pRegSATA->dwSError;            // read SError register
        DEBUGMSG(ZONE_IO, (_T(
            "ATAPI!CST202T_SATA::GetSATAError> Change in SError detected!  SError(3) = 0x%x\r\n"
            ), dwSError));
        }

    m_pRegSATA->dwSError = dwSError;            // Clear SATA errors

    EnableSErrorInt(TRUE);  // Re-enable SError interrupt if it was cleared

    // Optimize for the case where there is no error.
    if ( !(dwSError & SATA_ERR_MASK) )
        {
        return SATA_ERROR_NO_ERROR;  // No errors
        }

    // SATA 1.0A, section 10.1.2: abort on persistent communication or data integrity errors
    if (dwSError & SATA_SERROR_NONRECOVERED_PERSISTENT_COMM_ERR)  // Non-recoverable persistent error.  Bail out.
        {
        RETAILMSG(ZONE_IO, (_T(
            "ATAPI!CST202T_SATA::GetSATAError> Persistent error detected. SError = 0x%x\r\n"
            ), dwSError));

        return SATA_ERROR_PERSISTENT;
        }
    // SATA 1.0A, section 10.1.2: reset and retry on non-recovered transient errors
    else
        {
        DEBUGMSG(ZONE_IO, (_T(
            "ATAPI!CST202T_SATA::GetSATAError> Transient error detected. SError = 0x%x\r\n"
            ), dwSError));

        return SATA_ERROR_TRANSIENT;
        }

}
