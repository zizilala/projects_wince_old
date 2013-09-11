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
    oaldma.h

Abstract:
    Contains definitions for DMA API functions.

Notes:

--*/

#ifndef   __OALDMA_H__
#define   __OALDMA_H__


//
// Defines for DMA functions.
//

#define DMA_ADAPTER_ANY 0xFFFFFFFF
#define DMA_CHANNEL_ANY 0xFFFFFFFF

#define DMA_FLAGS_WRITE_TO_DEVICE       0x00000001
#define DMA_FLAGS_INC_DEVICE_ADDRESS    0x00000002
#define DMA_FLAGS_USER_OPTIONAL_DEVICE  0x00000004
#define DMA_FLAGS_AUTO_CLOSE_TRANSFER   0x00000008
#define DMA_FLAGS_QUEUE_TRANSFER        0x00000010


//
// DMA transfer status enums.
//

typedef enum _DMA_INTERRUPT_STATUS {
  DmaIntNone = 0,
  DmaIntRunning,
  DmaIntCompleted,
  DmaIntPaused,
  DmaIntError
} DMA_INTERRUPT_STATUS, *PDMA_INTERRUPT_STATUS;


//
// DMA access speed enums.
//

typedef enum _DMA_ACCESS_SPEED {
  DmaSpeedCompatible = 0,
  DmaSpeedSlow,
  DmaSpeedMedium,
  DmaSpeedHigh,
  DmaSpeedHighest
} DMA_ACCESS_SPEED, *PDMA_ACCESS_SPEED;


//
// DMA access width enums.
//

typedef enum _DMA_ACCESS_WIDTH {
  DmaWidth8Bits = 1,
  DmaWidth16Bits = 2,
  DmaWidth32Bits = 4,
  DmaWidth64Bits = 8
} DMA_ACCESS_WIDTH, *PDMA_ACCESS_WIDTH;


//
// DMA transfer status enums.
//

typedef enum _DMA_TRANSFER_STATUS {
  DmaStatusNone = 0,
  DmaStatusPending,
  DmaStatusTransferring,
  DmaStatusCompleted,
  DmaStatusCanceled,
  DmaStatusError
} DMA_TRANSFER_STATUS, *PDMA_TRANSFER_STATUS;


//
//  Size: Size of this structure in bytes.
//  InterfaceType: Interface type on this device. It could be internal, pci, and etc.
//  BusNumber:    Bus number that indicates location of this hardware.
//  DmaAdapter: This is optional parameter. If this system only has one DMA Adapter or application
//      does not care which DMA adapter it should use, set it to DMA_ADAPTER_ANY.
//  DemandMode: Set this to true if Device and DMA hardware has hardware handshaking.
//  DmandWidth: DMA transfer width.
//  DmaSpeed: DMA Speed is used by this function to determine the DMA Adapter or Channel. The fastesnotet
//      device should allocate highest priority DMA Adaptor and Channel.
//  This structure is used by function OALGetDmaAdapter.
//

typedef struct _DEVICE_DMA_REQUIREMENT_INFO {
  USHORT  Size;
  INTERFACE_TYPE  InterfaceType;
  ULONG  BusNumber;
  ULONG  DmaAdapterNumber;
  BOOLEAN  DemandMode;
  DMA_ACCESS_WIDTH  DmaWidth;
  DMA_ACCESS_SPEED  DmaSpeed;
} DEVICE_DMA_REQUIREMENT_INFO, *PDEVICE_DMA_REQUIREMENT_INFO;


//
//  Size: Size of this structure in bytes.
//  InterfaceType: Interface type on this device. It could be internal, pci, and etc.
//  BusNumber:    Bus number that indicates location of this hardware.
//  DmaAdapter: This system adjusted value which return by OALGetDmaAdapt.
//  DmaAdapterObj: Pointer to a DMA adapter object that reprsents this hardware.
//  NumberOfChannels: It indicates how many DMA channels the DMA hardware has.
//  NumberOfMapRegisters: It indicates how many DMA transfer hardware can queue per channel.
//    Although hardware only can activate one DMA transfer at one time, multiple map register support
//    will reduce software turn around time and make the data stream much more consistent.
//  MaximunSizeOfEachRegister: It indicates hardware limitation for each transfer. If any size of
//    issued transfer by OALIssueTransfer is bigger than MaxmimunSizeOfEachRegister, it will fail.
//  DmaWith, DmaSpeed & DemandMode: Copied from DMA_REQUIREMENT_INFO. It should be same.
//

typedef struct _DMA_ADAPTER_OBJ {
  USHORT            Size;
  INTERFACE_TYPE    InterfaceType;
  ULONG             BusNumber;
  ULONG             DmaAdapterNumber;
  PVOID             DmaAdapterObj;
  ULONG             NumberOfChannels;
  ULONG             NumberOfMapRegisters;
  ULONG             MaximunSizeOfEachRegister;
  // Copy From DMA_REQUIREMENT_INFO
  BOOLEAN           DemandMode;
  DMA_ACCESS_WIDTH  DmaWidth;
  DMA_ACCESS_SPEED  DmaSpeed;
} DMA_ADAPTER_OBJ, *PDMA_ADAPTER_OBJ;


