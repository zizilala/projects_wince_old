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
//------------------------------------------------------------------------------
//
//  File:  SH4202T_intc2_irq.h
//
//  This file defines names for IRQ. This names have no other role than make
//  code more readable. For SoC where device - IRQ mapping is defined by
//  silicon and it can't be changed by software.
//
#ifndef __SH4202T_INTC_IRQ_H
#define __SH4202T_INTC_IRQ_H

//------------------------------------------------------------------------------
// Hardware Supported Exception Codes (INTC2)
//------------------------------------------------------------------------------

#define IRQ_RESERVED                (-1)
#define IRQ_INTC2_MIN               EXCEPTIONCODETOIRQ(0xA00)

#define IRQ_PIO0                    EXCEPTIONCODETOIRQ(0xC00)
#define IRQ_PIO1                    EXCEPTIONCODETOIRQ(0xC80)
#define IRQ_PIO2                    EXCEPTIONCODETOIRQ(0xD00)
#define IRQ_PIO3                    EXCEPTIONCODETOIRQ(0x1060)
#define IRQ_PIO4                    EXCEPTIONCODETOIRQ(0x1040)
#define IRQ_PIO5                    EXCEPTIONCODETOIRQ(0x1020)
#define IRQ_SSC0                    EXCEPTIONCODETOIRQ(0x10E0)
#define IRQ_SSC1                    EXCEPTIONCODETOIRQ(0x10C0)
#define IRQ_SSC2                    EXCEPTIONCODETOIRQ(0x10A0)
#define IRQ_UART0                   EXCEPTIONCODETOIRQ(0x1160)
#define IRQ_UART1                   EXCEPTIONCODETOIRQ(0x1140)
#define IRQ_UART2                   EXCEPTIONCODETOIRQ(0x1120)
#define IRQ_UART3                   EXCEPTIONCODETOIRQ(0x1100)
#define IRQ_MAFE                    EXCEPTIONCODETOIRQ(0x11E0)
#define IRQ_PWM                     EXCEPTIONCODETOIRQ(0x11C0)
#define IRQ_IRB                     EXCEPTIONCODETOIRQ(0x11A0)
#define IRQ_IRB_WAKEUP              EXCEPTIONCODETOIRQ(0x1180)
#define IRQ_7109_ETH_MAC            EXCEPTIONCODETOIRQ(0x12A0)
#define IRQ_TTXT                    EXCEPTIONCODETOIRQ(0x1260)
#define IRQ_DAA                     EXCEPTIONCODETOIRQ(0x1240)
#define IRQ_DISEQC                  EXCEPTIONCODETOIRQ(0x1220)
#define IRQ_DCXO                    EXCEPTIONCODETOIRQ(0x1340)
#define IRQ_ST231_AUD               EXCEPTIONCODETOIRQ(0x1320)
#define IRQ_ST231_DELPHI            EXCEPTIONCODETOIRQ(0x1300)
#define IRQ_CPXM                    EXCEPTIONCODETOIRQ(0x13E0)
#define IRQ_I2S2SPDIF               EXCEPTIONCODETOIRQ(0x13C0)
#define IRQ_FDMA_GP0                EXCEPTIONCODETOIRQ(0x13A0)
#define IRQ_FDMA_MBOX               EXCEPTIONCODETOIRQ(0x1380)
#define IRQ_SPDIFPLYR               EXCEPTIONCODETOIRQ(0x1460)
#define IRQ_PCMRDR                  EXCEPTIONCODETOIRQ(0x1440)
#define IRQ_PCMPLYR1                EXCEPTIONCODETOIRQ(0x1420)
#define IRQ_PCMPLYR0                EXCEPTIONCODETOIRQ(0x1400)
#define IRQ_H264_MBE                EXCEPTIONCODETOIRQ(0x14E0)
#define IRQ_H264_PRE1               EXCEPTIONCODETOIRQ(0x14C0)
#define IRQ_H264_PRE0               EXCEPTIONCODETOIRQ(0x14A0)
#define IRQ_MPEG2                   EXCEPTIONCODETOIRQ(0x1480)
#define IRQ_VTG2                    EXCEPTIONCODETOIRQ(0x1560)
#define IRQ_VTG1                    EXCEPTIONCODETOIRQ(0x1540)
#define IRQ_LMU                     EXCEPTIONCODETOIRQ(0x1520)
#define IRQ_HDCP                    EXCEPTIONCODETOIRQ(0x15E0)
#define IRQ_HDMI                    EXCEPTIONCODETOIRQ(0x15C0)
#define IRQ_DVP                     EXCEPTIONCODETOIRQ(0x15A0)
#define IRQ_BLT                     EXCEPTIONCODETOIRQ(0x1580)
#define IRQ_PTI                     EXCEPTIONCODETOIRQ(0x1600)
#define IRQ_SATA                    EXCEPTIONCODETOIRQ(0x1740)
#define IRQ_EHCI                    EXCEPTIONCODETOIRQ(0x1720)
#define IRQ_OHCI                    EXCEPTIONCODETOIRQ(0x1700)

#define IRQ_INTC2_MAX               EXCEPTIONCODETOIRQ(0x17E0)

// Define the starting event code for priority group 0.
#define INTC2_GROUP0_BASECODE       (0x1000)

//------------------------------------------------------------------------------

typedef enum {
    INTC2_REGBANK_RESERVED  = -1,
    INTC2_REGBANK00         =  0,
    INTC2_REGBANK04         =  4,
    INTC2_REGBANK08         =  8
} INTC2_REGBANK;

typedef enum {
    INTC2_NOGROUP     = -1,
    INTC2_GROUP0      =  0,
    INTC2_GROUP1      =  1,
    INTC2_GROUP2      =  2,
    INTC2_GROUP3      =  3,
    INTC2_GROUP4      =  4,
    INTC2_GROUP5      =  5,
    INTC2_GROUP6      =  6,
    INTC2_GROUP7      =  7,
    INTC2_GROUP8      =  8,
    INTC2_GROUP9      =  9,
    INTC2_GROUP10     = 10,
    INTC2_GROUP11     = 11,
    INTC2_GROUP12     = 12,
    INTC2_GROUP13     = 13,
    INTC2_GROUP14     = 14,
    INTC2_GROUP15     = 15,
    INTC2_GROUPCOMPAT = 16
} INTC2_GROUPS;

//------------------------------------------------------------------------------

#define SH4202T_IRQ_MAXIMUM         (IRQ_INTC2_MAX + 1)
#define SH4202T_IRQ_PER_SYSINTR     (4)

//------------------------------------------------------------------------------

#endif // __SH4202T_INTC_IRQ_H
