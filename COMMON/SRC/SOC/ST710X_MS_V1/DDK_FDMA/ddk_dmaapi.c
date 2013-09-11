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
    ddk_dmaapi.c

Abstract:
    Contains code for DMA API functions.

Notes:

--*/

#include <windows.h>
#include <types.h>
#include <ceddk.h>
#include <ddkreg.h>
#include <nkintr.h>
#include <safeint.hxx>

#include "stb7100reg.h"
#include "stb7100_ioport.h"
#include "sh4_intc.h"
#include "SH4202T_intc2_irq.h"

#include "oaldma.h"
#include "mdddma.h"
#include "pdddma.h"


//
// MDD defines.
//

#define MAX_NOTIFY_COUNT 32


//
// MDD adapter globals.
//

static ULONG gNumberOfAdapters = 0;
static PMDD_ADAPTER_OBJ gDmaAdapter = NULL;


//
// MDD internal routines.
//

VOID DumpMRQueues(IN DMA_CHANNEL_HANDLE hDmaChannel);


//
// DPC thread to handle notify events.
//

VOID OALDpcThread(
  IN PMDD_ADAPTER_OBJ pMddAdapter
)
{
  ULONG Chan, NotifyCount;
  PLIST_ENTRY pListEntry;
  PMDD_CHANNEL_OBJ pMddChannel;
  PMDD_MAP_REGISTER pMddMR;
  LPDMA_TRANSFER_NOTIFY_ROUTINE Routine;
  PVOID Context[MAX_NOTIFY_COUNT];

  //
  // Check parameter.
  //

  if (!pMddAdapter || pMddAdapter->Size != sizeof(MDD_ADAPTER_OBJ)) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return;
  }

  //
  // Bump up the thread priority.
  //

  CeSetThreadPriority(GetCurrentThread(), DMA_DPC_PRIORITY_LEVEL);

  //
  // Process any pending DPC events.
  //

  while (1) {

    WaitForSingleObject(pMddAdapter->hDpcEvent, INFINITE);

    DEBUGMSG(0, (TEXT("DMA DPC event\r\n")));

    //
    // See if we need to exit.
    //

    if (pMddAdapter->AllThreadsExit) {

      DEBUGMSG(0, (TEXT("DMA DPC thread exiting\r\n")));

      break;
    }

    //
    // Check all the channels.
    //

    for (Chan = DMA_CHANNEL_START; Chan < pMddAdapter->AdapterDesc.NumberOfChannels; Chan++) {

      pMddChannel = &(pMddAdapter->Channel[Chan]);

      if (pMddChannel->InUse && !IsListEmpty(&(pMddChannel->MRNotify))) {

        DEBUGMSG(0, (TEXT("DMA DPC notify for channel %x\r\n"), Chan));

Again:  EnterCriticalSection(&(pMddChannel->CSChannel));

        NotifyCount = 0;

        do {

          pListEntry = pMddChannel->MRNotify.Flink;

          pMddMR = CONTAINING_RECORD(pListEntry, MDD_MAP_REGISTER, ListEntry);

          //
          // Save away the notify routine.
          //

          if (NotifyCount == 0) {

            Routine = pMddMR->NotifyRoutine;
          }

          //
          // Queue up multiple notifications if they are to the same routine.  We'll
          // pass multiple notify contexts to the same notify routine to be
          // more efficient.
          //

          if (Routine == pMddMR->NotifyRoutine) {

            Context[NotifyCount] = pMddMR->NotifyContext;

            NotifyCount++;

          } else {

            //
            // Queued up as many notifications as we can for this round.
            //

            break;
          }

          //
          // Auto close the DMA transfer, if necessary.
          //

          if (pMddMR->Flags & DMA_FLAGS_AUTO_CLOSE_TRANSFER) {

            DEBUGMSG(0, (TEXT("DMA DPC auto closed MR %x\r\n"), pListEntry));

            OALCloseDmaTransfer(pMddMR);

          } else {

            //
            // Move map register from notify to done queue.
            //

            DEBUGMSG(0, (TEXT("DMA DPC Move MR %x to done queue\r\n"), pListEntry));

            pListEntry = RemoveHeadList(&(pMddChannel->MRNotify));

            InsertTailList(&(pMddChannel->MRDone), pListEntry);
          }

        } while (NotifyCount < MAX_NOTIFY_COUNT && !IsListEmpty(&(pMddChannel->MRNotify)));

        LeaveCriticalSection(&(pMddChannel->CSChannel));

        //
        // Call the notify routine, if available.  Do not hold the critical sections
        // when making callback as it may cause deadlock.
        //

        if (NotifyCount && Routine) {

          DEBUGMSG(0, (TEXT("DMA DPC call notify routine for channel %x\r\n"), Chan));

          (Routine)(NotifyCount, Context);
        }

        //
        // Do it again if there are still map registers in this channel's notify queue.
        //

        if (!IsListEmpty(&(pMddChannel->MRNotify))) {

          goto Again;

        }
      }
    }
  }

  return;
}


//
// Ist thread to handle DMA interrupts.
//

