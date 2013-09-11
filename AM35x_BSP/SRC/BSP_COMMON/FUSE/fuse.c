// All rights reserved ADENEO EMBEDDED 2010
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File:  fuse.c
//

#include "bsp.h"
#include "am3517_config.h"
#include "oal_clock.h"


//------------------------------------------------------------------------------
//
//  Function:  ReadMacAddressFromFuse
//
//
void ReadMacAddressFromFuse(UCHAR mac[6])
{
    DWORD low,high;
    OMAP_SYSC_GENERAL_REGS *pSys = (OMAP_SYSC_GENERAL_REGS*) OALPAtoUA(OMAP_SYSC_GENERAL_REGS_PA);
    
    low = INREG32(&pSys->CONTROL_FUSE_EMAC_LSB);
    high = INREG32(&pSys->CONTROL_FUSE_EMAC_MSB);

    mac[0] = (UCHAR) ((high >> 16) & 0xFF);
    mac[1] = (UCHAR) ((high >>  8) & 0xFF);
    mac[2] = (UCHAR) ((high >>  0) & 0xFF);
    mac[3] = (UCHAR) ((low >>  16) & 0xFF);
    mac[4] = (UCHAR) ((low >>   8) & 0xFF);
    mac[5] = (UCHAR) ((low >>   0) & 0xFF);
    RETAILMSG(1,(TEXT("%x %x -> %x %x %x %x %x %x\r\n"),high,low,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]));
}