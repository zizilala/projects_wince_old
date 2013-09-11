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

// File: IRC_Ioctl.h
//
// This file includes function protototypes for the functions which are
// exported by the remote control protocol handlers.

#ifndef __RC_Ioctl_H_
#define __RC_Ioctl_H_

#include <windows.h>
#include <types.h>
#include <ceddk.h>

//
// Supported protocols
//
#define IRC_PROTOCOL_RCMM                1

#define IRC_PROTOCOL_RCMM_MODE_BASIC     1
#define IRC_PROTOCOL_RCMM_MODE_OEM_SHORT 2
#define IRC_PROTOCOL_RCMM_MODE_OEM_LONG  4

#define IRC_PROTOCOL_RC6                 2
#define IRC_PROTOCOL_RC6_MODE_0          1

#define IRC_PROTOCOL_RC5                 4
#define IRC_PROTOCOL_RC5_MODE_BASIC      1

//
// Other protocols are unsupported, but may be implemented by OEMs.
// Their defines should go here.
//

#define FILE_DEVICE_IRC_PORT            30000

//
// RC IOCTLs
//
#define IRC_IOCTL_CODE_GET_CAPABILITIES       1
#define IRC_IOCTL_CODE_SET_MODE               2
#define IRC_IOCTL_CODE_GET_ERROR_STATS        3
#define IRC_IOCTL_CODE_CONFIGURE_PORT         4
#define IRC_IOCTL_CODE_GET_PORT_CONFIGURATION 5

#define IOCTL_IRC_GET_CAPABILITIES          CTL_CODE(FILE_DEVICE_IRC_PORT, IRC_IOCTL_CODE_GET_CAPABILITIES, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IRC_SET_MODE                  CTL_CODE(FILE_DEVICE_IRC_PORT, IRC_IOCTL_CODE_SET_MODE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IRC_GET_ERROR_STATS           CTL_CODE(FILE_DEVICE_IRC_PORT, IRC_IOCTL_CODE_GET_ERROR_STATS, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IRC_CONFIGURE_PORT            CTL_CODE(FILE_DEVICE_IRC_PORT, IRC_IOCTL_CODE_CONFIGURE_PORT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IRC_GET_PORT_CONFIGURATION    CTL_CODE(FILE_DEVICE_IRC_PORT, IRC_IOCTL_CODE_GET_PORT_CONFIGURATION, METHOD_BUFFERED, FILE_ANY_ACCESS)

///// Structure Definitions

typedef struct
{
    DWORD   dwSupportedProtocol;        // Protocol supported
    DWORD   dwSupportedProtocolSubmode; // Protocol submode supported
    DWORD   dwProvSpec1;                // Provider Specific
    DWORD   dwProvSpec2;                // Provider Specific
} IRC_Capabilities, *pIRC_Capabilities;

typedef struct
    {
    DWORD   dwProtocol;                 // Protocol to use
    DWORD   dwProtocolSubmode;          // Protocol submode to use
    DWORD   dwProvSpec1;                // Provider Specific
    DWORD   dwProvSpec2;                // Provider Specific
} IRC_Mode, *pIRC_Mode;

typedef struct
{
    DWORD dwUnderrunErrorCount;
    DWORD dwOverrunErrorCount;
    DWORD dwDecodingErrorCount;
    DWORD dwProvSpec;                   // Provider specific
} IRC_Error, *pIRC_Error;

//
// Watermark values
//

#define IRC_INTERRUPT_WATERMARK_ONE   1
#define IRC_INTERRUPT_WATERMARK_HALF  2
#define IRC_INTERRUPT_WATERMARK_FULL  3

#define IS_VALID_IRC_INTERRUPT_WATERMARK(v) ((v)>=IRC_INTERRUPT_WATERMARK_ONE ||\
                                             (v)<=IRC_INTERRUPT_WATERMARK_FULL)

//
// Settings configurable by application
//

#define IRC_CONFIG_RX_WATERMARK      1
#define IRC_CONFIG_TX_WATERMARK      2
#define IRC_CONFIG_WAKEUP_ON_REMOTE  3

typedef struct
{
    DWORD   dwParam;              // configuration parameter code
    DWORD   dwValue;              // data
} IRC_Config, *pIRC_Config;

#endif //__RC_Ioctl_H_