VOID OALIstThread(
  IN PMDD_ADAPTER_OBJ pMddAdapter
)
{
  BOOL Status;
  BOOL DpcNotify;
  ULONG Chan;
  PLIST_ENTRY pListEntry;
  PMDD_CHANNEL_OBJ pMddChannel;
  PMDD_MAP_REGISTER pMddMR;
  DMA_INTERRUPT_STATUS IntStatus;

  //
  // Check parameter.
  //

  if (!pMddAdapter || pMddAdapter->Size != sizeof(MDD_ADAPTER_OBJ)) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return;
  }

  //
  // Bump up the thread priority.
  //

  CeSetThreadPriority(GetCurrentThread(), DMA_IST_PRIORITY_LEVEL);

  //
  // Process any pending Ist events.
  //

  while (1) {

    WaitForSingleObject(pMddAdapter->hIstEvent, INFINITE);

    DEBUGMSG(0, (TEXT("DMA interrupt\r\n")));

    DpcNotify = FALSE;

    //
    // See if we need to exit.
    //

    if (pMddAdapter->AllThreadsExit) {

      DEBUGMSG(0, (TEXT("DMA IST thread exiting\r\n")));

      break;
    }

    //
    // Check at all the channels.
    //

    for (Chan = DMA_CHANNEL_START; Chan < pMddAdapter->AdapterDesc.NumberOfChannels; Chan++) {

      pMddChannel = &(pMddAdapter->Channel[Chan]);

      if (pMddChannel->InUse && !IsListEmpty(&(pMddChannel->MRTransfer))) {

        EnterCriticalSection(&(pMddChannel->CSChannel));

        IntStatus = FdmaServiceInterrupt(Chan,
                                         pMddAdapter->pPddAdapter,
                                         pMddChannel->pPddChannel);

        DEBUGMSG(0, (TEXT("Channel: %x, Int: %x\r\n"), Chan, IntStatus));

        if (IntStatus == DmaIntCompleted ||
            IntStatus == DmaIntError) {

EmptyQueue:

          //
          // DMA transfer completed, move all map registers to notify queue.
          //

          while (!IsListEmpty(&(pMddChannel->MRTransfer))) {

            //
            // Move map register from transfer to notify queue.
            //

            pListEntry = RemoveHeadList(&(pMddChannel->MRTransfer));

            //
            // Set the appropriate map register status.
            //

            pMddMR = CONTAINING_RECORD(pListEntry, MDD_MAP_REGISTER, ListEntry);

            if (IntStatus == DmaIntCompleted) {

              pMddMR->MRStatus = DmaStatusCompleted;

            } else {

              RETAILMSG(1, (TEXT("DMA IST completed with error\r\n")));

              pMddMR->MRStatus = DmaStatusError;

            }

            //
            // Is notification needed?
            //

           if (pMddMR->NotifyRoutine) {

              //
              // Notification requested.
              //

              DEBUGMSG(0, (TEXT("DMA IST Move MR %x to notify queue\r\n"), pListEntry));

              InsertTailList(&(pMddChannel->MRNotify), pListEntry);

              //
              // Let the DPC thread call the notify routine.
              //

              DpcNotify = TRUE;

            } else {

              //
              // Notification not requested.
              //

              if (pMddMR->Flags & DMA_FLAGS_AUTO_CLOSE_TRANSFER) {

                //
                // Auto close the DMA transfer, if necessary.
                //

                DEBUGMSG(0, (TEXT("DMA IST auto closed MR %x\r\n"), pListEntry));

                OALCloseDmaTransfer(pMddMR);

              } else {

                //
                // Transfer is done, wait for close.
                //

                DEBUGMSG(0, (TEXT("DMA IST Move MR %x to done queue\r\n"), pListEntry));

                InsertTailList(&(pMddChannel->MRDone), pListEntry);
              }
            }
          }

          //
          // Inform PDD to close the transfer.
          //

          if (!FdmaCloseTransfer(pMddChannel->pPddChannel)) {

            RETAILMSG(1, (TEXT("DMA IST PDD could not close the transfer\r\n")));
          }

        } else if (IntStatus == DmaIntNone) {

          //
          // This channel did not interrupt, go to next channel.
          //

          goto Done;

        } else if (IntStatus == DmaIntRunning) {

          //
          // DMA is running, don't need to do anything.
          //

          RETAILMSG(1, (TEXT("DMA IST DMA engine still running \r\n")));

          goto Done;

        } else if (IntStatus == DmaIntPaused) {

          //
          // DMA paused, re-start is needed later.
          //

          RETAILMSG(1, (TEXT("DMA IST DMA engine paused\r\n")));

          goto Done;

        } else {

          //
          // Not a recongized interrupt.
          //

          RETAILMSG(1, (TEXT("DMA IST not a recongized interrupt\r\n")));

          goto Done;
        }

        //
        // Start any pending transfers for this channel.
        //

        if (!IsListEmpty(&(pMddChannel->MRPend))) {

          DEBUGMSG(0, (TEXT("DMA IST starting pending transfers\r\n")));

          while (!IsListEmpty(&(pMddChannel->MRPend))) {

            //
            // Remove a map register from pend queue.
            //

            pListEntry = RemoveHeadList(&(pMddChannel->MRPend));

            DEBUGMSG(0, (TEXT("DMA IST move MR %x to transfer queue\r\n"), pListEntry));

            pMddMR = CONTAINING_RECORD(pListEntry, MDD_MAP_REGISTER, ListEntry);

            //
            // Allow the PDD to queue the transfer.
            //

            Status = FdmaQueueTransfer(pMddChannel->pPddChannel,
                                       pMddMR->pPddMR);

            //
            // Set the appropriate map register status.
            //

            if (Status) {

              //
              // PDD queued transfer, set MR status.
              //

              pMddMR->MRStatus = DmaStatusTransferring;

              InsertTailList(&(pMddChannel->MRTransfer), pListEntry);

            } else {

              //
              // PDD failed to queue transfer, set MR status.  Let DPC thread call the notify routine.
              //

              RETAILMSG(1, (TEXT("DMA IST failed to queue the transfer\r\n")));

              pMddMR->MRStatus = DmaStatusError;

              InsertTailList(&(pMddChannel->MRNotify), pListEntry);

              DpcNotify = TRUE;
            }
          }

          //
          // Allow the PDD to start the transfer, if any.
          //

          if (!IsListEmpty(&(pMddChannel->MRTransfer))) {

            Status = FdmaStartTransfer(pMddChannel->pPddChannel,
                                       pMddChannel->ChannelNumber);

            //
            // Transfer failed to start, set error and move it to the notify queue.
            //

            if (!Status) {

              RETAILMSG(1, (TEXT("DMA IST failed to start the transfer\r\n")));
        
              IntStatus = DmaIntError;

              goto EmptyQueue;
            }
          }

        } else {

          //
          // No other pending transfers for this channel, signal DMA completion event.
          //

          SetEvent(pMddChannel->hDmaCompleteEvent);
        }

Done:   LeaveCriticalSection(&(pMddChannel->CSChannel));
      }
    }

    //
    // Notify the DPC if needed.
    //

    if (DpcNotify) {

      DEBUGMSG(0, (TEXT("DMA IST notify DMA DPC\r\n")));

      SetEvent(pMddAdapter->hDpcEvent);
    }

    //
    // Re-enable the interrupt.
    //

    InterruptDone(pMddAdapter->SysIntr);
  }

  return;
}


//
// The OALDeinitializeDmaAdapter routine shuts down the hardware and frees
// all the allocations for all available adapters.
//

