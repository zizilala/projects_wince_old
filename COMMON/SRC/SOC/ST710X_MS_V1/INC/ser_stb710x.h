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
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

Abstract:

    ASC serial PDD for ST Micro 710x development board.

Notes:
--*/
#ifndef __PDD_710x_SER_H_
#define __PDD_710x_SER_H_

#include "pdd_asc_ser.h"
#include <stb7100_ioport.h>


class CPdd710xSerial : public CPddASCSerial
{
public:
    CPdd710xSerial(LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj)
        : CPddASCSerial(lpActivePath, pMdd, pHwObj)
    {
        m_pIOPregs = NULL;
    }

    ~CPdd710xSerial() ;

    virtual BOOL Init();

    virtual void    SetDefaultConfiguration();
    virtual BOOL    InitModem(BOOL bInit);
    virtual void    SetRTS(BOOL bSet);
    virtual ULONG   GetModemStatus();

private:
    volatile STB7100_IOPORT_REG * m_pIOPregs;
    ULONG   m_ulCTSStatus;
};

#endif

