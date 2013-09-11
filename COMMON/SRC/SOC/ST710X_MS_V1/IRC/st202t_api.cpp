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
/*++

Module Name:

Abstract:

    API implementation for IR PDD for STMicro ST202T SoC (common code).

Notes:
--*/
#include <windows.h>
#include <types.h>
#include <ceddk.h>

#include <ddkreg.h>
#include <ircdebug.h>
#include <st202t_ir.h>

BOOL
ST202T_IR::Open()
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // we only allow driver to simultanously be open by one client
    //

    LONG bWasOpen = InterlockedExchange(&m_bCurrentlyOpen, 1);

    if ( bWasOpen )
    {
        dwError = ERROR_SHARING_VIOLATION;
        goto ExitPoint;
    }

    // initialize instance values from global defaults
    m_dwRxWaterMark = g_DefaultTxWatermark;
    m_dwTxWaterMark = g_DefaultRxWatermark;
    m_bWakeupOnRemote = g_DefaultWakeupOnRx;

    // Receiver will be disabled until IOCTL_RC_SET_MODE is called
    m_bModeSet = FALSE;

ExitPoint:

    if (dwError)
    {
        if (!bWasOpen)
        {
            m_bCurrentlyOpen = 0;
        }
    }

    return dwError == ERROR_SUCCESS;
}

BOOL
ST202T_IR::Close()
{
    LONG bWasOpen = InterlockedExchange(&m_bCurrentlyOpen, 0);

    CancelReceive();

    m_bModeSet = FALSE;

    InitReceive(FALSE);

    return bWasOpen != 0;
}