BOOL OALDeinitializeDmaAdapters (
  VOID
)
{
  ULONG AdapterIndex, Chan;
  PMDD_CHANNEL_OBJ pMddChannel;

  //
  // Disable and free all adapters.
  //

  if (gDmaAdapter) {

    for (AdapterIndex = 0; AdapterIndex < gNumberOfAdapters; AdapterIndex++) {

      if (gDmaAdapter[AdapterIndex].Size) {

        //
        // Prepare for the shutdown.
        //

        gDmaAdapter[AdapterIndex].Size = 0;

        gDmaAdapter[AdapterIndex].AllThreadsExit = TRUE;

        EnterCriticalSection(&(gDmaAdapter[AdapterIndex].CSAdapter));

        //
        // Disable and free all channels.
        //

        for (Chan = DMA_CHANNEL_START; Chan < gDmaAdapter[AdapterIndex].AdapterDesc.NumberOfChannels; Chan++) {

          pMddChannel = &(gDmaAdapter[AdapterIndex].Channel[Chan]);

          if (pMddChannel->InUse) {

            OALFreeDmaChannel(pMddChannel);
          }
        }

        //
        // De-allocate space for the channel MDD structures.
        //

        if (gDmaAdapter[AdapterIndex].Channel) {

          LocalFree(gDmaAdapter[AdapterIndex].Channel);
        }

        //
        // Disable and free the SysIntr.
        //

        if (gDmaAdapter[AdapterIndex].Irq && gDmaAdapter[AdapterIndex].SysIntr != SYSINTR_UNDEFINED) {

          InterruptDisable(gDmaAdapter[AdapterIndex].SysIntr);

          KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR,
                          &(gDmaAdapter[AdapterIndex].SysIntr),
                          sizeof(gDmaAdapter[AdapterIndex].SysIntr),
                          NULL,
                          0,
                          NULL);
        }

        //
        // Terminate the IST thread.
        //

        if (gDmaAdapter[AdapterIndex].hIstThread) {

          SetEvent(gDmaAdapter[AdapterIndex].hIstEvent);

          WaitForSingleObject(gDmaAdapter[AdapterIndex].hIstThread, 1000);

          CloseHandle(gDmaAdapter[AdapterIndex].hIstThread);
        }

        //
        // Close the IST event.
        //

        if (gDmaAdapter[AdapterIndex].hIstEvent) {

          CloseHandle(gDmaAdapter[AdapterIndex].hIstEvent);
        }

        //
        // Terminate the DPC thread.
        //

        if (gDmaAdapter[AdapterIndex].hDpcThread) {

          SetEvent(gDmaAdapter[AdapterIndex].hDpcEvent);

          WaitForSingleObject(gDmaAdapter[AdapterIndex].hDpcThread, 1000);

          CloseHandle(gDmaAdapter[AdapterIndex].hDpcThread);

          gDmaAdapter[AdapterIndex].hDpcThread = NULL;
        }

        //
        // Close the DPC event.
        //

        if (gDmaAdapter[AdapterIndex].hDpcEvent) {

          CloseHandle(gDmaAdapter[AdapterIndex].hDpcEvent);

          gDmaAdapter[AdapterIndex].hDpcEvent = NULL;
        }

        //
        // Allow the PDD to shutdown the hardware.
        //

        FdmaDeinitializeDmaAdapter(AdapterIndex,
                                   &gDmaAdapter[AdapterIndex].AdapterDesc,
                                   &gDmaAdapter[AdapterIndex].pPddAdapter);

        LeaveCriticalSection(&(gDmaAdapter[AdapterIndex].CSAdapter));

        DeleteCriticalSection(&(gDmaAdapter[AdapterIndex].CSAdapter));
      }
    }

    LocalFree(gDmaAdapter);

    gNumberOfAdapters = 0;
    gDmaAdapter = NULL;
  }

  return TRUE;
}


//
// The OALInitializeDmaAdapter routine fills out the MDD DMA adapter structure
// and initializes the hardware for all available adapters.
//

BOOL OALInitializeDmaAdapters (
  VOID
)
{
  ULONG AdapterIndex;
  UINT ChannelSize = 0;

  //
  // Get the number of DMA adapters in the system.
  //

  gNumberOfAdapters = FdmaGetNumberOfAdapters();

  //
  // Make sure there is at least one DMA adapter.
  //

  if (gNumberOfAdapters == 0) {

    SetLastError(ERROR_NO_SYSTEM_RESOURCES);

    return FALSE;
  }

  //
  // Allocate space for the MDD adapter objects.
  //

  gDmaAdapter = LocalAlloc(LMEM_ZEROINIT, sizeof(MDD_ADAPTER_OBJ) * gNumberOfAdapters);

  if (!gDmaAdapter) {

    SetLastError(ERROR_OUTOFMEMORY);

    return FALSE;
  }

  //
  // Initialize each of the DMA adapters.
  //

  for (AdapterIndex = 0; AdapterIndex < gNumberOfAdapters; AdapterIndex++) {

    //
    // Fill in the size of the DMA adapter descriptor object.
    //

    gDmaAdapter[AdapterIndex].AdapterDesc.Size = sizeof(DMA_ADAPTER_OBJ);

    //
    // Initialize the adapter.
    //

    if (!FdmaInitializeDmaAdapter(AdapterIndex,
                                  &(gDmaAdapter[AdapterIndex].AdapterDesc),
                                  &(gDmaAdapter[AdapterIndex].pPddAdapter))) {

      goto Error;
    }

    //
    // Fill in the size of the MDD adapter object.
    //

    gDmaAdapter[AdapterIndex].Size = sizeof(MDD_ADAPTER_OBJ);

    //
    // Allocate space for the channel MDD structures.
    //

    if (!safeIntUMul((UINT)sizeof(MDD_CHANNEL_OBJ), (UINT)gDmaAdapter[AdapterIndex].AdapterDesc.NumberOfChannels, &ChannelSize)){
      SetLastError(ERROR_INVALID_PARAMETER);
      goto Error;
    }

    gDmaAdapter[AdapterIndex].Channel = LocalAlloc(LMEM_ZEROINIT, ChannelSize);

    if (!gDmaAdapter[AdapterIndex].Channel) {

      SetLastError(ERROR_OUTOFMEMORY);

      goto Error;
    }

    //
    // Initialize the critical section.
    //

    InitializeCriticalSection(&(gDmaAdapter[AdapterIndex].CSAdapter));

    //
    // Prepare to start up the IST and DPC threads for this adapter.
    //

    gDmaAdapter[AdapterIndex].AllThreadsExit = FALSE;

    //
    // Create the DPC event.
    //

    gDmaAdapter[AdapterIndex].hDpcEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!gDmaAdapter[AdapterIndex].hDpcEvent) {

      goto Error;
    }

    //
    // Create the DPC thread.
    //

    gDmaAdapter[AdapterIndex].hDpcThread = CreateThread(NULL,
                                                        0,
                                                        (LPTHREAD_START_ROUTINE)OALDpcThread,
                                                        &(gDmaAdapter[AdapterIndex]),
                                                        0,
                                                        NULL);

    if (!gDmaAdapter[AdapterIndex].hDpcThread) {

      goto Error;
    }

    //
    // Translate IRQ to SYSINTR, create event, and initialize the interrupt.
    //

    if (!FdmaGetAdapterIrq(AdapterIndex, &(gDmaAdapter[AdapterIndex].Irq))) {

      goto Error;
    }

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
                         &(gDmaAdapter[AdapterIndex].Irq),
                         sizeof(gDmaAdapter[AdapterIndex].Irq),
                         &(gDmaAdapter[AdapterIndex].SysIntr),
                         sizeof(gDmaAdapter[AdapterIndex].SysIntr),
                         NULL)) {

      goto Error;
    }

    if (gDmaAdapter[AdapterIndex].SysIntr == SYSINTR_UNDEFINED) {

      goto Error;
    }

    gDmaAdapter[AdapterIndex].hIstEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!gDmaAdapter[AdapterIndex].hIstEvent) {

      goto Error;
    }

    if (!InterruptInitialize(gDmaAdapter[AdapterIndex].SysIntr, gDmaAdapter[AdapterIndex].hIstEvent, 0, 0)) {

      goto Error;
    }

    //
    // Create the IST thread.
    //

    gDmaAdapter[AdapterIndex].hIstThread = CreateThread(NULL,
                                                        0,
                                                        (LPTHREAD_START_ROUTINE)OALIstThread,
                                                        &(gDmaAdapter[AdapterIndex]),
                                                        0,
                                                        NULL);

    if (!gDmaAdapter[AdapterIndex].hIstThread) {

      goto Error;
    }
  }

  return TRUE;

