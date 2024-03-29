// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  Header: boot_cfg.h
//
#ifndef __BOOT_CFG_H
#define __BOOT_CFG_H

//------------------------------------------------------------------------------

OAL_KITL_DEVICE g_bootDevices[] = {
	{
		L"Internal EMAC", Internal, OMAP_CPGMAC_REGS_PA,
		0, OAL_KITL_TYPE_ETH, &g_kitlEthEMAC
	},
    { 
        L"LAN9311 MAC", Internal, BSP_LAN9311_REGS_PA, 
        0, OAL_KITL_TYPE_ETH, &g_kitlEthLan911x
	}, 
	{
        L"USBFn RNDIS ", Internal, OMAP_USBHS_REGS_PA, 
        0, OAL_KITL_TYPE_ETH, &g_kitlUsbRndis
    }, 
	{
        L"SDCard FILE ", Internal, OMAP_MMCHS1_REGS_PA, 
        0, BOOT_SDCARD_TYPE, NULL
    }, 
	{
        L"NK from NAND", Internal, BSP_NAND_REGS_PA + 0x20, 
        0, OAL_KITL_TYPE_FLASH, NULL
    }, 
	{
        NULL, 0, 0, 0, 0, NULL
    }
};    


//------------------------------------------------------------------------------

#endif
