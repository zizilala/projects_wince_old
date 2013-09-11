// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File: oem_pm.c
//
#include "bsp.h"
#include "oalex.h"
#include <nkintr.h>
#include "ceddkex.h"
//#include <pkfuncs.h>
//#include "omap3530_dvfs.h"
#include "am3517_prcm.h"
#include "oal_prcm.h"
//#include "oal_sr.h"
#include "oal_i2c.h"
#include "oal_clock.h"
#include "interrupt_struct.h"
#include "omap_vrfb_regs.h"
#include "oal_gptimer.h"
#include "omap_dvfs.h"


extern void OEMEnableDebugOutput(BOOL bEnable);


#define DISPLAY_TIMEOUT     1100    // based on 32khz clk (~30fps)

#define OMAP_GPIO_BANK_TO_RESTORE       6

#define T2_WAKEON_COUNT                 12
#define T2_SLEEPOFF_COUNT               8
#define T2_WARMRESET_COUNT              6

#define T2_A2S_SEQ_START_ADDR           0x2B
#define T2_S2A12_SEQ_START_ADDR         0x2F
#define T2_WARMRESET_SEQ_START_ADDR     0x38
#define T2_WARMRESET_SEQ_START_ADDR     0x38
#define T2_S2A_SEQ_START_ADDR           0x2D

/*
TWL_PMB_ENTRY _rgS2A12[] = {
    {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VPLL1_RES_ID, TWL_RES_ACTIVE), 0x30
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD1_RES_ID, TWL_RES_ACTIVE), 0x04
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD2_RES_ID, TWL_RES_ACTIVE), 0x02
    }, {
        TWL_PBM(TWL_PROCESSOR_GRP123, TWL_RES_GRP_ALL, 0, 7, TWL_RES_ACTIVE), 0x2
    }, {
        0, 0
    }        
};

TWL_PMB_ENTRY _rgA2S[] = {
    {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD1_RES_ID, TWL_RES_SLEEP), 0x4
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD2_RES_ID, TWL_RES_SLEEP), 0x2
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VPLL1_RES_ID, TWL_RES_OFF), 0x3
    }, {
        TWL_PBM(TWL_PROCESSOR_GRP123, TWL_RES_GRP_ALL, 0, 7, TWL_RES_SLEEP), 0x2
    }, {
        0, 0
    }
};

TWL_PMB_ENTRY _rgWarmRst[] = {
    {
        TWL_PSM(TWL_PROCESSOR_GRP_NULL, TWL_TRITON_RESET, TWL_RES_OFF), 0x02
    }, {        
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD1_RES_ID, TWL_RES_WRST), 0x0E
    }, {        
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD2_RES_ID, TWL_RES_WRST), 0x0E
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VPLL1_RES_ID, TWL_RES_WRST), 0x60
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP123, TWL_CLKEN_ID, TWL_RES_ACTIVE), 0x02
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_HFCLKOUT_ID, TWL_RES_ACTIVE), 0x02
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP_NULL, TWL_TRITON_RESET, TWL_RES_ACTIVE), 0x02
    }, {
        0, 0
    }
};
*/

//-----------------------------------------------------------------------------
// prototypes
//
extern void UpdateRetentionVoltages(IOCTL_RETENTION_VOLTAGES *pData);
extern BOOL SetOpp(DWORD *rgDomains, DWORD *rgOpps, DWORD count);
VOID OALContextRestoreInit();

//-----------------------------------------------------------------------------
// Global : g_pIntr
//  pointer to interrupt structure.
//
extern OMAP_INTR_CONTEXT const    *g_pIntr;
//------------------------------------------------------------------------------
//
//  Global:  dwOEMPRCMCLKSSetupTime
//
//  Timethe PRCM waits for system clock stabilization. 
//  Reinitialized in config.bib (FIXUPVAR)
//
extern const volatile DWORD dwOEMPRCMCLKSSetupTime;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmPrm
//
//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
BOOL                        g_PrcmDebugSuspendResume = FALSE;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmPrm
//
//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
OMAP_PRCM_PRM               g_PrcmPrm;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmCm
//
//  Reference to all PRCM-CM registers. Initialized in PrcmInit
//
OMAP_PRCM_CM                g_PrcmCm;

//------------------------------------------------------------------------------
//
//  Volatile :  g_pIdCodeReg
//
//  reference to ID CODE register part of system control general register set
//
volatile UINT32            *g_pIdCodeReg;

//------------------------------------------------------------------------------
//
//  External:  g_pSysCtrlGenReg
//
//  reference to system control general register set
//
extern OMAP_SYSC_GENERAL_REGS      *g_pSysCtrlGenReg;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMSRAMSaveAddr
//
//  location to store Secure SRAM context
//
extern DWORD dwOEMSRAMSaveAddr;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmPrm
//
//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
OMAP_PRCM_PRM               g_PrcmPrm;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmCm
//
//  Reference to all PRCM-CM registers. Initialized in PrcmInit
//
OMAP_PRCM_CM                g_PrcmCm;

//------------------------------------------------------------------------------
//
//  Volatile :  g_pIdCodeReg
//
//  reference to ID CODE register part of system control general register set
//
volatile UINT32            *g_pIdCodeReg;

//-----------------------------------------------------------------------------
//
//  External:  g_pTimerRegs
//
//  References the GPTimer1 registers.  Initialized in OALTimerInit().
//
OMAP_GPTIMER_REGS          *g_pTimerRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pGPMCRegs
//
//  References the gpmc registers.  Initialized in OALPowerInit().
//
OMAP_GPMC_REGS             *g_pGPMCRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pSDRCRegs
//
//  References the sdrc registers.  Initialized in OALPowerInit().
//
OMAP_SDRC_REGS             *g_pSDRCRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pSMSRegs
//
//  References the SMS registers.  Initialized in OALPowerInit().
//
OMAP_SMS_REGS              *g_pSMSRegs = NULL;

//------------------------------------------------------------------------------
//
//  Global:  g_pVRFBRegs
//
//  References the VRFB registers.  Initialized in OALPowerInit().
//
OMAP_VRFB_REGS              *g_pVRFBRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pContextRestore
//
//  Reference to context restore registers. Initialized in OALPowerInit()
//
OMAP_CONTEXT_RESTORE_REGS  *g_pContextRestore = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pSdrcRestore
//
//  Reference to Sdrc restore registers. Initialized in OALPowerInit()
//
OMAP_SDRC_RESTORE_REGS  *g_pSdrcRestore = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pPrcmRestore
//
//  Reference to Prcm restore registers. Initialized in OALPowerInit()
//
OMAP_PRCM_RESTORE_REGS  *g_pPrcmRestore = NULL;

//------------------------------------------------------------------------------
//
//  Global:  g_ffContextSaveMask
//
//  determines if the context for a particular register set need to be saved
//
UINT32                      g_ffContextSaveMask = 0;

//-----------------------------------------------------------------------------
//
//  static: s_rgGpioRegsAddr
//
//  GPIO register address
//
static  OMAP_GPIO_REGS *s_rgGpioRegsAddr[OMAP_GPIO_BANK_TO_RESTORE]; //We have 6 GPIO banks

//-----------------------------------------------------------------------------
//
//  static: s_rgGpioContext
//
//  To save GPIO context
//
static  OMAP_GPIO_REGS s_rgGpioContext[OMAP_GPIO_BANK_TO_RESTORE];   //We have 6 GPIO banks

//-----------------------------------------------------------------------------
//
//  static:
//  PRCM related variables to store the PRCM context.
//
//
static OMAP_PRCM_WKUP_PRM_REGS              s_wkupPrmContext;
static OMAP_PRCM_CORE_PRM_REGS              s_corePrmContext;
static OMAP_PRCM_MPU_PRM_REGS               s_mpuPrmContext;
static OMAP_PRCM_CLOCK_CONTROL_PRM_REGS     s_clkCtrlPrmContext;

static OMAP_PRCM_OCP_SYSTEM_CM_REGS         s_ocpSysCmContext;
static OMAP_PRCM_WKUP_CM_REGS               s_wkupCmContext;
static OMAP_PRCM_CORE_CM_REGS               s_coreCmContext;
static OMAP_PRCM_CLOCK_CONTROL_CM_REGS      s_clkCtrlCmContext;
static OMAP_PRCM_GLOBAL_CM_REGS             s_globalCmContext;
static OMAP_PRCM_MPU_CM_REGS                s_mpuCmContext;
static OMAP_PRCM_EMU_CM_REGS                s_emuCmContext;

//-----------------------------------------------------------------------------
//
//  Global:  s_SMSRegs
//
//  variable to store SMS context.
//
static OMAP_SMS_REGS                        s_smsContext;

//-----------------------------------------------------------------------------
//
//  Global:  s_vrfbRegs
//
//  variable to store SMS context.
//
static OMAP_VRFB_REGS                        s_vrfbContext;

//-----------------------------------------------------------------------------
//
//  static:  s_sdrcContext
//
//  variable to store SDRC context
//
static OMAP_SDRC_REGS         s_sdrcContext;

//-----------------------------------------------------------------------------
//
//  static:  s_intcContext
//
//  variable to store MPU Interrupt controller context
//
static OMAP_INTC_MPU_REGS        s_intcContext;

//-----------------------------------------------------------------------------
//
//  static:  s_gpmcContext
//
//  variable to store GPMC context
//
static OMAP_GPMC_REGS          s_gpmcContext;

//-----------------------------------------------------------------------------
//
//  static:
//
//  variable to store System control module context
//
static OMAP_SYSC_GENERAL_REGS  s_syscGenContext;
static OMAP_SYSC_INTERFACE_REGS s_syscIntContext;
static OMAP_SYSC_INTERFACE_REGS   *s_pSyscIFContext = NULL;


