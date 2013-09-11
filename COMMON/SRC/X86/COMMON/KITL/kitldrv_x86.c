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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//  
// -----------------------------------------------------------------------------
#include <windows.h>
#include <x86kitl.h>

static OAL_KITL_ETH_DRIVER DrvNE2k  = OAL_ETHDRV_NE2000;     // NE2000 
static OAL_KITL_ETH_DRIVER DrvRTL   = OAL_ETHDRV_RTL8139;    // RTL8139
static OAL_KITL_ETH_DRIVER DrvDP    = OAL_ETHDRV_DP83815;    // DP83815
static OAL_KITL_ETH_DRIVER DrvRndis = OAL_ETHDRV_RNDIS;      // RNDIS
static OAL_KITL_ETH_DRIVER Drv3C90  = OAL_ETHDRV_3C90X;      // 3C90X


const SUPPORTED_NIC g_NicSupported []=
{
//   VenId   DevId   UpperMAC      Type              Name   Drivers
//  ---------------------------------------------------------------------------------
    {0x0000, 0x0000, 0x004033, EDBG_ADAPTER_NE2000,  "AD", &DrvNE2k  }, /* Addtron */\
    {0x1050, 0x0940, 0x004005, EDBG_ADAPTER_NE2000,  "LS", &DrvNE2k  }, /* LinkSys */\
    {0x1050, 0x0940, 0x002078, EDBG_ADAPTER_NE2000,  "LS", &DrvNE2k  }, /* LinkSys */\
    {0x10EC, 0x8029, 0x00C0F0, EDBG_ADAPTER_NE2000,  "KS", &DrvNE2k  }, /* Kingston */\
    {0x10EC, 0x8129, 0x000000, EDBG_ADAPTER_RTL8139, "RT", &DrvRTL   }, /* RealTek */\
    {0x10EC, 0x8139, 0x00900B, EDBG_ADAPTER_RTL8139, "RT", &DrvRTL   }, /* RealTek  */\
    {0x10EC, 0x8139, 0x00D0C9, EDBG_ADAPTER_RTL8139, "RT", &DrvRTL   }, /* RealTek */\
    {0x10EC, 0x8139, 0x00E04C, EDBG_ADAPTER_RTL8139, "RT", &DrvRTL   }, /* RealTek */\
    {0x1186, 0x1300, 0x0050BA, EDBG_ADAPTER_RTL8139, "DL", &DrvRTL   }, /* D-Link */\
    {0x100B, 0x0020, 0x00A0CC, EDBG_ADAPTER_DP83815, "NG", &DrvDP    }, /* Netgear */\
    {0x10B7, 0x9050, 0x006008, EDBG_ADAPTER_3C90X,   "3C", &Drv3C90  }, /* 3Com */\
    {0x10B7, 0x9200, 0x000476, EDBG_ADAPTER_3C90X,   "3C", &Drv3C90  }, /* 3Com */
    {0x10b5, 0x9054, 0x00800f, EDBG_USB_RNDIS,       "NC", &DrvRndis }, /* NetChip */
};

const int g_nNumNicSupported = sizeof (g_NicSupported) / sizeof (g_NicSupported[0]);

