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

#ifndef _ST202T_SATA_H_
#define _ST202T_SATA_H_
#include <pm.h>
#include <atapipcicd.h>
#include "debug.h"      // for ZONE_INIT
#include "atapipci.h"   // for class CPCIDisk


//
// ATA/ATAPI-6 definition of IDENTIFY_DEVICE results.  This is for 48-bit LBA support.
//
#pragma pack(1)
typedef struct _IDENTIFY_DATA6 {
    USHORT GeneralConfiguration;            // 00   Mandatory for ATAPI
    USHORT NumberOfCylinders;               // 01   Not used for ATAPI
    USHORT Reserved1;                       // 02   Not used for ATAPI
    USHORT NumberOfHeads;                   // 03   Not used for ATAPI
    USHORT UnformattedBytesPerTrack;        // 04   Not used for ATAPI
    USHORT UnformattedBytesPerSector;       // 05   Not used for ATAPI
    USHORT SectorsPerTrack;                 // 06   Not used for ATAPI
    USHORT VendorUnique1[3];                // 07-09    Not used for ATAPI
    USHORT SerialNumber[10];                // 10   Optional for ATAPI
    USHORT BufferType;                      // 20   Not used for ATAPI
    USHORT BufferSectorSize;                // 21   Not used for ATAPI
    USHORT NumberOfEccBytes;                // 22   Not used for ATAPI
    USHORT FirmwareRevision[4];             // 23   Mandatory for ATAPI
    USHORT ModelNumber[20];                 // 27   Mandatory for ATAPI
    UCHAR  MaximumBlockTransfer;            // 47 low byte     Not used for ATAPI
    UCHAR  VendorUnique2;                   // 47 high byte    Not used for ATAPI
    USHORT DoubleWordIo;                    // 48   Not used for ATAPI
    USHORT Capabilities;                    // 49   Mandatory for ATAPI
    USHORT Capabilities2;                   // 50 bit 0 = 1 to indicate a device specific Standby timer value minimum
    UCHAR  VendorUnique3;                   // 51 low byte      Mandatory for ATAPI
    UCHAR  PioCycleTimingMode;              // 51 high byte     Mandatory for ATAPI
    UCHAR  VendorUnique4;                   // 52 low byte      Mandatory for ATAPI
    UCHAR  DmaCycleTimingMode;              // 52 high byte     Mandatory for ATAPI
    USHORT TranslationFieldsValid;          // 53 (low bit)     Mandatory for ATAPI
    USHORT NumberOfCurrentCylinders;        // 54   Not used for ATAPI
    USHORT NumberOfCurrentHeads;            // 55   Not used for ATAPI
    USHORT CurrentSectorsPerTrack;          // 56   Not used for ATAPI
    ULONG  CurrentSectorCapacity;           // 57 & 58          Not used for ATAPI
    UCHAR  MultiSectorCount;                // 59 low           Not used for ATAPI
    UCHAR  MultiSectorSettingValid;         // 59 high (low bit)Not used for ATAPI
    ULONG  TotalUserAddressableSectors;     // 60 & 61          Not used for ATAPI
    UCHAR  SingleDmaModesSupported;         // 62 low byte      Mandatory for ATAPI
    UCHAR  SingleDmaTransferActive;         // 62 high byte     Mandatory for ATAPI
    UCHAR  MultiDmaModesSupported;          // 63 low byte      Mandatory for ATAPI
    UCHAR  MultiDmaTransferActive;          // 63 high byte     Mandatory for ATAPI
    UCHAR  AdvancedPIOxferreserved;         // 64 low byte      Mandatory for ATAPI
    UCHAR  AdvancedPIOxfer;                 // 64 high byte     Mandatory for ATAPI
    USHORT MinimumMultiwordDMATime;         // 65 Mandatory for ATAPI
    USHORT ManuRecomendedDMATime;           // 66 Mandatory for ATAPI
    USHORT MinimumPIOxferTimeWOFlow;        // 67 Mandatory for ATAPI
    USHORT MinimumPIOxferTimeIORDYFlow;     // 68 Mandatory for ATAPI
    USHORT ReservedADVPIOSupport[2];        // 69 Not used for ATAPI
    USHORT TypicalProcTimeForOverlay;       // 71 Optional for ATAPI
    USHORT TypicalRelTimeForOverlay;        // 72 Optional for ATAPI
    USHORT MajorRevisionNumber;             // 73 Optional for ATAPI
    USHORT MinorRevisionNumber;             // 74 Optional for ATAP
    USHORT QueueDepth;                      // 75
    USHORT Reserved6[4];                    // 76-79
    USHORT MajorVersionNumber;              // 80
    USHORT MinorVersionNumber;              // 81
    USHORT CommandSetSupported1;            // 82
    USHORT CommandSetSupported2;            // 83
    USHORT CommandSetFeaturesSupported;     // 84
    USHORT CommandSetFeatureEnabled1;       // 85
    USHORT CommandSetFeatureEnabled2;       // 86
    USHORT CommandSetFeatureDefault ;       // 87
    UCHAR  UltraDMASupport;                 // 88 Low
    UCHAR  UltraDMAActive;                  // 88 High
    USHORT TimeRequiredForSecurityErase;    // 89 Time Required For Security Erase Unit Completion
    USHORT TimeReuiregForEnhancedSecurtity; // 90 Time Required For Enhanced Security Erase Unit Completion
    USHORT CurrentAdvancePowerMng;          // 91 CurrentAdvanced Power Managemnt Value
    USHORT MasterPasswordRevisionCode;      // 92 Master Password Revision Code
    USHORT HardwareResetResult;             // 93 Hardware Reset Result
    UCHAR  CurrentAcousticManagement;       // 94 Acoustic Management (low byte = current; high byte = vendor recommended)
    UCHAR  VendorAcousticManagement;
    USHORT Reserved7a[99-95+1];             // 95-99
    ULONG  lMaxLBAAddress[2];               // 100-103 Maximum User LBA for 48-bit Address feature set
    USHORT Reserved7b[126-104+1];           // 104-126
    USHORT MediaStatusNotification:2;       // 127 Removable Media Status Notification Support
    USHORT SecurityStatus;                  // 128 Security Status
    USHORT Reserved8[31];                   // 129-159 Vendor Specific
    USHORT CFAPowerMode1;                   // 160
    USHORT Reserved9[94];                   // 161-254
    USHORT IntegrityWord;                   // 255 Checksum & Signature
} IDENTIFY_DATA6, *PIDENTIFY_DATA6;
#pragma pack()

