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

// File: MsgQueue.cpp
//
// This file implements a class to queue N byte messages in a circular buffer

#include <windows.h>
#include <assert.h>
#include <MsgQueue.h>

CMsgQueue::CMsgQueue(DWORD dwMsgSize, DWORD dwMsgCount)
    : m_MsgSize( dwMsgSize ),
      m_BufSizeBytes( dwMsgSize * dwMsgCount ),
      m_BufSizeMsgs( dwMsgCount ),
      m_pMsgBuf( NULL ),
      m_Head( 0 ),
      m_Tail( 0 ),
      m_CurrentMsgCount( 0 )
{
}

CMsgQueue::~CMsgQueue()
{
    if (m_pMsgBuf)
    {
        LocalFree( m_pMsgBuf );
    }
}

BOOL CMsgQueue::Init()
{
    assert( m_BufSizeBytes );
    m_pMsgBuf = (PBYTE)LocalAlloc( LPTR, m_BufSizeBytes );
    return m_pMsgBuf != NULL;
}

DWORD CMsgQueue::GetMsgs(PBYTE pBuf, DWORD dwBufLen)
{
    assert( m_pMsgBuf );

    DWORD BytesWritten = 0;

    // calculate how many bytes can we write overall:
    // - available
    // - room in DST
    // - truncate to integral message size
    DWORD BytesToWrite = m_CurrentMsgCount * m_MsgSize;
    if (BytesToWrite > dwBufLen)
    {
        BytesToWrite = dwBufLen;
    }
    BytesToWrite -= (BytesToWrite % m_MsgSize);

    //
    // copy the first part, in case the message wraps around
    //
    if (BytesToWrite > (m_BufSizeBytes - m_Tail))
    {
        DWORD BytesInChunk = m_BufSizeBytes - m_Tail;
        CopyMemory( pBuf, m_pMsgBuf + m_Tail, BytesInChunk );
        BytesToWrite -= BytesInChunk;
        BytesWritten += BytesInChunk;
        pBuf += BytesInChunk;
        m_Tail = 0;
    }

    //
    // Copy the rest of the wrapped around buffer, or the single continuous buffer
    //
    if (BytesToWrite > 0)
    {
        CopyMemory( pBuf, m_pMsgBuf + m_Tail, BytesToWrite );
        BytesWritten += BytesToWrite;
        m_Tail += BytesToWrite;
    }

    assert( (BytesWritten % m_MsgSize) == 0 );

    InterlockedExchangeAdd(&m_CurrentMsgCount, -1 * (BytesWritten / m_MsgSize));

    return BytesWritten;
}

BOOL CMsgQueue::AddMsg(PBYTE pMsg)
{
    assert( m_pMsgBuf );
    assert( pMsg );
    if (IsFull())
    {
        return FALSE;
    }
    CopyMemory(m_pMsgBuf + m_Head, pMsg, m_MsgSize);
    m_Head += m_MsgSize;
    if (m_Head >= m_BufSizeBytes)
    {
        m_Head = 0;
    }
    InterlockedIncrement( &m_CurrentMsgCount );
    return TRUE;
}

