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

#include "bsp.h"
#include "oalex.h"
#include "ceddkex.h"

#include "oal_alloc.h"
#include "oal_i2c.h"
#include "bsp_cfg.h"
#include "oal_padcfg.h"
#include "bsp_padcfg.h"
#include "oal_clock.h"
#include "s35390_rtc.h"
#include "oal_gptimer.h"

//------------------------------------------------------------------------------
//  External functions
//
extern DWORD GetCp15ControlRegister(void);
extern DWORD GetCp15AuxiliaryControlRegister(void);
extern void EnableUnalignedAccess(void);


//------------------------------------------------------------------------------
//  Global FixUp variables
//
//
const volatile DWORD dwOEMDrWatsonSize     = 0x0004B000;
const volatile DWORD dwOEMHighSecurity     = OEM_HIGH_SECURITY_GP;

//------------------------------------------------------------------------------
//  Global variables

//-----------------------------------------------------------------------------
//
//  Global:  g_CpuFamily
//
//  Set during OEMInit to indicate CPU family.
//
DWORD g_dwCpuFamily = CPU_FAMILY_AM35XX;

//-----------------------------------------------------------------------------
//
//  Global:  g_CpuFamily
//
//  Set during OEMInit to indicate CPU family.
//

DWORD g_dwCpuRevision = CPU_REVISION_UNKNOWN;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMSRAMStartOffset
//
//  offset to start of SRAM where SRAM routines will be copied to.
//  Reinitialized in config.bib (FIXUPVAR)
//
DWORD dwOEMSRAMStartOffset = 0x00008000;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMMPUContextRestore
//
//  location to store context restore information from off mode (PA)
//
const volatile DWORD dwOEMMPUContextRestore = CPU_INFO_ADDR_PA;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMMaxIdlePeriod
//
//  maximum idle period during OS Idle in milliseconds
//
DWORD dwOEMMaxIdlePeriod = 1000;

//------------------------------------------------------------------------------

//extern DWORD gdwFailPowerPaging;
//extern DWORD cbNKPagingPoolSize;

//------------------------------------------------------------------------------
//
//  Global:  g_oalKitlEnabled
//
//  Save kitl state return by KITL intialization
//
//
DWORD g_oalKitlEnabled;

//-----------------------------------------------------------------------------
//
//  Global:  g_oalRetailMsgEnable
//
//  Used to enable retail messages
//
BOOL   g_oalRetailMsgEnable = FALSE;

//-----------------------------------------------------------------------------
//
//  Global:  g_ResumeRTC
//
//  Used to inform RTC code that a resume occured
//
BOOL g_ResumeRTC = FALSE;

//-----------------------------------------------------------------------------
//
//  Global:  g_dwMeasuredSysClkFreq
//
//  The measured SysClk frequency
//
extern DWORD g_dwMeasuredSysClkFreq;

//-----------------------------------------------------------------------------
//
//  Global:  g_pTimerRegs
//
//  32K timer register
//
extern OMAP_GPTIMER_REGS* g_pTimerRegs;

//------------------------------------------------------------------------------
//  Local functions
//
static void OALGPIOSetDefaultValues();
static void OALCalibrateSysClk();