// Register layout of SATA block
#define SATA_AHB2STB_BASE_OFFSET    0
#define SATA_DMA_BASE_OFFSET        0x400
#define SATA_HBA_BASE_OFFSET        0x800

// AHB <-> ST Bus Protocol Converter registers
#define SATA_AHB2STB_OPC            0       // ST Bus Opcode Configuration
#define SATA_AHB2STB_MESSAGESIZE    0x4     // ST Bus Message Size
#define SATA_AHB2STB_CHUNKSIZE      0x8     // ST Bus Chunk Size
#define SATA_AHB2STB_SRST           0xC     // Soft Reset
#define SATA_AHB2STB_STATUS         0x10    // Protocol converter status (1 = idle, 0 = busy)
#define SATA_AHB2STB_GLUE           0x14    // Glue logic

typedef struct __SATA_AHB2STB_T
{
    DWORD   dwOPC;          // Opcode
    DWORD   dwMsgSize;      // Message Size
    DWORD   dwChunkSize;    // Chunk Size
    DWORD   dwSRST;         // Soft Reset
    DWORD   dwStat;         // Status
    DWORD   dwGlue;         // Glue logic
} SATA_AHB2STB, *pSATA_AHB2STB;

// SATA DMA Controller register offsets (replicated on a per-channel basis)
// These offsets represent the offset from the channel's DMA controller base
#define SATA_REG_DMA_SAR0           0       // Channel source address
#define SATA_REG_DMA_DAR0           0x8     // Channel destination address
#define SATA_REG_DMA_CTL0_LSB       0x18    // Channel control LSB (reset value should not be modified)
#define SATA_REG_DMA_CTL0_MSB       0x2C    // Channel control MSB (reset value should not be modified)
#define SATA_REG_DMA_CFG0_LSB       0x40    // Channel configuration LSB (reset value should not be modified)
#define SATA_REG_DMA_CFG0_MSB       0x44    // Channel configuration MSB (reset value should not be modified)

