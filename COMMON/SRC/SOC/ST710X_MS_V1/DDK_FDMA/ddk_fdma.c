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
    ddk_fdma.c

Abstract:
    Contains code for FDMA functions.

Notes:

--*/

#include <windows.h>
#include <types.h>
#include <ceddk.h>
#include <ddkreg.h>
#include <nkintr.h>

#include "stb7100reg.h"
#include "stb7100_ioport.h"
#include "sh4_intc.h"
#include "SH4202T_intc2_irq.h"

#include "oaldma.h"
#include "pdddma.h"
#include "fdma.h"


//
// Returns the number of FDMA adapters.
//

ULONG FdmaGetNumberOfAdapters (
  VOID
)
{
  //
  // Return the number of FDMA adapters.
  //

  return FDMA_MAX_ADAPTERS;
}


//
// Returns the IRQ for a FDMA adapters.
//

BOOL FdmaGetAdapterIrq(
  ULONG AdapterNumber,
  PULONG AdapterIrq
)
{
  //
  // Only FDMA adapter 0 is valid.
  //

  if (AdapterNumber != 0) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  *AdapterIrq = IRQ_FDMA_MBOX;

  return TRUE;
}


//
// Initialize FDMA adapter.
//

BOOL FdmaInitializeDmaAdapter (
  IN ULONG AdapterNumber,
  IN PDMA_ADAPTER_OBJ pDmaAdapter,
  IN OUT PPVOID ppPddAdapter
)
{
  PPDD_ADAPTER_OBJ pFdmaAdapter;
  PHYSICAL_ADDRESS FdmaAddress;
  ULONG AddressSpace;

  //
  // check adapter number.
  //

  if (AdapterNumber >= FDMA_MAX_ADAPTERS) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // Check the parameter.
  //

  if (!pDmaAdapter || pDmaAdapter->Size != sizeof(DMA_ADAPTER_OBJ)) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // Allocate storage for the FDMA adapter pdd.
  //

  pFdmaAdapter = LocalAlloc(LMEM_ZEROINIT, sizeof(PDD_ADAPTER_OBJ));

  if (!pFdmaAdapter) {

    SetLastError(ERROR_OUTOFMEMORY);

    return FALSE;
  }

  //
  // Map the FDMA base address.
  //

  FdmaAddress.QuadPart = FDMA_INTERFACE_ADDRESS;
  AddressSpace = 0;
  if (!TransBusAddrToVirtual(pDmaAdapter->InterfaceType,
                             pDmaAdapter->BusNumber,
                             FdmaAddress,
                             sizeof(FDMA_INTERFACE_REGS),
                             &AddressSpace,
                             &(pFdmaAdapter->pInterfRegs))) {

    SetLastError(ERROR_GEN_FAILURE);

    goto Error;
  }

  FdmaAddress.QuadPart = FDMA_CHANNEL_ADDRESS;
  AddressSpace = 0;
  if (!TransBusAddrToVirtual(pDmaAdapter->InterfaceType,
                             pDmaAdapter->BusNumber,
                             FdmaAddress,
                             sizeof(FDMA_CHANNEL_REGS),
                             &AddressSpace,
                             &(pFdmaAdapter->pChanRegs))) {

    SetLastError(ERROR_GEN_FAILURE);

    goto Error;
  }

  FdmaAddress.QuadPart = FDMA_MAILBOX_ADDRESS;
  AddressSpace = 0;
  if (!TransBusAddrToVirtual(pDmaAdapter->InterfaceType,
                             pDmaAdapter->BusNumber,
                             FdmaAddress,
                             sizeof(FDMA_MAILBOX_REGS),
                             &AddressSpace,
                             &(pFdmaAdapter->pMBRegs))) {

    SetLastError(ERROR_GEN_FAILURE);

    goto Error;
  }

  //
  // Save the FDMA adapter pdd.
  //

  *ppPddAdapter = pFdmaAdapter;

  //
  // Fill in the adapter description.
  //

  pDmaAdapter->Size = sizeof(DMA_ADAPTER_OBJ);
  pDmaAdapter->InterfaceType = Internal;
  pDmaAdapter->BusNumber = 0;
  pDmaAdapter->DmaAdapterNumber = 0;
  pDmaAdapter->DmaAdapterObj;
  pDmaAdapter->NumberOfChannels = FDMA_MAX_CHANNELS;
  pDmaAdapter->NumberOfMapRegisters = FDMA_MAX_MAP_REGISTERS;
  pDmaAdapter->MaximunSizeOfEachRegister = FDMA_MAX_TRANSFER_SIZE;
  pDmaAdapter->DemandMode = TRUE;
  pDmaAdapter->DmaWidth = DmaWidth64Bits;
  pDmaAdapter->DmaSpeed = DmaSpeedHighest;

  return TRUE;

Error:

  //
  // Free any allocation.
  //

  if (pFdmaAdapter) {

    LocalFree(pFdmaAdapter);
  }

  return FALSE;
}


