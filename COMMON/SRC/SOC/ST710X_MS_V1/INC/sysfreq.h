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
/*++

Module Name:
    sysfreq.h

Abstract:
    Contains definitions for system frequency informational functions

Notes:

--*/

#ifndef   __SYSFREQ_H__
#define   __SYSFREQ_H__

typedef struct STB710X_SYSFREQS_T
{
    DWORD PLL0;
    DWORD PLL1;
    DWORD CLK_ST40_CPU;
    DWORD CLK_ST40_BUS;
    DWORD CLK_ST40_PERIPHERAL;
    DWORD EMI_BUS;
    DWORD LMI_BUS;
} STB710X_SYSFREQS, *pSTB710X_SYSFREQS;

#define IOCTL_HAL_GET_SYS_FREQ    CTL_CODE(FILE_DEVICE_HAL, 1000, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif    __SYSFREQ_H__
