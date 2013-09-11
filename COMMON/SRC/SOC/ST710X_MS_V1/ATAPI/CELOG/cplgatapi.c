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
#define UNICODE 1
#include <windows.h>
#include <stdio.h>
#include <..\..\..\..\..\..\..\public\common\sdk\inc\celog.h>
#include <..\..\..\..\..\..\..\public\common\oak\drivers\block\atapi\atapiio.h>

// Add the following to the registry to enable this plugin:
// [HKEY_CURRENT_USER\Software\Microsoft\CeLog Reader\Plugins]
// "<path>\cplgatapi.dll"=""


#define CELID_ATAPI_BASE    (CELID_USER + 0x180)

#define CELID_ATAPI_STARTIOCTL          (CELID_ATAPI_BASE + 0)
#define CELID_ATAPI_COMPLETEIOCTL       (CELID_ATAPI_BASE + 1)
#define CELID_ATAPI_BASESTATUS          (CELID_ATAPI_BASE + 2)
#define CELID_ATAPI_IOCOMMAND           (CELID_ATAPI_BASE + 3)
#define CELID_ATAPI_WAITINTERRUPT       (CELID_ATAPI_BASE + 4)
#define CELID_ATAPI_INTERRUPTDONE       (CELID_ATAPI_BASE + 5)
#define CELID_ATAPI_ENABLEINTERRUPT     (CELID_ATAPI_BASE + 6)
#define CELID_ATAPI_WAITDRQ             (CELID_ATAPI_BASE + 7)
#define CELID_ATAPI_STATUSWAITDRQ       (CELID_ATAPI_BASE + 8)
#define CELID_ATAPI_STARTREADBUFFER     (CELID_ATAPI_BASE + 9)
#define CELID_ATAPI_COMPLETEREADBUFFER  (CELID_ATAPI_BASE + 10)
#define CELID_ATAPI_STARTWRITEBUFFER    (CELID_ATAPI_BASE + 11)
#define CELID_ATAPI_COMPLETEWRITEBUFFER (CELID_ATAPI_BASE + 12)
#define CELID_ATAPI_SETUPDMA_COPY       (CELID_ATAPI_BASE + 13)
#define CELID_ATAPI_WAITDMAINTERRUPT    (CELID_ATAPI_BASE + 14)
#define CELID_ATAPI_DMAINTERRUPTDONE    (CELID_ATAPI_BASE + 15)
#define CELID_ATAPI_SETUPDMA_ALIGNED    (CELID_ATAPI_BASE + 16)
#define CELID_ATAPI_SETDEVICEPOWER      (CELID_ATAPI_BASE + 17)
#define CELID_ATAPI_POWERCOMMAND        (CELID_ATAPI_BASE + 18)
#define CELID_ATAPI_LASTIDENTIFIER      CELID_ATAPI_POWERCOMMAND

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL GetEvents(
               WORD**  pIDList,      // Pointer to array of IDs interpreted by the plugin
               // (Will be allocated with LocalAlloc)
               DWORD*  pdwNumIDs     // Pointer to number of IDs in the list
               )
{
    const WORD NUM_EVENTS = (CELID_ATAPI_LASTIDENTIFIER - CELID_ATAPI_BASE) + 1;
    WORD i;

    *pIDList = LocalAlloc(LMEM_FIXED, NUM_EVENTS*sizeof(WORD));
    if (!(*pIDList)) {
        *pdwNumIDs = 0;
        return FALSE;
    }

    // Fill the buffer
    for(i = 0; i <= NUM_EVENTS; i++) {
        (*pIDList)[i] = CELID_ATAPI_BASE + i;
    }
    *pdwNumIDs = NUM_EVENTS;

    return TRUE;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL GetEventString(
                    WCHAR** ppOutBuf,     // Event string will be written to this output buffer
                    // (may realloc with LocalAlloc)
                    DWORD*  pdwOutBufLen, // Length of output buffer, in bytes
                    PBYTE   pInBuf,       // Input buffer of raw data from the log
                    DWORD   dwInBufLen,   // Length of input buffer, in bytes
                    WORD    wID,          // ID of event
                    WORD    wFlag,        // Flag that was passed with the event, if present
                    BOOL    fFlagged      // Was the flag present?
                    )
{
    WCHAR szMsg[MAX_PATH];  // Temporarily write the message here to get size
    DWORD dwMsgLen = 0;

    switch (wID) {

    case CELID_ATAPI_STARTIOCTL:
        {
            PIOREQ pIOReq = (PIOREQ) pInBuf;
            if(dwInBufLen != sizeof(*pIOReq)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"Start IOCTL 0x%08x:\r\n\tinbuf 0x%08x, insize %u (0x%x)\r\n\toutbuf 0x%08x, outsize %u (0x%x)\r\n",
                pIOReq->dwCode,
                pIOReq->pInBuf, pIOReq->dwInBufSize, pIOReq->dwInBufSize,
                pIOReq->pOutBuf, pIOReq->dwOutBufSize, pIOReq->dwOutBufSize);
        }
        break;
    case CELID_ATAPI_COMPLETEIOCTL:
        {
            PDWORD pdw = (PDWORD) pInBuf;
            if(dwInBufLen != sizeof(*pdw)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"Complete IOCTL: status is %u\r\n", *pdw);
        }
        break;
    case CELID_ATAPI_BASESTATUS:
        {
            PBYTE pb = (PBYTE) pInBuf;
            if(dwInBufLen != sizeof(*pb)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"GetBaseStatus(): status is 0x%02x\r\n", *pb);
        }
        break;
    case CELID_ATAPI_IOCOMMAND:
        {
            PBYTE pb = (PBYTE) pInBuf;
            if(dwInBufLen != sizeof(*pb)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"SendIOCommand(): sending 0x%02x\r\n", *pb);
        }
        break;
    case CELID_ATAPI_WAITINTERRUPT:
        {
            PDWORD pdw = (PDWORD) pInBuf;
            if(dwInBufLen != sizeof(*pdw)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"WaitForInterrupt(): timeout is %u\r\n", *pdw);
        }
        break;
    case CELID_ATAPI_INTERRUPTDONE:
        {
            PBOOL pBool = (PBOOL) pInBuf;
            if(dwInBufLen != sizeof(*pBool)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"WaitForInterrupt(): returning %u\r\n", *pBool);
        }
        break;
    case CELID_ATAPI_ENABLEINTERRUPT:
        dwMsgLen += swprintf(szMsg, L"EnableInterrupt(): enabling interrupts\r\n");
        break;
    case CELID_ATAPI_WAITDRQ:
        dwMsgLen += swprintf(szMsg, L"WaitForDRQ(): waiting for ready\r\n");
        break;
    case CELID_ATAPI_STATUSWAITDRQ:
        {
            PBYTE pb = (PBYTE) pInBuf;
            if(dwInBufLen != sizeof(*pb)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"WaitForDRQ(): status is 0x%02x\r\n", *pb);
        }
        break;
    case CELID_ATAPI_STARTREADBUFFER:
        {
            PDWORD pdw = (PDWORD) pInBuf;
            if(dwInBufLen != sizeof(*pdw)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"ReadBuffer(): reading %u (0x%x) bytes\r\n", *pdw, *pdw);
        }
        break;
    case CELID_ATAPI_COMPLETEREADBUFFER:
        {
            PBYTE pb = (PBYTE) pInBuf;
            if(dwInBufLen != sizeof(*pb)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"ReadBuffer(): final status is 0x%02x\r\n", *pb);
        }
        break;
    case CELID_ATAPI_STARTWRITEBUFFER:
        {
            PDWORD pdw = (PDWORD) pInBuf;
            if(dwInBufLen != sizeof(*pdw)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"WriteBuffer(): writing %u (0x%x) bytes\r\n", *pdw, *pdw);
        }
        break;
    case CELID_ATAPI_COMPLETEWRITEBUFFER:
        {
            PBYTE pb = (PBYTE) pInBuf;
            if(dwInBufLen != sizeof(*pb)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"WriteBuffer(): final status is 0x%02x\r\n", *pb);
        }
        break;

    case CELID_ATAPI_SETUPDMA_ALIGNED:
        {
            PDWORD pdw = (PDWORD) pInBuf;
            if(dwInBufLen != sizeof(*pdw)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"SetupDMA(): created %d aligned buffer chains\r\n", *pdw, *pdw);
        }
        break;

    case CELID_ATAPI_SETUPDMA_COPY:
        {
            PDWORD pdw = (PDWORD) pInBuf;
            if(dwInBufLen != sizeof(*pdw)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"SetupDMA(): created %d unaligned copy buffer chains\r\n", *pdw, *pdw);
        }
        break;

    case CELID_ATAPI_WAITDMAINTERRUPT:
        {
            PDWORD pdw = (PDWORD) pInBuf;
            if(dwInBufLen != sizeof(*pdw)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"WaitForDMAInterrupt(): timeout is %u\r\n", *pdw);
        }
        break;

    case CELID_ATAPI_DMAINTERRUPTDONE:
        {
            PBOOL pBool = (PBOOL) pInBuf;
            if(dwInBufLen != sizeof(*pBool)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"WaitForDMAInterrupt(): returning %u\r\n", *pBool);
        }
        break;

    case CELID_ATAPI_SETDEVICEPOWER:
        {
            PDWORD pdw = (PDWORD) pInBuf;
            if(dwInBufLen != sizeof(*pdw)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"SetDiskPowerState(): entering D%u\r\n", *pdw);
        }
        break;

    case CELID_ATAPI_POWERCOMMAND:
        {
            PBYTE pb = (PBYTE) pInBuf;
            if(dwInBufLen != sizeof(*pb)) {
                return FALSE;
            }
            dwMsgLen += swprintf(szMsg, L"SendDiskPowerCommand(): sending 0x%02x\r\n", *pb);
        }
        break;

    default:
        // We were given an event we shouldn't have been
        return FALSE;
    }

    // Convert dwMsgLen from chars to bytes
    dwMsgLen++;
    dwMsgLen *= sizeof(WCHAR);

    // Realloc the output buffer if it's not large enough
    if (dwMsgLen > *pdwOutBufLen) {
        if (*ppOutBuf) {
            LocalFree(*ppOutBuf);
        }

        *ppOutBuf = LocalAlloc(LMEM_FIXED, dwMsgLen);
        if (*ppOutBuf == NULL) {
            *pdwOutBufLen = 0;
            return FALSE;
        }
        *pdwOutBufLen = dwMsgLen;
    }

    memcpy(*ppOutBuf, szMsg, dwMsgLen);
    *pdwOutBufLen = dwMsgLen;

    return TRUE;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// DLL entry
BOOL WINAPI CeLogReaderPluginEntry(HINSTANCE DllInstance, INT Reason, LPVOID Reserved)
{
    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(DllInstance);
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