//
// Deinitialize FDMA adapter.
//

BOOL FdmaDeinitializeDmaAdapter (
  IN ULONG AdapterNumber,
  IN PDMA_ADAPTER_OBJ pDmaAdapter,
  IN OUT PPVOID ppPddAdapter
)
{
  PPDD_ADAPTER_OBJ pFdmaAdapter = *ppPddAdapter;

  //
  // Check adapter number.
  //

  if (AdapterNumber >= FDMA_MAX_ADAPTERS) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // Unmap the FDMA addresses.
  //

  pFdmaAdapter = *ppPddAdapter;

  if (pFdmaAdapter) {

    if (pFdmaAdapter->pInterfRegs) {

      MmUnmapIoSpace(pFdmaAdapter->pInterfRegs,
                     sizeof(FDMA_INTERFACE_REGS));
    }

    if (pFdmaAdapter->pChanRegs) {

      MmUnmapIoSpace(pFdmaAdapter->pChanRegs,
                     sizeof(FDMA_CHANNEL_REGS));
    }

    if (pFdmaAdapter->pMBRegs) {

      MmUnmapIoSpace(pFdmaAdapter->pMBRegs,
                     sizeof(FDMA_MAILBOX_REGS));
    }
  }

  //
  // Free the FDMA adapter pdd.
  //

  LocalFree(pFdmaAdapter);

  *ppPddAdapter = NULL;

  return TRUE;
}


//
// Initialize the FDMA channel.
//