// Structure representing DMA Channel registers.
typedef struct __SATA_DMAChannel_T
{
    DWORD   dwSAR;      // Channel Source Address
    DWORD   Reserved1;
    DWORD   dwDAR;      // Channel Destination Address
    DWORD   Reserved2;
    DWORD   dwLLP;      // Channel Linked List Pointer Address
    DWORD   Reserved3;
    DWORD   dwCTL_LSB;  // Channel control (LSB)
    DWORD   dwCTL_MSB;  // Channel control (MSB)
    DWORD   Reserved4[8];
    DWORD   dwCFG_LSB;  // Channel configuration (LSB)
    DWORD   dwCFG_MSB;  // Channel configuration (MSB)
} SATA_DMAChannel, *pSATA_DMAChannel;

// Structure representing a DMA link list item (see figure 61 in dw_ahb_dmac_db.pdf)
typedef struct __SATA_DMA_LLI_T
{
    DWORD           SAR;        // Block source address
    DWORD           DAR;        // Block destination address
    __SATA_DMA_LLI_T*   LLP;    // Pointer to next block descriptor
    DWORD           CTL_LSB;    // Channel control (LSB)
    DWORD           CTL_MSB;    // Channel control (MSB)
    DWORD           SSTAT;      // Source status write back (will this be used?)
    DWORD           DSTAT;      // Destination status write back (will this be used?)
} SATA_DMA_LLI, *pSATA_DMA_LLI;

// DMA Controller interrupt registers (one bank for all channels)
#define SATA_REG_DMA_RAW_TFR        0x6C0
#define SATA_REG_DMA_RAW_BLOCK      0x6C8
#define SATA_REG_DMA_RAW_SRC_TRAN   0x6D0
#define SATA_REG_DMA_RAW_DST_TRAN   0x6D8
#define SATA_REG_DMA_RAW_ERR        0x6E0
#define SATA_REG_DMA_TFR_STA        0x6E8
#define SATA_REG_DMA_BLOCK_STA      0x6F0
#define SATA_REG_DMA_SRC_TRAN_STA   0x6F8
#define SATA_REG_DMA_DST_TRAN_STA   0x700
#define SATA_REG_DMA_ERR_STA        0x708
#define SATA_REG_DMA_MASK_TFR       0x710
#define SATA_REG_DMA_MASK_BLK       0x718
#define SATA_REG_DMA_MASK_SRC_TRAN  0x720
#define SATA_REG_DMA_MASK_DST_TRAN  0x728
#define SATA_REG_DMA_MASK_ERR       0x730
#define SATA_REG_DMA_CLEAR_TFR      0x738
#define SATA_REG_DMA_CLEAR_BLOCK    0x740
#define SATA_REG_DMA_CLEAR_SRC_TRAN 0x748
#define SATA_REG_DMA_CLEAR_DST_TRAN 0x750
#define SATA_REG_DMA_CLEAR_ERR      0x758
#define SATA_REG_DMA_STATUS_INT     0x760
#define SATA_REG_DMA_DMA_CFG        0x798
#define SATA_REG_DMA_CH_EN          0x7A0

// SATA host controller register offsets (replicated on a per-channel basis)
// These offsets represent the offset from the channel's host controller base
#define SATA_REG_DATA               0x0
#define SATA_REG_FEATURE            0x4     // write
#define SATA_REG_ERROR              0x4     // read
#define SATA_REG_SECT_CNT           0x8
#define SATA_REG_SECT_NUM           0xC     // CHS mode
#define SATA_REG_CYL_LOW            0x10    // CHS mode
#define SATA_REG_CYL_HIGH           0x14    // CHS mode
#define SATA_REG_LBA_LOW            0xC     // LBA mode
#define SATA_REG_LBA_MID            0x10    // LBA mode
#define SATA_REG_LBA_HIGH           0x14    // LBA mode
#define SATA_REG_DEV_HEAD           0x18
#define SATA_REG_COMMAND            0x1C    // write
#define SATA_REG_STATUS             0x1C    // read     (reading this acknowledges the interrupt)
#define SATA_REG_DEV_CTRL           0x20    // write
#define SATA_REG_ALT_STATUS         0x20    // read     (reading this does not ack the interrupt)