//------------------------------------------------------------------------------
//
//  Function:  OEMInit
//
//  This is Windows CE OAL initialization function. It is called from kernel
//  after basic initialization is made.
//
VOID
OEMInit(
    )
{    
    BOOL           *pColdBoot;
    BOOL           *pRetailMsgEnable;
    static const PAD_INFO gpioPads[] = {GPIO_PADS END_OF_PAD_ARRAY};
    static UCHAR allocationPool[32768];


    //----------------------------------------------------------------------
    // Initialize OAL log zones
    //----------------------------------------------------------------------

    OALLogSetZones( //(0xFFFF & ~((1<<OAL_LOG_CACHE)|(1<<OAL_LOG_INTR)))|
    //           (1<<OAL_LOG_VERBOSE)  |
    //           (1<<OAL_LOG_INFO)     |
               (1<<OAL_LOG_ERROR)    |
               (1<<OAL_LOG_WARN)     |
    //           (1<<OAL_LOG_IOCTL)    |
    //           (1<<OAL_LOG_FUNC)     |
    //           (1<<OAL_LOG_INTR)     |
               0);

    OALMSG(OAL_FUNC, (L"+OEMInit\r\n"));

    //----------------------------------------------------------------------
    // Initialize the OAL memory allocation system (TI code)
    //----------------------------------------------------------------------
    OALLocalAllocInit(allocationPool,sizeof(allocationPool));

    //----------------------------------------------------------------------
    // Update platform specific variables
    //----------------------------------------------------------------------

    //----------------------------------------------------------------------
    // Update kernel variables
    //----------------------------------------------------------------------

    dwNKDrWatsonSize = dwOEMDrWatsonSize;

    // Set alarm resolution
    dwNKAlarmResolutionMSec = RTC_RESOLUTION_MS;

    // Set extension functions
    pOEMIsProcessorFeaturePresent = OALIsProcessorFeaturePresent;
    pfnOEMSetMemoryAttributes     = OALSetMemoryAttributes;
    
    //----------------------------------------------------------------------
    // Windows Mobile backward compatibility issue...
    //----------------------------------------------------------------------
/*
    switch (dwOEMTargetProject)
        {
        case OEM_TARGET_PROJECT_SMARTFON:
        case OEM_TARGET_PROJECT_WPC:
            CEProcessorType = PROCESSOR_STRONGARM;
            break;
        }
*/
    //----------------------------------------------------------------------
    // Initialize cache globals
    //----------------------------------------------------------------------

    OALCacheGlobalsInit();

    EnableUnalignedAccess();
    
    #ifdef DEBUG
        OALMSG(1, (L"CPU CP15 Control Register = 0x%x\r\n", GetCp15ControlRegister()));
        OALMSG(1, (L"CPU CP15 Auxiliary Control Register = 0x%x\r\n", GetCp15AuxiliaryControlRegister()));
    #endif
        
    //----------------------------------------------------------------------
    // Initialize Power Domains
    //----------------------------------------------------------------------
    
    OALPowerInit();

    //----------------------------------------------------------------------
    // Initialize Vector Floating Point co-processor
    //----------------------------------------------------------------------

    OALVFPInitialize(g_pOemGlobal);

    //----------------------------------------------------------------------
    // Initialize interrupt
    //----------------------------------------------------------------------

    if (!OALIntrInit())
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: OEMInit: failed to initialize interrupts\r\n"
            ));
        goto cleanUp;
        }

    //----------------------------------------------------------------------
    // Initialize system clock
    //----------------------------------------------------------------------

    if (!OALTimerInit(1, 0, 0))
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: OEMInit: Failed to initialize system clock\r\n"
            ));
        goto cleanUp;
        }

    //----------------------------------------------------------------------
    // Initialize PAD cfg
    //----------------------------------------------------------------------
    OALPadCfgInit();

    //----------------------------------------------------------------------
    // configure pin mux
    //----------------------------------------------------------------------
    ConfigurePadArray(BSPGetAllPadsInfo());

    // Configure the pads for the DSS (to keep the splashscreen active)
    // do not request it, it may make the DSS driver fail to load (because it will not ba able to request its pads)
    ConfigurePadArray(BSPGetDevicePadInfo(OMAP_DEVICE_DSS));
    //same thing for the CPGMAC (may be used by KITL)
    ConfigurePadArray(BSPGetDevicePadInfo(OMAP_DEVICE_CPGMAC));
    //same thing for the UART3 (used for our OAL serial output
    ConfigurePadArray(BSPGetDevicePadInfo(OMAP_DEVICE_UART3));
	//nmcca: Configure CCDC Pads (VRFB)
	ConfigurePadArray(BSPGetDevicePadInfo(OMAP_DEVICE_VPFE));
    ConfigurePadArray(BSPGetDevicePadInfo(OMAP_DEVICE_HSOTGUSB));

    //all other pads are to be requested (GPMC is never reserved by drivers, I2C is handled by the kernel)
    // GPIOs reservation may be split on per-GPIO basis and moved into the drivers that needs the GPIO. TBD
    if (!RequestDevicePads(OMAP_DEVICE_GPMC)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for GPMC\r\n")));
    if (!RequestDevicePads(OMAP_DEVICE_I2C1)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for I2C1\r\n")));
    if (!RequestDevicePads(OMAP_DEVICE_I2C2)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for I2C2\r\n")));
    if (!RequestDevicePads(OMAP_DEVICE_I2C3)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for I2C3\r\n")));
    if (!RequestAndConfigurePadArray(gpioPads)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for the GPIOs\r\n")));

    GPIOInit();

    //----------------------------------------------------------------------
    // Set GPIOs default values (like the buffers' OE)
    //----------------------------------------------------------------------
    OALGPIOSetDefaultValues();

    //----------------------------------------------------------------------
    // Initialize SRAM Functions
    //----------------------------------------------------------------------
    OALSRAMFnInit();

    //----------------------------------------------------------------------
    // kSYS_CLK calibration
    // Now compute the real kSYS_CLK clock value. 
    //----------------------------------------------------------------------
    OALCalibrateSysClk();

    //----------------------------------------------------------------------
    // Initialize high performance counter and profiling function pointers
    //----------------------------------------------------------------------
    OALPerformanceTimerInit();

    //----------------------------------------------------------------------
    // Initialize the RTC
    //----------------------------------------------------------------------
#ifdef BSP_RTC_WAKES_CPU
    OALS35390RTCInit(RTC_S35390_I2C_DEVICE,RTC_S35390_BAUD_INDEX,RTC_S35390_ADDRESS,TRUE);
#else
    OALS35390RTCInit(RTC_S35390_I2C_DEVICE,RTC_S35390_BAUD_INDEX,RTC_S35390_ADDRESS,FALSE);
#endif

    //----------------------------------------------------------------------
    // Initialize KITL
    //----------------------------------------------------------------------
    EnableDeviceClocks(OMAP_DEVICE_EFUSE,TRUE); //Turn on the EFUSE clock because KITL init may use it to get the default MAC address

    g_oalKitlEnabled = KITLIoctl(IOCTL_KITL_STARTUP, NULL, 0, NULL, 0, NULL);

    EnableDeviceClocks(OMAP_DEVICE_EFUSE,FALSE);

    //----------------------------------------------------------------------
    // Initialize the watchdog
    //----------------------------------------------------------------------
#ifdef BSP_OMAP_WATCHDOG
    OALWatchdogInit(BSP_WATCHDOG_PERIOD_MILLISECONDS,BSP_WATCHDOG_THREAD_PRIORITY);
#endif

    //----------------------------------------------------------------------
    // Check for retail messages enabled
    //----------------------------------------------------------------------

    pRetailMsgEnable = OALArgsQuery(OAL_ARGS_QUERY_OALFLAGS);
    if (pRetailMsgEnable && (*pRetailMsgEnable & OAL_ARGS_OALFLAGS_RETAILMSG_ENABLE))
        g_oalRetailMsgEnable = TRUE;

    //----------------------------------------------------------------------
    // Deinitialize serial debug
    //----------------------------------------------------------------------

    if (!g_oalRetailMsgEnable)
        OEMDeinitDebugSerial();

// not available under CE6
#if 0
    //----------------------------------------------------------------------
    // Make Page Tables walk L2 cacheable. There are 2 new fields in OEMGLOBAL
    // that we need to update:
    // dwTTBRCacheBits - the bits to set for TTBR to change page table walk
    //                   to be L2 cacheable. (Cortex-A8 TRM, section 3.2.31)
    //                   Set this to be "Outer Write-Back, Write-Allocate".
    // dwPageTableCacheBits - bits to indicate cacheability to access Level
    //                   L2 page table. We need to set it to "inner no cache,
    //                   outer write-back, write-allocate. i.e.
    //                      TEX = 0b101, and C=B=0.
    //                   (ARM1176 TRM, section 6.11.2, figure 6.7, small (4k) page)
    //----------------------------------------------------------------------
    g_pOemGlobal->dwTTBRCacheBits = 0x8;            // TTBR RGN set to 0b01 - outer write back, write-allocate
    g_pOemGlobal->dwPageTableCacheBits = 0x140;     // Page table cacheability uses 1BB/AA format, where AA = 0b00 (inner non-cached)
#endif

    g_oalIoCtlClockSpeed = 600;

    //----------------------------------------------------------------------
    // Check for a clean boot of device
    //----------------------------------------------------------------------
    pColdBoot = OALArgsQuery(OAL_ARGS_QUERY_COLDBOOT);
    if ((pColdBoot == NULL)|| ((pColdBoot != NULL) && *pColdBoot))
        NKForceCleanBoot();
cleanUp:
    OALMSG(OAL_FUNC, (L"-OEMInit\r\n"));
}

//------------------------------------------------------------------------------

void
OALCalibrateSysClk()
{
    DWORD dw32k_prev,dw32k, dw32k_diff;
    DWORD dwSysk_prev,dwSysk, dwSys_diff;
    DWORD dwOld;
    OMAP_DEVICE gptPerfDevice = BSPGetGPTPerfDevice();
    OMAP_GPTIMER_REGS   *pPerfTimer = OALPAtoUA(GetAddressByDevice(gptPerfDevice));
    EnableDeviceClocks(gptPerfDevice, TRUE);

    // configure performance timer
    //---------------------------------------------------
    // Soft reset GPTIMER and wait until finished
    SETREG32(&pPerfTimer->TIOCP, SYSCONFIG_SOFTRESET);
    while ((INREG32(&pPerfTimer->TISTAT) & GPTIMER_TISTAT_RESETDONE) == 0);
 
    // Enable smart idle and autoidle
    // Set clock activity - FCLK can be  switched off, 
    // L4 interface clock is maintained during wkup.
    OUTREG32(&pPerfTimer->TIOCP, 
        0x200 | SYSCONFIG_SMARTIDLE|SYSCONFIG_ENAWAKEUP|
            SYSCONFIG_AUTOIDLE);
    // clear interrupts
    OUTREG32(&pPerfTimer->TISR, 0x00000000);

    //  Start the timer.  Also set for auto reload
    SETREG32(&pPerfTimer->TCLR, GPTIMER_TCLR_ST);
    while ((INREG32(&pPerfTimer->TWPS) & GPTIMER_TWPS_TCLR) != 0);
    
#if SHOW_SYS_CLOCK_VARIATION
    int i;
    for (i=0; i<100;i++)
    {
#endif

    dwOld = OALTimerGetReg(&g_pTimerRegs->TCRR);
    do 
    {
        dwSysk_prev = INREG32(&pPerfTimer->TCRR); 
        dw32k_prev = OALTimerGetReg(&g_pTimerRegs->TCRR);
    } while (dw32k_prev == dwOld);

    OALStall(100000);

    dwOld = OALTimerGetReg(&g_pTimerRegs->TCRR);
    do
    {
        dwSysk = INREG32(&pPerfTimer->TCRR);
        dw32k = OALTimerGetReg(&g_pTimerRegs->TCRR);
    } while (dw32k == dwOld);

    dw32k_diff = dw32k - dw32k_prev;
    dwSys_diff = dwSysk - dwSysk_prev;

    g_dwMeasuredSysClkFreq =  (DWORD) (((INT64)dwSys_diff * 32768) / ((INT64)dw32k_diff)) ;

    DEBUGMSG(1,(L"SysClock calibrate Frequency = %d\r\n", g_dwMeasuredSysClkFreq));    

#if SHOW_SYS_CLOCK_VARIATION
    }
#endif

    EnableDeviceClocks(gptPerfDevice, FALSE);

}

//------------------------------------------------------------------------------


DWORD
OALMux_UpdateOnDeviceStateChange(
    UINT devId,
    UINT oldState,
    UINT newState,
    BOOL bPreStateChange
    )
{
    UNREFERENCED_PARAMETER(devId);
    UNREFERENCED_PARAMETER(oldState);
    UNREFERENCED_PARAMETER(newState);
    UNREFERENCED_PARAMETER(bPreStateChange);
    return (DWORD) -1;
}
void
OALMux_InitMuxTable(
    )
{
}

void EnableDebugSerialClock()
{               
#define OMAP_PRCM_PER_CM_REGS_PA            0x48005000
#define CM_CLKEN_UART3                      (1 << 11)

    OMAP_CM_REGS* pCmRegs;
    pCmRegs = (OMAP_CM_REGS*) (OMAP_PRCM_PER_CM_REGS_PA);
    SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_UART3);
    SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_UART3);
    while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_UART3);
}

//------------------------------------------------------------------------------
// External Variables
extern DEVICE_IFC_GPIO Omap_Gpio;
extern UINT32 g_ffContextSaveMask;

void BSPGpioInit()
{
   BSPInsertGpioDevice(0,&Omap_Gpio,NULL);
}

VOID MmUnmapIoSpace( 
  PVOID BaseAddress, 
  ULONG NumberOfBytes 
)
{
    UNREFERENCED_PARAMETER(BaseAddress);
    UNREFERENCED_PARAMETER(NumberOfBytes);
}

PVOID MmMapIoSpace( 
  PHYSICAL_ADDRESS PhysicalAddress, 
  ULONG NumberOfBytes, 
  BOOLEAN CacheEnable 
)
{
    UNREFERENCED_PARAMETER(NumberOfBytes);
    return OALPAtoVA(PhysicalAddress.LowPart,CacheEnable);
}
void
HalContextUpdateDirtyRegister(
                              UINT32 ffRegister
                              )
{
    g_ffContextSaveMask |= ffRegister;
}

void OALGPIOSetDefaultValues()
{
/*
    HANDLE hGPIO = GPIOOpen();
    GPIOClose(hGPIO);
*/
}
