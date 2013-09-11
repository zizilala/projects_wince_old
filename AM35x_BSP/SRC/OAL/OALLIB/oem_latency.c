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
//  File: oem_latency.c
//
#include "bsp.h"
#include "oalex.h"
#include <nkintr.h>
//#include <pkfuncs.h>

#include "omap_dvfs.h"
#include "oal_prcm.h"

//-----------------------------------------------------------------------------
#define MAX_OPM                 kOpm6
#ifndef MAX_INT
#define MAX_INT                 0x7FFFFFFF
#endif

#define STATE_NOCPUIDLE         (-1)

//-----------------------------------------------------------------------------
// CSWR = Clock Stopped With Retention
// OSWR = Off State With Retention
#define LATENCY_STATE_CHIP_OFF      0   // CORE+MPU+OTHER = OFF
#define LATENCY_STATE_CHIP_OSWR     1   // CORE+OTHER = OSWR, MPU = CSWR
#define LATENCY_STATE_CHIP_CSWR     2   // CORE+OTHER = CSWR, MPU = CSWR
#define LATENCY_STATE_CORE_CSWR     3   // OTHER=OFF/OSWR/CSWR/INACTIVE, CORE = CSWR, MPU=CSWR
#define LATENCY_STATE_CORE_INACTIVE 4   // OTHER=OFF/OSWR/CSWR/INACTIVE, CORE = INACTIVE, MPU=CSWR
#define LATENCY_STATE_MPU_INACTIVE  5   // OTHER=OFF/OSWR/CSWR/INACTIVE, CORE+MPU = INACTIVE
#define LATENCY_STATE_COUNT         6

//-----------------------------------------------------------------------------
#define RNG_RESET_DELAY             (1620 * BSP_ONE_MILLISECOND_TICKS)    // l4 @ 41.5 MHZs

//-----------------------------------------------------------------------------
const DWORD                     k32khzFrequency = 32768;

//-----------------------------------------------------------------------------
// externals

//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
extern OMAP_PRCM_PRM           *g_pPrcmPrm;

//-----------------------------------------------------------------------------
//
//  Global:  g_wakeupLatencyConstraintTickCount
//
//  latency time, in 32khz ticks, associated with current latency state
//
extern INT g_wakeupLatencyConstraintTickCount;


//-----------------------------------------------------------------------------
// typedefs and structs

typedef struct {
    float                       fixedOffset;
    float                       chipOffOffset;
    float                       chipOSWROffset;
    float                       chipCSWROffset;
    float                       coreCSWROffset;
    float                       coreInactiveOffset;
    float                       mpuInactiveOffset;
} LatencyOffsets;

typedef struct {
    DWORD                       ticks;
    float                       us;
} LatencyEntry;


//-----------------------------------------------------------------------------
// global variables

// Wakeup offset table - allows customization in the offset calculation from
// various sleep states
//
static
LatencyOffsets _rgLatencyOffsetTable[] = {
    {
      0.000335f, 0.004944f,   0.000174f,   0.000008f,  0.000045f,  0.000045f,  0.000045f       // OPM0
    },{
      0.000335f, 0.004699f,   0.000130f,   0.000008f,  0.000045f,  0.000045f,  0.000045f       // OPM1
    },{
      0.000335f, 0.002807f,   0.000100f,   0.000008f,  0.000045f,  0.000045f,  0.000045f       // OPM2
    },{
      0.000335f, 0.002807f,   0.000069f,   0.000008f,  0.000045f,  0.000045f,  0.000045f       // OPM3
    },{
      0.000335f, 0.002807f,   0.000069f,   0.000008f,  0.000045f,  0.000045f,  0.000045f       // OPM4
    },{
      0.000335f, 0.002807f,   0.000069f,   0.000008f,  0.000045f,  0.000045f,  0.000045f       // OPM5
    },{
      0.000335f, 0.002807f,   0.000069f,   0.000008f,  0.000045f,  0.000045f,  0.000045f       // OPM6
    },{
      0.0f,      0.0f,        0.0f,        0.0f,       0.0f,       0.0f,       0.0f            // OPM7
    },{
      0.0f,      0.0f,        0.0f,        0.0f,       0.0f,       0.0f,       0.0f            // OPM8
    }
};