//-----------------------------------------------------------------------------
//
//  Global :  s_pSyscPadWkupRegs
//
//  Address for SYSC WKUP pad conf registers
//
OMAP_SYSC_PADCONFS_WKUP_REGS               *g_pSyscPadWkupRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global :  s_pSyscPadWkupRegs
//
//  Address for SYSC WKUP pad conf registers
//
OMAP_SYSC_PADCONFS_REGS                    *g_pSyscPadConfsRegs = NULL;

//-----------------------------------------------------------------------------
//
//  static:
//
//  variable to save DMA context
//
static OMAP_SDMA_REGS                       s_dmaController;
static OMAP_SDMA_REGS                      *s_pDmaController = NULL;

//-----------------------------------------------------------------------------
//
//  static: s_bCoreOffModeSet
//
//  Flag to indicate PER and NEON domains are configured for CORE OFF
//
static BOOL                                 s_bCoreOffModeSet = FALSE;

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveGPIO
//
//  Saves the GPIO Context, clears the IRQ for OFF mode
//
VOID
OALContextSaveGPIO ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveGPMC
//
//  Stores the GPMC Registers in shadow register
//
VOID
OALContextSaveGPMC ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveSCM
//
//  Stores the SCM Registers in shadow register
//
VOID
OALContextSaveSCM ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSavePRCM
//
//  Stores the PRCM Registers in shadow register
//
VOID
OALContextSavePRCM ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveINTC
//
//  Stores the MPU IC Registers in shadow register
//
VOID
OALContextSaveINTC ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveDMA
//
//  Saves the DMA Registers
//
VOID
OALContextSaveDMA();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveMux
//
//  Saves pinmux
//
VOID
OALContextSaveMux();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveSMS
//
//  Saves SMS
//
VOID
OALContextSaveSMS();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveVRFB
//
//  Saves VRFB
//
VOID
OALContextSaveVRFB();

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDeviceSetAutoIdleState
//
//  Sets the autoidle state of a device.
//
BOOL OALIoCtlPrcmDeviceSetAutoIdleState(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DEVICE_ENABLE_IN *pInfo;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDeviceSetAutoIdleState\r\n"));
    if (pInBuffer == NULL || inSize < sizeof(IOCTL_PRCM_DEVICE_ENABLE_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pInfo = (IOCTL_PRCM_DEVICE_ENABLE_IN*)(pInBuffer);        
    PrcmDeviceEnableAutoIdle(pInfo->devId, pInfo->bEnable);

    rc = TRUE;

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDeviceSetAutoIdleState(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDeviceGetDeviceStatus
//
//  returns the current clock and autoidle status of a device
//
BOOL OALIoCtlPrcmDeviceGetDeviceStatus(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DEVICE_GET_DEVICESTATUS_OUT *pOut;

    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDeviceGetDeviceStatus\r\n"));
    
    // validate parameters
    //
    if (pInBuffer == NULL || inSize != sizeof(UINT) ||
        pOutBuffer == NULL || outSize != sizeof(IOCTL_PRCM_DEVICE_GET_DEVICESTATUS_OUT))
        {
        goto cleanUp;
        }

    if (pOutSize != NULL) *pOutSize = 0;
    
    // update function pointers
    //
    pOut = (IOCTL_PRCM_DEVICE_GET_DEVICESTATUS_OUT*)pOutBuffer;
    if (PrcmDeviceGetEnabledState(*(UINT*)pInBuffer, &pOut->bEnabled) == FALSE ||
        PrcmDeviceGetAutoIdleState(*(UINT*)pInBuffer, &pOut->bAutoIdle) == FALSE)
        {
        goto cleanUp;
        }

    rc = TRUE;
cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDeviceGetDeviceStatus(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDeviceGetSourceClockInfo
//
//  returns information about a devices clock
//
BOOL OALIoCtlPrcmDeviceGetSourceClockInfo(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT *pOut;

    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDeviceGetSourceClockInfo\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(UINT) ||
        pOutBuffer == NULL || outSize != sizeof(IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pOut = (IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT*)(pOutBuffer);        
    rc = PrcmDeviceGetSourceClockInfo(*(UINT*)pInBuffer, pOut);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDeviceGetSourceClockInfo(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockGetSourceClockInfo
//
//  returns information about a source clock
//
BOOL OALIoCtlPrcmClockGetSourceClockInfo(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    SourceClockInfo_t info;
    IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN *pIn;
    IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT *pOut;

    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockGetSourceClockInfo\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN) ||
        pOutBuffer == NULL || outSize != sizeof(IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN*)pInBuffer;
    pOut = (IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT*)pOutBuffer;

    memset(pOut, 0, sizeof(IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT));
    rc = PrcmClockGetParentClockRefcount(pIn->clockId, pIn->clockLevel, &pOut->refCount);
    if (PrcmClockGetParentClockInfo(pIn->clockId, pIn->clockLevel, &info))
        {        
        pOut->parentId = info.clockId;
        pOut->parentLevel = info.nLevel;
        pOut->parentRefCount = info.refCount;
        }

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockGetSourceClockInfo(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockSetSourceClock
//
//  sets the source clock for a given functional clock
//
BOOL OALIoCtlPrcmClockSetSourceClock(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_CLOCK_SET_SOURCECLOCK_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockSetSourceClock\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_SET_SOURCECLOCK_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_SET_SOURCECLOCK_IN*)pInBuffer;
    rc = PrcmClockSetParent(pIn->clkId, pIn->newParentClkId);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockSetSourceClock(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockSetSourceClockDivisor
//
//  sets the source clock divisor for a given functional clock
//
BOOL OALIoCtlPrcmClockSetSourceClockDivisor(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockSetSourceClockDivisor\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN*)pInBuffer;
    rc = PrcmClockSetDivisor(pIn->clkId, pIn->parentClkId, pIn->divisor);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockSetSourceClockDivisor(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockSetDpllState
//
//  updates the current dpll settings
//
BOOL 
OALIoCtlPrcmClockSetDpllState(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockSetDpllState\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN*)pInBuffer;
    rc = PrcmClockSetDpllState(pIn);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockSetDpllState(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDomainSetWakeupDependency
//
//  updates the wake-up dependency for a power domain
//
BOOL 
OALIoCtlPrcmDomainSetWakeupDependency(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDomainSetWakeupDependency\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP_IN*)pInBuffer;
    rc = PrcmDomainSetWakeupDependency(pIn->powerDomain, pIn->ffWakeDep, pIn->bEnable);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDomainSetWakeupDependency(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDomainSetSleepDependency
//
//  updates the sleep dependency for a power domain
//
BOOL 
OALIoCtlPrcmDomainSetSleepDependency(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DOMAIN_SET_SLEEPDEP_IN *pIn;
    
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDomainSetSleepDependency\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_DOMAIN_SET_SLEEPDEP_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_DOMAIN_SET_SLEEPDEP_IN*)pInBuffer;
    rc = PrcmDomainSetSleepDependency(pIn->powerDomain, pIn->ffSleepDep, pIn->bEnable);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDomainSetSleepDependency(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDomainSetPowerState
//
//  updates the power state for a power domain
//
BOOL 
OALIoCtlPrcmDomainSetPowerState(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = TRUE;
    IOCTL_PRCM_DOMAIN_SET_POWERSTATE_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDomainSetPowerState\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_DOMAIN_SET_POWERSTATE_IN))
        {
        rc = FALSE;
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_DOMAIN_SET_POWERSTATE_IN*)pInBuffer;

    switch (pIn->powerDomain)
        {
        case POWERDOMAIN_MPU:
        case POWERDOMAIN_NEON:
        case POWERDOMAIN_PERIPHERAL:
                {
                PrcmDomainSetPowerState(pIn->powerDomain,
                    pIn->powerState,
                    pIn->logicState
                    );
                }
            break;

        case POWERDOMAIN_CORE:
                {
                PrcmDomainSetPowerState(pIn->powerDomain,
                    pIn->powerState,
                    pIn->logicState
                    );
                }
            break;

        default:
            PrcmDomainSetPowerState(pIn->powerDomain,
                    pIn->powerState,
                    pIn->logicState
                    );
        }

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDomainSetPowerState(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmSetSuspendState
//
//  updates the chip state to enter on suspend
//
BOOL
OALIoCtlPrcmSetSuspendState(
    UINT32 code,
    VOID *pInBuffer,
    UINT32 inSize,
    VOID *pOutBuffer,
    UINT32 outSize,
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmSetSuspendState\r\n"));

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    if (pInBuffer == NULL || inSize != sizeof(DWORD)) goto cleanUp;

    rc = OALWakeupLatency_SetSuspendState(*(DWORD*)pInBuffer);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmSetSuspendState(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlOppRequest
//
//  updates the current operating point
//
BOOL 
OALIoCtlOppRequest(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_OPP_REQUEST_IN *pOppRequest;
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlOppRequest\r\n"));

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    if (pInBuffer == NULL || inSize != sizeof(IOCTL_OPP_REQUEST_IN)) goto cleanUp;
    
    pOppRequest = (IOCTL_OPP_REQUEST_IN*)pInBuffer;
    if (pOppRequest->dwCount > MAX_DVFS_DOMAINS) goto cleanUp;
    
    rc = SetOpp(pOppRequest->rgDomains, 
            pOppRequest->rgOpps, 
            pOppRequest->dwCount
            );

cleanUp:    
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlOppRequest(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALSmartReflex1Intr
//
//  This function implements a stub.
//  This routine is called from the OEMInterruptHandler
//
UINT32 
OALSmartReflex1Intr()
{
    return SYSINTR_NOP;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALSmartReflex2Intr
//
//  This function implements a stub.
//  This routine is called from the OEMInterruptHandler
//
UINT32 
OALSmartReflex2Intr()
{
    return SYSINTR_NOP;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlUpdateRetentionVoltages
//
//  changes VDD1 and VDD2 retention voltages
//
BOOL
OALIoCtlUpdateRetentionVoltages(
    UINT32 code,
    VOID *pInBuffer,
    UINT32 inSize,
    VOID *pOutBuffer,
    UINT32 outSize,
    UINT32 *pOutSize
    )
{
    IOCTL_RETENTION_VOLTAGES *pData;
    BOOL rc = FALSE;

    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlUpdateRetentionVoltages\r\n"));

    if (pInBuffer == NULL || inSize < sizeof(IOCTL_RETENTION_VOLTAGES)) goto cleanUp;

    pData = (IOCTL_RETENTION_VOLTAGES *)pInBuffer;
    UpdateRetentionVoltages(pData);

    rc = TRUE;

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlUpdateRetentionVoltages(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalContextSaveGetBuffer
//
//  returns a pointer to the buffer which holds the context save mask
//  this is a *fast path* to reduce the number of Kernel IOCTL's
//  necessary to indicate a context save is requred
//
BOOL
OALIoCtlHalContextSaveGetBuffer(
    UINT32 code,
    VOID *pInBuffer,
    UINT32 inSize,
    VOID *pOutBuffer,
    UINT32 outSize,
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalContextSaveGetBuffer\r\n"));

    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(code);


    if (pOutBuffer == NULL || outSize < sizeof(UINT32**)) goto cleanUp;

    *(UINT32**)pOutBuffer = &g_ffContextSaveMask;

    rc = TRUE;

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC,
        (L"-OALIoCtlHalContextSaveGetBuffer(rc = %d)\r\n", rc)
        );
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  ForceStandbyUSB
//
//  Force USB into standby mode
//
void 
ForceStandbyUSB()
{
}

//-----------------------------------------------------------------------------
//
//  Function:  ResetDisplay()
//
//  properly resets the display and turns it off
//
void 
ResetDisplay()
{
    unsigned int        val;
    unsigned int        tcrr;
    OMAP_DISPC_REGS    *pDispRegs = OALPAtoUA(OMAP_DISC1_REGS_PA);

    // enable all the interface and functional clocks
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS, TRUE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS1, TRUE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS2, TRUE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_TVOUT, TRUE);

    // check if digital output or the lcd output are enabled
    val = INREG16(&pDispRegs->DISPC_CONTROL);
    if(val & (DISPC_CONTROL_DIGITALENABLE | DISPC_CONTROL_LCDENABLE))
    {
        // disable the lcd output and digital output
        val &= ~(DISPC_CONTROL_DIGITALENABLE | DISPC_CONTROL_LCDENABLE);
        OUTREG32(&pDispRegs->DISPC_CONTROL, val);

        // wait until frame is done
        tcrr = OALTimerGetReg(&g_pTimerRegs->TCRR);
        OUTREG32(&pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_FRAMEDONE);                
        while ((INREG32(&pDispRegs->DISPC_IRQSTATUS) & DISPC_IRQSTATUS_FRAMEDONE) == 0)
        {
           if ((g_pTimerRegs->TCRR - tcrr) > DISPLAY_TIMEOUT) break;
        }        
    }

    // reset the display controller
    SETREG32(&pDispRegs->DISPC_SYSCONFIG, DISPC_SYSCONFIG_SOFTRESET);
    
    // wait until reset completes OR timeout occurs   
    tcrr = OALTimerGetReg(&g_pTimerRegs->TCRR);
    while ((INREG32(&pDispRegs->DISPC_SYSSTATUS) & DISPC_SYSSTATUS_RESETDONE) == 0)
    {
        // delay
        if ((g_pTimerRegs->TCRR - tcrr) > DISPLAY_TIMEOUT) break;
    }

    // Configure for smart-idle mode
    OUTREG32( &pDispRegs->DISPC_SYSCONFIG,
                  DISPC_SYSCONFIG_AUTOIDLE |
                  SYSCONFIG_SMARTIDLE |
                  SYSCONFIG_ENAWAKEUP |
                  SYSCONFIG_CLOCKACTIVITY_I_ON |
                  SYSCONFIG_SMARTSTANDBY
                  );

    // restore old clock settings
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS1, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS2, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_TVOUT, FALSE);

}

//-----------------------------------------------------------------------------
//
//  Function:  ForceIdleMMC()
//
//  puts the mmc in force idle.  If mmc is not in force idle core will not
//  enter retention.
//
void 
ForceIdleMMC()
{
    OMAP_MMCHS_REGS    *pMmcRegs;
    
    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC1, TRUE);
    pMmcRegs = OALPAtoUA(OMAP_MMCHS1_REGS_PA);
    OUTREG32(&pMmcRegs->MMCHS_SYSCONFIG, SYSCONFIG_FORCEIDLE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC1, FALSE);

    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC2, TRUE);
    pMmcRegs = OALPAtoUA(OMAP_MMCHS2_REGS_PA);
    OUTREG32(&pMmcRegs->MMCHS_SYSCONFIG, SYSCONFIG_FORCEIDLE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC2, FALSE);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALPowerInit
//
//  configures power and autoidle settings specific to a board
//
void 
OALPowerInit()
{ 
    PrcmInitInfo info;
    OMAP_SYSC_INTERFACE_REGS   *pSyscIF;
    OALMSG(OAL_FUNC, (L"+OALPowerInit()\r\n"));

    pSyscIF                             = OALPAtoUA(OMAP_SYSC_INTERFACE_REGS_PA);
    g_pGPMCRegs                         = OALPAtoUA(OMAP_GPMC_REGS_PA);
    g_pSDRCRegs                         = OALPAtoUA(OMAP_SDRC_REGS_PA);
    g_pSMSRegs                          = OALPAtoUA(OMAP_SMS_REGS_PA);
    g_pVRFBRegs                         = OALPAtoUA(OMAP_VRFB_REGS_PA);
    g_pSysCtrlGenReg                    = OALPAtoUA(OMAP_SYSC_GENERAL_REGS_PA);

    g_PrcmPrm.pOMAP_GLOBAL_PRM          = OALPAtoUA(OMAP_PRCM_GLOBAL_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_OCP_SYSTEM_PRM      = OALPAtoUA(OMAP_PRCM_OCP_SYSTEM_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_CLOCK_CONTROL_PRM   = OALPAtoUA(OMAP_PRCM_CLOCK_CONTROL_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_WKUP_PRM            = OALPAtoUA(OMAP_PRCM_WKUP_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_PER_PRM             = OALPAtoUA(OMAP_PRCM_PER_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_CORE_PRM            = OALPAtoUA(OMAP_PRCM_CORE_PRM_REGS_PA);    
    g_PrcmPrm.pOMAP_MPU_PRM             = OALPAtoUA(OMAP_PRCM_MPU_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_DSS_PRM             = OALPAtoUA(OMAP_PRCM_DSS_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_SGX_PRM             = OALPAtoUA(OMAP_PRCM_SGX_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_NEON_PRM            = OALPAtoUA(OMAP_PRCM_NEON_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_EMU_PRM             = OALPAtoUA(OMAP_PRCM_EMU_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_USBHOST_PRM         = OALPAtoUA(OMAP_PRCM_USBHOST_PRM_REGS_PA);
    
    g_PrcmCm.pOMAP_GLOBAL_CM            = OALPAtoUA(OMAP_PRCM_GLOBAL_CM_REGS_PA);
    g_PrcmCm.pOMAP_OCP_SYSTEM_CM        = OALPAtoUA(OMAP_PRCM_OCP_SYSTEM_CM_REGS_PA);
    g_PrcmCm.pOMAP_CLOCK_CONTROL_CM     = OALPAtoUA(OMAP_PRCM_CLOCK_CONTROL_CM_REGS_PA);
    g_PrcmCm.pOMAP_WKUP_CM              = OALPAtoUA(OMAP_PRCM_WKUP_CM_REGS_PA);
    g_PrcmCm.pOMAP_PER_CM               = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
    g_PrcmCm.pOMAP_CORE_CM              = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
    g_PrcmCm.pOMAP_MPU_CM               = OALPAtoUA(OMAP_PRCM_MPU_CM_REGS_PA);
    g_PrcmCm.pOMAP_DSS_CM               = OALPAtoUA(OMAP_PRCM_DSS_CM_REGS_PA);
    g_PrcmCm.pOMAP_SGX_CM               = OALPAtoUA(OMAP_PRCM_SGX_CM_REGS_PA);
    g_PrcmCm.pOMAP_NEON_CM              = OALPAtoUA(OMAP_PRCM_NEON_CM_REGS_PA);
    g_PrcmCm.pOMAP_EMU_CM               = OALPAtoUA(OMAP_PRCM_EMU_CM_REGS_PA); 
    g_PrcmCm.pOMAP_USBHOST_CM           = OALPAtoUA(OMAP_PRCM_USBHOST_CM_REGS_PA);

    g_pContextRestore                   = OALPAtoUA(OMAP_CONTEXT_RESTORE_REGS_PA);
    g_pPrcmRestore                      = OALPAtoUA(OMAP_PRCM_RESTORE_REGS_PA);
    g_pSdrcRestore                      = OALPAtoUA(OMAP_SDRC_RESTORE_REGS_PA);
    g_pSyscPadWkupRegs                  = OALPAtoUA(OMAP_SYSC_PADCONFS_WKUP_REGS_PA);
    g_pSyscPadConfsRegs                 = OALPAtoUA(OMAP_SYSC_PADCONFS_REGS_PA);
    g_pIdCodeReg                        = OALPAtoUA(OMAP_IDCORE_REGS_PA);
    info.pPrcmPrm                       = &g_PrcmPrm;
    info.pPrcmCm                        = &g_PrcmCm;

     // initialize the context restore module
    OALContextRestoreInit();
    SETSYSREG32(OMAP_PRCM_CORE_CM_REGS, CM_ICLKEN1_CORE, CM_CLKEN_SCMCTRL);

    // initialize all devices to autoidle
    OUTSYSREG32(OMAP_PRCM_CORE_CM_REGS, CM_AUTOIDLE1_CORE, CM_AUTOIDLE1_CORE_INIT);
    OUTSYSREG32(OMAP_PRCM_CORE_CM_REGS, CM_AUTOIDLE2_CORE, CM_AUTOIDLE2_CORE_INIT);
    OUTSYSREG32(OMAP_PRCM_CORE_CM_REGS, CM_AUTOIDLE3_CORE, CM_AUTOIDLE3_CORE_INIT);

    OUTSYSREG32(OMAP_PRCM_WKUP_CM_REGS, CM_AUTOIDLE_WKUP, CM_AUTOIDLE_WKUP_INIT);

    OUTREG32(&g_PrcmCm.pOMAP_PER_CM->CM_AUTOIDLE_PER, CM_AUTOIDLE_PER_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_DSS_CM->CM_AUTOIDLE_DSS, CM_AUTOIDLE_DSS_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_USBHOST_CM->CM_AUTOIDLE_USBHOST, CM_AUTOIDLE_USBHOST_INIT);

    // clear all sleep dependencies.
    OUTREG32(&g_PrcmCm.pOMAP_SGX_CM->CM_SLEEPDEP_SGX, CM_SLEEPDEP_SGX_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_DSS_CM->CM_SLEEPDEP_DSS, CM_SLEEPDEP_DSS_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_PER_CM->CM_SLEEPDEP_PER, CM_SLEEPDEP_PER_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_USBHOST_CM->CM_SLEEPDEP_USBHOST, CM_SLEEPDEP_USBHOST_INIT);

    // clear all wake dependencies.
    OUTREG32(&g_PrcmPrm.pOMAP_MPU_PRM->PM_WKDEP_MPU, CM_WKDEP_MPU_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_SGX_PRM->PM_WKDEP_SGX, CM_WKDEP_SGX_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_DSS_PRM->PM_WKDEP_DSS, CM_WKDEP_DSS_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_PER_PRM->PM_WKDEP_PER, CM_WKDEP_PER_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_NEON_PRM->PM_WKDEP_NEON, CM_WKDEP_NEON_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_USBHOST_PRM->PM_WKDEP_USBHOST, CM_WKDEP_USBHOST_INIT);

    // clear all wake ability
    OUTREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_WKEN_WKUP_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_CORE_PRM->PM_WKEN1_CORE, CM_WKEN1_CORE_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_PER_PRM->PM_WKEN_PER, CM_WKEN_PER_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_DSS_PRM->PM_WKEN_DSS, CM_WKEN_DSS_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_USBHOST_PRM->PM_WKEN_USBHOST, CM_WKEN_USBHOST_INIT);

    // UNDONE:
    // For now have allow all devices mpu wakeup cap.
    OUTREG32(OALPAtoVA(0x48306CA4, FALSE), 0x3CB);          // PM_MPUGRPSEL_WKUP
    OUTREG32(OALPAtoVA(0x48306AA4, FALSE), 0xC33FFE18);     // PM_MPUGRPSEL1_CORE
    OUTREG32(OALPAtoVA(0x48306AF8, FALSE), 0x00000004);     // PM_MPUGRPSEL3_CORE
    OUTREG32(OALPAtoVA(0x483070A4, FALSE), 0x0003EFFF);     // PM_MPUGRPSEL_PER
    OUTREG32(OALPAtoVA(0x483074A4, FALSE), 0x00000001);     // PM_MPUGRPSEL_USBHOST

    // If a UART is used during the basic intialization, the system will lock up.
    // The problem appears to be cause by enabling domain power management as soon 
    // as the first device in the domain is initialized, instead of initializing all
    // devices in domain before enabling domain autoidle. Since the system is single
    // threaded when this function is called, the UART use during init is the only problem.
    OALMSG(OAL_FUNC, (L" Disable serial debug messages during PRCM DeviceInitialize\r\n"));
    OEMEnableDebugOutput(FALSE);

    OUTREG32(&g_PrcmPrm.pOMAP_GLOBAL_PRM->PRM_RSTST,
        INREG32(&g_PrcmPrm.pOMAP_GLOBAL_PRM->PRM_RSTST)
        );

    // set SDRC_POWER_REG register
    OUTREG32(&g_pSDRCRegs->SDRC_POWER, BSP_SDRC_POWER_REG);

    // initialize prcm library
    PrcmInit(&info);
    
    //----------------------------------------------------------------------
    // Initialize I2C devices
    //----------------------------------------------------------------------
    OALI2CInit(OMAP_DEVICE_I2C1);
    OALI2CInit(OMAP_DEVICE_I2C2);
    OALI2CInit(OMAP_DEVICE_I2C3);

    //----------------------------------------------------------------------
    // clock GPIO banks
    //----------------------------------------------------------------------
    EnableDeviceClocks(OMAP_DEVICE_GPIO1,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO2,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO3,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO4,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO5,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO6,TRUE);

    // initialize pin mux table
    OALMux_InitMuxTable();


    // we need to write FORCEIDLE to the GPMC register or else it will prevent
    // the core from entering retention.  After the write smartidle can be
    // selected
    OUTREG32(&g_pGPMCRegs->GPMC_SYSCONFIG, SYSCONFIG_FORCEIDLE | SYSCONFIG_AUTOIDLE);

    // configure SCM, SDRC, SMS, and GPMC to smartidle/autoidle
//    OUTREG32(&s_pSyscIFContext->CONTROL_SYSCONFIG, SYSCONFIG_SMARTIDLE | SYSCONFIG_AUTOIDLE);
    OUTREG32(&g_pSMSRegs->SMS_SYSCONFIG, SYSCONFIG_AUTOIDLE | SYSCONFIG_SMARTIDLE);
    OUTREG32(&g_pSDRCRegs->SDRC_SYSCONFIG, SYSCONFIG_SMARTIDLE);
    OUTREG32(&g_pGPMCRegs->GPMC_SYSCONFIG, SYSCONFIG_SMARTIDLE | SYSCONFIG_AUTOIDLE);

    #ifdef DEBUG_PRCM_SUSPEND_RESUME
        g_PrcmDebugSuspendResume = TRUE;
    #endif
		
    // re-enable serial debug output
    OEMEnableDebugOutput(TRUE);
    OALMSG(OAL_FUNC, (L" Serial debug messages renabled\r\n"));

    OALMSG(OAL_FUNC, (L"-OALPowerInit()\r\n"));
}

//-----------------------------------------------------------------------------
//
//  Function:  OALPowerPostInit
//
//  Called by kernel to allow system to continue initialization with
//  a full featured kernel.
//
VOID
OALPowerPostInit()
{
    IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN dpllInfo;
    
    DWORD rgDomain = DVFS_MPU1_OPP;
    DWORD * rgOpp = OALArgsQuery(OAL_ARGS_QUERY_OPP_MODE);    

    OALMSG(OAL_FUNC, (L"+OALPowerPostInit\r\n"));

    // Allow power compontents to initialize with a full featured kernel
    // before threads are scheduled
    PrcmPostInit();
    OALI2CPostInit();
    PrcmContextRestoreInit();
    
    SetOpp(&rgDomain,rgOpp,1);

    OALSDRCRefreshCounter(BSP_ARCV, BSP_ARCV >> 1);

    // ES 1.0
    // Need to force USB into standby mode to allow OMAP to go
    // into full retention
    ForceStandbyUSB();

    // Force MMC into idle
    ForceIdleMMC();

    // Disable clocks enabled by bootloader, except display
    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC1, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS2, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_TVOUT, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_WDT2, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_GPTIMER1, FALSE);

    // - PowerIC controlled via I2C
    // - T2 doesn't support OFF command via I2C
    PrcmVoltSetControlMode(
        SEL_VMODE_I2C | SEL_OFF_SIGNALLINE,
        SEL_VMODE | SEL_OFF
        );

    // configure memory on/retention/off levels
    PrcmDomainSetMemoryState(POWERDOMAIN_MPU,
        L2CACHEONSTATE_MEMORYON_DOMAINON | L2CACHERETSTATE_MEMORYRET_DOMAINRET |
        LOGICRETSTATE_LOGICRET_DOMAINRET,
        L2CACHEONSTATE | L2CACHERETSTATE |
        LOGICRETSTATE
        );

    PrcmDomainSetMemoryState(POWERDOMAIN_CORE,
        MEM2ONSTATE_MEMORYON_DOMAINON | MEM1ONSTATE_MEMORYON_DOMAINON |
        MEM2RETSTATE_MEMORYRET_DOMAINRET | MEM2RETSTATE_MEMORYRET_DOMAINRET |
        LOGICRETSTATE_LOGICRET_DOMAINRET,
        MEM2ONSTATE | MEM1ONSTATE |
        MEM2RETSTATE | MEM2RETSTATE |
        LOGICRETSTATE
        );

    PrcmDomainSetMemoryState(POWERDOMAIN_PERIPHERAL,
        LOGICRETSTATE_LOGICRET_DOMAINRET,
        LOGICRETSTATE
        );


    // ES3.1 fix
    if (g_dwCpuRevision == CPU_FAMILY_35XX_REVISION_ES_3_1)
        {
        PrcmDomainSetMemoryState(POWERDOMAIN_CORE, SAVEANDRESTORE, SAVEANDRESTORE);

        PrcmDomainSetMemoryState(POWERDOMAIN_USBHOST,
            LOGICRETSTATE_LOGICRET_DOMAINRET | MEMONSTATE_MEMORYON_DOMAINON,
            LOGICRETSTATE | MEMONSTATE
            );
        }

    // update dpll settings    
    dpllInfo.size = sizeof(IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN);
    dpllInfo.lowPowerEnabled = FALSE;
    dpllInfo.driftGuardEnabled = TRUE;
    dpllInfo.dpllMode = DPLL_MODE_LOCK; 
    dpllInfo.ffMask = DPLL_UPDATE_ALL;
    dpllInfo.rampTime = DPLL_RAMPTIME_20;
    dpllInfo.dpllAutoidleState = DPLL_AUTOIDLE_LOWPOWERSTOPMODE;
   
    // dpll1
    dpllInfo.dpllId = kDPLL1;
    PrcmClockSetDpllState(&dpllInfo);

    // dpll2
    dpllInfo.dpllId = kDPLL2;  
    PrcmClockSetDpllState(&dpllInfo);

    // dpll3
    dpllInfo.rampTime = DPLL_RAMPTIME_DISABLE;
    dpllInfo.dpllId = kDPLL3;
    PrcmClockSetDpllState(&dpllInfo);

    // dpll4
    dpllInfo.dpllId = kDPLL4;
    PrcmClockSetDpllState(&dpllInfo);

    // dpll5
    dpllInfo.dpllId = kDPLL5;
    PrcmClockSetDpllState(&dpllInfo);

    OALWakeupLatency_Initialize();

    PrcmDomainSetClockState(POWERDOMAIN_CORE, CLOCKDOMAIN_L3, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_CORE, CLOCKDOMAIN_L4, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_CORE, CLOCKDOMAIN_D2D, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_MPU, CLOCKDOMAIN_MPU, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_PERIPHERAL, CLOCKDOMAIN_PERIPHERAL, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_DSS, CLOCKDOMAIN_DSS, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_SGX, CLOCKDOMAIN_SGX, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_USBHOST, CLOCKDOMAIN_USBHOST, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_EMULATION, CLOCKDOMAIN_EMULATION, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_NEON, CLOCKDOMAIN_NEON, CLKSTCTRL_AUTOMATIC);
    // Enable IO interrupt
    PrcmInterruptEnable(PRM_IRQENABLE_IO_EN, TRUE);

    // save context for all registers restored by the kernel
    OALContextSaveSMS();
    OALContextSaveGPMC();
    OALContextSaveINTC();
    OALContextSavePRCM();
    OALContextSaveGPIO();
    OALContextSaveSCM();
    OALContextSaveDMA();
    OALContextSaveMux();
    OALContextSaveVRFB();

    OALMSG(OAL_FUNC, (L"-OALPowerPostInit\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  OALPowerVFP
//
//  Controls the VFP/Neon power domain.
//
BOOL
OALPowerVFP(DWORD dwCommand)
{
#if 1
    UNREFERENCED_PARAMETER(dwCommand);
#else
    switch (dwCommand)
    {
    case VFP_CONTROL_POWER_ON:
        PrcmDomainSetPowerState(POWERDOMAIN_NEON, POWERSTATE_ON, LOGICRETSTATE);
        return TRUE;

    case VFP_CONTROL_POWER_OFF:
        PrcmDomainSetPowerState(POWERDOMAIN_NEON, POWERSTATE_OFF, LOGICRETSTATE);
        return TRUE;

    case VFP_CONTROL_POWER_RETENTION:
        PrcmDomainSetPowerState(POWERDOMAIN_NEON, POWERSTATE_RETENTION, LOGICRETSTATE);
        return TRUE;
    }
#endif
    return FALSE;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALPrcmIntrHandler
//
//  This function implements prcm interrupt handler. This routine is called
//  from the OEMInterruptHandler
//
UINT32
OALPrcmIntrHandler()
{
    const UINT clear_mask = PRM_IRQENABLE_VP1_NOSMPSACK_EN |
                            PRM_IRQENABLE_VP2_NOSMPSACK_EN |
                            PRM_IRQENABLE_VC_SAERR_EN |
                            PRM_IRQENABLE_VC_RAERR_EN |
                            PRM_IRQENABLE_VC_TIMEOUTERR_EN |
                            PRM_IRQENABLE_WKUP_EN |
                            PRM_IRQENABLE_TRANSITION_EN |
                            PRM_IRQENABLE_MPU_DPLL_RECAL_EN |
                            PRM_IRQENABLE_CORE_DPLL_RECAL_EN |
                            PRM_IRQENABLE_VP1_OPPCHANGEDONE_EN |
                            PRM_IRQENABLE_VP2_OPPCHANGEDONE_EN |
                            PRM_IRQENABLE_IO_EN ;
    UINT sysIntr = SYSINTR_NOP;

    OALMSG(OAL_FUNC, (L"+OALPrcmIntrHandler\r\n"));

    // get cause of interrupt
    sysIntr = PrcmInterruptProcess(clear_mask);

    OALMSG(OAL_FUNC, (L"-OALPrcmIntrHandler\r\n"));
    return sysIntr;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveGPIO
//
//  Saves the GPIO Context, clears the IRQ for OFF mode
//
VOID
OALContextSaveGPIO ()
{
    UINT32 i;

    for(i=1; i< OMAP_GPIO_BANK_TO_RESTORE; i++)
        {
        // disable gpio clocks
        s_rgGpioContext[i].SYSCONFIG        =   INREG32(&s_rgGpioRegsAddr[i]->SYSCONFIG);
        s_rgGpioContext[i].IRQENABLE1       =   INREG32(&s_rgGpioRegsAddr[i]->IRQENABLE1);
        s_rgGpioContext[i].WAKEUPENABLE     =   INREG32(&s_rgGpioRegsAddr[i]->WAKEUPENABLE);
        s_rgGpioContext[i].IRQENABLE2       =   INREG32(&s_rgGpioRegsAddr[i]->IRQENABLE2);
        s_rgGpioContext[i].CTRL             =   INREG32(&s_rgGpioRegsAddr[i]->CTRL);
        s_rgGpioContext[i].OE               =   INREG32(&s_rgGpioRegsAddr[i]->OE);
        s_rgGpioContext[i].LEVELDETECT0     =   INREG32(&s_rgGpioRegsAddr[i]->LEVELDETECT0);
        s_rgGpioContext[i].LEVELDETECT1     =   INREG32(&s_rgGpioRegsAddr[i]->LEVELDETECT1);
        s_rgGpioContext[i].RISINGDETECT     =   INREG32(&s_rgGpioRegsAddr[i]->RISINGDETECT);
        s_rgGpioContext[i].FALLINGDETECT    =   INREG32(&s_rgGpioRegsAddr[i]->FALLINGDETECT);
        s_rgGpioContext[i].DEBOUNCENABLE    =   INREG32(&s_rgGpioRegsAddr[i]->DEBOUNCENABLE);
        s_rgGpioContext[i].DEBOUNCINGTIME   =   INREG32(&s_rgGpioRegsAddr[i]->DEBOUNCINGTIME);
        s_rgGpioContext[i].DATAOUT          =   INREG32(&s_rgGpioRegsAddr[i]->DATAOUT);
        }

    // clear dirty bit for gpio
    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_GPIO;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveGPMC
//
//  Stores the GPMC Registers in shadow register
//
VOID
OALContextSaveGPMC ()
{
    // Read the GPMC registers value and store it in shadow variable.
    s_gpmcContext.GPMC_SYSCONFIG = INREG32(&g_pGPMCRegs->GPMC_SYSCONFIG);
    s_gpmcContext.GPMC_IRQENABLE = INREG32(&g_pGPMCRegs->GPMC_IRQENABLE);
    s_gpmcContext.GPMC_TIMEOUT_CONTROL = INREG32(&g_pGPMCRegs->GPMC_TIMEOUT_CONTROL);
    s_gpmcContext.GPMC_CONFIG = INREG32(&g_pGPMCRegs->GPMC_CONFIG);
    s_gpmcContext.GPMC_PREFETCH_CONFIG1 = INREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONFIG1);
    s_gpmcContext.GPMC_PREFETCH_CONFIG2 = INREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONFIG2);
    s_gpmcContext.GPMC_PREFETCH_CONTROL = INREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONTROL);

    // Store the GPMC CS0 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_0) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_0);
        s_gpmcContext.GPMC_CONFIG2_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_0);
        s_gpmcContext.GPMC_CONFIG3_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_0);
        s_gpmcContext.GPMC_CONFIG4_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_0);
        s_gpmcContext.GPMC_CONFIG5_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_0);
        s_gpmcContext.GPMC_CONFIG6_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_0);
        s_gpmcContext.GPMC_CONFIG7_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_0);
    }

    // Store the GPMC CS1 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_1) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_1);
        s_gpmcContext.GPMC_CONFIG2_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_1);
        s_gpmcContext.GPMC_CONFIG3_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_1);
        s_gpmcContext.GPMC_CONFIG4_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_1);
        s_gpmcContext.GPMC_CONFIG5_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_1);
        s_gpmcContext.GPMC_CONFIG6_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_1);
        s_gpmcContext.GPMC_CONFIG7_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_1);
    }

    // Store the GPMC CS2 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_2) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_2);
        s_gpmcContext.GPMC_CONFIG2_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_2);
        s_gpmcContext.GPMC_CONFIG3_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_2);
        s_gpmcContext.GPMC_CONFIG4_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_2);
        s_gpmcContext.GPMC_CONFIG5_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_2);
        s_gpmcContext.GPMC_CONFIG6_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_2);
        s_gpmcContext.GPMC_CONFIG7_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_2);
    }

    // Store the GPMC CS3 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_3) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_3);
        s_gpmcContext.GPMC_CONFIG2_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_3);
        s_gpmcContext.GPMC_CONFIG3_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_3);
        s_gpmcContext.GPMC_CONFIG4_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_3);
        s_gpmcContext.GPMC_CONFIG5_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_3);
        s_gpmcContext.GPMC_CONFIG6_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_3);
        s_gpmcContext.GPMC_CONFIG7_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_3);
    }

    // Store the GPMC CS4 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_4) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_4);
        s_gpmcContext.GPMC_CONFIG2_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_4);
        s_gpmcContext.GPMC_CONFIG3_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_4);
        s_gpmcContext.GPMC_CONFIG4_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_4);
        s_gpmcContext.GPMC_CONFIG5_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_4);
        s_gpmcContext.GPMC_CONFIG6_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_4);
        s_gpmcContext.GPMC_CONFIG7_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_4);
    }

    // Store the GPMC CS5 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_5) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_5);
        s_gpmcContext.GPMC_CONFIG2_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_5);
        s_gpmcContext.GPMC_CONFIG3_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_5);
        s_gpmcContext.GPMC_CONFIG4_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_5);
        s_gpmcContext.GPMC_CONFIG5_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_5);
        s_gpmcContext.GPMC_CONFIG6_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_5);
        s_gpmcContext.GPMC_CONFIG7_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_5);
    }

    // Store the GPMC CS6 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_6) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_6);
        s_gpmcContext.GPMC_CONFIG2_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_6);
        s_gpmcContext.GPMC_CONFIG3_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_6);
        s_gpmcContext.GPMC_CONFIG4_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_6);
        s_gpmcContext.GPMC_CONFIG5_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_6);
        s_gpmcContext.GPMC_CONFIG6_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_6);
        s_gpmcContext.GPMC_CONFIG7_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_6);
    }

    // Store the GPMC CS7 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_7) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_7);
        s_gpmcContext.GPMC_CONFIG2_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_7);
        s_gpmcContext.GPMC_CONFIG3_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_7);
        s_gpmcContext.GPMC_CONFIG4_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_7);
        s_gpmcContext.GPMC_CONFIG5_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_7);
        s_gpmcContext.GPMC_CONFIG6_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_7);
        s_gpmcContext.GPMC_CONFIG7_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_7);
    }

    // clear dirty bit
    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_GPMC;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveSCM
//
//  Stores the SCM Registers in shadow register
//
VOID
OALContextSaveSCM ()
{
    // Read the SCM registers value and store it in shadow variable.

    s_syscIntContext.CONTROL_SYSCONFIG = INREG32(&s_pSyscIFContext->CONTROL_SYSCONFIG);

    s_syscGenContext.CONTROL_DEVCONF0      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEVCONF0);
    s_syscGenContext.CONTROL_MEM_DFTRW0    = INREG32(&g_pSysCtrlGenReg->CONTROL_MEM_DFTRW0);
    s_syscGenContext.CONTROL_MEM_DFTRW1    = INREG32(&g_pSysCtrlGenReg->CONTROL_MEM_DFTRW1);
    s_syscGenContext.CONTROL_MSUSPENDMUX_0 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_0);
    s_syscGenContext.CONTROL_MSUSPENDMUX_1 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_1);
    s_syscGenContext.CONTROL_MSUSPENDMUX_2 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_2);
    s_syscGenContext.CONTROL_MSUSPENDMUX_3 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_3);
    s_syscGenContext.CONTROL_MSUSPENDMUX_4 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_4);
    s_syscGenContext.CONTROL_MSUSPENDMUX_5 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_5);
    s_syscGenContext.CONTROL_SEC_CTRL      = INREG32(&g_pSysCtrlGenReg->CONTROL_SEC_CTRL);
    s_syscGenContext.CONTROL_DEVCONF1      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEVCONF1);
    s_syscGenContext.CONTROL_DEBOBS_0      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_0);
    s_syscGenContext.CONTROL_DEBOBS_1      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_1);
    s_syscGenContext.CONTROL_DEBOBS_2      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_2);
    s_syscGenContext.CONTROL_DEBOBS_3      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_3);
    s_syscGenContext.CONTROL_DEBOBS_4      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_4);
    s_syscGenContext.CONTROL_DEBOBS_5      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_5);
    s_syscGenContext.CONTROL_DEBOBS_6      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_6);
    s_syscGenContext.CONTROL_DEBOBS_7      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_7);
    s_syscGenContext.CONTROL_DEBOBS_8      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_8);

    s_syscGenContext.CONTROL_DSS_DPLL_SPREADING     = INREG32(&g_pSysCtrlGenReg->CONTROL_DSS_DPLL_SPREADING);
    s_syscGenContext.CONTROL_CORE_DPLL_SPREADING    = INREG32(&g_pSysCtrlGenReg->CONTROL_CORE_DPLL_SPREADING);
    s_syscGenContext.CONTROL_PER_DPLL_SPREADING     = INREG32(&g_pSysCtrlGenReg->CONTROL_PER_DPLL_SPREADING);
    s_syscGenContext.CONTROL_USBHOST_DPLL_SPREADING = INREG32(&g_pSysCtrlGenReg->CONTROL_USBHOST_DPLL_SPREADING);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_SCM;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSavePRCM
//
//  Stores the PRCM Registers in shadow register
//
VOID
OALContextSavePRCM ()
{
    s_ocpSysCmContext.CM_SYSCONFIG  = INREG32(&g_PrcmCm.pOMAP_OCP_SYSTEM_CM->CM_SYSCONFIG);

    s_coreCmContext.CM_AUTOIDLE1_CORE = INREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE1_CORE);
    s_coreCmContext.CM_AUTOIDLE2_CORE = INREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE2_CORE);
    s_coreCmContext.CM_AUTOIDLE3_CORE = INREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE3_CORE);

    s_wkupCmContext.CM_FCLKEN_WKUP    = INREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_FCLKEN_WKUP);
    s_wkupCmContext.CM_ICLKEN_WKUP    = INREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_ICLKEN_WKUP);
    s_wkupCmContext.CM_AUTOIDLE_WKUP  = INREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_AUTOIDLE_WKUP);

    s_clkCtrlCmContext.CM_CLKEN2_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKEN2_PLL);
    s_clkCtrlCmContext.CM_CLKSEL4_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKSEL4_PLL);
    s_clkCtrlCmContext.CM_CLKSEL5_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKSEL5_PLL);
    s_clkCtrlCmContext.CM_AUTOIDLE2_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_AUTOIDLE2_PLL);

    s_globalCmContext.CM_POLCTRL = INREG32(&g_PrcmCm.pOMAP_GLOBAL_CM->CM_POLCTRL);

    s_clkCtrlCmContext.CM_CLKOUT_CTRL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKOUT_CTRL);
    s_clkCtrlCmContext.CM_AUTOIDLE_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_AUTOIDLE_PLL);

    s_emuCmContext.CM_CLKSEL1_EMU = INREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL1_EMU);
    s_emuCmContext.CM_CLKSTCTRL_EMU = INREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSTCTRL_EMU);
    s_emuCmContext.CM_CLKSEL2_EMU = INREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL2_EMU);
    s_emuCmContext.CM_CLKSEL3_EMU = INREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL3_EMU);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_PRCM;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveINTC
//
//  Stores the MPU IC Registers in shadow register
//
VOID
OALContextSaveINTC ()
{
    UINT32 i;

    s_intcContext.INTC_SYSCONFIG = INREG32(&g_pIntr->pICLRegs->INTC_SYSCONFIG);
    s_intcContext.INTC_PROTECTION = INREG32(&g_pIntr->pICLRegs->INTC_PROTECTION);
    s_intcContext.INTC_IDLE = INREG32(&g_pIntr->pICLRegs->INTC_IDLE);
    s_intcContext.INTC_THRESHOLD = INREG32(&g_pIntr->pICLRegs->INTC_THRESHOLD);

    for (i = 0; i < dimof(g_pIntr->pICLRegs->INTC_ILR); i++)
        s_intcContext.INTC_ILR[i] = INREG32(&g_pIntr->pICLRegs->INTC_ILR[i]);

    s_intcContext.INTC_MIR0 = INREG32(&g_pIntr->pICLRegs->INTC_MIR0);
    s_intcContext.INTC_MIR1 = INREG32(&g_pIntr->pICLRegs->INTC_MIR1);
    s_intcContext.INTC_MIR2 = INREG32(&g_pIntr->pICLRegs->INTC_MIR2);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_INTC;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveDMA
//
//  Saves the DMA Registers
//
VOID
OALContextSaveDMA()
{
    s_dmaController.DMA4_GCR           = INREG32(&s_pDmaController->DMA4_GCR);
    s_dmaController.DMA4_OCP_SYSCONFIG = INREG32(&s_pDmaController->DMA4_OCP_SYSCONFIG);
    s_dmaController.DMA4_IRQENABLE_L0  = INREG32(&s_pDmaController->DMA4_IRQENABLE_L0);
    s_dmaController.DMA4_IRQENABLE_L1  = INREG32(&s_pDmaController->DMA4_IRQENABLE_L1);
    s_dmaController.DMA4_IRQENABLE_L2  = INREG32(&s_pDmaController->DMA4_IRQENABLE_L2);
    s_dmaController.DMA4_IRQENABLE_L3  = INREG32(&s_pDmaController->DMA4_IRQENABLE_L3);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_DMA;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveSMS
//
//  Saves the SMS Registers
//
VOID
OALContextSaveSMS()
{
    s_smsContext.SMS_SYSCONFIG         = INREG32(&g_pSMSRegs->SMS_SYSCONFIG);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_SMS;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveVRFB
//
//  Saves the VRFB Registers
//
VOID
OALContextSaveVRFB()
{
    int i;

    for (i = 0; i < VRFB_ROTATION_CONTEXTS; ++i)
        {
        s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_CONTROL =
            INREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_CONTROL);

        s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_SIZE =
            INREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_SIZE);

        s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_PHYSICAL_BA =
            INREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_PHYSICAL_BA);
        }

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_VRFB;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveMux
//
//  Saves pinmux
//
VOID
OALContextSaveMux()
{
    // Save all the PADCONF so the values are retained on wakeup from CORE OFF
    SETREG32(&g_pSysCtrlGenReg->CONTROL_PADCONF_OFF, STARTSAVE);
    while ((INREG32(&g_pSysCtrlGenReg->CONTROL_GENERAL_PURPOSE_STATUS) & SAVEDONE) == 0);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_PINMUX;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreGPIO
//
//  Restores the GPIO Registers
//
VOID
OALContextRestoreGPIO()
{
    UINT32 i;

    for(i=1; i< OMAP_GPIO_BANK_TO_RESTORE; i++)
        {
        OUTREG32(&s_rgGpioRegsAddr[i]->SYSCONFIG, s_rgGpioContext[i].SYSCONFIG );
        OUTREG32(&s_rgGpioRegsAddr[i]->CTRL, s_rgGpioContext[i].CTRL);
        OUTREG32(&s_rgGpioRegsAddr[i]->DATAOUT, s_rgGpioContext[i].DATAOUT);
        OUTREG32(&s_rgGpioRegsAddr[i]->OE, s_rgGpioContext[i].OE);
        OUTREG32(&s_rgGpioRegsAddr[i]->LEVELDETECT0, s_rgGpioContext[i].LEVELDETECT0);
        OUTREG32(&s_rgGpioRegsAddr[i]->LEVELDETECT1, s_rgGpioContext[i].LEVELDETECT1);
        OUTREG32(&s_rgGpioRegsAddr[i]->RISINGDETECT, s_rgGpioContext[i].RISINGDETECT);
        OUTREG32(&s_rgGpioRegsAddr[i]->FALLINGDETECT, s_rgGpioContext[i].FALLINGDETECT);

        // Note : Context restore of debouncing register is removed since no modules are 
        // using h/w debouncing. If debounce registers are restored, gpio fclk should be 
        // enabled and enough delay should be provided before disabling the gpio fclk
        // so that debouncing logic sync in h/w and Per domain acks idle request.

        //OUTREG32(&s_rgGpioRegsAddr[i]->DEBOUNCENABLE, s_rgGpioContext[i].DEBOUNCENABLE);
        //OUTREG32(&s_rgGpioRegsAddr[i]->DEBOUNCINGTIME, s_rgGpioContext[i].DEBOUNCINGTIME);

        OUTREG32(&s_rgGpioRegsAddr[i]->IRQENABLE1, s_rgGpioContext[i].IRQENABLE1);
        OUTREG32(&s_rgGpioRegsAddr[i]->WAKEUPENABLE, s_rgGpioContext[i].WAKEUPENABLE);
        OUTREG32(&s_rgGpioRegsAddr[i]->IRQENABLE2, s_rgGpioContext[i].IRQENABLE2);
        }
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreINTC
//
//  Restores the MPU IC Registers from shadow register
//
VOID
OALContextRestoreINTC()
{
    UINT32 i;

    OUTREG32(&g_pIntr->pICLRegs->INTC_SYSCONFIG, s_intcContext.INTC_SYSCONFIG);
    OUTREG32(&g_pIntr->pICLRegs->INTC_PROTECTION, s_intcContext.INTC_PROTECTION);
    OUTREG32(&g_pIntr->pICLRegs->INTC_IDLE, s_intcContext.INTC_IDLE);
    OUTREG32(&g_pIntr->pICLRegs->INTC_THRESHOLD, s_intcContext.INTC_THRESHOLD);

    for (i = 0; i < dimof(g_pIntr->pICLRegs->INTC_ILR); i++)
        OUTREG32(&g_pIntr->pICLRegs->INTC_ILR[i], s_intcContext.INTC_ILR[i]);

    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR0, ~s_intcContext.INTC_MIR0);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR1, ~s_intcContext.INTC_MIR1);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR2, ~s_intcContext.INTC_MIR2);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestorePRCM
//
//  Restores the PRCM Registers from shadow register
//
VOID
OALContextRestorePRCM ()
{
    PrcmContextRestore();
    
    OUTREG32(&g_PrcmCm.pOMAP_OCP_SYSTEM_CM->CM_SYSCONFIG, s_ocpSysCmContext.CM_SYSCONFIG);

    OUTREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE1_CORE, s_coreCmContext.CM_AUTOIDLE1_CORE);
    OUTREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE2_CORE, s_coreCmContext.CM_AUTOIDLE2_CORE);
    OUTREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE3_CORE, s_coreCmContext.CM_AUTOIDLE3_CORE);

    OUTREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_FCLKEN_WKUP, s_wkupCmContext.CM_FCLKEN_WKUP);
    OUTREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_ICLKEN_WKUP, s_wkupCmContext.CM_ICLKEN_WKUP);
    OUTREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_AUTOIDLE_WKUP, s_wkupCmContext.CM_AUTOIDLE_WKUP);

    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_AUTOIDLE2_PLL, DPLL_AUTOIDLE_DISABLED);
    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKSEL4_PLL, s_clkCtrlCmContext.CM_CLKSEL4_PLL);
    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKSEL5_PLL, s_clkCtrlCmContext.CM_CLKSEL5_PLL);
    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKEN2_PLL, s_clkCtrlCmContext.CM_CLKEN2_PLL);
    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_AUTOIDLE2_PLL, s_clkCtrlCmContext.CM_AUTOIDLE2_PLL);

    OUTREG32(&g_PrcmCm.pOMAP_GLOBAL_CM->CM_POLCTRL, s_globalCmContext.CM_POLCTRL);

    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKOUT_CTRL, s_clkCtrlCmContext.CM_CLKOUT_CTRL);

    // Restore EMU CM Registers
    OUTREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL1_EMU,  s_emuCmContext.CM_CLKSEL1_EMU);
    OUTREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSTCTRL_EMU, s_emuCmContext.CM_CLKSTCTRL_EMU);
    OUTREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL2_EMU, s_emuCmContext.CM_CLKSEL2_EMU);
    OUTREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL3_EMU, s_emuCmContext.CM_CLKSEL3_EMU);

    PrcmDomainSetClockState(POWERDOMAIN_EMULATION, CLOCKDOMAIN_EMULATION, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_NEON, CLOCKDOMAIN_NEON, CLKSTCTRL_AUTOMATIC);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreGPMC
//
//  Restores the GPMC Registers from shadow register
//
VOID
OALContextRestoreGPMC()
{

    OUTREG32(&g_pGPMCRegs->GPMC_SYSCONFIG, s_gpmcContext.GPMC_SYSCONFIG);
    OUTREG32(&g_pGPMCRegs->GPMC_IRQENABLE, s_gpmcContext.GPMC_IRQENABLE);
    OUTREG32(&g_pGPMCRegs->GPMC_TIMEOUT_CONTROL, s_gpmcContext.GPMC_TIMEOUT_CONTROL);
    OUTREG32(&g_pGPMCRegs->GPMC_CONFIG, s_gpmcContext.GPMC_CONFIG);
    OUTREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONFIG1, s_gpmcContext.GPMC_PREFETCH_CONFIG1);
    OUTREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONFIG2, s_gpmcContext.GPMC_PREFETCH_CONFIG2);
    OUTREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONTROL, s_gpmcContext.GPMC_PREFETCH_CONTROL);
    if(s_gpmcContext.GPMC_CONFIG7_0 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_0, s_gpmcContext.GPMC_CONFIG1_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_0, s_gpmcContext.GPMC_CONFIG2_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_0, s_gpmcContext.GPMC_CONFIG3_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_0, s_gpmcContext.GPMC_CONFIG4_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_0, s_gpmcContext.GPMC_CONFIG5_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_0, s_gpmcContext.GPMC_CONFIG6_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_0, s_gpmcContext.GPMC_CONFIG7_0);
    }
    if(s_gpmcContext.GPMC_CONFIG7_1 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_1, s_gpmcContext.GPMC_CONFIG1_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_1, s_gpmcContext.GPMC_CONFIG2_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_1, s_gpmcContext.GPMC_CONFIG3_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_1, s_gpmcContext.GPMC_CONFIG4_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_1, s_gpmcContext.GPMC_CONFIG5_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_1, s_gpmcContext.GPMC_CONFIG6_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_1, s_gpmcContext.GPMC_CONFIG7_1);
    }
    if(s_gpmcContext.GPMC_CONFIG7_2 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_2, s_gpmcContext.GPMC_CONFIG1_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_2, s_gpmcContext.GPMC_CONFIG2_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_2, s_gpmcContext.GPMC_CONFIG3_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_2, s_gpmcContext.GPMC_CONFIG4_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_2, s_gpmcContext.GPMC_CONFIG5_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_2, s_gpmcContext.GPMC_CONFIG6_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_2, s_gpmcContext.GPMC_CONFIG7_2);
    }
    if(s_gpmcContext.GPMC_CONFIG7_3 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_3, s_gpmcContext.GPMC_CONFIG1_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_3, s_gpmcContext.GPMC_CONFIG2_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_3, s_gpmcContext.GPMC_CONFIG3_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_3, s_gpmcContext.GPMC_CONFIG4_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_3, s_gpmcContext.GPMC_CONFIG5_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_3, s_gpmcContext.GPMC_CONFIG6_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_3, s_gpmcContext.GPMC_CONFIG7_3);
    }
    if(s_gpmcContext.GPMC_CONFIG7_4 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_4, s_gpmcContext.GPMC_CONFIG1_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_4, s_gpmcContext.GPMC_CONFIG2_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_4, s_gpmcContext.GPMC_CONFIG3_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_4, s_gpmcContext.GPMC_CONFIG4_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_4, s_gpmcContext.GPMC_CONFIG5_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_4, s_gpmcContext.GPMC_CONFIG6_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_4, s_gpmcContext.GPMC_CONFIG7_4);
    }
    if(s_gpmcContext.GPMC_CONFIG7_5 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_5, s_gpmcContext.GPMC_CONFIG1_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_5, s_gpmcContext.GPMC_CONFIG2_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_5, s_gpmcContext.GPMC_CONFIG3_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_5, s_gpmcContext.GPMC_CONFIG4_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_5, s_gpmcContext.GPMC_CONFIG5_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_5, s_gpmcContext.GPMC_CONFIG6_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_5, s_gpmcContext.GPMC_CONFIG7_5);
    }
    if(s_gpmcContext.GPMC_CONFIG7_6 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_6, s_gpmcContext.GPMC_CONFIG1_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_6, s_gpmcContext.GPMC_CONFIG2_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_6, s_gpmcContext.GPMC_CONFIG3_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_6, s_gpmcContext.GPMC_CONFIG4_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_6, s_gpmcContext.GPMC_CONFIG5_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_6, s_gpmcContext.GPMC_CONFIG6_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_6, s_gpmcContext.GPMC_CONFIG7_6);
    }
    if(s_gpmcContext.GPMC_CONFIG7_7 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_7, s_gpmcContext.GPMC_CONFIG1_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_7, s_gpmcContext.GPMC_CONFIG2_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_7, s_gpmcContext.GPMC_CONFIG3_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_7, s_gpmcContext.GPMC_CONFIG4_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_7, s_gpmcContext.GPMC_CONFIG5_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_7, s_gpmcContext.GPMC_CONFIG6_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_7, s_gpmcContext.GPMC_CONFIG7_7);
    }
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreSCM
//
//  Restores the SCM Registers from shadow register
//
VOID
OALContextRestoreSCM ()
{
    OUTREG32(&s_pSyscIFContext->CONTROL_SYSCONFIG, s_syscIntContext.CONTROL_SYSCONFIG);

    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEVCONF0, s_syscGenContext.CONTROL_DEVCONF0);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MEM_DFTRW0, s_syscGenContext.CONTROL_MEM_DFTRW0);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MEM_DFTRW1, s_syscGenContext.CONTROL_MEM_DFTRW1);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_0, s_syscGenContext.CONTROL_MSUSPENDMUX_0);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_1, s_syscGenContext.CONTROL_MSUSPENDMUX_1);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_2, s_syscGenContext.CONTROL_MSUSPENDMUX_2);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_3, s_syscGenContext.CONTROL_MSUSPENDMUX_3);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_4, s_syscGenContext.CONTROL_MSUSPENDMUX_4);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_5, s_syscGenContext.CONTROL_MSUSPENDMUX_5);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_SEC_CTRL, s_syscGenContext.CONTROL_SEC_CTRL);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEVCONF1, s_syscGenContext.CONTROL_DEVCONF1);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_0, s_syscGenContext.CONTROL_DEBOBS_0);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_1, s_syscGenContext.CONTROL_DEBOBS_1);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_2, s_syscGenContext.CONTROL_DEBOBS_2);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_3, s_syscGenContext.CONTROL_DEBOBS_3);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_4, s_syscGenContext.CONTROL_DEBOBS_4);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_5, s_syscGenContext.CONTROL_DEBOBS_5);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_6, s_syscGenContext.CONTROL_DEBOBS_6);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_7, s_syscGenContext.CONTROL_DEBOBS_7);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_8, s_syscGenContext.CONTROL_DEBOBS_8);

    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DSS_DPLL_SPREADING, s_syscGenContext.CONTROL_DSS_DPLL_SPREADING);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_CORE_DPLL_SPREADING, s_syscGenContext.CONTROL_CORE_DPLL_SPREADING);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_PER_DPLL_SPREADING, s_syscGenContext.CONTROL_PER_DPLL_SPREADING);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_USBHOST_DPLL_SPREADING, s_syscGenContext.CONTROL_USBHOST_DPLL_SPREADING);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreDMA
