// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007 BSQUARE Corporation. All rights reserved.

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
//------------------------------------------------------------------------------
//
//  File:  trans.c
//
#pragma warning (push)
#pragma warning (disable: 4115 4201 4214)
#include <windows.h>
#include <oal.h>

#include "bsp.h"
#include "soc_cfg.h"
#include "omap_musbcore.h"

#pragma warning (pop)

static OMAP_SYSC_GENERAL_REGS* pSysConfRegs = NULL;
static AM3517_OTG_REGS* pUsbRegs = NULL;

#ifdef BUILDING_BOOTLOADER

extern BOOL EnableDeviceClocks(UINT devId, BOOL bEnable);

#endif

static void WaitMilliSeconds(DWORD ms)
{
	OALStall(ms * 1000);
}

//------------------------------------------------------------------------------
//
//  Function:  InitializeHardware
//

BOOL InitializeHardware()
{
    BOOL   rc = FALSE;

    OALMSG(TRUE/*OAL_ETHER&&OAL_FUNC*/, (L"+InitializeHardware\r\n"));

	pSysConfRegs = OALPAtoUA(OMAP_SYSC_GENERAL_REGS_PA);
	if (pSysConfRegs == NULL)
	{
		goto exit;
	}

	pUsbRegs = OALPAtoUA(OMAP_USBHS_REGS_PA);
	if (pUsbRegs == NULL)
	{
		goto exit;
	}

	rc = TRUE;

exit:

    OALMSG(TRUE/*OAL_ETHER&&OAL_FUNC*/, (L"-InitializeHardware - rc = %d\r\n", rc));

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ConnectHardware
//
void ResetHardware()
{
	// Global Reset 
	SETREG32(&pSysConfRegs->CONTROL_IP_SW_RESET, USB20OTGSS_SW_RST);
    WaitMilliSeconds(10);
	CLRREG32(&pSysConfRegs->CONTROL_IP_SW_RESET, USB20OTGSS_SW_RST);

	// Reset the controller
	SETREG32(&pUsbRegs->CONTROL, OTG_CONTROL_RESET);
	while(INREG32(&pUsbRegs->CONTROL) & OTG_CONTROL_RESET);

    WaitMilliSeconds(10);

	CLRREG32(&pSysConfRegs->CONTROL_DEVCONF2, DEVCONF2_USBOTG_PHY_RESET);

    WaitMilliSeconds(10);

	MASKREG32(&pSysConfRegs->CONTROL_DEVCONF2, DEVCONF2_USBOTG_REFFREQ, DEVCONF2_USBOTG_REFFREQ_13MHZ);

	SETREG32(&pSysConfRegs->CONTROL_DEVCONF2, DEVCONF2_USBOTG_PHY_PLLON		|
											  DEVCONF2_USBOTG_SESSENDEN		|
											  DEVCONF2_USBOTG_VBUSDETECTEN	|
											  DEVCONF2_USBOTG_REFFREQ_13MHZ	|
											  DEVCONF2_USBOTG_DATAPOLARITY	);

	CLRREG32(&pSysConfRegs->CONTROL_DEVCONF2, DEVCONF2_USBOTG_OTGMODE		|
											  DEVCONF2_USBOTG_POWERDOWNOTG	|
											  DEVCONF2_USBPHY_GPIO_MODE		|
											  DEVCONF2_USBOTG_PHY_PD		);

    WaitMilliSeconds(15);
}

//------------------------------------------------------------------------------
//
//  Function:  ConnectHardware
//

void ConnectHardware()
{
    OALMSG(1, (L"+ConnectHardware\r\n"));

	// Enable non-PDR USB interrupts
	OUTREG32(&pUsbRegs->CONTROL, OTG_CONTROL_UINT);

    OALMSG(1, (L"-ConnectHardware\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  DisconnectHardware
//

void DisconnectHardware()
{
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+DisconnectHardware\r\n"));

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-DisconnectHardware\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  EnableUSBClocks
//

BOOL EnableUSBClocks(BOOL bEnable)
{
#ifdef BUILDING_BOOTLOADER

	return EnableDeviceClocks(OMAP_DEVICE_HSOTGUSB, bEnable);

#else

	UNREFERENCED_PARAMETER(bEnable);

	return TRUE;

#endif
}

//------------------------------------------------------------------------------

