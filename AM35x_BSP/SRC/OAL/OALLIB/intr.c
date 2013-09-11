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
//  File:  intr.c
//
//  Interrupt BSP callback functions. OMAP SoC has one external interrupt
//  pin. In general solution it can be used for cascade interrupt controller.
//  However in most cases it will be used as simple interrupt input. To avoid
//  unnecessary penalty not all BSPIntrXXXX functions are called by default
//  interrupt module implementation.
//
//
#include "bsp.h"
#include "bsp_cfg.h"
#include "sdk_gpio.h"

#include "oalex.h"
#include "oal_prcm.h"

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrInit
//
//  This function is called from OALIntrInit to initialize on-board secondary
//  interrupt controller if exists. As long as GPIO interrupt edge registers
//  are initialized in startup.s code function is stub only.
//
BOOL
BSPIntrInit()
{    
    
    // Associate the External LAN Irq with a fixed sysintr.
    // The reason is that the LAN irq is greater than 255 and that is not 
    // suported by NDIS
    OALIntrStaticTranslate(SYSINTR_LAN9311, BSPGetGpioIrq(LAN9311_IRQ_GPIO));

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  BSPIntrRequestIrqs
//
//  This function is called from OALIntrRequestIrq to obtain IRQ for on-board
//  devices if exists.
//
BOOL
BSPIntrRequestIrqs(
    DEVICE_LOCATION *pDevLoc, 
    UINT32 *pCount, 
    UINT32 *pIrq
    )
{
    BOOL rc = FALSE;

    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"+BSPIntrRequestIrq(0x%08x->%d/%d/0x%08x/%d, 0x%08x)\r\n", pDevLoc, 
        pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc,
        pDevLoc->Pin, pIrq
        ));

    switch (pDevLoc->IfcType)
        {
        case Internal:
            switch ((ULONG)pDevLoc->LogicalLoc)
                {
                case BSP_LAN9311_REGS_PA:
                    {
                        HANDLE hGPIO;
                        hGPIO = GPIOOpen();
                        if (hGPIO)
                        {
                            GPIOSetMode(hGPIO, LAN9311_IRQ_GPIO,GPIO_DIR_INPUT | GPIO_INT_LOW);
                            GPIOClose(hGPIO);
                            *pCount = 1;
                            *pIrq = BSPGetGpioIrq(LAN9311_IRQ_GPIO);
                            rc = TRUE;
                        }
                        else
                        {
                            rc = FALSE;
                        }
                    }
                    break;
				case OMAP_CPGMAC_REGS_PA:
					{
						*pCount = 1;
						*pIrq = GetIrqByDevice(OMAP_DEVICE_CPGMAC,L"RX");
						rc = TRUE;
					}
					break;
                }
            break;
        }

    OALMSG(OAL_INTR&&OAL_FUNC, (L"-BSPIntrRequestIrq(rc = %d)\r\n", rc));

    return rc;
}

//------------------------------------------------------------------------------

