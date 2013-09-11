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

// File: RC_ProtocolHandler.h
//
// This file includes function protototypes for the functions which are
// exported by the remote control protocol handlers.

#ifndef __RC_ProtocolHandler_H_
#define __RC_ProtocolHandler_H_

#include <windows.h>
#include <types.h>
#include <ceddk.h>
#include "IRC_Ioctl.h"

#ifndef INOUT
#define INOUT
#endif

typedef struct
{
    DWORD   dwFrequency;                // Subcarrier frequency for this protocol
    DWORD   dwDutyCycle;                // Subcarrier duty cycle for this protocol
    DWORD   dwProvSpec;                 // Provider Specific
} IRC_SignalDesc, *pIRC_SignalDesc;

///// Function prototypes

BOOL RC_Decode
(
    IN      DWORD  dwOnTime,
    IN      DWORD  dwSymbolPeriod,
    IN      PBYTE  pbRxBuf,
    INOUT   PDWORD pdwRxBufLen,
    IN      BOOL   bResetState,
    INOUT   DWORD  dwProvSpec
);

typedef BOOL (* pfnRC_Decode)
(
    IN      DWORD  dwOnTime,
    IN      DWORD  dwSymbolPeriod,
    IN      PBYTE  pbRxBuf,
    INOUT   PDWORD pdwRxBufLen,
    IN      BOOL   bResetState,
    INOUT   DWORD  dwProvSpec
);

BOOL RC_Encode
(
    IN      PBYTE  pbXmitBuf,
    INOUT   PDWORD pdwXmitBufLen,
    IN      PDWORD pdwOnTimeBuf,
    INOUT   PDWORD pdwOnTimeBufLen,
    IN      PDWORD pdwPeriodBuf,
    INOUT   PDWORD pdwPeriodBufLen,
    INOUT   DWORD  dwProvSpec
);

typedef BOOL (* pfnRC_Encode)
(
    IN      PBYTE  pbXmitBuf,
    INOUT   PDWORD pdwXmitBufLen,
    IN      PDWORD pdwOnTimeBuf,
    INOUT   PDWORD pdwOnTimeBufLen,
    IN      PDWORD pdwPeriodBuf,
    INOUT   PDWORD pdwPeriodBufLen,
    INOUT   DWORD  dwProvSpec
);

BOOL RC_GetCapabilities
(
    IN      pIRC_Capabilities   pCapabilities,
    IN      DWORD               dwBufLen,
    INOUT   DWORD               dwProvSpec
);

typedef BOOL (* pfnRC_GetCapabilities)
(
    IN      pIRC_Capabilities   pCapabilities,
    IN      DWORD               dwBufLen,
    INOUT   DWORD               dwProvSpec
);

BOOL RC_SetMode
(
    IN      pIRC_Mode           pMode,
    INOUT   DWORD               dwProvSpec
);

typedef BOOL (* pfnRC_SetMode)
(
    IN      pIRC_Mode           pMode,
    INOUT   DWORD               dwProvSpec
);

BOOL RC_GetSignalProperties
(
    IN      pIRC_SignalDesc     pSignalDesc,
    INOUT   PDWORD              pdwBufLen,
    INOUT   DWORD               dwProvSpec
);

typedef BOOL (* pfnRC_GetSignalProperties)
(
    IN      pIRC_SignalDesc     pSignalDesc,
    INOUT   PDWORD              pdwBufLen,
    INOUT   DWORD               dwProvSpec
);

#endif // __RC_ProtocolHandler_H