Error:

  //
  // Free up any allocation.
  //

  OALDeinitializeDmaAdapters();

  return FALSE;
}


//
// The OALGetDmaAdapter routine fill out a pointer to the DMA adapter structure for a physical device.
//

BOOL OALGetDmaAdapter (
    IN PDEVICE_DMA_REQUIREMENT_INFO pDeviceDmaRequirementInfo,
    IN OUT PDMA_ADAPTER_OBJ pDmaAdapter
)
{
  ULONG AdapterIndex;
  PMDD_ADAPTER_OBJ pMddAdapter = NULL;

  //
  // Perform some initial checks.
  //

  if (!pDeviceDmaRequirementInfo ||!pDmaAdapter ||
    pDeviceDmaRequirementInfo->Size != sizeof(DEVICE_DMA_REQUIREMENT_INFO) ||
    pDmaAdapter->Size != sizeof(DMA_ADAPTER_OBJ)) {

    SetLastError(ERROR_INVALID_PARAMETER);
    
    return FALSE;
  }

  //
  // Perform some additional checks.
  //

  if (pDeviceDmaRequirementInfo->DmaAdapterNumber != DMA_ADAPTER_ANY &&
      pDeviceDmaRequirementInfo->DmaAdapterNumber >= gNumberOfAdapters) {

    SetLastError(ERROR_NO_SYSTEM_RESOURCES);
    
    return FALSE;
  }

  //
  // Search for the best matching adapter.
  //

  for (AdapterIndex = 0; AdapterIndex < gNumberOfAdapters; AdapterIndex++) {

    if (gDmaAdapter && gDmaAdapter[AdapterIndex].Size == sizeof(MDD_ADAPTER_OBJ) &&
        pDeviceDmaRequirementInfo->InterfaceType == gDmaAdapter[AdapterIndex].AdapterDesc.InterfaceType &&
        pDeviceDmaRequirementInfo->BusNumber == gDmaAdapter[AdapterIndex].AdapterDesc.BusNumber &&
        (pDeviceDmaRequirementInfo->DmaAdapterNumber == DMA_ADAPTER_ANY ||
         pDeviceDmaRequirementInfo->DmaAdapterNumber == gDmaAdapter[AdapterIndex].AdapterDesc.DmaAdapterNumber) &&
        pDeviceDmaRequirementInfo->DemandMode <= gDmaAdapter[AdapterIndex].AdapterDesc.DemandMode &&
        pDeviceDmaRequirementInfo->DmaWidth <= gDmaAdapter[AdapterIndex].AdapterDesc.DmaWidth &&
        pDeviceDmaRequirementInfo->DmaSpeed <= gDmaAdapter[AdapterIndex].AdapterDesc.DmaSpeed) {

      //
      // Found one.
      //

      pMddAdapter = &(gDmaAdapter[AdapterIndex]);

      break;
    }
  }

  //
  // Exit if one was not found.
  //

  if (!pMddAdapter) {

    SetLastError(ERROR_NO_SYSTEM_RESOURCES);
    
    return FALSE;
  }

  //
  // Set the adapter information and return it.
  //

  pDmaAdapter->InterfaceType = gDmaAdapter[AdapterIndex].AdapterDesc.InterfaceType;
  pDmaAdapter->BusNumber = gDmaAdapter[AdapterIndex].AdapterDesc.BusNumber;
  pDmaAdapter->DmaAdapterNumber = gDmaAdapter[AdapterIndex].AdapterDesc.DmaAdapterNumber;
  pDmaAdapter->DmaAdapterObj = pMddAdapter;
  pDmaAdapter->NumberOfChannels = gDmaAdapter[AdapterIndex].AdapterDesc.NumberOfChannels;
  pDmaAdapter->NumberOfMapRegisters = gDmaAdapter[AdapterIndex].AdapterDesc.NumberOfMapRegisters;
  pDmaAdapter->MaximunSizeOfEachRegister = gDmaAdapter[AdapterIndex].AdapterDesc.MaximunSizeOfEachRegister;
  pDmaAdapter->DemandMode = pDeviceDmaRequirementInfo->DemandMode;
  pDmaAdapter->DmaWidth = pDeviceDmaRequirementInfo->DmaWidth;
  pDmaAdapter->DmaSpeed = pDeviceDmaRequirementInfo->DmaSpeed;

  return TRUE;
}


//
// The AllocateAdapterChannel routine prepares the system for a DMA operation on behalf of the target device
// and return handle that can be used by OALIssueDMATransfer.
//

