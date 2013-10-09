// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  Header:  eboot.h
//
//  This header file is comprised of component header files that defines
//  the standard include hierarchy for the bootloader. It also defines few
//  trivial constant.
//
#ifndef __EBOOT_H
#define __EBOOT_H

//------------------------------------------------------------------------------
#include "bsp.h"

#pragma warning(push)
#pragma warning(disable: 4201 4115)

#include <blcommon.h>
#include <nkintr.h>
#include <halether.h>
#include <fmd.h>
#include <bootpart.h>
#include <oal.h>
#include <oal_blmenu.h>

#pragma warning(pop)

#include "boot_args.h"
#include "args.h"


//------------------------------------------------------------------------------

#define EBOOT_VERSION_MAJOR         1
#define EBOOT_VERSION_MINOR         0

//------------------------------------------------------------------------------

typedef struct {
    UINT32 start;
    UINT32 length;
    UINT32 base;
} REGION_INFO_EX;

//------------------------------------------------------------------------------

#define DOWNLOAD_TYPE_UNKNOWN       0
#define DOWNLOAD_TYPE_RAM           1
#define DOWNLOAD_TYPE_BINDIO        2
#define DOWNLOAD_TYPE_XLDR          3
#define DOWNLOAD_TYPE_EBOOT         4           
#define DOWNLOAD_TYPE_IPL           5
#define DOWNLOAD_TYPE_FLASHNAND     6
#define DOWNLOAD_TYPE_FLASHNOR      7
#define DOWNLOAD_TYPE_LOGO			8
#define DOWNLOAD_TYPE_EXT			9

//------------------------------------------------------------------------------

#define LOGO_NB0_FILE				"logo.nb0"
#define LOGO_NB0_FILE_LEN           8

//------------------------------------------------------------------------------

typedef struct {
    OAL_KITL_TYPE bootDeviceType;
    UINT32 type;
    UINT32 numRegions;
    UINT32 launchAddress;
    REGION_INFO_EX region[BL_MAX_BIN_REGIONS];

    UINT32 recordOffset;
    UINT8  *pReadBuffer;
    UINT32 readSize;
} EBOOT_CONTEXT;

//------------------------------------------------------------------------------

extern BOOT_CFG g_bootCfg;
extern EBOOT_CONTEXT g_eboot;
extern OAL_KITL_DEVICE g_bootDevices[];
extern OAL_KITL_DEVICE g_kitlDevices[];

extern UINT32   g_ulFlashBase;

//------------------------------------------------------------------------------

VOID OEMMultiBinNotify(MultiBINInfo *pInfo);

//------------------------------------------------------------------------------

VOID   BLMenu(BOOL bForced);    
BOOL   BLReadBootCfg(BOOT_CFG *pBootCfg);
BOOL   BLWriteBootCfg(BOOT_CFG *pBootCfg);
BOOL   BLReserveBootBlocks();
BOOL   BLConfigureFlashPartitions(BOOL bForceEnable);
BOOL   BLShowLogo();
UINT32 BLEthDownload(BOOT_CFG *pBootCfg, OAL_KITL_DEVICE *pBootDevices);
BOOL   BLEthReadData(ULONG size, UCHAR *pData);
VOID   BLEthConfig(BSP_ARGS *pArgs);
UINT32 BLSDCardDownload(WCHAR *filename);
BOOL   BLSDCardReadData(ULONG size, UCHAR *pData);
UINT32 BLFlashDownload(BOOT_CFG *pConfig, OAL_KITL_DEVICE *pBootDevices);
BOOL   BLSDCardReadLogo(WCHAR *filename, UCHAR *pData, DWORD size);
//UINT32 BLVAtoPA(UINT32 address);

UINT32 OALStringToUINT32(LPCWSTR psz);

//------------------------------------------------------------------------------

#endif

//------------------------------------------------------------------------------
//
//  Below define Fuel Gauge With Direct Battery BQ27410
//
#ifndef __BQ27410_H
#define __BQ27410_H

#define BQ27410_SLAVE_ADDRESS	0x55 

// Fuel gauge registers
#define bq27410CMD_CNTL_LSB  0x00
#define bq27410CMD_CNTL_MSB  0x01
#define bq27410CMD_TEMP_LSB  0x02
#define bq27410CMD_TEMP_MSB  0x03
#define bq27410CMD_VOLT_LSB  0x04
#define bq27410CMD_VOLT_MSB  0x05
#define bq27410CMD_FLAGS_LSB 0x06
#define bq27410CMD_FLAGS_MSB 0x07
#define bq27410CMD_NAC_LSB   0x08
#define bq27410CMD_NAC_MSB   0x09
#define bq27410CMD_FAC_LSB   0x0a
#define bq27410CMD_FAC_MSB   0x0b
#define bq27410CMD_RM_LSB    0x0c
#define bq27410CMD_RM_MSB    0x0d
#define bq27410CMD_FCC_LSB   0x0e
#define bq27410CMD_FCC_MSB   0x0f
#define bq27410CMD_AI_LSB    0x10
#define bq27410CMD_AI_MSB    0x11
#define bq27410CMD_SI_LSB    0x12
#define bq27410CMD_SI_MSB    0x13
#define bq27410CMD_MLI_LSB   0x14
#define bq27410CMD_MLI_MSB   0x15
#define bq27410CMD_AE_LSB    0x16
#define bq27410CMD_AE_MSB    0x17
#define bq27410CMD_AP_LSB    0x18
#define bq27410CMD_AP_MSB    0x19
#define bq27410CMD_SOC_LSB   0x1c
#define bq27410CMD_SOC_MSB   0x1d
#define bq27410CMD_ITEMP_LSB 0x1e
#define bq27410CMD_ITEMP_MSB 0x1f
#define bq27410CMD_SCH_LSB   0x20
#define bq27410CMD_SCH_MSB   0x21
#define bq27410CMD_OPCFG_LSB 0x3a
#define bq27410CMD_OPCFG_MSB 0x3b
#define bq27410CMD_DCAP_LSB  0x3c
#define bq27410CMD_DCAP_MSB  0x3d
#define bq27410CMD_DFCLS     0x3e
#define bq27410CMD_DFBLK     0x3f
#define bq27410CMD_DFD       0x40
#define bq27410CMD_DFDCKS    0x60
#define bq27410CMD_DFDCNTL   0x61
#define bq27410CMD_DNAMELEN  0x62
#define bq27410CMD_DNAME     0x63


// 0x14-0x6D are reserved XXXXX
#define BQ_EE_CMD           0x6E
#define BQ_EE_ILMD          0x76
#define BQ_EE_SEDVF         0x77
#define BQ_EE_SEDV1         0x78
#define BQ_EE_ISLC          0x79
#define BQ_EE_DMFSD         0x7A
#define BQ_EE_TAPER         0x7B
#define BQ_EE_PKCFG         0x7C
#define BQ_EE_ID3           0x7D
#define BQ_EE_DCOMP         0x7E
#define BQ_EE_TCOMP         0x7F

// BQ flags register bits
#define BQ_FLAGS_BAT_DET	(1 << 3)
#define BQ_FLAGS_SOC1		(1 << 2)
#define BQ_FLAGS_SOCF		(1 << 1)
#define BQ_FLAGS_DSG		(1 << 0)

#endif