BOOL FdmaInitializeDmaChannel (
  IN PDMA_ADAPTER_OBJ pDmaAdapter,
  IN ULONG Channel,
  IN PVOID pPddAdapter,
  IN OUT PPVOID ppPddChannel
)
{
  PPDD_ADAPTER_OBJ pFdmaAdapter = pPddAdapter;
  PPDD_CHANNEL_OBJ pFdmaChannel;
  PNODE_REGISTERS pNode;
  ULONG Index;

  //
  // Perform some initial checks.
  //

  if (!pDmaAdapter || pDmaAdapter->Size != sizeof(DMA_ADAPTER_OBJ) ||
      Channel >= FDMA_MAX_CHANNELS ||
      !pPddAdapter || !ppPddChannel) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // Allocate storage for FDMA channel PDD data structure.
  //

  pFdmaChannel = LocalAlloc(LMEM_ZEROINIT, sizeof(PDD_CHANNEL_OBJ));

  if (!pFdmaChannel) {

    SetLastError(ERROR_OUTOFMEMORY);

    goto Error;
  }

  //
  // HalAllocateCommonBuffer allocates physically contiguous memory
  // aligned on 64k boundary.  This will work fine with the FDMA engine
  // since the FDMA node structures need to be aligned on 32byte boundary.
  //

  pFdmaChannel->NodeVirtBase = HalAllocateCommonBuffer((PDMA_ADAPTER_OBJECT)pDmaAdapter,
                                                       sizeof(NODE_REGISTERS) * FDMA_MAX_MAP_REGISTERS,
                                                       &(pFdmaChannel->NodePhyBase),
                                                       FALSE);

  if (!pFdmaChannel->NodeVirtBase) {

    SetLastError(ERROR_OUTOFMEMORY);

    goto Error;
  }

  //
  // Fill out the FDMA channel pdd.
  //

  pFdmaChannel->pCmdSet = (PULONG)&(pFdmaAdapter->pMBRegs->CmdSet);
  pFdmaChannel->pChanCmd = (PULONG)&(pFdmaAdapter->pChanRegs->ChanCmdStat[Channel]);
  pFdmaChannel->pNextNode = (PULONG)&(pFdmaAdapter->pChanRegs->NodeParam[Channel].NextNode);
  pFdmaChannel->pSaddr = (PULONG)&(pFdmaAdapter->pChanRegs->NodeParam[Channel].Saddr);
  pFdmaChannel->pDaddr = (PULONG)&(pFdmaAdapter->pChanRegs->NodeParam[Channel].Daddr);
  pFdmaChannel->pChanCnt = &(pFdmaAdapter->pChanRegs->NodeParam[Channel].Cnt);
  pFdmaChannel->pChanReqCtl = (PULONG)&(pFdmaAdapter->pChanRegs->ReqCtrl[Channel]);

  //
  // Set up the DMA node queues.
  //

  InitQueue(&(pFdmaChannel->NodeFree));
  InitQueue(&(pFdmaChannel->NodePend));
  InitQueue(&(pFdmaChannel->NodeTransfer));
  InitQueue(&(pFdmaChannel->NodeDone));

  pNode = (PNODE_REGISTERS)pFdmaChannel->NodeVirtBase;

  for (Index = 0; Index < FDMA_MAX_MAP_REGISTERS; Index++) {

    EnqueueNode(&(pFdmaChannel->NodeFree), pNode);

    pNode++;
  }

  //
  // Initialize the channel.
  //

  WRITE_FDMA_REG(&(pFdmaAdapter->pMBRegs->CmdClr),  (FDMA_CHANNEL_CMD_MASK << FDMA_CHAN_SHIFT(Channel)));
  WRITE_FDMA_REG(&(pFdmaAdapter->pMBRegs->IntClr),  (FDMA_CHANNEL_CMD_MASK << FDMA_CHAN_SHIFT(Channel)));

  *ppPddChannel = pFdmaChannel;

  return TRUE;

Error:

  if (pFdmaChannel) {

    if (pFdmaChannel->NodeVirtBase) {

      HalFreeCommonBuffer((PDMA_ADAPTER_OBJECT)pDmaAdapter,
                          sizeof(NODE_REGISTERS) * FDMA_MAX_MAP_REGISTERS,
                          pFdmaChannel->NodePhyBase,
                          pFdmaChannel->NodeVirtBase,
                          FALSE);
    }

    LocalFree(pFdmaChannel);
  }

  return FALSE;
}


//
// Deinitialize the FDMA channel.
//

BOOL FdmaDeinitializeDmaChannel (
  IN PDMA_ADAPTER_OBJ pDmaAdapter,
  IN ULONG Channel,
  IN PVOID pPddAdapter,
  IN OUT PPVOID ppPddChannel
)
{
  PPDD_ADAPTER_OBJ pFdmaAdapter = pPddAdapter;
  PPDD_CHANNEL_OBJ pFdmaChannel = *ppPddChannel;
  ULONG Temp;

  //
  // Perform some initial checks.
  //

  if (!pDmaAdapter || pDmaAdapter->Size != sizeof(DMA_ADAPTER_OBJ) ||
      Channel >= FDMA_MAX_CHANNELS ||
      !pPddAdapter || !ppPddChannel) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // Deinitialize the channel.
  //

  Temp = READ_FDMA_REG(&(pFdmaAdapter->pMBRegs->IntMask));
  Temp &= ~(FDMA_CHANNEL_INT_MASK << FDMA_CHAN_SHIFT(Channel));
  WRITE_FDMA_REG(&(pFdmaAdapter->pMBRegs->IntMask), Temp);

  WRITE_FDMA_REG(&(pFdmaAdapter->pMBRegs->CmdSet),  (FDMA_CHANNEL_CMD_FLUSH << FDMA_CHAN_SHIFT(Channel)));
  WRITE_FDMA_REG(&(pFdmaAdapter->pMBRegs->CmdClr),  (FDMA_CHANNEL_CMD_MASK << FDMA_CHAN_SHIFT(Channel)));
  WRITE_FDMA_REG(&(pFdmaAdapter->pMBRegs->IntClr),  (FDMA_CHANNEL_CMD_MASK << FDMA_CHAN_SHIFT(Channel)));

  Temp = READ_FDMA_REG(&(pFdmaAdapter->pMBRegs->CmdMask));
  Temp &= ~(FDMA_CHANNEL_CMD_MASK << FDMA_CHAN_SHIFT(Channel));
  WRITE_FDMA_REG(&(pFdmaAdapter->pMBRegs->CmdMask), Temp);

  //
  // Free common buffer allocated for DMA nodes.
  //

  HalFreeCommonBuffer((PDMA_ADAPTER_OBJECT)pDmaAdapter,
                      sizeof(NODE_REGISTERS) * FDMA_MAX_MAP_REGISTERS,
                      pFdmaChannel->NodePhyBase,
                      pFdmaChannel->NodeVirtBase,
                      FALSE);

  //
  // Free the FDMA channel pdd.
  //

  LocalFree(*ppPddChannel);

  *ppPddChannel = NULL;

  return TRUE;
}