DWORD
ST202T_IR::Read(
    PBYTE pBuffer,
    DWORD Count,
    PDWORD pActualOut)
{
    assert( pActualOut );
    static LONG ExclusiveReader = 0;
    DWORD retVal = ERROR_SUCCESS;

    // can't read until mode has been set and receiver initialized
    if (!m_bModeSet)
    {
        return ERROR_NOT_READY;
    }

    // we need a buffer to write into
    if (pBuffer == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (Count < m_pRxMsg->MessageSize())
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // protect the reader from concurrent read calls. it is designed for non-re-entrancy.

    LONG ReaderBusy = InterlockedExchange(&ExclusiveReader, 1);

    if (ReaderBusy)
    {
        return ERROR_SHARING_VIOLATION;
    }

    for (int i = 0; m_pRxMsg->IsEmpty() && (i++ < 2); )
    {
        //
        // if there is no data in the queue, wait until something comes in.
        // We'll retry this once in case the event remained signaled from last
        // time we fetched data.
        //
        if (WAIT_OBJECT_0 != WaitForSingleObject( m_hRxEvent, INFINITE))
        {
            retVal = ERROR_NOT_READY;
            goto LExit;
        }
    }

    //
    // read the data from the message queue
    //
    *pActualOut = m_pRxMsg->GetMsgs(pBuffer, Count);

    retVal = *pActualOut ? ERROR_SUCCESS : ERROR_NOT_READY;

LExit:

    ExclusiveReader = 0;

    return retVal;
}

DWORD
ST202T_IR::IOControl(
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL bLocalErrFound = FALSE;

    // validat the control code is intended for us. this is a good way to verify
    // the application used the correct constant definitions

    DWORD CtlDeviceType = (dwCode >> 16) & 0xFFFF;
    if (CtlDeviceType != FILE_DEVICE_IRC_PORT)
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch ((dwCode >> 2) & 0x0FFF)
    {
    case IRC_IOCTL_CODE_GET_CAPABILITIES:
        //
        // whether successful or not, let caller know output buffer size
        //
        if (pdwActualOut != NULL)
        {
            *pdwActualOut = sizeof(IRC_Capabilities);
        }

        if ( !m_pfnGetCapabilities( (pIRC_Capabilities)pBufOut, dwLenOut, 0 ))
        {
            dwError = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        break;

    case IRC_IOCTL_CODE_SET_MODE:
        if ((pBufIn == NULL) || (dwLenIn < sizeof(IRC_Mode)))
        {
            dwError = ERROR_INVALID_PARAMETER;
            break;
        }

        //
        // we do not allow changing the mode.
        //
        if (m_bModeSet)
        {
            dwError = ERROR_ALREADY_INITIALIZED;
            break;
        }

        m_hRxEvent = CreateEvent( NULL, FALSE, FALSE, NULL);
        if (m_hRxEvent == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto ExitSetMode;
        }

        //
        // Initialize the protocol handler
        //
        if ( !m_pfnSetMode((pIRC_Mode)pBufIn, 0) )
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto ExitSetMode;
        }

        // The Rx interrupt handler needs to know how large of a buffer to
        // allocate for decoded data bytes.  The m_pfnDecode function provides
        // this information when it is called with an invalid buffer length.
        m_dwBytesPerMessage = 0;
        m_pfnDecode(0, 0, NULL, &m_dwBytesPerMessage, FALSE, 0);

        if ( m_dwBytesPerMessage == 0 )
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto ExitSetMode;
        }

        m_pRxMsg = new CMsgQueue( m_dwBytesPerMessage, g_MsgBufCount);

        if ( !m_pRxMsg || !m_pRxMsg->Init())
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto ExitSetMode;
        }

        m_bModeSet = TRUE;

        InitReceive( TRUE );

ExitSetMode:
        if (dwError)
        {
            if (m_pRxMsg)
            {
                delete m_pRxMsg;
                m_pRxMsg = NULL;
            }

            if (m_hRxEvent)
            {
                CloseHandle( m_hRxEvent );
                m_hRxEvent = NULL;
            }
        }

        break;

    case IRC_IOCTL_CODE_GET_ERROR_STATS:
        {
        //
        // whether successful or not, let caller know output buffer size
        //
        if (pdwActualOut != NULL)
        {
            *pdwActualOut = sizeof(IRC_Error);
        }

        if ((pBufOut == NULL) || (dwLenOut < sizeof(IRC_Error)))
        {
            dwError = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        pIRC_Error pErr = (pIRC_Error)pBufOut;
        ZeroMemory(pErr, sizeof(IRC_Error));
        pErr->dwUnderrunErrorCount = m_dwUnderrunErrorCount;
        pErr->dwOverrunErrorCount = m_dwOverrunErrorCount;
        pErr->dwDecodingErrorCount = m_dwDecodingErrorCount;
        break;
        }

    case IRC_IOCTL_CODE_CONFIGURE_PORT:
        if ((pBufIn == NULL) || (dwLenIn < sizeof(IRC_Config)))
        {
            dwError = ERROR_INVALID_PARAMETER;
            break;
        }

        bLocalErrFound = FALSE;
        for (pIRC_Config pConfig = (pIRC_Config)pBufIn;
             dwLenIn >= sizeof(IRC_Config) && !bLocalErrFound;
             dwLenIn -= sizeof(IRC_Config), pConfig++)
        {
            switch (pConfig->dwParam)
            {
            case IRC_CONFIG_RX_WATERMARK :

                // Only go through configuration if the value changed
                if (m_dwRxWaterMark != pConfig->dwValue)
                {
                    // is the new value legal?
                    if (!IS_VALID_IRC_INTERRUPT_WATERMARK(pConfig->dwValue))
                    {
                        dwError = ERROR_INVALID_PARAMETER;
                        bLocalErrFound = TRUE;
                        break;
                    }

                    // update configuration
                    m_dwRxWaterMark = pConfig->dwValue;

                    // if the receiver is already enabled, also change in HW
                    if (!m_bModeSet)
                    {
                        break;
                    }

                    ULONG ulData = m_pRegST202T_IR->Read_RX_INTEN();
                    ulData &= ~STIR_RX_INT_WATERMARK_MASK;
                    ulData |= WatermarkBits[m_dwRxWaterMark];
                    m_pRegST202T_IR->Write_RX_INTEN(ulData);
                }
                break;

            case IRC_CONFIG_TX_WATERMARK :
                if (m_dwTxWaterMark != pConfig->dwValue)
                {
                    if (!IS_VALID_IRC_INTERRUPT_WATERMARK(pConfig->dwValue))
                    {
                        dwError = ERROR_INVALID_PARAMETER;
                        bLocalErrFound = TRUE;
                        break;
                    }
                    m_dwTxWaterMark = pConfig->dwValue;

#if ST202T_XMIT
                    ULONG ulData = m_pRegST202T_IR->Read_TX_INTEN();
                    ulData &= ~STIR_TX_INT_WATERMARK_MASK;
                    ulData |= WatermarkBits[m_dwTxWaterMark];
                    m_pRegST202T_IR->Write_TX_INTEN(ulData);
#endif
                }
                break;

            case IRC_CONFIG_WAKEUP_ON_REMOTE :
                if (m_bWakeupOnRemote != !!pConfig->dwValue)
                {
                    m_bWakeupOnRemote = !!pConfig->dwValue;
//***************
// BUGBUG                    XXXX update
//***************
                }
                break;

            default:
                dwError = ERROR_INVALID_PARAMETER;
                bLocalErrFound = TRUE;
            }
        }

        break;

    case IRC_IOCTL_CODE_GET_PORT_CONFIGURATION:
      {
        //
        // whether successful or not, let caller know output buffer size
        //
        if (pdwActualOut != NULL)
        {
            *pdwActualOut = sizeof(IRC_Config);
        }

        if ((pBufOut == NULL) || (dwLenOut < sizeof(IRC_Config)))
        {
            dwError = ERROR_INVALID_PARAMETER;
            break;
        }

        bLocalErrFound = FALSE;
        DWORD ActualOut = 0;
        for (pIRC_Config pConfig = (pIRC_Config)pBufOut;
             dwLenOut >= sizeof(IRC_Config) && !bLocalErrFound;
             dwLenOut -= sizeof(IRC_Config), pConfig++, ActualOut += sizeof(IRC_Config))
        {
            switch (pConfig->dwParam)
            {
            case IRC_CONFIG_RX_WATERMARK :
                pConfig->dwValue = m_dwRxWaterMark;
                break;

            case IRC_CONFIG_TX_WATERMARK :
                pConfig->dwValue = m_dwTxWaterMark;
                break;

            case IRC_CONFIG_WAKEUP_ON_REMOTE :
                pConfig->dwValue = m_bWakeupOnRemote;
                break;

            default:
                dwError = ERROR_INVALID_PARAMETER;
                bLocalErrFound = TRUE;
            }
        }

        if (pdwActualOut != NULL)
        {
            *pdwActualOut = ActualOut;
        }

        break;
      }

    default:
        dwError = ERROR_INVALID_PARAMETER;
    }

    return dwError;
}


