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
#include <PCIReg.h>
#include <x86kitl.h>

// BUGBUG: STUBS, for NOW
#pragma warning(disable:4026)
#define WrapFunc(type, funcname, funcptr) \
    type __declspec(naked) funcname ()  { \
        __asm { mov eax, [g_pX86KitlInfo] } \
        __asm { jmp [eax].funcptr } \
    }

WrapFunc (BOOL, PCIReadBARs, pfnPCIReadBARs)
WrapFunc (BOOL, PCIReg, pfnPCIReg)
WrapFunc (void, PCIInitInfo, pfnPCIInitInfo)
WrapFunc (ULONG, PCIGetBusDataByOffset, pfnPCIGetBusDataByOffset)
WrapFunc (ULONG, PCIReadBusData, pfnPCIReadBusData)
WrapFunc (ULONG, PCIWriteBusData, pfnPCIWriteBusData)