// This table defines the possible transition states from an initial state
//
static 
int _mapLatencyTransitionTable [LATENCY_STATE_COUNT][LATENCY_STATE_COUNT + 1] = {
    {
       LATENCY_STATE_CHIP_OFF,      LATENCY_STATE_CHIP_OSWR,        LATENCY_STATE_CHIP_CSWR,        LATENCY_STATE_CORE_CSWR,        LATENCY_STATE_CORE_INACTIVE,    LATENCY_STATE_MPU_INACTIVE,     STATE_NOCPUIDLE
    }, {
       LATENCY_STATE_CHIP_OSWR,     LATENCY_STATE_CHIP_CSWR,        LATENCY_STATE_CORE_CSWR,        LATENCY_STATE_CORE_INACTIVE,    LATENCY_STATE_MPU_INACTIVE,     STATE_NOCPUIDLE
    }, {
       LATENCY_STATE_CHIP_CSWR,     LATENCY_STATE_CORE_CSWR,        LATENCY_STATE_CORE_INACTIVE,    LATENCY_STATE_MPU_INACTIVE,     STATE_NOCPUIDLE
    }, {
       LATENCY_STATE_CORE_CSWR,     LATENCY_STATE_CORE_INACTIVE,    LATENCY_STATE_MPU_INACTIVE,     STATE_NOCPUIDLE
    }, {
       LATENCY_STATE_CORE_INACTIVE, LATENCY_STATE_MPU_INACTIVE,     STATE_NOCPUIDLE
    }, {
       LATENCY_STATE_MPU_INACTIVE,  STATE_NOCPUIDLE
    }
};

// Maps all combination of opp in Vdd1/Vdd2 to an kOpm value
//
static
DWORD _mapLatencyIndex[kOppCount][kOppCount] = {
    /*
    Vdd2 = 
    kOpp1   kOpp2   kOpp3   kOpp4   kOpp5   kOpp6   kOpp7   kOpp8   kOpp9
    -----   -----   -----   -----   -----   -----   -----   -----   -----   */
    {
      0,      1,      1,      1,      1,      1,      1,      1,      1     // Vdd1 = kOpp1
    }, {
      2,      2,      2,      2,      2,      2,      2,      2,      2     // Vdd1 = kOpp2
    }, {
      3,      3,      3,      3,      3,      3,      3,      3,      3     // Vdd1 = kOpp3
    }, {
      4,      4,      4,      4,      4,      4,      4,      4,      4     // Vdd1 = kOpp4
    }, {
      5,      5,      5,      5,      5,      5,      5,      5,      5     // Vdd1 = kOpp5
    }, {
      6,      6,      6,      6,      6,      6,      6,      6,      6     // Vdd1 = kOpp6
    }, {
      6,      6,      6,      6,      6,      6,      6,      6,      6     // Vdd1 = kOpp7
    }, {
      6,      6,      6,      6,      6,      6,      6,      6,      6     // Vdd1 = kOpp8
    }, {
      6,      6,      6,      6,      6,      6,      6,      6,      6     // Vdd1 = kOpp9
    }
};


// Latency table  
//
static LatencyEntry _rgLatencyTable[kOpmCount][LATENCY_STATE_COUNT];

// off mode method
static DWORD _bOffMode_SignalMode = TRUE;

// maintains current OPP's for all voltage domains
//
static Dvfs_OperatingPoint_e _vddCore = kOpp1;
static Dvfs_OperatingPoint_e _vddMpu = kOpp3;

// maintains current power domain state for mpu and core domains
//
static CRITICAL_SECTION _csLatency;

static DWORD _suspendState = /* LATENCY_STATE_CHIP_OFF */ LATENCY_STATE_CHIP_OSWR;
static DWORD _domainMpu = POWERSTATE_ON;
static DWORD _domainCore = POWERSTATE_ON;
static DWORD _logicCore = LOGICRETSTATE_LOGICRET_DOMAINRET;
static DWORD _domainInactiveMask = 0;
static DWORD _domainRetentionMask = 0;