//
// The FdmaAllocateMapRegister routine allocates, builds a
// FDMA node/map register for transfer.
//

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
)
{
  PPDD_CHANNEL_OBJ pFdmaChannel = pPddChannel;
  PNODE_REGISTERS pNode;

  //
  // Peform some initial checks.
  //

  if (!(pPddChannel || !Length || !ppPddMR)) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // Allocate a FDMA node/map register.
  //

  DequeueNode(&(pFdmaChannel->NodeFree), pNode);

  if (!pNode) {

    SetLastError(ERROR_NO_SYSTEM_RESOURCES);

    return FALSE;
  }

  //
  // Set up the FDMA node/map register.
  //

  if (Flags & DMA_FLAGS_WRITE_TO_DEVICE) {

    pNode->Saddr  = BufferAddr.LowPart;
    pNode->Daddr  = DeviceAddr.LowPart;
    pNode->Nbytes = Length;

    pNode->Ctrl.Reserved = 0;
    pNode->Ctrl.SrcInc = AddressIncrementing;

    if (Flags & DMA_FLAGS_INC_DEVICE_ADDRESS) {

      pNode->Ctrl.DstInc = AddressIncrementing;
      pNode->Length = pNode->Nbytes;
      pNode->Sstride = 0;
      pNode->Dstride = 0;

    } else {

      pNode->Ctrl.DstInc = AddressConstant;
#if 0
      pNode->Length = DmaWidth;
      pNode->Sstride = pNode->Length;
      pNode->Dstride = 0;
#else
      pNode->Length = 0x20;
      pNode->Sstride = 0x20;
      pNode->Dstride = 0;
#endif

    }

  } else {

    pNode->Saddr  = DeviceAddr.LowPart;
    pNode->Daddr  = BufferAddr.LowPart;
    pNode->Nbytes = Length;

    pNode->Ctrl.Reserved = 0;
    pNode->Ctrl.DstInc = AddressIncrementing;

    if (Flags & DMA_FLAGS_INC_DEVICE_ADDRESS) {

      pNode->Ctrl.SrcInc = AddressIncrementing;
#if 0
      pNode->Length = pNode->Nbytes;
      pNode->Sstride = 0;
      pNode->Dstride = 0;
#else
      pNode->Length = 0x20;
      pNode->Sstride = 0;
      pNode->Dstride = 0x20;
#endif

    } else {

      pNode->Ctrl.SrcInc = AddressConstant;
#if 0
      pNode->Length = DmaWidth;
      pNode->Sstride = 0;
      pNode->Dstride = pNode->Length;
#else
      pNode->Length = 0x20;
      pNode->Sstride = 0;
      pNode->Dstride = 0x20;
#endif

    }
  }

  //
  // Return the FDMA node/map register.
  //

  *ppPddMR = pNode;

  return TRUE;
}