DMA_CHANNEL_HANDLE OALAllocateDmaChannel (
    IN PDMA_ADAPTER_OBJ pDmaAdapter,
    IN ULONG ulRequestedChannel,
    IN ULONG ulAddressSpace,
    IN PHYSICAL_ADDRESS DeviceIoAddress
)
{
  ULONG Index;
  PMDD_ADAPTER_OBJ pMddAdapter;
  PMDD_CHANNEL_OBJ pMddChannel;
  PMDD_MAP_REGISTER pMRTemp;

  //
  // Perform some initial checks.
  //

  if (!pDmaAdapter || pDmaAdapter->Size != sizeof(DMA_ADAPTER_OBJ)) {

    SetLastError(ERROR_INVALID_PARAMETER);
    
    return NULL;
  }

  pMddAdapter = (PMDD_ADAPTER_OBJ)pDmaAdapter->DmaAdapterObj;

  //
  // Perform some additional checks.
  //

  if (!pMddAdapter || pMddAdapter->Size != sizeof(MDD_ADAPTER_OBJ) ||
      (ulRequestedChannel != DMA_CHANNEL_ANY &&
       ulRequestedChannel >= pMddAdapter->AdapterDesc.NumberOfChannels)) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return NULL;
  }

  pMddChannel = NULL;

  //
  // Allocate the DMA channel.
  //

  EnterCriticalSection(&(pMddAdapter->CSAdapter));

  if (ulRequestedChannel == DMA_CHANNEL_ANY) {

    //
    // See if any DMA channel is available.
    //

    for (ulRequestedChannel = DMA_CHANNEL_START; ulRequestedChannel < pMddAdapter->AdapterDesc.NumberOfChannels; ulRequestedChannel++) {

      if (pMddAdapter->Channel[ulRequestedChannel].Size == 0 &&
          pMddAdapter->Channel[ulRequestedChannel].InUse == FALSE) {
        
        pMddChannel = &(pMddAdapter->Channel[ulRequestedChannel]);

        break;
      }
    }

  } else {

    //
    // See if the specific channel is available.
    //

    if (pMddAdapter->Channel[ulRequestedChannel].Size == 0 &&
        pMddAdapter->Channel[ulRequestedChannel].InUse == FALSE) {
        
      pMddChannel = &(pMddAdapter->Channel[ulRequestedChannel]);
    }
  }

  //
  // If channel is avaiable, allocate it and fill in the information.
  //

  if (pMddChannel) {

    //
    // Initialize the map register list entries.
    //

    InitializeListHead(&(pMddChannel->MRFree));
    InitializeListHead(&(pMddChannel->MRPend));
    InitializeListHead(&(pMddChannel->MRTransfer));
    InitializeListHead(&(pMddChannel->MRNotify));
    InitializeListHead(&(pMddChannel->MRDone));

    //
    // Create the DMA completion event.
    //

    pMddChannel->hDmaCompleteEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    if (!pMddChannel->hDmaCompleteEvent) {

      goto Error;
    }

    //
    // Allocate the map registers.
    //

    pMddChannel->pMRAlloc = LocalAlloc(LMEM_ZEROINIT, sizeof(MDD_MAP_REGISTER) * pMddAdapter->AdapterDesc.NumberOfMapRegisters);

    if (!(pMddChannel->pMRAlloc)) {

      SetLastError(ERROR_OUTOFMEMORY);

      goto Error;
    }

    pMRTemp = pMddChannel->pMRAlloc;

    for (Index = 0; Index < pMddAdapter->AdapterDesc.NumberOfMapRegisters; Index++) {

      InsertTailList(&(pMddChannel->MRFree), &(pMRTemp->ListEntry));

      pMRTemp++;
    }

    //
    // Initialize the channel hardware.
    //

    if (!FdmaInitializeDmaChannel(pDmaAdapter,
                                  ulRequestedChannel,
                                  pMddAdapter->pPddAdapter,
                                  &(pMddChannel->pPddChannel))) {

      goto Error;
    }

    //
    // Fill in DMA Channel information.
    //

    pMddChannel->Size = sizeof(MDD_CHANNEL_OBJ);
    pMddChannel->InUse = TRUE;
    pMddChannel->pAdapter = pMddAdapter;
    pMddChannel->ChannelNumber = ulRequestedChannel;
    pMddChannel->DeviceAddr = DeviceIoAddress;
    pMddChannel->DemandMode = pDmaAdapter->DemandMode;
    pMddChannel->DmaWidth = pDmaAdapter->DmaWidth;
    pMddChannel->DmaSpeed = pDmaAdapter->DmaSpeed;

    //
    // Initialize the channel critical section.
    //

    InitializeCriticalSection(&(pMddChannel->CSChannel));

  } else {

    //
    // Channel not available.
    //

    SetLastError(ERROR_NO_SYSTEM_RESOURCES);

  }

  LeaveCriticalSection(&(pMddAdapter->CSAdapter));

  DumpMRQueues(pMddChannel);

  return (DMA_CHANNEL_HANDLE)pMddChannel;

Error:

  //
  // Free any memory allocated.
  //

  if (pMddChannel->hDmaCompleteEvent) {

    CloseHandle(pMddChannel->hDmaCompleteEvent);

    pMddChannel->hDmaCompleteEvent = NULL;
  }

  if (pMddChannel->pMRAlloc) {

    LocalFree(pMddChannel->pMRAlloc);

    pMddChannel->pMRAlloc = NULL;
  }

  LeaveCriticalSection(&(pMddAdapter->CSAdapter));

  return NULL;
}


//
// This function frees a DMA Channel buffer allocated by OALAllocateAdapterChannel, along with all resources
// the DMA Channel uses.
//

BOOL OALFreeDmaChannel(
   IN DMA_CHANNEL_HANDLE hDmaChannel
)
{
  PMDD_ADAPTER_OBJ pMddAdapter;
  PMDD_CHANNEL_OBJ pMddChannel = hDmaChannel;

  //
  // Perform some initial checks.
  //

  if (!hDmaChannel || pMddChannel->Size != sizeof(MDD_CHANNEL_OBJ) ||
      !(pMddChannel->InUse) || !(pMddChannel->pAdapter) ||
      pMddChannel->pAdapter->Size != sizeof(MDD_ADAPTER_OBJ)) {

    SetLastError(ERROR_INVALID_HANDLE);

    return FALSE;
  }

  pMddAdapter = pMddChannel->pAdapter;

  //
  // Free the DMA channel.
  //

  pMddChannel->Size = 0;
  pMddChannel->InUse = FALSE;

  EnterCriticalSection(&(pMddAdapter->CSAdapter));
  EnterCriticalSection(&(pMddChannel->CSChannel));

  //
  // Deinitialize the channel hardware.
  //

  FdmaDeinitializeDmaChannel(&(pMddAdapter->AdapterDesc),
                             pMddChannel->ChannelNumber,
                             pMddAdapter->pPddAdapter,
                             &(pMddChannel->pPddChannel));

  pMddChannel->pAdapter = NULL;

  //
  // Free the DMA completion event.
  //

  SetEvent(pMddChannel->hDmaCompleteEvent);

  if (pMddChannel->hDmaCompleteEvent) {

    CloseHandle(pMddChannel->hDmaCompleteEvent);

    pMddChannel->hDmaCompleteEvent = NULL;
  }

  //
  // Free map register memory allocated.
  //

  if (pMddChannel->pMRAlloc) {

    LocalFree(pMddChannel->pMRAlloc);

    pMddChannel->pMRAlloc = NULL;
  }

  LeaveCriticalSection(&(pMddChannel->CSChannel));
  LeaveCriticalSection(&(pMddAdapter->CSAdapter));

  //
  // Delete the channel critical section.
  //

  DeleteCriticalSection(&(pMddChannel->CSChannel));

  return TRUE;
}


//
// The OALIssueDMATransfer routine sets up map descriptor registers for a channel to map a DMA transfer
// from a locked-down buffer, if there is no other DMA transfers queued in the DMA Channel. Otherwise, this
// transfer will be queued.
//