// SATA-specific registers
#define SATA_REG_SSTATUS            0x24    // SATA Status register
#define SATA_REG_SERROR             0x28    // SATA Error register
#define SATA_REG_SCONTROL           0x2C    // SATA Control Register
#define SATA_REG_SACTIVE            0x30    // SATA Active Register
#define SATA_REG_SNOTIFICATION      0x34    // SATA Notification Register

// Structure representing SATA-specific register set
typedef struct __SATA_Regs_T
{
    DWORD   dwSStatus;
    DWORD   dwSError;
    DWORD   dwSControl;
    DWORD   dwSActive;
    DWORD   dwSNotification;
} SATA_Regs, *pSATA_Regs;

// SControl bit definitions
#define SATA_SCONTROL_DET_DEASSERT_INITCOM  (0)
#define SATA_SCONTROL_DET_ASSERT_INITCOM    (1 << 0)
#define SATA_SCONTROL_DET_PHYOFFLINE        (1 << 2)
#define SATA_SCONTROL_SPD_NORESTRICTIONS    (0)
#define SATA_SCONTROL_SPD_1_5_SPEEDLIMIT    (1 << 4)
#define SATA_SCONTROL_SPD_3_0_SPEEDLIMIT    (1 << 5)
#define SATA_SCONTROL_IPM_NORESTRICTIONS    (0)
#define SATA_SCONTROL_IPM_PARTIAL_DISABLED  (1 << 8)
#define SATA_SCONTROL_IPM_SLUMBER_DISABLED  (1 << 9)

// SStatus bit definitions
#define SATA_SSTATUS_DEVICE_NOT_PRESENT     (0)
#define SATA_SSTATUS_DEVICE_DETECTED        (1 << 1)
#define SATA_SSTATUS_PHY_COMM_ESTABLISHED   (1 << 2)
#define SATA_SSTATUS_PHY_OFFLINE            (1 << 3)
#define SATA_SSTATUS_SPD_1_5_NEGOTIATED     (1 << 4)
#define SATA_SSTATUS_SPD_3_0_NEGOTIATED     (1 << 5)
#define SATA_SSTATUS_IPM_ACTIVE             (1 << 8)
#define SATA_SSTATUS_IPM_PARTIAL            (1 << 9)
#define SATA_SSTATUS_IPM_SLUMBER            (SATA_SSTATUS_IPM_PARTIAL | (1 << 10))

// SError bit definitions
#define SATA_SERROR_RECOVERED_DATAINTEGRITY_ERR                 (1 << 0)
#define SATA_SERROR_RECOVERED_COMM_ERR                          (1 << 1)
#define SATA_SERROR_NONRECOVERED_TRANSIENT_DATAINTEGRITY_ERR    (1 << 8)
#define SATA_SERROR_NONRECOVERED_PERSISTENT_COMM_ERR            (1 << 9)
#define SATA_SERROR_PROTOCOL_ERROR                              (1 << 10)
#define SATA_SERROR_INTERNAL_ERROR                              (1 << 11)

// These are the SATA errors of interest in this driver
#define SATA_ERR_MASK       (SATA_SERROR_NONRECOVERED_PERSISTENT_COMM_ERR | \
                             SATA_SERROR_NONRECOVERED_TRANSIENT_DATAINTEGRITY_ERR | \
                             SATA_SERROR_PROTOCOL_ERROR | \
                             SATA_SERROR_INTERNAL_ERROR)

// SATA errors can be persistent or transient in nature.  GetSATAError() returns this enum.
enum SError_Type
{
    SATA_ERROR_NO_ERROR = 0,
    SATA_ERROR_PERSISTENT,
    SATA_ERROR_TRANSIENT
};

