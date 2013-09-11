/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File: oem_padwakeup.c
//
#include "bsp.h"
#include "oalex.h"

//-----------------------------------------------------------------------------
//
//  Extern :  g_pSyscPadConfsRegs
//
//  Address for SYSC pad conf registers
//
extern OMAP_SYSC_PADCONFS_REGS      *g_pSyscPadConfsRegs;

//-----------------------------------------------------------------------------
//
//  Extern :  g_PrcmPrm
//
//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
extern OMAP_PRCM_PRM               g_PrcmPrm;

//------------------------------------------------------------------------------
//
//  Function:  OEMEnableIOPadWakeup
//
//  Enable/Disable IO PAD wakeup capability for a particular GPIO
//
//
void
OEMEnableIOPadWakeup(
    DWORD   gpio,
    BOOL    bEnable
    )
{
    // NOTE:
    //  For better performance only the GPIO which are identified as wakeup
    // capable is implemented. This shall be extended if any other modules
    // GPIO requires IO PAD Wakeup
   
    UNREFERENCED_PARAMETER(gpio);
    UNREFERENCED_PARAMETER(bEnable);

    // Below code is a reference code to return the status IO PAD wakeup event
#if 0    
    switch (gpio)
        {      

        case 0:
            if (bEnable)
                {
                // Set the wake-up capability of T2 (GPIO 0)
                SETREG16(&g_pSyscPadConfsRegs->CONTROL_PADCONF_SYS_NIRQ, OFF_WAKE_ENABLE);
                }
            else 
                {
                // Clear the wake-up capability of T2 (GPIO 0)
                CLRREG16(&g_pSyscPadConfsRegs->CONTROL_PADCONF_SYS_NIRQ, OFF_WAKE_ENABLE);     
                }
            
                OALContextUpdateDirtyRegister(HAL_CONTEXTSAVE_PINMUX);            
            break;
        default:
            break;
        }
    #endif
}

//------------------------------------------------------------------------------
//
//  Function:  OEMGetIOPadWakeupStatus
//
//  pGpioPadIntrStatus is an arry off all GPIO banks, check for all GPIO pads 
//  which is enabled for wakeup, if a wakeup event is set on a IO PAD, set the
//  corresponding bit in corresponding array and bit position
//
//  Returns TRUE  - IO pad wakeup event occured 
//          FALSE - No IO PAD Wakeup event.
//
BOOL
OEMGetIOPadWakeupStatus(UINT32 *pGpioPadIntrStatus)
{
    BOOL rc = FALSE;

    // IO PAD Wakeup capability is required for all GPIO's to wakeup the device
    // from OFF mode. But since GPIO0 is in wakeup domain, the interrupt status
    // will be maintained and after wakeup, omap will generate a interrupt so
    // s/w should use PAD wakeup event only for PER domain GPIO

    UNREFERENCED_PARAMETER(pGpioPadIntrStatus);

    // Below code is a reference code to return the status IO PAD wakeup event
#if 0    
    if (INREG16(&g_pSyscPadConfsRegs->CONTROL_PADCONF_SYS_NIRQ) & OFF_PAD_WAKEUP_EVENT)
    {
        pGpioPadIntrStatus[GPIO_BANK(GPIO_0)] |= (1 << (GPIO_0 & GPIO_BITS_PER_BANK));
        rc = TRUE;
    }
#endif

    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALEnableIOPadWakeup
//
//  Enables the configuration for IO PAD wakeup
//
VOID
OALEnableIOPadWakeup()
{
    // clear wakeup status
    OUTREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKST_WKUP, CM_CLKEN_IO);

    // Enable the IO pad wakeup
    SETREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_CLKEN_IO);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALDisableIOPadWakeup
//
//  Disable the configuration for IO PAD wakeup
//
VOID
OALDisableIOPadWakeup()
{
    // Disable the IO pad wakeup
    CLRREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_CLKEN_IO);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALEnableIOWakeupDaisyChain
//
//  Enables the IO Wakeup scheme for wakeup from OFF Mode
//
VOID
OALEnableIOWakeupDaisyChain()
{
#if 0
    if (IS_SILICON_ES3_1())
        {
        SETREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_CLKEN_IO_CHAIN);

        // Wait till the IO Wakeup Scheme is enabled
        while((INREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKST_WKUP) & CM_ST_IO_CHAIN) == 0);

        // Clear IO Wakeup Scheme status
        SETREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKST_WKUP, CM_ST_IO_CHAIN);
        }
#endif
}

//-----------------------------------------------------------------------------
//
//  Function:  OALDisableIOWakeupDaisyChain
//
//  Disables the IO Wakeup scheme for wakeup from OFF Mode
//
VOID
OALDisableIOWakeupDaisyChain()
{
#if 0
    if (IS_SILICON_ES3_1())
        {
        // Disable IO Wakeup Daisy Chain
        CLRREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_CLKEN_IO_CHAIN);
        }
#endif
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIOPadWakeupEnableStatus
//
//  Returns IO Pad WakeupEnableStatus
//
BOOL
OALIOPadWakeupEnableStatus()
{
#if 0
    if (INREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP) & CM_CLKEN_IO)
        {
        return TRUE;
        }
#endif
    return FALSE;
}