//
//  Restores the DMA Registers from shadow register
//
VOID
OALContextRestoreDMA()
{
    OUTREG32(&s_pDmaController->DMA4_GCR, s_dmaController.DMA4_GCR);
    OUTREG32(&s_pDmaController->DMA4_OCP_SYSCONFIG, s_dmaController.DMA4_OCP_SYSCONFIG);
    OUTREG32(&s_pDmaController->DMA4_IRQENABLE_L0, s_dmaController.DMA4_IRQENABLE_L0);
    OUTREG32(&s_pDmaController->DMA4_IRQENABLE_L1, s_dmaController.DMA4_IRQENABLE_L1);
    OUTREG32(&s_pDmaController->DMA4_IRQENABLE_L2, s_dmaController.DMA4_IRQENABLE_L2);
    OUTREG32(&s_pDmaController->DMA4_IRQENABLE_L3, s_dmaController.DMA4_IRQENABLE_L3);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreSMS
//  
//  Restores the SMS Registers from shadow register
//
VOID
OALContextRestoreSMS()
{
    OUTREG32(&g_pSMSRegs->SMS_SYSCONFIG, s_smsContext.SMS_SYSCONFIG);
        }

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreVRFB
//
//  Restores the VRFB Registers from shadow register
//
VOID
OALContextRestoreVRFB()
{
    int i;

    for (i = 0; i < VRFB_ROTATION_CONTEXTS; ++i)
        {
        OUTREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_CONTROL,
            s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_CONTROL
            );

        OUTREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_SIZE,
            s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_SIZE
            );

        OUTREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_PHYSICAL_BA,
            s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_PHYSICAL_BA
            );
        }
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreInit
//
//  Initialize the context Restore.
//
VOID
OALContextRestoreInit()
{
    // Initalize
    s_pSyscIFContext = OALPAtoUA(OMAP_SYSC_INTERFACE_REGS_PA);
    s_pDmaController = OALPAtoUA(OMAP_SDMA_REGS_PA);

    s_rgGpioRegsAddr[0] = OALPAtoUA(OMAP_GPIO1_REGS_PA);
    s_rgGpioRegsAddr[1] = OALPAtoUA(OMAP_GPIO2_REGS_PA);
    s_rgGpioRegsAddr[2] = OALPAtoUA(OMAP_GPIO3_REGS_PA);
    s_rgGpioRegsAddr[3] = OALPAtoUA(OMAP_GPIO4_REGS_PA);
    s_rgGpioRegsAddr[4] = OALPAtoUA(OMAP_GPIO5_REGS_PA);
    s_rgGpioRegsAddr[5] = OALPAtoUA(OMAP_GPIO6_REGS_PA);

    // Configure the OFFMODE values for SYS_NIRQ to get wakeup from T2
    OUTREG16(&g_pSyscPadConfsRegs->CONTROL_PADCONF_SYS_NIRQ,
                       (OFF_WAKE_ENABLE | OFF_INPUT_PULL_UP | INPUT_ENABLE |
                        PULL_UP | MUX_MODE_4));     
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextUpdateDirtyRegister
//
void
OALContextUpdateDirtyRegister(
    UINT32  ffRegisterSet
    )
{
    g_ffContextSaveMask |= ffRegisterSet;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextUpdateCleanRegister
//
void
OALContextUpdateCleanRegister(
    UINT32  ffRegisterSet
    )
{
    switch (ffRegisterSet)
        {
        case HAL_CONTEXTSAVE_GPIO:
            OALContextSaveGPIO();
            break;

        case HAL_CONTEXTSAVE_SCM:
            OALContextSaveSCM();
            break;

        case HAL_CONTEXTSAVE_GPMC:
            OALContextSaveGPMC();
            break;

        case HAL_CONTEXTSAVE_DMA:
            OALContextSaveDMA();
            break;

        case HAL_CONTEXTSAVE_PINMUX:
            OALContextSaveDMA();
            break;

        case HAL_CONTEXTSAVE_SMS:
            OALContextSaveSMS();
            break;

        case HAL_CONTEXTSAVE_VRFB:
            OALContextSaveVRFB();
            break;
}
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSave
//  
//  Saves the SCM, CM, GPMC context to shadow registers
//
BOOL
OALContextSave()
{
    return TRUE;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestore
//
//  Restores the SCM, CM, GPMC context from shadow registers
//
VOID
OALContextRestore(
    UINT32 prevMpuState,
    UINT32 prevCoreState,
    UINT32 prevPerState
    )
{
    UNREFERENCED_PARAMETER(prevMpuState);
    UNREFERENCED_PARAMETER(prevCoreState);
    UNREFERENCED_PARAMETER(prevPerState);
 }

//-----------------------------------------------------------------------------
//
//  Function:  OutShadowReg32
//
// Interface to update the GPMC, SDRC, MPUIC, CM register and corresponding
// shadow registers
//
VOID
OutShadowReg32(
    UINT32 deviceGroup,
    UINT32 offset,
    UINT32 value
    )
{
    UINT32 *pShadowRegBase = NULL;

    // change the byte offset to 4 bytes offset
    offset = offset/4;

    switch(deviceGroup)
        {
        case OMAP_PRCM_OCP_SYSTEM_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_ocpSysCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_OCP_SYSTEM_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;

        case OMAP_PRCM_WKUP_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_wkupCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_WKUP_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;

        case OMAP_PRCM_CORE_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_coreCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_CORE_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;


        case OMAP_PRCM_CLOCK_CONTROL_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_clkCtrlCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_CLOCK_CONTROL_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;

        case OMAP_PRCM_GLOBAL_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_globalCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_GLOBAL_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;


        default:
            break;
        }
}

//-----------------------------------------------------------------------------
//
//  Function:  SetShadowReg32
//
// Interface to update the GPMC, SDRC, MPUIC, CM register and corresponding
// shadow registers
//
VOID
SetShadowReg32(
    UINT32 deviceGroup,
    UINT32 offset,
    UINT32 value
    )
{
    UINT32 *pShadowRegBase = NULL;

    // change the byte offset to 4 bytes offset
    offset = offset/4;

    switch(deviceGroup)
        {
        case OMAP_PRCM_OCP_SYSTEM_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_ocpSysCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_OCP_SYSTEM_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_OCP_SYSTEM_CM+offset)|value);
            break;

        case OMAP_PRCM_WKUP_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_wkupCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_WKUP_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_WKUP_CM+offset)|value);
            break;

        case OMAP_PRCM_CORE_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_coreCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_CORE_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_CORE_CM+offset)|value);
            break;


        case OMAP_PRCM_CLOCK_CONTROL_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_clkCtrlCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_CLOCK_CONTROL_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_CLOCK_CONTROL_CM+offset)|value);
            break;

        case OMAP_PRCM_GLOBAL_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_globalCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_GLOBAL_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_GLOBAL_CM+offset)|value);
            break;

        default:
            break;
        }
}
//-----------------------------------------------------------------------------

