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

// File: MsgQueue.h
//
// This file implements a class to queue N byte messages in a circular buffer

#ifndef __MsgQueue_H_
#define __MsgQueue_H_

#include <windows.h>
#include <assert.h>

class CMsgQueue
{
public:
    CMsgQueue(DWORD dwMsgSize, DWORD dwMsgCount);
    ~CMsgQueue();
    BOOL Init();
    VOID Clear() { m_CurrentMsgCount = m_Head = m_Tail = 0; }
    inline BOOL IsFull() { return m_CurrentMsgCount == m_BufSizeMsgs; }
    inline BOOL IsEmpty() { return m_CurrentMsgCount == 0; }
    inline DWORD NumMsgs() { return m_CurrentMsgCount; }
    inline DWORD MessageSize() { return m_MsgSize; }
    DWORD GetMsgs(PBYTE pBuf, DWORD dwBufLen);
    BOOL AddMsg(PBYTE pMsg);

private:
    const DWORD m_MsgSize;
    const DWORD m_BufSizeBytes;
    const DWORD m_BufSizeMsgs;
    PBYTE m_pMsgBuf;
    DWORD m_Head, m_Tail;
    LONG  m_CurrentMsgCount;
};

#endif // __MsgQueue_H_

