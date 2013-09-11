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
//
// FILE: stb7100reg.h
//

#ifndef __STB7100REG_H
#define __STB7100REG_H

//------------------------------------------------------------------------------
// STb7100 system registers
//------------------------------------------------------------------------------

// See chapter 18 in "7100_DS 25-aug-05.pdf"

#define STB7100_SYSCONF_REGS_BASE           0xB9001000
#define STB7100_CLOCKGENA_REGS_BASE         0xB9213000

// Clock Generator control registers (STb7100 variant)
#define STB7100_CLOCKGENA_LOCK              (STB7100_CLOCKGENA_REGS_BASE + 0x00)
#define STB7100_CLOCKGENA_MD_STATUS         (STB7100_CLOCKGENA_REGS_BASE + 0x04)
#define STB7100_CLOCKGENA_PLL0_CFG          (STB7100_CLOCKGENA_REGS_BASE + 0x08)
#define STB7100_CLOCKGENA_PLL0_STATUS       (STB7100_CLOCKGENA_REGS_BASE + 0x10)
#define STB7100_CLOCKGENA_PLL0_CLK1_CTRL    (STB7100_CLOCKGENA_REGS_BASE + 0x14)
#define STB7100_CLOCKGENA_PLL0_CLK2_CTRL    (STB7100_CLOCKGENA_REGS_BASE + 0x18)
#define STB7100_CLOCKGENA_PLL0_CLK3_CTRL    (STB7100_CLOCKGENA_REGS_BASE + 0x1c)
#define STB7100_CLOCKGENA_PLL0_CLK4_CTRL    (STB7100_CLOCKGENA_REGS_BASE + 0x20)
#define STB7100_CLOCKGENA_PLL1_CFG          (STB7100_CLOCKGENA_REGS_BASE + 0x24)
#define STB7100_CLOCKGENA_PLL1_STATUS       (STB7100_CLOCKGENA_REGS_BASE + 0x2c)
#define STB7100_CLOCKGENA_CLK_DIV           (STB7100_CLOCKGENA_REGS_BASE + 0x30)
#define STB7100_CLOCKGENA_CLOCK_ENABLE      (STB7100_CLOCKGENA_REGS_BASE + 0x34)
#define STB7100_CLOCKGENA_OUT_CTRL          (STB7100_CLOCKGENA_REGS_BASE + 0x38)
#define STB7100_CLOCKGENA_PLL1_BYPASS       (STB7100_CLOCKGENA_REGS_BASE + 0x3c)

typedef struct {
    UINT32 Lock;                            // (0x00) Clockgen A Lock
    UINT32 ModeStatus;                      // (0x04) Mode pins status
    UINT32 PLL0_Cfg;                        // (0x08) PLL0 Configuration
    UINT32 Reserved1;                       // (0x0C) Reserved
    UINT32 PLL0_Lock_Stat;                  // (0x10) PLL0 Lock Status
    UINT32 PLL0_Clock1;                     // (0x14) CLK_ST40 ratio
    UINT32 PLL0_Clock2;                     // (0x18) CLK_ST40_IC ratio
    UINT32 PLL0_Clock3;                     // (0x1C) CLK_ST40_PER ratio
    UINT32 PLL0_Clock4;                     // (0x20) CLK_FDMA ratio
    UINT32 PLL1_Cfg;                        // (0x24) PLL1 Configuration
    UINT32 Reserved2;                       // (0x28) Reserved
    UINT32 PLL1_Lock_Stat;                  // (0x2C) PLL1 Lock Status
    UINT32 Clocks_Division;                 // (0x30) Clocks division
    UINT32 Clocks_Enable;                   // (0x34) Clocks enabling/stopping
    UINT32 ClockOut_Select;                 // (0x38) SYSCLKOUT clock source selection.
    UINT32 PLL1_Bypass;                     // (0x3C) PLL1 bypass and FDMA clock source.
} STB7100_CLOCKGENA_REG, *pSTB7100_CLOCKGENA_REG;

