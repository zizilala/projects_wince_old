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
#ifndef _IRCDEBUG_H_
#define _IRCDEBUG_H_

// Debug zones

#define ZONEID_INIT       1
#define ZONEID_DEINIT     2
#define ZONEID_WARNING    3
#define ZONEID_ERROR      4
#define ZONEID_API        5
#define ZONEID_READ       6
#define ZONEID_OPEN       7
#define ZONEID_THREAD     8
#define ZONEID_FUNCTION   9


#define ZONE_INIT         DEBUGZONE(ZONEID_INIT)
#define ZONE_DEINIT       DEBUGZONE(ZONEID_DEINIT)
#define ZONE_WARNING      DEBUGZONE(ZONEID_WARNING)
#define ZONE_ERROR        DEBUGZONE(ZONEID_ERROR)
#define ZONE_API          DEBUGZONE(ZONEID_API)
#define ZONE_READ         DEBUGZONE(ZONEID_READ)
#define ZONE_OPEN         DEBUGZONE(ZONEID_OPEN)
#define ZONE_THREAD       DEBUGZONE(ZONEID_THREAD)
#define ZONE_FUNCTION     DEBUGZONE(ZONEID_FUNCTION)

#ifdef DEBUG
  #define DEBUG_ASSERT(x)  ASSERT(x)
#else
  #define DEBUG_ASSERT(x)
#endif

extern DBGPARAM dpCurSettings;

#endif //_IRCDEBUG_H_

