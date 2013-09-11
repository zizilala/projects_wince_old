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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
//------------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  
//------------------------------------------------------------------------------
#ifndef _KITLETH_X86_H_
#define _KITLETH_X86_H_

#include <oal_ethdrv.h>
#include <ceddk.h>
#include <oal_kitl.h>
#include <pcireg.h>
#include <x86boot.h>

typedef struct KTIL_NIC_INFO {
    DWORD               dwIrq;          // IRQ
    DWORD               dwIoBase;       // IO Base
    DWORD               dwBus;          // bus number
    DWORD               dwDevice;       // device number
    DWORD               dwFunction;     // function number
    DWORD               dwType;         // adaptor type
    PCI_COMMON_CONFIG   pciConfig;      // pciConfig information
    const OAL_KITL_ETH_DRIVER *pDriver; // the KITL driver
} KTIL_NIC_INFO, *PKTIL_NIC_INFO;

typedef const KTIL_NIC_INFO *PCKITL_NIC_INFO;

//
// Ethernet debug controller vendor and PCI information.
//
typedef struct _SUPPORTED_NIC // NIC vendor ID
{
    USHORT wVenId;             // PCI Vendor ID
    USHORT wDevId;             // PCI Device ID
    DWORD  dwUpperMAC;         // 1st 3 bytes of mac address
    UCHAR  Type;               // adapter type
    UCHAR  szAbbrev[3];        // Vendor name abbreviation
    const OAL_KITL_ETH_DRIVER *pDriver; // corresponding driver
} SUPPORTED_NIC, *PSUPPORTED_NIC;

typedef const SUPPORTED_NIC *PCSUPPORTED_NIC;

PCKITL_NIC_INFO InitKitlNIC (DWORD dwIrq, DWORD dwIoBase, DWORD dwDfltType);
BOOL x86KitlCreateName(CHAR *pPrefix, UINT16 mac[], CHAR *pBuffer);

const OAL_KITL_SERIAL_DRIVER *GetKitlSerialDriver (void);

extern const SUPPORTED_NIC g_NicSupported [];
extern const int g_nNumNicSupported;

BOOL
PCIReadBARs(
    PPCI_REG_INFO pInfo
    );

// this is the structure that we use to pass informaiton between OAL and KITL
typedef struct {
    PX86BootInfo pX86Info;
    
    // BUGBUG:
    // temporary to get KITL linked. working on other way to do it correctly
    FARPROC pfnPCIReadBARs;
    FARPROC pfnPCIInitInfo;
    FARPROC pfnPCIReadBusData;
    FARPROC pfnPCIWriteBusData;
    FARPROC pfnPCIGetBusDataByOffset;
    FARPROC pfnPCIReg;
    
} X86KITLINFO, *PX86KITLINFO;

extern PX86KITLINFO g_pX86KitlInfo;

#define LEGACY_KITL_DEVICE_BYTEPATTERN 0xFF

#endif
