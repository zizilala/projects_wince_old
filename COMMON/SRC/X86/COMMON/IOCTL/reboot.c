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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//  
// -----------------------------------------------------------------------------
#include <windows.h>
#include <x86boot.h>
#include <kitl.h>
#include <oal.h>

// Hardware reset fix-up var (see config.bib).
//
DWORD *pdwHardReset = (DWORD *)-1;


// Reboot address
//
extern DWORD dwRebootAddress;
extern void StartUp(void);

BOOL x86IoCtlHalReboot (
    UINT32 code, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32 *lpBytesReturned
) {
    // There are two ways to reboot this system: a hardware reset (which will cause
    // the BIOS to run again) and a soft reset (which jumps back to the bootloader
    // which is already in RAM).  The former will reinitialize all hardware (of
    // most interest is the PCI bus and PCI devices) and will also clear RAM.  The
    // latter will not clear RAM and thus will allow the RAM filesystem to persist
    // across reboots.  While the soft reset is faster and keeps the RAM filesystem
    // intact, because it doesn't reset and reinitialize the PCI bus, PCI devices
    // don't know that the system has been reset and outstanding DMA operations or
    // interrupts can cause problems.
    //
    // The correct way to handle both the hardware and the soft resets is to make
    // use of the power manager to transition into a reboot power state.  The power
    // manager will request that all drivers quiesce their devices before it calls
    // this IOCTL.  When done this way, the soft reboot case won't have the 
    // DMA/interrupt problem noted above (it's only an issue when this IOCTL is 
    // called directly outside of the power manager).
    // 
    // In order to minimize the cases in which we do a soft reboot (to thus minimize
    // the possibility problems), we'll use the following algorithm to determine 
    // when to hardware reboot:
    //
    // 1. If the user has built this image with the variable BSP_HARDRESET == 1 (note
    //    this is a romimage fixup variable so only makeimg needs to be run when
    //    the value is changed), then we'll force a hard reset.  Otherwise:
    // 2. If there isn't a KITL connection, then we'll force a hard reset.  Otherwise:
    // 3. Do a soft reboot.
    //
    
    // Are we doing a hardware reset (see config.bib)?
    //
    if (pdwHardReset || !(g_pX86Info->KitlTransport & ~KTS_PASSIVE_MODE) || (g_pX86Info->KitlTransport == KTS_NONE)) {
        // Perform a hardware reset of the device...
        //
        __asm
        {
            cli                     ; Disable Interrupts.
            mov     al, 0FEh        ; Reset CPU.
            out     064h, al		;
            jmp     near $          ; Should not get here.
        }
    } else {
        // Perform a soft reset of the device...
        //
        // OEMIoControl runs at Ring 1, but to reset, we need to be running at Ring 0.
        // To accomplish this, we set dwRebootAddress which is checked in the PerpISR
        // routine (ISRs run at Ring 0).  If set, the PerpISR will reset the device.
        //
        dwRebootAddress = g_pX86Info->dwRebootAddr;

        RETAILMSG(1, (TEXT("Rebooting to startup address 0x%08X...\r\n"), dwRebootAddress));
    }
     
    return TRUE;
}