//
// DMA channel and transfer handle.
//

typedef HANDLE DMA_CHANNEL_HANDLE, *PDMA_CHANNEL_HANDLE;
typedef HANDLE DMA_TRANSFER_HANDLE, *PDMA_TRANSFER_HANDLE;


//
// The OALInitializeDmaAdapter routine fills out the MDD DMA adapter structure
// and initializes the hardware for all available adapters.
//

BOOL OALInitializeDmaAdapters (
  VOID
);


//
// The OALDeinitializeDmaAdapter routine shuts down the hardware and frees
// all the allocations for all available adapters.
//

BOOL OALDeinitializeDmaAdapters (
  VOID
);


//
// DMA transfer complete notification routine.
//

typedef DWORD (WINAPI *LPDMA_TRANSFER_NOTIFY_ROUTINE)(ULONG NotifyCount, LPVOID lpvNotifyParameter[]);


//
// The OALGetDmaAdapter routine fill out a pointer to the DMA adapter structure for a physical device.
//

BOOL OALGetDmaAdapter (
    IN PDEVICE_DMA_REQUIREMENT_INFO pDeviceDmaRequirementInfo, //  Device Description. It descript what is capable Adapt supported.
    IN OUT PDMA_ADAPTER_OBJ  pDmaAdapter
);


//
// The AllocateAdapterChannel routine prepares the system for a DMA operation on behalf of the target device
// and return handle that can be used by OALIssueDMATransfer.
//

DMA_CHANNEL_HANDLE OALAllocateDmaChannel(
    IN PDMA_ADAPTER_OBJ pDmaAdapter,
    IN ULONG ulRequestedChannel,
    IN ULONG ulAddressSpace,
    IN PHYSICAL_ADDRESS DeviceIoAddress
);


//
// This function frees a DMA Channel buffer allocated by OALAllocateAdapterChannel, along with all resources
// the DMA Channel uses.
//

BOOL OALFreeDmaChannel(
   IN DMA_CHANNEL_HANDLE hDmaChannel
);


//
// The OALIssueDMATransfer routine sets up map descriptor registers for a channel to map a DMA transfer
// from a locked-down buffer if there is no other DMA transfers queued in the DMA Channel. Otherwise, this
// transfer should be queued.
//

BOOL OALIssueDmaTransfer(
  IN DMA_CHANNEL_HANDLE hDmaChannel,
  IN OUT PDMA_TRANSFER_HANDLE phDmaHandle,
  IN DWORD  dwFlags,
  IN PHYSICAL_ADDRESS SystemMemoryPhysicalAddress,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS OpDeviceIoAddress,
  IN LPDMA_TRANSFER_NOTIFY_ROUTINE NotifyRoutine,
  IN PVOID NotifyContext
);


//
// The OALStartDMATransfer starts any pending DMA transfers for a idle channel.
//

BOOL OALStartDmaTransfer(
  IN DMA_CHANNEL_HANDLE hDmaChannel
);


//
// This function cancels a pending DMA transfer.
//

BOOL OALCancelDmaTransfer(
    IN DMA_TRANSFER_HANDLE DmaHandle
);


//
// This function closes a DMA Transfer and release all related resource.
//

BOOL OALCloseDmaTransfer(
    IN DMA_TRANSFER_HANDLE DmaHandle
);


//
// This function get current active or queued DMA transfer status.
//

BOOL OALGetDmaStatus (
  IN DMA_TRANSFER_HANDLE hDmaHandle,
  OUT PDWORD lpCompletedLength,
  OUT PDMA_TRANSFER_STATUS lpCompletionCode
);


//
// This function waits for a channel DMA completion event.
//

DWORD OALWaitForDmaComplete (
  IN DMA_CHANNEL_HANDLE hDmaChannel,
  IN DWORD WaitTime
);


//
// Used to issue DMA transfer directly to hardware.
//

BOOL OALIssueRawDmaTransfer(
  IN DMA_CHANNEL_HANDLE hDmaChannel,
  IN OUT PDMA_TRANSFER_HANDLE phDmaHandle,
  IN DWORD  dwFlags,
  IN PVOID lpInPtr,
  IN DWORD nInLen,
  IN LPDMA_TRANSFER_NOTIFY_ROUTINE NotifyRoutine,
  IN PVOID NotifyContext
);


//
// Used to control DMA that status in DMA_TRANSFER_IN_PROGRESS by hardware.
//

BOOL OALRawDmaControl(
  IN DMA_TRANSFER_HANDLE hDmaHandle,
  IN DWORD dwIoControl,
  IN PVOID lpInPtr,
  IN DWORD nInLen,
  IN OUT LPVOID lpOutBuffer,
  IN DWORD nOutBufferSize,
  IN LPDWORD lpBytesReturned
);

#endif