// Host controller control registers
#define SATA_REG_FPTAGR             0x64    // First Party DMA Tag
#define SATA_REG_FPBOR              0x68    // First Party DMA Buffer Offset
#define SATA_REG_FPTCR              0x6C    // First Party DMA Transfer Count
#define SATA_REG_DMACR              0x70    // DMA Control
#define SATA_REG_DBTSR              0x74    // DMA Burst Transaction Size
#define SATA_REG_INTPR              0x78    // Interrupt Pending
#define SATA_REG_INTMR              0x7C    // Interrupt Mask
#define SATA_REG_ERRMR              0x80    // Error Mask
#define SATA_REG_LLCR               0x84    // Link Layer Control
#define SATA_REG_PHYCR              0x88    // PHY Control
#define SATA_REG_PHYSR              0x8C    // PHY Status
#define SATA_REG_VERSIONR           0xF8    // SATA Host version
#define SATA_REG_IDR                0xFC    // SATA host ID

// SATA host controller interrupt pending (INTPR) bit definitions
#define SATA_INTPR_DMAT             (1 << 0)
#define SATA_INTPR_NEWFP            (1 << 1)
#define SATA_INTPR_PMABORT          (1 << 2)
#define SATA_INTPR_ERR              (1 << 3)
#define SATA_INTPR_NEWBIST          (1 << 4)
#define SATA_INTPR_IPF              (1 << 31)

// Values for programmable DMA configuration registers
#define SATA_DMA_CTL_LSB_READ       0x0220D825
#define SATA_DMA_CFG_MSB_READ       0x0802
#define SATA_DMA_CFG_LSB_READ       0
#define SATA_HBA_DBTSR_READ         0x00100010
#define SATA_HBA_DMACR_READ         0x3
#define SATA_DMA_CTL_LSB_WRITE      0x0090C825
#define SATA_DMA_CFG_MSB_WRITE      0x0802
#define SATA_DMA_CFG_LSB_WRITE      0x1800
#define SATA_HBA_DBTSR_WRITE        0x00100010
#define SATA_HBA_DMACR_WRITE        0x3

#define SATA_DMA_CTL_LSB_LINKED_READ    0x1A20D825
#define SATA_DMA_CTL_LSB_LINKED_WRITE  0x1890C825

// Maximum number of bytes transferred per FIS
#define SATA_MAX_FIS_SIZE                                       8192

// Structure representing SATA host controller registers
typedef struct __SATAHostController_Tag
{
    DWORD   dwFPTAGR;       // First party DMA tag
    DWORD   dwFPBOR;        // First party DMA buffer offset
    DWORD   dwFPTCR;        // First party DMA transfer count
    DWORD   dwDMACR;        // DMA Control
    DWORD   dwDBTSR;        // DMA burst transaction size
    DWORD   dwINTPR;        // Interrupt pending
    DWORD   dwINTMR;        // Interrupt mask
    DWORD   dwERRMR;        // Error mask
    DWORD   dwLLCR;         // Link layer control
    DWORD   dwPHYCR;        // PHY control
    DWORD   dwPHYSR;        // PHY status
    DWORD   Reserved[25];   // Reserved bytes
    DWORD   dwVERSIONR;     // SATA host version
    DWORD   dwIDR;          // SATA host ID
} SATA_HostController, *pSATA_HostController;

// 48-bit LBA support
#define  ATA_CMD_FLUSH_CACHE_EXT                0xEA
#define  ATA_CMD_READ_DMA_EXT                   0x25
#define  ATA_CMD_READ_DMA_QUEUED_EXT            0x26
#define  ATA_CMD_READ_LOG_EXT                   0x2F
#define  ATA_CMD_READ_MULTIPLE_EXT              0x29
#define  ATA_CMD_READ_NATIVE_MAX_ADDRESS_EXT    0x27
#define  ATA_CMD_READ_SECTOR_EXT                0x24
#define  ATA_CMD_READ_VERIFY_SECTOR             0x42
#define  ATA_CMD_SET_MAX_ADDRESS_EXT            0x37
#define  ATA_CMD_WRITE_DMA_EXT                  0x35
#define  ATA_CMD_DMA_QUEUED_EXT                 0x36
#define  ATA_CMD_WRITE_LOG_EXT                  0x3F
#define  ATA_CMD_WRITE_MULTIPLE_EXT             0x39
#define  ATA_CMD_WRITE_SECTOR_EXT               0x34

#define MAX_SECT_PER_EXT_COMMAND                65536