//
// The FdmaFreeMapRegister routine frees a FDMA node/map register and
// places it back onto the free queue.
//

BOOL FdmaFreeMapRegister (
  IN PVOID pPddChannel,
  OUT PPVOID ppPddMR
)
{
  PPDD_CHANNEL_OBJ pFdmaChannel = pPddChannel;
  PNODE_REGISTERS pNode;

  //
  // Peform some initial checks.
  //

  if (!pPddChannel || !ppPddMR) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  pNode = *ppPddMR;

  //
  // Insert the FDMA node/map register to the free queue.
  //

  EnqueueNode(&(pFdmaChannel->NodeFree), pNode);

  *ppPddMR = NULL;

  return TRUE;
}


//
// The FdmaQueueTransfer routine queues a FDMA node/map register for transfer.
//

BOOL FdmaQueueTransfer (
  IN PVOID pPddChannel,
  IN PVOID pPddMR
)
{
  PPDD_CHANNEL_OBJ pFdmaChannel = pPddChannel;

  //
  // perform some initial checks.
  //

  if (!pPddChannel || !pPddMR) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // Link the node into the FDMA pending queue.
  //

  EnqueueNode(&(pFdmaChannel->NodePend), (PNODE_REGISTERS)pPddMR);

  return TRUE;
}


//
// The FdmaStartTransfer routine starts the FDMA engine to transfer all pending nodes.
//

BOOL FdmaStartTransfer (
  IN PVOID pPddChannel,
  IN ULONG ChannelNumber
)
{
  PPDD_CHANNEL_OBJ pFdmaChannel = pPddChannel;
  FDMA_CMD_STAT FdmaCmdStat;
  PNODE_REGISTERS pNode, pTemp;
  ULONG Index;

  //
  // Perform some initial checks.
  //

  if (!pPddChannel) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // There should be node(s) in the pending queue and no nodes in the transferring queue.
  //

  if (IsQueueEmpty(&(pFdmaChannel->NodePend)) || !IsQueueEmpty(&(pFdmaChannel->NodeTransfer))) {

    DEBUGMSG(0, (TEXT("No pending nodes or nodes already in transfer queue\r\n")));

    SetLastError(ERROR_IO_PENDING);

    return FALSE;
  }

  //
  // The FDMA engine should not be in a running state.
  //

  FdmaCmdStat.All = READ_FDMA_REG(&(pFdmaChannel->pChanCmd));

  if (FdmaCmdStat.Read.Sta == FDMA_CHANNEL_STAT_RUNNING) {

    DEBUGMSG(0, (TEXT("FDMA eninge already in running state\r\n")));

    SetLastError(ERROR_IO_PENDING);

    return FALSE;
  }

  //
  // Move all the nodes in the pending queue to the transfer queue.
  //

  MoveQueue(&(pFdmaChannel->NodePend), &(pFdmaChannel->NodeTransfer));

  //
  // Translate all the node links from virtual address to physical address so
  // the FDMA engine can traverse them.
  //

  pNode = pFdmaChannel->NodeTransfer.Head;

  for (Index = 0; Index < (GetQueueCount(&(pFdmaChannel->NodeTransfer)) - 1); Index++) {

    pTemp = pNode;

    pNode = pNode->NextPtr;

    pTemp->NextPtr = (PNODE_REGISTERS)VirtToPhy(pTemp->NextPtr, pFdmaChannel->NodeVirtBase, pFdmaChannel->NodePhyBase);
  }

  //
  // Kick off the FDMA engine.
  //

  FdmaCmdStat.Write.Data = VirtToPhy(pFdmaChannel->NodeTransfer.Head,
                                     pFdmaChannel->NodeVirtBase,
                                     pFdmaChannel->NodePhyBase) >> FDMA_NODE_ALIGNMENT_SHIFT;

  FdmaCmdStat.Write.Cmd = FDMA_CHANNEL_CMD_START;

  WRITE_FDMA_REG(pFdmaChannel->pChanCmd, FdmaCmdStat.All);

  WRITE_FDMA_REG(pFdmaChannel->pCmdSet, FDMA_CHANNEL_CMD_START << FDMA_CHAN_SHIFT(ChannelNumber));

  return TRUE;
}