// _coreDevice and _otherDevice is used even before OALWakeupLatency_Initialize
// so make sure it is initialized
static DWORD _coreDevice = 0;
static DWORD _otherDevice = 0;

static DWORD _powerDomainCoreState;
static DWORD _powerDomainMpuState;
static DWORD _powerDomainPerState;
static DWORD _powerDomainNeonState;

// Tick count on Wake-up from OFF mode, used for RNG Reset completiion delay
static DWORD s_coreOffWaitTickCount = 0;
static BOOL  s_tickRollOver = FALSE;

//-----------------------------------------------------------------------------
// prototypes
//
extern BOOL IsSmartReflexMonitoringEnabled(UINT channel);

#ifdef DEBUG_PRCM_SUSPEND_RESUME
    static DWORD DeviceEnabledCount[OMAP_DEVICE_COUNT];
    static DWORD SavedDeviceEnabledCount[OMAP_DEVICE_COUNT];
    static DWORD SavedSuspendState;

    static PTCHAR SavedSuspendStateName[] = {
        L"LATENCY_STATE_CHIP_OFF      0, CORE+MPU+OTHER = OFF",
        L"LATENCY_STATE_CHIP_OSWR     1, CORE+OTHER = OSWR, MPU = CSWR",
        L"LATENCY_STATE_CHIP_CSWR     2, CORE+OTHER = CSWR, MPU = CSWR",
        L"LATENCY_STATE_CORE_CSWR     3, OTHER=OFF/OSWR/CSWR/INACTIVE, CORE = CSWR, MPU=CSWR",
        L"LATENCY_STATE_CORE_INACTIVE 4, OTHER=OFF/OSWR/CSWR/INACTIVE, CORE = INACTIVE, MPU=CSWR",
        L"LATENCY_STATE_MPU_INACTIVE  5, OTHER=OFF/OSWR/CSWR/INACTIVE, CORE+MPU = INACTIVE"
    };

    extern PTCHAR DeviceNames[];
#endif
	
//-----------------------------------------------------------------------------
//
//  Function: _OALWakeupLatency_Lock
//
//  Desc:
//
static
void
_OALWakeupLatency_Lock()
{    
    // check if constraint needs to be updated
    EnterCriticalSection(&_csLatency);
}