// Main SATA class representing a SATA device.  We inherit from CPCIDiskAndCD even
// though this system is not PCI based as the CPCIDiskAndCD class contains a great deal
// of relevant code.
class CST202T_SATA : public CPCIDiskAndCD
{
public:
    CST202T_SATA(HKEY hKey);
    ~CST202T_SATA();

    // Initialization routines
    virtual BOOL Init(HKEY hActiveKey);
    virtual VOID ConfigureRegisterBlock(DWORD dwStride);
    virtual BOOL ConfigPort();
    // Enable write cache through SET FEATURES
    virtual BOOL SetWriteCacheMode(BOOL fEnable);
    // Enable read look-ahead through SET FEATURES
    virtual BOOL SetLookAhead();

    // PIO routines
    virtual BOOL WaitForInterrupt(DWORD dwTimeOut);
    virtual DWORD ReadDisk(PIOREQ pIoReq);
    virtual DWORD WriteDisk(PIOREQ pIoReq);

    // DMA routines
    virtual DWORD ReadWriteDiskDMA(PIOREQ pIOReq, BOOL fRead = TRUE);
    virtual BOOL SetupDMA(PSG_BUF pSgBuf, DWORD dwSgCount, BOOL fRead);
    virtual BOOL EndDMA();
    virtual BOOL AbortDMA();
    virtual BOOL TranslateAddress(PDWORD pdwAddr);

    // Common routines
    virtual BOOL SendIOCommand(DWORD dwStartSector, DWORD dwNumberOfSectors, BYTE bCmd);
    virtual BOOL ResetController(BOOL bSoftReset = FALSE);

private:
    // Common interrupt handling routines
    static DWORD InterruptThreadStub(IN PVOID lParam);
    DWORD InterruptThread(IN PVOID lParam);

    // DMA Routines
    DWORD DoReadWriteDiskDMA(PIOREQ pIOReq, BOOL fRead);
    BOOL WaitForDMAInterrupt(DWORD dwTimeOut);
    BOOL ProcessDMA(BOOL fRead);
    VOID StartDMATransaction(BOOL fRead, const pSATA_DMA_LLI pPRD);
    BOOL CombineLLITables(BOOL fRead);
    BOOL BuildLLITables(PSG_BUF pSgBuf, DWORD dwSgCount, BOOL fRead);

    // Common Routines
    void ConfigLBA48(void);
    VOID ToggleOOB(DWORD dwSpeedLimit);
    SError_Type GetSATAError(void);


    ////////////////////////////
    // Private helper functions
    ////////////////////////////

    inline void WriteLBALow(BYTE bValue)
    {
        WRITE_REGISTER_ULONG((PULONG)(m_pATAReg + SATA_REG_LBA_LOW), (bValue & 0xFF));
    }

    inline void WriteLBAMid(BYTE bValue)
    {
        WRITE_REGISTER_ULONG((PULONG)(m_pATAReg + SATA_REG_LBA_MID), (bValue & 0xFF));
    }

    inline void WriteLBAHigh(BYTE bValue)
    {
        WRITE_REGISTER_ULONG((PULONG)(m_pATAReg + SATA_REG_LBA_HIGH), (bValue & 0xFF));
    }

    inline void EnableDMAChannel(BOOL Enable)
    {
        DWORD   dwChannelEnable;

        PULONG Address = (PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_CH_EN);

        // Bits 15:8 contain the modification mask
        // Bits 7:0 contain value to be written
        // For example, to enable channel 0, we write 0x0101 to DMA_CH_EN
        dwChannelEnable = ( (1 << 8) | Enable );

        WRITE_REGISTER_ULONG(Address, dwChannelEnable);

        // When disabling a channel, we must poll to determine when channel has really deactivated
        if (FALSE == Enable)
        {
            while ( (READ_REGISTER_ULONG(Address) & 1 ) != 0)
                ;
        }
    }

