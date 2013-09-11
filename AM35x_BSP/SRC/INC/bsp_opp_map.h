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
//  File:  bsp_opp_map.h
//

#ifndef __BSP_OPP_MAP_H
#define __BSP_OPP_MAP_H

#pragma warning (push)
#pragma warning (disable:4200)
//-----------------------------------------------------------------------------
typedef struct {
    VoltageProcessorSetting_t   vpInfo;
    int                         dpllCount;
    DpllFrequencySetting_t      rgFrequencySettings[];
} VddOppSetting_t;
#pragma warning (pop)


#define VDD_OPP_COUNT      AM35x_OPP_NUM


#if BSP_OPM_SELECT == 5
    // MPU[600Mhz @ 1.200V]
    #define INITIAL_VDD_OPP    (kOpp5)
#elif BSP_OPM_SELECT == 4
    // MPU[550Mhz @ 1.200V]
    #define INITIAL_VDD_OPP    (kOpp4)
#elif BSP_OPM_SELECT == 3
    // MPU[500Mhz @ 1.200V]
    #define INITIAL_VDD_OPP    (kOpp3)
#elif BSP_OPM_SELECT == 2
    // MPU[500Mhz @1.200V]
    #define INITIAL_VDD_OPP    (kOpp2)
#elif BSP_OPM_SELECT == 1
    // MPU[500Mhz @ 1.200V]
    #define INITIAL_VDD_OPP    (kOpp1)
#else
    #error Unsupported value for BSP_OPM_SELECT, use 1.5
#endif

#define MAX_VDD_OPP        (kOpp5)

//-----------------------------------------------------------------------------

// (just a placeholder)
static VddOppSetting_t vddOpp0Info = {
    {
        kVoltageProcessor1, 0, 0                        // voltage processor info
    }, 
        1,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 0,  (UINT)-1, 0,  0, 1                    // dpll1 (MPU)
       }
    }
};

// MPU[125Mhz @ 1.20V]
static VddOppSetting_t vddOpp1Info = {
    {
        kVoltageProcessor1, 1200,   1200                // voltage processor info
    }, 
        1,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 125, DIV_SYS_CLK * 250, 12, 7, 4                  // dpll1 (MPU)
       }
    }
};

// MPU[250Mhz @ 1.20V]
static VddOppSetting_t vddOpp2Info = {
    {
        kVoltageProcessor1, 1200,   1200                // voltage processor info
    }, 
        1,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 250, DIV_SYS_CLK * 250, 12, 7, 2                  // dpll1 (MPU)
       }
    }
};

// MPU[500Mhz @ 1.20V]
static VddOppSetting_t vddOpp3Info = {
    {
        kVoltageProcessor1, 1200,   1200                // voltage processor info
    }, 
        1,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 500, DIV_SYS_CLK * 250, 12, 7, 1                  // dpll1 (MPU)
       }
    }
};

// MPU[550Mhz @ 1.20V]
static VddOppSetting_t vddOpp4Info = {
    {
        kVoltageProcessor1, 1200,   1200                // voltage processor info
    }, 
        1,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 550, DIV_SYS_CLK * 275, 12, 7, 1                  // dpll1 (MPU)
       }
    }
};

// MPU[600Mhz @ 1.20V]
static VddOppSetting_t vddOpp5Info = {
    {
        kVoltageProcessor1, 1200,   1200                // voltage processor info
    }, 
        1,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 600, DIV_SYS_CLK * 300, 12, 7, 1                  // dpll1 (MPU)
       }
    }
};

//-----------------------------------------------------------------------------
static VddOppSetting_t  *_rgVddOppMap[VDD_OPP_COUNT] = {
    &vddOpp1Info,      // kOpp1
    &vddOpp2Info,      // kOpp2
    &vddOpp3Info,      // kOpp3
    &vddOpp4Info,      // kOpp4
    &vddOpp5Info,      // kOpp5
};


//-----------------------------------------------------------------------------
#endif // __BSP_OPP_MAP_H