BOOL OALIssueDmaTransfer(
  IN DMA_CHANNEL_HANDLE hDmaChannel,
  IN OUT PDMA_TRANSFER_HANDLE phDmaHandle,
  IN DWORD  dwFlags,
  IN PHYSICAL_ADDRESS SystemMemoryPhyiscalAddress,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS OpDeviceIoAddress,
  IN LPDMA_TRANSFER_NOTIFY_ROUTINE NotifyRoutine,
  IN PVOID NotifyContext
)
{
  BOOL Status;
  PLIST_ENTRY pListEntry;
  PMDD_ADAPTER_OBJ pMddAdapter;
  PMDD_CHANNEL_OBJ pMddChannel = hDmaChannel;
  PMDD_MAP_REGISTER pMddMR = NULL;

  //
  // Perform some initial checks.
  //

  if (!hDmaChannel || pMddChannel->Size != sizeof(MDD_CHANNEL_OBJ) ||
      !(pMddChannel->InUse) || !(pMddChannel->pAdapter)) {

    SetLastError(ERROR_INVALID_HANDLE);
    
    return FALSE;
  }

  //
  // Perform some additional checks.
  //

  if (!Length) {

    SetLastError(ERROR_INVALID_PARAMETER);
    
    return FALSE;
  }

  pMddAdapter = pMddChannel->pAdapter;

  //
  // If a return DMA handle is not specified, assume caller
  // wants to auto close transfer.
  //

  if (!phDmaHandle) {

    dwFlags |= DMA_FLAGS_AUTO_CLOSE_TRANSFER;
  }

  EnterCriticalSection(&(pMddChannel->CSChannel));

  //
  // Allocate and set up the map register.
  //

  if (!IsListEmpty(&(pMddChannel->MRFree))) {

    pListEntry = RemoveHeadList(&(pMddChannel->MRFree));

    pMddMR = CONTAINING_RECORD(pListEntry, MDD_MAP_REGISTER, ListEntry);

    //
    // Fill in the map register MDD information.
    //

    pMddMR->Size = sizeof (MDD_MAP_REGISTER);
    pMddMR->pChannel = pMddChannel;
    pMddMR->Flags = dwFlags;
    pMddMR->BytesToTransfer = Length;
    pMddMR->BytesTransferred = 0;
    pMddMR->NotifyRoutine = NotifyRoutine;
    pMddMR->NotifyContext = NotifyContext;
    pMddMR->pPddMR = NULL;

    //
    // Allow PDD to allocate and build its map register.
    //

    if (!FdmaAllocateMapRegister(pMddChannel->pPddChannel,
                                 dwFlags,
                                 (dwFlags & DMA_FLAGS_USER_OPTIONAL_DEVICE) ?
                                  OpDeviceIoAddress : pMddChannel->DeviceAddr,
                                 SystemMemoryPhyiscalAddress,
                                 Length,
                                 pMddChannel->DemandMode,
                                 pMddChannel->DmaWidth,
                                 pMddChannel->DmaSpeed,
                                 &(pMddMR->pPddMR))) {

      //
      // Out of map registers, can not transfer.
      //

      RETAILMSG(1, (TEXT("OALIssueDmaTransfer out of PDD map registers\r\n")));

      goto Error;
    }

    //
    // Save away the map register handle.
    //

    if (phDmaHandle) {

      *phDmaHandle = pMddMR;
    }

  } else {

    //
    // Out of map registers, can not transfer.
    //

    RETAILMSG(1, (TEXT("OALIssueDmaTransfer out of map registers\r\n")));

    SetLastError(ERROR_NO_SYSTEM_RESOURCES);

    Status = FALSE;

    goto Error;
  }

  //
  // Attempt to start the transfer if the channel is currently idle.
  //

  if (IsListEmpty(&(pMddChannel->MRTransfer)) && !(dwFlags & DMA_FLAGS_QUEUE_TRANSFER)) {

    DEBUGMSG(0, (TEXT("OALIssueDmaTransfer start transfer\r\n")));

    //
    // First queue the transfer to the PDD.
    //

    Status = FdmaQueueTransfer(pMddChannel->pPddChannel,
                               pMddMR->pPddMR);

    //
    // Notify the PDD to start the transfer.
    //

    if (Status) {

      pMddMR->MRStatus = DmaStatusTransferring;

      InsertTailList(&(pMddChannel->MRTransfer), &(pMddMR->ListEntry));

      DEBUGMSG(0, (TEXT("OALIssueDmaTransfer move MR %x to transfer queue\r\n"), pListEntry));

      ResetEvent(pMddChannel->hDmaCompleteEvent);

      Status = FdmaStartTransfer(pMddChannel->pPddChannel,
                                 pMddChannel->ChannelNumber);

    } else {

      RETAILMSG(1, (TEXT("OALIssueDmaTransfer PDD failed to queue transfer\r\n")));
    }

    //
    // Did the transfer start properly?
    //

    if (!Status) {

      //
      // Transfer failed to start
      //

      RETAILMSG(1, (TEXT("OALIssueDmaTransfer PDD failed to start transfer\r\n")));

      RemoveTailList(&(pMddChannel->MRTransfer));

      pMddMR->MRStatus = DmaStatusError;

      InsertTailList(&(pMddChannel->MRNotify), &(pMddMR->ListEntry));

      SetEvent(pMddChannel->hDmaCompleteEvent);

      SetEvent(pMddAdapter->hDpcEvent);
    }

  } else {

    //
    // Queue the transfer if the channel is currently busy and it will
    // be started later by the IST.
    //

    DEBUGMSG(0, (TEXT("OALIssueDmaTransfer queue transfer %x\r\n"), pListEntry));

    pMddMR->MRStatus = DmaStatusPending;

    InsertTailList(&(pMddChannel->MRPend), &(pMddMR->ListEntry));

    Status = TRUE;
  }

  LeaveCriticalSection(&(pMddChannel->CSChannel));

  return Status;

Error:

  //
  // Free any allocations.
  //

  if (pMddMR) {

    if (pMddMR->pPddMR) {

      FdmaFreeMapRegister(pMddChannel->pPddChannel,
                          &(pMddMR->pPddMR));
    }

    InsertTailList(&(pMddChannel->MRFree), &(pMddMR->ListEntry));
  }

  LeaveCriticalSection(&(pMddChannel->CSChannel));

  return Status;
}


//
// The OALStartDMATransfer starts any pending DMA transfers for a idle channel.
//