    inline void EnableDMAController(BOOL Enable)
    {
        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_DMA_CFG), Enable);
    }

    inline DWORD GetDMAErrorStatus()
    {
        DWORD ErrorStat;

        ErrorStat = READ_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_RAW_ERR));
        return (ErrorStat & 1);
    }

    inline DWORD GetDMATransferStatus()
    {
        DWORD TransferStat;

        TransferStat = READ_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_RAW_TFR));
        return (TransferStat & 1);
    }

    inline BOOL IsDMAChannelActive()
    {
        DWORD ChannelActive;

        ChannelActive = READ_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_CH_EN));
        return (ChannelActive & 1);
    }

    inline BOOL DMAError()
    {
        DWORD ErrorStat;

        ErrorStat = READ_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_RAW_ERR));
        return (ErrorStat & 1);
    }

    inline void WaitForAHBIdle(void)
    {
        while ( (m_pRegAHB2STB->dwStat & 0x1) == 0 )
            ;
    }

    inline void ClearDMAStatusRegs()
    {
        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_CLEAR_BLOCK), 1);
        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_CLEAR_DST_TRAN), 1);
        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_CLEAR_ERR), 1);
        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_CLEAR_SRC_TRAN), 1);
        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_CLEAR_TFR), 1);
    }

    inline void EnableBlockTfrInt(BOOL Enable)
    {
        DWORD dwBlockIntEnable;

        // Bits 15:8 contain the modification mask
        // Bits 7:0 contain value to be written
        dwBlockIntEnable = ( (1 << 8) | Enable );

        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_MASK_BLK), dwBlockIntEnable);
    }

    inline BOOL IsBlockTfrIntActive(void)
    {
        DWORD BlockIntStat;

        BlockIntStat = READ_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_BLOCK_STA));

        return (BlockIntStat & 0x1);
    }

    inline void ClearBlockTfrInt(void)
    {
        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_CLEAR_BLOCK), 1);
    }

    inline BOOL IsSErrorIntActive(void)
    {
        DWORD dwINTPR, dwINTMR;

        dwINTPR = READ_REGISTER_ULONG((PULONG)(m_pATAReg + SATA_REG_INTPR));
        dwINTMR = READ_REGISTER_ULONG((PULONG)(m_pATAReg + SATA_REG_INTMR));

        return (dwINTPR & dwINTMR & SATA_INTPR_ERR);
    }

    inline void EnableDMACTfrInt(BOOL Enable)
    {
        DWORD dwDMACTfrIntEnable;

        // Bit 8 contains the modification mask
        // Bit 0 contains the value to be written
        dwDMACTfrIntEnable = ((1<<8) | Enable);

        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_MASK_TFR), dwDMACTfrIntEnable);
    }

    inline BOOL IsDMACTfrIntActive(void)
    {
        DWORD DMACIntStat;

        DMACIntStat = READ_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_TFR_STA));

        return (DMACIntStat & 1);
    }

    inline void ClearDMACTfrInt(void)
    {
        WRITE_REGISTER_ULONG((PULONG)(m_pPort->m_pController->m_dwControllerBase + SATA_REG_DMA_CLEAR_TFR), 1);
    }

    inline void EnableSErrorInt(BOOL Enable)
    {
        DWORD dwINTMR;

        dwINTMR = READ_REGISTER_ULONG((PULONG)(m_pATAReg + SATA_REG_INTMR)) & ~SATA_INTPR_ERR;
        dwINTMR |= Enable ? SATA_INTPR_ERR : 0;

        // Setup error mask
        WRITE_REGISTER_ULONG((PULONG)(m_pATAReg + SATA_REG_ERRMR), SATA_ERR_MASK);

        // Enable or disable the error interrupt
        WRITE_REGISTER_ULONG((PULONG)(m_pATAReg + SATA_REG_INTMR), dwINTMR);
    }

    inline void ClearSError(void)
    {
        DWORD dwSError;

        dwSError = m_pRegSATA->dwSError;
        m_pRegSATA->dwSError = dwSError;
    }

    ///////////////////////////
    // Member variables
    ///////////////////////////

    BOOL        m_fUseLBA48;
    pSATA_DMA_LLI   m_LLITable;
    PHYSICAL_ADDRESS m_LLITablePhys;

protected:
    volatile pSATA_DMAChannel       m_pRegDMA;
    volatile pSATA_Regs             m_pRegSATA;
    volatile pSATA_HostController   m_pRegHBA;
    volatile pSATA_AHB2STB          m_pRegAHB2STB;

    DWORD      m_dwErrorCount;
};

#endif //_ST202T_SATA_H_