//
// The FdmaCloseTransfer routine closes the node/map registers which have been transferred.
//

BOOL FdmaCloseTransfer (
  IN PVOID pPddChannel
)
{
  FDMA_CMD_STAT FdmaCmdStat;
  PPDD_CHANNEL_OBJ pFdmaChannel = pPddChannel;

  //
  // Perform some initial checks.
  //

  if (!pPddChannel) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // The FDMA engine should not be in a running state.
  //

  FdmaCmdStat.All = READ_FDMA_REG(&(pFdmaChannel->pChanCmd));

  if (FdmaCmdStat.Read.Sta == FDMA_CHANNEL_STAT_RUNNING) {

    RETAILMSG(1, (TEXT("FdmaCloseTransfer can not close, FDMA still in running state\r\n")));

    SetLastError(ERROR_IO_PENDING);

    return FALSE;
  }

  //
  // Just re-initialize the transfer queue.  Althought it seems like we will
  // lose all the queued FDMA nodes/map registers, that is not really the case.
  // The FDMA nodes/map registers will be returned to the free queue when
  // FdmaFreeMapRegister is called.  FDMA nodes/map registers will just be
  // floating, not belonging to any queue for the moment.
  //

  InitQueue(&(pFdmaChannel->NodeTransfer));

  return TRUE;
}


//
// The FdmaCloseTransfer routine closes the node/map registers which have been transferred.
//

DMA_INTERRUPT_STATUS FdmaServiceInterrupt(
  IN ULONG Channel,
  IN PVOID pPddAdapter,
  IN PVOID pPddChannel
)
{
  PPDD_ADAPTER_OBJ pFdmaAdapter = pPddAdapter;
  PPDD_CHANNEL_OBJ pFdmaChannel = pPddChannel;
  FDMA_CMD_STAT FdmaCmdStat;
  DMA_INTERRUPT_STATUS IntStat;

  //
  // Perform some initial checks.
  //

  if (Channel > FDMA_MAX_CHANNELS ||
      !pPddAdapter || !pPddChannel) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
  }

  //
  // Check the interrupt for the channel.
  //

  IntStat = (READ_FDMA_REG(&(pFdmaAdapter->pMBRegs->IntStat)) >> FDMA_CHAN_SHIFT(Channel));

  DEBUGMSG(0, (TEXT("FDMA int: %x\r\n"), IntStat));

  if (IntStat & FDMA_CHANNEL_INT_ERR) {

    //
    // FDMA error interrupt.
    //

    IntStat = DmaIntError;

  } else if (IntStat & FDMA_CHANNEL_INT_MSG) {

    //
    // FDMA message interrupt.
    //

    FdmaCmdStat.All = READ_FDMA_REG(&(pFdmaChannel->pChanCmd));

    DEBUGMSG(0, (TEXT("FDMA status: %x\r\n"), FdmaCmdStat.Read.Sta));

    if (FdmaCmdStat.Read.Sta == FDMA_CHANNEL_STAT_IDLE) {

      //
      // FDMA engine completed the transfer.
      //

      IntStat = DmaIntCompleted;

    } else if (FdmaCmdStat.Read.Sta == FDMA_CHANNEL_STAT_PAUSED) {

      //
      // FDMA engine has been paused.
      //

      IntStat = DmaIntPaused;

    } else if (FdmaCmdStat.Read.Sta == FDMA_CHANNEL_STAT_RUNNING) {

      //
      // FDMA engine is currently running.
      //

      IntStat = DmaIntRunning;

    } else {

      //
      // Not a valid state.
      //

      IntStat = DmaIntNone;
    }

  } else {

    //
    // Not a valid channel interrupt.
    //

    IntStat = DmaIntNone;
  }

  //
  // Clear the interrupt.
  //

  WRITE_FDMA_REG(&(pFdmaAdapter->pMBRegs->IntClr), FDMA_CHANNEL_INT_MASK << FDMA_CHAN_SHIFT(Channel));

  return IntStat;
}