BOOL OALStartDmaTransfer(
  IN DMA_CHANNEL_HANDLE hDmaChannel
)
{
  BOOL Status = TRUE, DpcNotify = FALSE;
  PLIST_ENTRY pListEntry;
  PMDD_CHANNEL_OBJ pMddChannel = hDmaChannel;
  PMDD_MAP_REGISTER pMddMR;

  //
  // Perform some initial checks.
  //

  if (!hDmaChannel || pMddChannel->Size != sizeof(MDD_CHANNEL_OBJ) ||
      !(pMddChannel->InUse) || !(pMddChannel->pAdapter)) {

    SetLastError(ERROR_INVALID_HANDLE);
    
    return FALSE;
  }

  //
  // Attempt to start the transfer if the channel is currently idle.
  //

  EnterCriticalSection(&(pMddChannel->CSChannel));

  if (IsListEmpty(&(pMddChannel->MRTransfer)) && !IsListEmpty(&(pMddChannel->MRPend))) {

    //
    // Start any pending transfers for this channel.
    //

    DEBUGMSG(0, (TEXT("OALStartDmaTransfer starting pending transfers\r\n")));

    while (!IsListEmpty(&(pMddChannel->MRPend))) {

      //
      // Remove a map register from pend queue.
      //

      pListEntry = RemoveHeadList(&(pMddChannel->MRPend));

      DEBUGMSG(0, (TEXT("OALStartDmaTransfer move MR %x to transfer queue\r\n"), pListEntry));

      pMddMR = CONTAINING_RECORD(pListEntry, MDD_MAP_REGISTER, ListEntry);

      //
      // Allow the PDD to queue the transfer.
      //

      Status = FdmaQueueTransfer(pMddChannel->pPddChannel,
                                 pMddMR->pPddMR);

      //
      // Set the appropriate map register status.
      //

      if (Status) {

        //
        // PDD queued transfer, set MR status.
        //

        pMddMR->MRStatus = DmaStatusTransferring;

        InsertTailList(&(pMddChannel->MRTransfer), pListEntry);

      } else {

        //
        // PDD failed to queue transfer, set MR status.  Let DPC thread call the notify routine.
        //

        DEBUGMSG(0, (TEXT("OALStartDmaTransfer failed to queue transfer\r\n")));

        pMddMR->MRStatus = DmaStatusError;

        InsertTailList(&(pMddChannel->MRNotify), pListEntry);

        DpcNotify = TRUE;
      }
    }

    //
    // Allow the PDD to start the transfers, if any.
    //

    if (!IsListEmpty(&(pMddChannel->MRTransfer))) {

      Status = FdmaStartTransfer(pMddChannel->pPddChannel,
                                 pMddChannel->ChannelNumber);

      //
      // Did the transfer start properly?
      //

      if (Status) {

        //
        // Transfer started.
        //

        ResetEvent(pMddChannel->hDmaCompleteEvent);

      } else {

        DEBUGMSG(0, (TEXT("OALStartDmaTransfer failed to start the transfer\r\n")));
        
        //
        // DMA transfer failed, move all map registers to notify queue.
        //

        while (!IsListEmpty(&(pMddChannel->MRTransfer))) {

          //
          // Move map register from transfer to notify queue.
          //

          pListEntry = RemoveHeadList(&(pMddChannel->MRTransfer));

          DEBUGMSG(0, (TEXT("OALStartDmaTransfer move MR %x to notify queue\r\n"), pListEntry));

          //
          // Set the appropriate map register status.
          //

          pMddMR = CONTAINING_RECORD(pListEntry, MDD_MAP_REGISTER, ListEntry);

          pMddMR->MRStatus = DmaStatusError;

          InsertTailList(&(pMddChannel->MRNotify), pListEntry);
        }

        //
        // Inform PDD to close the transfer.
        //

        FdmaCloseTransfer(pMddChannel->pPddChannel);

        //
        // Let the DPC thread call the notify routine.
        //

        DpcNotify = TRUE;
      }
    }
  }

  LeaveCriticalSection(&(pMddChannel->CSChannel));

  //
  // Notify the DPC if needed.
  //

  if (DpcNotify) {

    DEBUGMSG(0, (TEXT("OALStartDmaTransfer notify DPC\r\n")));

    SetEvent(pMddChannel->pAdapter->hDpcEvent);
  }

  return Status;
}


//
// This function cancels a pending DMA transfer.
//

BOOL OALCancelDmaTransfer(
    IN DMA_TRANSFER_HANDLE DmaTransferHandle
)
{
  BOOL Status = TRUE;
  PMDD_MAP_REGISTER pMddMR = DmaTransferHandle;
  PMDD_CHANNEL_OBJ pMddChannel;

  //
  // Perform some initial checks.
  //

  if (!DmaTransferHandle || pMddMR->Size != sizeof(MDD_MAP_REGISTER) ||
      !(pMddMR->pChannel) || pMddMR->MRStatus == DmaStatusNone) {

    SetLastError(ERROR_INVALID_HANDLE);
    
    return FALSE;
  }

  pMddChannel = pMddMR->pChannel;

  EnterCriticalSection(&(pMddChannel->CSChannel));

  //
  // We can only canel a pending transfer
  //

  if (pMddMR->MRStatus == DmaStatusPending) {

    //
    // Move the MDD map register to the notify queue.
    //

    RemoveEntryList(&(pMddMR->ListEntry));

    pMddMR->MRStatus = DmaStatusCanceled;

    InsertTailList(&(pMddChannel->MRNotify), &(pMddMR->ListEntry));

    Status = TRUE;

  } else {

    SetLastError(ERROR_REQUEST_ABORTED);

    Status = FALSE;
  }

  LeaveCriticalSection(&(pMddChannel->CSChannel));

  return Status;
}


//
// This function closes a DMA Transfer and release all related resource.
//

BOOL OALCloseDmaTransfer(
    IN DMA_TRANSFER_HANDLE DmaTransferHandle
)
{
  BOOL Status;
  PMDD_MAP_REGISTER pMddMR = DmaTransferHandle;
  PMDD_CHANNEL_OBJ pMddChannel;

  //
  // Perform some initial checks.
  //

  if (!DmaTransferHandle || pMddMR->Size != sizeof(MDD_MAP_REGISTER) ||
      !(pMddMR->pChannel) || pMddMR->MRStatus == DmaStatusNone) {

    SetLastError(ERROR_INVALID_HANDLE);
    
    return FALSE;
  }

  pMddChannel = pMddMR->pChannel;

  EnterCriticalSection(&(pMddChannel->CSChannel));

  //
  // Check to see if the map register is currently transferring.
  //

  if (pMddMR->MRStatus == DmaStatusTransferring) {

    //
    // Currently transferring, we can not close now.
    //

    RETAILMSG(1, (TEXT("OALCloseDmaTransfer can not close, DMA is still transferring\r\n")));

    SetLastError(ERROR_IO_PENDING);

    Status = FALSE;

  } else {

    //
    // Notify PDD to free its map register.
    //

    FdmaFreeMapRegister(pMddChannel->pPddChannel,
                        &(pMddMR->pPddMR));

    //
    // Move the MDD map register to the free queue.
    //

    DEBUGMSG(0, (TEXT("OALCloseDmaTransfer move MR %x to free queue\r\n"), &(pMddMR->ListEntry)));

    RemoveEntryList(&(pMddMR->ListEntry));

    InsertTailList(&(pMddChannel->MRFree), &(pMddMR->ListEntry));

    Status = TRUE;
  }

  LeaveCriticalSection(&(pMddChannel->CSChannel));

  return Status;
}


