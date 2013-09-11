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
    mdddma.h

Abstract:
    Contains MDD DMA definitions.

Notes:

--*/

#ifndef   __MDDDMA_H__
#define   __MDDDMA_H__

#include <linklist.h>


//
// Starting DMA channel.
//

#define DMA_CHANNEL_START 9


//
// IST and DPC priority levels.
//

#define DMA_IST_PRIORITY_LEVEL 100
#define DMA_DPC_PRIORITY_LEVEL 110


//
// MDD Map Register.
//

typedef struct _MDD_MAP_REGISTER {

  //
  // List entry for placing the map register in a queue.
  //

  LIST_ENTRY ListEntry;

  //
  // Size of MDD map register.
  //

  USHORT Size;

  //
  // Pointer to the MDD channel object this MDD map register belongs to.
  //

  struct _MDD_CHANNEL_OBJ *pChannel;

  //
  // Current status for the map register.
  //

  DMA_TRANSFER_STATUS MRStatus;

  //
  // DMA transfer flags.
  //

 ULONG Flags;

  //
  // Number of bytes transferred.
  //

 ULONG BytesToTransfer;

  //
  // Number of bytes transferred.
  //

  ULONG BytesTransferred;

  //
  // Notification rouinte and context.
  //

  LPDMA_TRANSFER_NOTIFY_ROUTINE NotifyRoutine;
  PVOID NotifyContext;

  //
  // Pointer to PDD map register.
  //

  PVOID pPddMR;

} MDD_MAP_REGISTER, *PMDD_MAP_REGISTER;


//
// MDD Channel Object.
//

typedef struct _MDD_CHANNEL_OBJ {

  //
  // Size of MDD channel object.
  //

  USHORT Size;

  //
  // Channel in use.
  //

  BOOL InUse;

  //
  // Critical section to protect access to channel object.
  //

  CRITICAL_SECTION CSChannel;

  //
  // Pointer to the MDD adapter object this MDD channel object belongs to.
  //

  struct _MDD_ADAPTER_OBJ *pAdapter;

  //
  // Channel number.
  //

  ULONG ChannelNumber;

  //
  // Physical address of the device to DMA to.
  //

  PHYSICAL_ADDRESS DeviceAddr;

  //
  // Channel information copied form DMA adapter.
  //

  BOOLEAN DemandMode;
  DMA_ACCESS_WIDTH DmaWidth;
  DMA_ACCESS_SPEED DmaSpeed;

  //
  // DMA completion event.
  //

  HANDLE hDmaCompleteEvent;

  //
  // Memory allocated for map registers.
  //

  PMDD_MAP_REGISTER pMRAlloc;

  //
  // Free, pending, transferred, and completed map registers.
  //

  LIST_ENTRY MRFree;
  LIST_ENTRY MRPend;
  LIST_ENTRY MRTransfer;
  LIST_ENTRY MRNotify;
  LIST_ENTRY MRDone;

  //
  // Pointer to PDD channel object.
  //

  PVOID pPddChannel;

} MDD_CHANNEL_OBJ, *PMDD_CHANNEL_OBJ;


//
// MDD Adapter Object.
//

typedef struct _MDD_ADAPTER_OBJ {

  //
  // Size of MDD adapter object.
  //

  USHORT Size;

  //
  // Critical section to protect access to adapter object.
  //

  CRITICAL_SECTION CSAdapter;

  //
  // Indicate that existing IST and DPC threads should exit.
  //

  BOOL AllThreadsExit;

  //
  // Irq and SysIntr for the DMA adapter.
  //

  ULONG Irq, SysIntr;

  //
  // Handle for IST and DPC threads to process DMA.
  //

  HANDLE hDpcThread, hIstThread;

  //
  // Handle for IST, DPC, DMA complete events to process DMA.
  //

  HANDLE hIstEvent, hDpcEvent;

  //
  // Adapter description.
  //

 DMA_ADAPTER_OBJ AdapterDesc;

  //
  // MDD channel objects for this MDD adapter.
  //

  PMDD_CHANNEL_OBJ Channel;

  //
  // Pointer to PDD adapter object.
  //

  PVOID pPddAdapter;

} MDD_ADAPTER_OBJ, *PMDD_ADAPTER_OBJ;

#endif __MDDDMA_H__
