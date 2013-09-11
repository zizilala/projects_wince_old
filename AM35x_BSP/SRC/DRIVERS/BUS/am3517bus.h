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
#ifndef __AM3517BUS_H
#define __AM3517BUS_H

//-----------------------------------------------------------------------------
//
//  Class:  AM3517Bus_t
//
class AM3517Bus_t : public omapPowerBus_t
{
friend omapPowerBus_t;

    //-------------------------------------------------------------------------
public:     
    BOOL 
    InitializePowerBus(
        );
         
    void 
    DeinitializePowerBus(
        );

    virtual 
    DWORD 
    PreDevicePowerStateChange(
        DWORD devId, 
        CEDEVICE_POWER_STATE oldPowerState,
        CEDEVICE_POWER_STATE newPowerState
        );
    
    virtual 
    DWORD 
    PostDevicePowerStateChange(
        DWORD devId, 
        CEDEVICE_POWER_STATE oldPowerState,
        CEDEVICE_POWER_STATE newPowerState
        );

protected:
    AM3517Bus_t(
        );
};

//-----------------------------------------------------------------------------
#endif //__AM3517BUS_H