// System configuration registers (STb7100 variant)
#define STB7100_SYSCONF_DEVICEID_0          (STB7100_SYSCONF_REGS_BASE + 0x0000)
#define STB7100_SYSCONF_DEVICEID_1          (STB7100_SYSCONF_REGS_BASE + 0x0004)
#define STB7100_SYSCONF_DEVICEID            STB7100_SYSCONF_DEVICEID_0

#define STB7100_SYSCONF_SYS_STA00           (STB7100_SYSCONF_REGS_BASE + 0x0008)
#define STB7100_SYSCONF_SYS_STA01           (STB7100_SYSCONF_REGS_BASE + 0x000c)
#define STB7100_SYSCONF_SYS_STA02           (STB7100_SYSCONF_REGS_BASE + 0x0010)
#define STB7100_SYSCONF_SYS_STA03           (STB7100_SYSCONF_REGS_BASE + 0x0014)
#define STB7100_SYSCONF_SYS_STA04           (STB7100_SYSCONF_REGS_BASE + 0x0018)
#define STB7100_SYSCONF_SYS_STA05           (STB7100_SYSCONF_REGS_BASE + 0x001c)
#define STB7100_SYSCONF_SYS_STA06           (STB7100_SYSCONF_REGS_BASE + 0x0020)
#define STB7100_SYSCONF_SYS_STA07           (STB7100_SYSCONF_REGS_BASE + 0x0024)
#define STB7100_SYSCONF_SYS_STA08           (STB7100_SYSCONF_REGS_BASE + 0x0028)
#define STB7100_SYSCONF_SYS_STA09           (STB7100_SYSCONF_REGS_BASE + 0x002c)
#define STB7100_SYSCONF_SYS_STA10           (STB7100_SYSCONF_REGS_BASE + 0x0030)
#define STB7100_SYSCONF_SYS_STA11           (STB7100_SYSCONF_REGS_BASE + 0x0034)
#define STB7100_SYSCONF_SYS_STA12           (STB7100_SYSCONF_REGS_BASE + 0x0038)
#define STB7100_SYSCONF_SYS_STA13           (STB7100_SYSCONF_REGS_BASE + 0x003c)
#define STB7100_SYSCONF_SYS_STA14           (STB7100_SYSCONF_REGS_BASE + 0x0040)
#define STB7100_SYSCONF_SYS_CFG00           (STB7100_SYSCONF_REGS_BASE + 0x0100)
#define STB7100_SYSCONF_SYS_CFG01           (STB7100_SYSCONF_REGS_BASE + 0x0104)
#define STB7100_SYSCONF_SYS_CFG02           (STB7100_SYSCONF_REGS_BASE + 0x0108)
#define STB7100_SYSCONF_SYS_CFG03           (STB7100_SYSCONF_REGS_BASE + 0x010c)
#define STB7100_SYSCONF_SYS_CFG04           (STB7100_SYSCONF_REGS_BASE + 0x0110)
#define STB7100_SYSCONF_SYS_CFG05           (STB7100_SYSCONF_REGS_BASE + 0x0114)
#define STB7100_SYSCONF_SYS_CFG06           (STB7100_SYSCONF_REGS_BASE + 0x0118)
#define STB7100_SYSCONF_SYS_CFG07           (STB7100_SYSCONF_REGS_BASE + 0x011c)
#define STB7100_SYSCONF_SYS_CFG08           (STB7100_SYSCONF_REGS_BASE + 0x0120)
#define STB7100_SYSCONF_SYS_CFG09           (STB7100_SYSCONF_REGS_BASE + 0x0124)
#define STB7100_SYSCONF_SYS_CFG10           (STB7100_SYSCONF_REGS_BASE + 0x0128)
#define STB7100_SYSCONF_SYS_CFG11           (STB7100_SYSCONF_REGS_BASE + 0x012c)
#define STB7100_SYSCONF_SYS_CFG12           (STB7100_SYSCONF_REGS_BASE + 0x0130)
#define STB7100_SYSCONF_SYS_CFG13           (STB7100_SYSCONF_REGS_BASE + 0x0134)
#define STB7100_SYSCONF_SYS_CFG14           (STB7100_SYSCONF_REGS_BASE + 0x0138)
#define STB7100_SYSCONF_SYS_CFG15           (STB7100_SYSCONF_REGS_BASE + 0x013c)
#define STB7100_SYSCONF_SYS_CFG16           (STB7100_SYSCONF_REGS_BASE + 0x0140)
#define STB7100_SYSCONF_SYS_CFG17           (STB7100_SYSCONF_REGS_BASE + 0x0144)
#define STB7100_SYSCONF_SYS_CFG18           (STB7100_SYSCONF_REGS_BASE + 0x0148)
#define STB7100_SYSCONF_SYS_CFG19           (STB7100_SYSCONF_REGS_BASE + 0x014c)
#define STB7100_SYSCONF_SYS_CFG20           (STB7100_SYSCONF_REGS_BASE + 0x0150)
#define STB7100_SYSCONF_SYS_CFG21           (STB7100_SYSCONF_REGS_BASE + 0x0154)
#define STB7100_SYSCONF_SYS_CFG22           (STB7100_SYSCONF_REGS_BASE + 0x0158)
#define STB7100_SYSCONF_SYS_CFG23           (STB7100_SYSCONF_REGS_BASE + 0x015c)
#define STB7100_SYSCONF_SYS_CFG24           (STB7100_SYSCONF_REGS_BASE + 0x0160)
#define STB7100_SYSCONF_SYS_CFG25           (STB7100_SYSCONF_REGS_BASE + 0x0164)
#define STB7100_SYSCONF_SYS_CFG26           (STB7100_SYSCONF_REGS_BASE + 0x0168)
#define STB7100_SYSCONF_SYS_CFG27           (STB7100_SYSCONF_REGS_BASE + 0x016c)
#define STB7100_SYSCONF_SYS_CFG28           (STB7100_SYSCONF_REGS_BASE + 0x0170)
#define STB7100_SYSCONF_SYS_CFG29           (STB7100_SYSCONF_REGS_BASE + 0x0174)
#define STB7100_SYSCONF_SYS_CFG30           (STB7100_SYSCONF_REGS_BASE + 0x0178)
#define STB7100_SYSCONF_SYS_CFG31           (STB7100_SYSCONF_REGS_BASE + 0x017c)
#define STB7100_SYSCONF_SYS_CFG32           (STB7100_SYSCONF_REGS_BASE + 0x0180)
#define STB7100_SYSCONF_SYS_CFG33           (STB7100_SYSCONF_REGS_BASE + 0x0184)
#define STB7100_SYSCONF_SYS_CFG34           (STB7100_SYSCONF_REGS_BASE + 0x0188)
#define STB7100_SYSCONF_SYS_CFG35           (STB7100_SYSCONF_REGS_BASE + 0x018c)
#define STB7100_SYSCONF_SYS_CFG36           (STB7100_SYSCONF_REGS_BASE + 0x0190)
#define STB7100_SYSCONF_SYS_CFG37           (STB7100_SYSCONF_REGS_BASE + 0x0194)
#define STB7100_SYSCONF_SYS_CFG38           (STB7100_SYSCONF_REGS_BASE + 0x0198)
#define STB7100_SYSCONF_SYS_CFG39           (STB7100_SYSCONF_REGS_BASE + 0x019c)

// MAC configuration registers
#define MCSR_BASE_ADDRESS         0xb8110000
#define MCSR1_MAC_ADDR_HI         0x04
#define MCSR2_MAC_ADDR_LO         0x08
#endif /* __STB7100REG_H */
