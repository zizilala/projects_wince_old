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

#include "bsp.h"
#include "oal_clock.h"
#include "bsp_cfg.h"
#include "oalex.h"
#include "ceddkex.h"
#include "oal_gptimer.h"

extern void OEMDeinitDebugSerial();
extern void OEMInitDebugSerial();

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmPrm
//
//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
extern OMAP_PRCM_PRM               g_PrcmPrm;

//-----------------------------------------------------------------------------
//
//  External:  g_pTimerRegs
//
//  References the GPTimer1 registers.  Initialized in OALTimerInit().
//
extern OMAP_GPTIMER_REGS          *g_pTimerRegs;


//------------------------------------------------------------------------------
// WARNING: This function is called from OEMPowerOff - no system calls, critical 
// sections, OALMSG, etc., may be used by this function or any function that it calls.
//------------------------------------------------------------------------------
VOID
BSPPowerOff(
    )
{
    // clear wake-up enable capabilities for gptimer1
    CLRREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_CLKEN_GPT1);

    // stop GPTIMER1
    //OALTimerSetReg(&g_pTimerRegs->TCLR, OALTimerGetReg(&g_pTimerRegs->TCLR) & ~(GPTIMER_TCLR_ST));
    OALTimerStop();

    if (g_oalRetailMsgEnable)
    	{
        OEMDeinitDebugSerial();
        EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,FALSE);
	    }
}

//------------------------------------------------------------------------------
// WARNING: This function is called from OEMPowerOff - no system calls, critical 
// sections, OALMSG, etc., may be used by this function or any function that it calls.
//------------------------------------------------------------------------------
VOID
BSPPowerOn(
    )
{
    // reset wake-up enable capabilities for gptimer1
    SETREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_CLKEN_GPT1);


    if (g_oalRetailMsgEnable)
	    {
        EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,TRUE);
	    OEMInitDebugSerial();
    	}

    g_ResumeRTC = TRUE;
}

//------------------------------------------------------------------------------
