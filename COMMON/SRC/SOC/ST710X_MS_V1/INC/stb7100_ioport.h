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
// FILE: stb7100_ioport.h
//

#ifndef __STB7100_IOPORT_H
#define __STB7100_IOPORT_H

//------------------------------------------------------------------------------
// STb7100 system registers
//------------------------------------------------------------------------------
#define STB7100_PIO0_REGS_BASE              0xb8020000
#define STB7100_PIO1_REGS_BASE              0xb8021000
#define STB7100_PIO2_REGS_BASE              0xb8022000
#define STB7100_PIO3_REGS_BASE              0xb8023000
#define STB7100_PIO4_REGS_BASE              0xb8024000
#define STB7100_PIO5_REGS_BASE              0xb8025000

typedef struct {
    UINT32 PnOUT;                           // (0x0) PIO Output
    UINT32 SET_PnOUT;                       // (0x4) Set bits of PnOUT
    UINT32 CLR_PnOUT;                       // (0x8) Clear bits of PnOUT
    UINT32 Reserved1;                       // (0xC - 0xF) Reserved
    UINT32 PnIN;                            // (0x10) PIO Input
    UINT32 Reserved2[3];                    // (0x14 - 0x1F) Reserved
    UINT32 PnC0;                            // (0x20) PIO configuration 0
    UINT32 SET_PnC0;                        // (0x24) Set bits of PnC0
    UINT32 CLR_PnC0;                        // (0x28) Clear bits of PnC0
    UINT32 Reserved3;                       // (0x2C) Reserved
    UINT32 PnC1;                            // (0x30) PIO configuration 1
    UINT32 SET_PnC1;                        // (0x34) Set bits of PnC1
    UINT32 CLR_PnC1;                        // (0x38) Clear bits of PnC1
    UINT32 Reserved4;                       // (0x3C) Reserved
    UINT32 PnC2;                            // (0x40) PIO configuration 2
    UINT32 SET_PnC2;                        // (0x44) Set bits of PnC2
    UINT32 CLR_PnC2;                        // (0x48) Clear bits of PnC2
    UINT32 Reserved5;                       // (0x4C) Reserved
    UINT32 PnCOMP;                          // (0x50) PIO input comparison
    UINT32 SET_PnCOMP;                      // (0x54) Set bits of PnCOMP
    UINT32 CLR_PnCOMP;                      // (0x58) Clear bits of PnCOMP
    UINT32 Reserved6;                       // (0x5C) Reserved
    UINT32 PnMASK;                          // (0x60) PIO input comparison mask
    UINT32 SET_PnMASK;                      // (0x64) Set bits of PnMASK
    UINT32 CLR_PnMASK;                      // (0x68) Clear bits of PnMASK

} STB7100_IOPORT_REG, *PSTB7100_IOPORT_REG;

#endif /* __STB7100_IOPORT_H */