//
// This function gets the status of a current DMA Transfer.
//

BOOL OALGetDmaStatus (
  IN DMA_TRANSFER_HANDLE hDmaTransferHandle,
  OUT PDWORD lpCompletedLength,
  OUT PDMA_TRANSFER_STATUS lpCompletionCode
)
{
  PMDD_CHANNEL_OBJ pMddChannel;
  PMDD_MAP_REGISTER pMddMR = hDmaTransferHandle;

  //
  // Perform some initial checks.
  //

  if (!hDmaTransferHandle || pMddMR->Size != sizeof(MDD_MAP_REGISTER) ||
      !(pMddMR->pChannel) || pMddMR->MRStatus == DmaStatusNone) {

    SetLastError(ERROR_INVALID_HANDLE);
    
    return FALSE;
  }

  pMddChannel = pMddMR->pChannel;

  //
  // Get the transfer status.
  //

  EnterCriticalSection(&(pMddChannel->CSChannel));

  *lpCompletionCode = pMddMR->MRStatus;
  *lpCompletedLength = pMddMR->BytesTransferred;

  LeaveCriticalSection(&(pMddChannel->CSChannel));

  return TRUE;
}


//
// This function waits for a channel DMA completion event.
//

DWORD OALWaitForDmaComplete (
  IN DMA_CHANNEL_HANDLE hDmaChannel,
  IN DWORD WaitTime
)
{
  PMDD_CHANNEL_OBJ pMddChannel = hDmaChannel;

  //
  // Perform some initial checks.
  //

  if (!pMddChannel || pMddChannel->Size != sizeof(MDD_CHANNEL_OBJ)) {

    SetLastError(ERROR_INVALID_HANDLE);
    
    return FALSE;
  }

  //
  // Wait for the channel DMA completion event.
  //

  return (WaitForSingleObject(pMddChannel->hDmaCompleteEvent, WaitTime));
}


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
)
{
  //
  // Not supported.
  //

  SetLastError(ERROR_NOT_SUPPORTED);

  return FALSE;
}


//
// Used to control DMA that status in DMA_TRANSFER_IN_PROGRESSING by hardware.
//

BOOL OALRawDmaControl(
  IN DMA_TRANSFER_HANDLE hDmaHandle,
  IN DWORD dwIoControl,
  IN PVOID lpInPtr,
  IN DWORD nInLen,
  IN OUT LPVOID lpOutBuffer,
  IN DWORD nOutBufferSize,
  IN LPDWORD lpBytesReturned
)
{
  //
  // Not supported.
  //

  SetLastError(ERROR_NOT_SUPPORTED);

  return FALSE;
}


//
// Dump all the map register queues for a channel.
//

VOID DumpMRQueues (
  IN DMA_CHANNEL_HANDLE hDmaChannel
)
{
  ULONG Count;
  PLIST_ENTRY pListEntry;
  PMDD_CHANNEL_OBJ pMddChannel = hDmaChannel;

  if (!pMddChannel) {

    SetLastError(ERROR_INVALID_PARAMETER);

    return;
  }

  EnterCriticalSection(&(pMddChannel->CSChannel));

  RETAILMSG(1, (TEXT("Dumping queues for channel %d\r\n\r\n"), pMddChannel->ChannelNumber));

  //
  // Dump free queue.
  //

  Count = 0;

  RETAILMSG(1, (TEXT("MRFree queue:\r\n")));

  if (!IsListEmpty(&(pMddChannel->MRFree))) {

    pListEntry = pMddChannel->MRFree.Flink;

    do {

      RETAILMSG(1, (TEXT("%x"), pListEntry));

      pListEntry = pListEntry->Flink;

      Count++;

    } while (pListEntry != &(pMddChannel->MRFree));
  }

  RETAILMSG(1, (TEXT("Total MRFree count: %d\r\n\r\n"), Count));

  //
  // Dump pending queue.
  //

  Count = 0;

  RETAILMSG(1, (TEXT("MRPend queue:\r\n")));

  if (!IsListEmpty(&(pMddChannel->MRPend))) {

    pListEntry = pMddChannel->MRPend.Flink;

    do {

      RETAILMSG(1, (TEXT("%x"), pListEntry));

      pListEntry = pListEntry->Flink;

      Count++;

    } while (pListEntry != &(pMddChannel->MRPend));
  }

  RETAILMSG(1, (TEXT("Total MRPend count: %d\r\n\r\n"), Count));

  //
  // Dump transfering queue.
  //

  Count = 0;

  RETAILMSG(1, (TEXT("MRTransfer queue:\r\n")));

  if (!IsListEmpty(&(pMddChannel->MRTransfer))) {

    pListEntry = pMddChannel->MRTransfer.Flink;

    do {

      RETAILMSG(1, (TEXT("%x"), pListEntry));

      pListEntry = pListEntry->Flink;

      Count++;

    } while (pListEntry != &(pMddChannel->MRTransfer));
  }

  RETAILMSG(1, (TEXT("Total MRTransfer count: %d\r\n\r\n"), Count));

  //
  // Dump notify queue.
  //

  Count = 0;

  RETAILMSG(1, (TEXT("MRNotify queue:\r\n")));

  if (!IsListEmpty(&(pMddChannel->MRNotify))) {

    pListEntry = pMddChannel->MRNotify.Flink;

    do {

      RETAILMSG(1, (TEXT("%x"), pListEntry));

      pListEntry = pListEntry->Flink;

      Count++;

    } while (pListEntry != &(pMddChannel->MRNotify));
  }

  RETAILMSG(1, (TEXT("Total MRNotify count: %d\r\n\r\n"), Count));

  //
  // Dump done queue.
  //

  Count = 0;

  RETAILMSG(1, (TEXT("MRDone queue:\r\n")));

  if (!IsListEmpty(&(pMddChannel->MRDone))) {

    pListEntry = pMddChannel->MRDone.Flink;

    do {

      RETAILMSG(1, (TEXT("%x"), pListEntry));

      pListEntry = pListEntry->Flink;

      Count++;

    } while (pListEntry != &(pMddChannel->MRDone));
  }

  RETAILMSG(1, (TEXT("Total MRDone count: %d\r\n\r\n"), Count));

  LeaveCriticalSection(&(pMddChannel->CSChannel));
}