//-----------------------------------------------------------------------------
//
//  Function: _OALWakeupLatency_Unlock
//
//  Desc:
//
static
void
_OALWakeupLatency_Unlock()
{    
    // check if constraint needs to be updated
    LeaveCriticalSection(&_csLatency);
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_Initialize
//
//  Desc:
//      initializes the wake-up latency table for the platform.
//
BOOL
OALWakeupLatency_Initialize(
    )
{
    return TRUE;
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_SetOffModeConstraint
//
//  Desc:
//
//       Set the wait period for CORE to go OFF again
//       Delay for RNG reset completion in HS Device
//
VOID
OALWakeupLatency_SetOffModeConstraint(
    DWORD tcrr
    )
{
    UNREFERENCED_PARAMETER(tcrr);
} 

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_PushState
//
//  Desc:
//      Updates the hw and sw to put the device into a specific
//  wake-up interrupt state.  Saves the previous state into a private
//  stack structure.  This routine should only be called in OEMIdle.
//
DWORD
OALWakeupLatency_PushState(
    DWORD state
    )
{
    UNREFERENCED_PARAMETER(state);
    return TRUE;
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_PopState
//
//  Desc:
//      Updates the hw and sw to rollback the device from a wake-up interrupt
//  state.
//
void
OALWakeupLatency_PopState()
{
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_GetCurrentState
//
//  Desc:
//      returns the current state of the wakeup latency
//
DWORD
OALWakeupLatency_GetCurrentState()
{
    DWORD rc;

    rc = LATENCY_STATE_MPU_INACTIVE;
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_GetDelayInTicks
//
//  Desc:
//      Returns the wake-up latency time based on ticks
//
INT
OALWakeupLatency_GetDelayInTicks(
    DWORD state
    )
{
    UNREFERENCED_PARAMETER(state);

    // No latency because no retention and off mode.
    return 0;
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_FindStateByMaxDelayInTicks
//
//  Desc:
//      Returns the best match latency-state based on current state of
//  active devices and mpu/core domains.
//
DWORD
OALWakeupLatency_FindStateByMaxDelayInTicks(
    INT delayTicks
    )
{
    UNREFERENCED_PARAMETER(delayTicks);

    return (DWORD)LATENCY_STATE_MPU_INACTIVE;
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_UpdateOpp
//
//  Desc:
//      updates the latency state machine with the OPP of a
//  voltage domain.
//
BOOL
OALWakeupLatency_UpdateOpp(
    DWORD *rgDomains,
    DWORD *rgOpps,
    DWORD  count
    )
{
    UNREFERENCED_PARAMETER(rgDomains);
    UNREFERENCED_PARAMETER(rgOpps);
    UNREFERENCED_PARAMETER(count);

    return TRUE;
}


//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_UpdateDomainState
//
//  Desc:
//      updates the latency state machine with the state information of a
//  power domain.
//
BOOL
OALWakeupLatency_UpdateDomainState(
    DWORD powerDomain,
    DWORD powerState,
    DWORD logicState
    )
{
    UNREFERENCED_PARAMETER(powerDomain);
    UNREFERENCED_PARAMETER(powerState);
    UNREFERENCED_PARAMETER(logicState);

    // No power domain manage by prcm

    return TRUE;
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_DeviceEnabled
//
//  Desc:
//      notification from prcm device manager when a hardware device is
//  enabled or disabled.
//
BOOL
OALWakeupLatency_DeviceEnabled(
    DWORD devId,
    BOOL bEnabled
    )
{
   
    switch (devId)
        {
        case OMAP_DEVICE_I2C1:
        case OMAP_DEVICE_I2C2:
        case OMAP_DEVICE_I2C3:
        case OMAP_DEVICE_MMC1:
        case OMAP_DEVICE_MMC2:
        case OMAP_DEVICE_MMC3:
        case OMAP_DEVICE_USBTLL:
        case OMAP_DEVICE_HDQ:
        case OMAP_DEVICE_MCBSP1:
        case OMAP_DEVICE_MCBSP5:
        case OMAP_DEVICE_MCSPI1:
        case OMAP_DEVICE_MCSPI2:
        case OMAP_DEVICE_MCSPI3:
        case OMAP_DEVICE_MCSPI4:
        case OMAP_DEVICE_UART1:
        case OMAP_DEVICE_UART2:
        case OMAP_DEVICE_TS:
        case OMAP_DEVICE_GPTIMER10:
        case OMAP_DEVICE_GPTIMER11:
		case OMAP_DEVICE_IPSS:
        case OMAP_DEVICE_EFUSE:
            if (bEnabled == TRUE)
                {
                #ifdef DEBUG_PRCM_SUSPEND_RESUME
                    DeviceEnabledCount[devId]++;
				#endif
                ++_coreDevice;
                }
            else
                {
                #ifdef DEBUG_PRCM_SUSPEND_RESUME
                    if (DeviceEnabledCount[devId] > 0)
                        DeviceEnabledCount[devId]--;
				#endif
                if (_coreDevice > 0) 
				    --_coreDevice;
                }
            break;

        /*
        GPIO clocks not included, always on unless in suspend
        case OMAP_DEVICE_GPIO2:
        case OMAP_DEVICE_GPIO3:
        case OMAP_DEVICE_GPIO4:
        case OMAP_DEVICE_GPIO5:
        case OMAP_DEVICE_GPIO6:
        */
        case OMAP_DEVICE_MCBSP2:          
        case OMAP_DEVICE_MCBSP3:
        case OMAP_DEVICE_MCBSP4:
        case OMAP_DEVICE_GPTIMER2:
        case OMAP_DEVICE_GPTIMER3:
        case OMAP_DEVICE_GPTIMER4:
        case OMAP_DEVICE_GPTIMER5:
        case OMAP_DEVICE_GPTIMER6:
        case OMAP_DEVICE_GPTIMER7: 
        case OMAP_DEVICE_GPTIMER8:
        case OMAP_DEVICE_GPTIMER9:    
        case OMAP_DEVICE_UART3:
        case OMAP_DEVICE_WDT3:       
        case OMAP_DEVICE_DSS1:
        case OMAP_DEVICE_DSS2:
        case OMAP_DEVICE_TVOUT:
        case OMAP_DEVICE_CSI2: 
        case OMAP_DEVICE_2D: 
        case OMAP_DEVICE_3D:
        case OMAP_DEVICE_SGX:
        case OMAP_DEVICE_OHCI: 
        case OMAP_DEVICE_EHCI: 
        case OMAP_DEVICE_USBHOST1: 
        case OMAP_DEVICE_USBHOST2:
        case OMAP_DEVICE_USBHOST3:
            if (bEnabled == TRUE)
                {
                #ifdef DEBUG_PRCM_SUSPEND_RESUME
                    DeviceEnabledCount[devId]++;
                #endif
                ++_otherDevice;
                }
            else
                {
                #ifdef DEBUG_PRCM_SUSPEND_RESUME
                    if (DeviceEnabledCount[devId] > 0)
                        DeviceEnabledCount[devId]--;
                #endif
                if (_otherDevice > 0) 
    				--_otherDevice;
                }
            break; 
        }

    return TRUE;
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_IsChipOff
//
//  Desc:
//      returns CORE OFF status for a particular latency state
//
BOOL
OALWakeupLatency_IsChipOff(
    DWORD latencyState
    )
{
    return  (latencyState == LATENCY_STATE_CHIP_OFF);
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_GetSuspendState
//
//  Desc:
//      returns the default suspend state
//
DWORD
OALWakeupLatency_GetSuspendState(
    )
{
    return LATENCY_STATE_MPU_INACTIVE;
}

//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_SetSuspendState
//
//  Desc:
//      returns the default suspend state
//
BOOL
OALWakeupLatency_SetSuspendState(
    DWORD suspendState
    )
{
    BOOL rc = FALSE;
    if (suspendState == LATENCY_STATE_MPU_INACTIVE)
        {
        _suspendState = suspendState;
        rc = TRUE;
        }

    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlInterruptLatencyConstraint
//
//  updates the current interrupt latency constraint
//
BOOL 
OALIoCtlInterruptLatencyConstraint(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;

    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pInBuffer);

    // No time constraint is possible because no transition.
    
    rc = TRUE;
    return rc;
}

//-----------------------------------------------------------------------------

#ifdef DEBUG_PRCM_SUSPEND_RESUME

    // save values for later display
    VOID OALWakeupLatency_SaveSnapshot()
    {
        int i;
	
        SavedSuspendState = OALWakeupLatency_GetSuspendState();

        for (i = 0; i < OMAP_DEVICE_COUNT; i++)
	    {
            // latency ref count
    	    SavedDeviceEnabledCount[i] = DeviceEnabledCount[i];
	    }
    }

    // dump saved values
    VOID OALWakeupLatency_DumpSnapshot()
    {
        int i;
	
        OALMSG(1, (L"\r\nOALWakeupLatency Saved Snapshot::\r\n"));

        OALMSG(1, (L"\r\nSaved Non-Zero Interrupt Latency Device Enabled Counts:\r\n"));
        for (i = 0; i < OMAP_DEVICE_COUNT; i++)
            if (SavedDeviceEnabledCount[i])
    	        OALMSG(1, (L"Saved Interrupt Latency Device Enabled Count %s = %d\r\n", DeviceNames[i], SavedDeviceEnabledCount[i]));

        OALMSG(1, (L"\r\nSaved suspend state: %d, %s\r\n", SavedSuspendState, SavedSuspendStateName[SavedSuspendState]));
    }

#else

    VOID OALWakeupLatency_SaveSnapshot()
    {
    }

    VOID OALWakeupLatency_DumpSnapshot()
    {
    }

#endif