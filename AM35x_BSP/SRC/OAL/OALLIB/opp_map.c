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
//  File:  opp_map.c
//
#include "bsp.h"
#include "oalex.h"
#include "oal_prcm.h"
#include "omap_led.h"
#include "omap_dvfs.h"
#include "bsp_opp_map.h"

#include "triton.h"
#include "tps65023.h"

#define MAX_VOLT_DOMAINS        (2)

#define VP_VOLTS(x) (((x) * 125 + 6000) / 10000)
#define VP_MILLIVOLTS(x) (((x) * 125 + 6000) % 10000)

#pragma warning(push)
#pragma warning(disable: 6385)
#pragma warning(disable: 6386)

//-----------------------------------------------------------------------------
static UINT _rgOppVdd[1] = {INITIAL_VDD_OPP};


//-----------------------------------------------------------------------------
void 
UpdateRetentionVoltages(IOCTL_RETENTION_VOLTAGES *pData)
{
    UNREFERENCED_PARAMETER(pData);
}

//-----------------------------------------------------------------------------
BOOL
SetVoltageOppViaVoltageProcessor(
    VddOppSetting_t        *pVddOppSetting,
    UINT                   *retentionVoltages
    )
{
    BOOL rc = FALSE;  
    UINT32 mv1;
    HANDLE hTwl;

    UNREFERENCED_PARAMETER(retentionVoltages);

    hTwl = TWLOpen();
    TWLGetVoltage(VDCDC1,&mv1);
    TWLSetVoltage(VDCDC1,pVddOppSetting->vpInfo.initVolt);
    TWLGetVoltage(VDCDC1,&mv1);  
    TWLClose(hTwl);

    rc = TRUE;

    return rc;
}

//-----------------------------------------------------------------------------
BOOL
SetFrequencyOpp(
    VddOppSetting_t        *pVddOppSetting
    )
{
    int i;
    BOOL rc = FALSE;

    // iterate through and set the dpll frequency settings    
    for (i = 0; i < pVddOppSetting->dpllCount; ++i)
        {
        PrcmClockSetDpllFrequency(
            pVddOppSetting->rgFrequencySettings[i].dpllId,
            pVddOppSetting->rgFrequencySettings[i].m,
            pVddOppSetting->rgFrequencySettings[i].n,
            pVddOppSetting->rgFrequencySettings[i].freqSel,
            pVddOppSetting->rgFrequencySettings[i].outputDivisor
            );
        };

    rc = TRUE;

    return rc;
}

//-----------------------------------------------------------------------------
BOOL
SetVoltageOpp(
    VddOppSetting_t    *pVddOppSetting
    )
{
    return SetVoltageOppViaVoltageProcessor(pVddOppSetting, NULL);
}

//-----------------------------------------------------------------------------
BOOL 
SetOpp(
    DWORD *rgDomains,
    DWORD *rgOpps,    
    DWORD  count
    )
{
    UINT                opp;
    UINT                i;
    int                 vdd = 0;
    VddOppSetting_t   **ppVoltDomain;

    // loop through and update all changing voltage domains
    //
    for (i = 0; i < count; ++i)
        {
        // select the Opp table to use
        switch (rgDomains[i])
            {
            case DVFS_MPU1_OPP:
                // validate parameters
                if (rgOpps[i] > MAX_VDD_OPP) continue;
                
                vdd = kVDD1;
                ppVoltDomain = _rgVddOppMap;
                break;

            default:
                continue;
            }

        // check if the operating point is actually changing
        opp = rgOpps[i];
        if (_rgOppVdd[vdd] == opp) continue;

        // depending on which way the transition is occurring change
        // the frequency and voltage levels in the proper order
        if (opp > _rgOppVdd[vdd])
            {
            // transitioning to higher performance, change voltage first
            //SetVoltageOpp(ppVoltDomain[opp]);
            SetFrequencyOpp(ppVoltDomain[opp]);         
            }
        else
            {
            // transitioning to lower performance, change frequency first
            SetFrequencyOpp(ppVoltDomain[opp]); 
            //SetVoltageOpp(ppVoltDomain[opp]);         
            }
            
        // update opp for voltage domain
        _rgOppVdd[vdd] = opp;
    
        }

    // update latency table
    OALWakeupLatency_UpdateOpp(rgDomains, rgOpps, count);

    return TRUE;    
}


BOOL 
SmartReflex_EnableMonitor(
    UINT                    channel,
    BOOL                    bEnable
    )
{
    UNREFERENCED_PARAMETER(channel);
    UNREFERENCED_PARAMETER(bEnable);

    return FALSE;
}

BOOL 
IsSmartReflexMonitoringEnabled(UINT channel)
{   
    UNREFERENCED_PARAMETER(channel);
    return FALSE;
}



#pragma warning(pop)

//-----------------------------------------------------------------------------

