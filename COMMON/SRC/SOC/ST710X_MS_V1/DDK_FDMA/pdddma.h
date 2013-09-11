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
    pdddma.h

Abstract:
    Contains PDD DMA definitions.

Notes:

--*/

#ifndef   __PDDDMA_H__
#define   __PDDDMA_H__


//
// FDMA PDD function proto-types.
//

ULONG FdmaGetNumberOfAdapters(
  VOID
);

BOOL FdmaGetAdapterIrq(
  ULONG AdapterNumber,
  PULONG AdapterIrq
);

BOOL FdmaGetDmaAdapter (
  IN PDEVICE_DMA_REQUIREMENT_INFO pDeviceDmaRequirementInfo,
  IN OUT PDMA_ADAPTER_OBJ pDmaAdapter
);

BOOL FdmaInitializeDmaAdapter (
  IN ULONG AdapterNumber,
  IN PDMA_ADAPTER_OBJ pDmaAdapter,
  IN OUT PPVOID ppPddAdapter
);

BOOL FdmaDeinitializeDmaAdapter (
  IN ULONG AdapterNumber,
  IN PDMA_ADAPTER_OBJ pDmaAdapter,
  IN OUT PPVOID ppPddAdapter
);

BOOL FdmaInitializeDmaChannel (
  IN PDMA_ADAPTER_OBJ pDmaAdapter,
  IN ULONG ChannelNumber,
  IN PVOID pPddAdapter,
  IN OUT PPVOID ppPddChannel
);

BOOL FdmaDeinitializeDmaChannel (
  IN PDMA_ADAPTER_OBJ pDmaAdapter,
  IN ULONG ChannelNumber,
  IN PVOID pPddAdapter,
  IN OUT PPVOID ppPddChannel
);

BOOL FdmaAllocateMapRegister (
  IN PVOID pPddChannel,
  IN ULONG Flags,
  IN PHYSICAL_ADDRESS DeviceAddr,
  IN PHYSICAL_ADDRESS BufferAddr,
  IN ULONG Length,
  IN BOOLEAN DemandMode,
  IN DMA_ACCESS_WIDTH DmaWidth,
  IN DMA_ACCESS_SPEED DmaSpeed,
  OUT PPVOID ppPddMR
);

BOOL FdmaFreeMapRegister (
  IN PVOID pPddChannel,
  IN OUT PPVOID ppPddMR
);

BOOL FdmaQueueTransfer (
  IN PVOID pPddChannel,
  IN PVOID pPddMR
);

BOOL FdmaStartTransfer (
  IN PVOID pPddChannel,
  IN ULONG ChannelNumber
);

BOOL FdmaCloseTransfer (
  IN PVOID pPddChannel
);

DMA_INTERRUPT_STATUS FdmaServiceInterrupt(
  IN ULONG Channel,
  IN PVOID pPddAdapter,
  IN PVOID pPddChannel
);

#endif __PDDDMA_H__
