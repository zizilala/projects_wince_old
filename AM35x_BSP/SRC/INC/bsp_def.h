// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

//
//=============================================================================
//            Texas Instruments OMAP(TM) Platform Software
// (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
//
//  Use of this software is controlled by the terms and conditions found
// in the license agreement under which this software has been supplied.
//
//=============================================================================
//

//------------------------------------------------------------------------------
//
//  File:  bsp_def.h
//
#ifndef __BSP_DEF_H
#define __BSP_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

#define AM35x_OPP_NUM    5

//#define DIV_SYS_CLK  1 //SYS_CLK = 26MHz
#define DIV_SYS_CLK  2   //SYS_CLK = 26/2 = 13 MHz (USB OTG won't work with SYS_CLK at 26MHz)

//------------------------------------------------------------------------------
//
//  Select SYS_CLK divider.
//
#if (DIV_SYS_CLK  == 2)
#define BSP_PRM_CLKSRC_CTRL (2<<6)
#else
#define BSP_PRM_CLKSRC_CTRL (1<<6)
#endif

//------------------------------------------------------------------------------
//
//  Select initial XLDR CPU speed and VDD1 voltage using BSP_OPM_SELECT
//

#if BSP_OPM_SELECT == 5
    // MPU[600Mhz @ 1.3500V], IVA2[430Mhz @ 1.35V]
    #define BSP_SPEED_CPUMHZ                600
    #define VDD1_INIT_VOLTAGE_VALUE         0x3c
#elif BSP_OPM_SELECT == 4
    // MPU[550Mhz @ 1.2750V], IVA2[400Mhz @ 1.27V]
    #define BSP_SPEED_CPUMHZ                550
    #define VDD1_INIT_VOLTAGE_VALUE         0x36
#elif BSP_OPM_SELECT == 3
    // MPU[500Mhz @ 1.2000V], IVA2[360Mhz @ 1.20V]
    #define BSP_SPEED_CPUMHZ                500
    #define VDD1_INIT_VOLTAGE_VALUE         0x30
#elif BSP_OPM_SELECT == 2
    // MPU[250Mhz @ 1.000V], IVA2[180Mhz @ 1.00V]
    #define BSP_SPEED_CPUMHZ                250
    #define VDD1_INIT_VOLTAGE_VALUE         0x20
#elif BSP_OPM_SELECT == 1
    // MPU[125Mhz @ 0.975V], IVA2[ 90Mhz @ 0.975V]
    #define BSP_SPEED_CPUMHZ                125
    #define VDD1_INIT_VOLTAGE_VALUE         0x1e
#else
    #error Unsupported value for BSP_OPM_SELECT
#endif

//------------------------------------------------------------------------------
//
//  Define:  BSP_DEVICE_PREFIX
//
//  This define is used as device name prefix when KITL creates device name.
//
#define BSP_DEVICE_PREFIX       "AM35x-"

//-----------------------------------------------------------------------------
// UNDONE:
//  currently used for OFF mode validation.  need to eventually settle
// on off mode values and delete these constants
//
#define PRM_VDD1_SETUP_TIME             (0x32 << 16)
#define PRM_VDD2_SETUP_TIME             (0x32)
#define PRM_OFFMODE_SETUP_TIME          (0x0)
#define PRM_COREOFF_OFFSET_TIME         (0x41)
#define PRM_CLK_SETUP_TIME              (0x0000)
#define PRM_AUTO_OFF_ENABLED            (0x1 << 2)
#define PRM_SYS_OFF_SIGNAL_ENABLED      (0x1 << 3)

//------------------------------------------------------------------------------
//
//  Define: BSP_PRM_CLKSETUP
//
//  Determines the latency for src clk stabilization.
//  Used to update PRM_CLKSETUP
//
//  Allowed values:
//
//      [0-0xFFFF]  - in 32khz tick value
//
#define BSP_PRM_CLKSETUP                (4)     // ~122 usec
#define BSP_PRM_CLKSETUP_OFFMODE        (0xA0) // 5 ms oscillator startup

//------------------------------------------------------------------------------
//
//  Define: BSP_PRM_VOLTSETUP1_RET
//
//  Determines the latency for VDD1 & VDD2 stabilization.
//  Used to update PRM_VOLTSETUP1
//
//  Allowed values:
//
#define BSP_VOLTSETUP1_VDD2_INIT        (0x0112 << 16)  // 1 ms ramp time
#define BSP_VOLTSETUP1_VDD1_INIT        (0x0112 << 0)   // 1 ms ramp time
#define BSP_PRM_VOLTSETUP1_INIT         (BSP_VOLTSETUP1_VDD2_INIT | \
                                         BSP_VOLTSETUP1_VDD1_INIT)

// This is used when I2C is used instead of SYS_OFFMODE pin
#define BSP_VOLTSETUP1_VDD2_OFF_MODE    (0xB3 << 0)   // 55 usec ramp time
#define BSP_VOLTSETUP1_VDD1_OFF_MODE    (0xA0 << 16)  // 49 usec ramp time
#define BSP_PRM_VOLTSETUP1_OFF_MODE     (BSP_VOLTSETUP1_VDD2_OFF_MODE | \
                                         BSP_VOLTSETUP1_VDD1_OFF_MODE)


//------------------------------------------------------------------------------
//
//  Define: BSP_PRM_VOLTSETUP2
//
//  Determines the latency for VDD1 & VDD2 stabilization.
//  Used to update PRM_VOLTSETUP1
//
//  Allowed values:
//
#define BSP_PRM_VOLTSETUP2              (0x0)   // just use PRM_VOLTSETUP1
                                                // for voltage stabilization
                                                // latency

//------------------------------------------------------------------------------
//
//  Define: BSP_PRM_VOLTOFFSET
//
//  Determines the latency of sys_offmode signal upon wake-up from OFF mode
//  when the OFF sequence is supervised by the Power IC
//
//  Allowed values:
//
#define BSP_PRM_VOLTOFFSET              (0x0)   // just use PRM_VOLTSETUP1
                                                // for voltage stabilization
                                                // latency

//------------------------------------------------------------------------------
//
//  Define: BSP_PRM_CLKSEL
//
//  Determines the system clock frequency.  Used to update PRM_CLKSEL
//
//  Allowed values:
//
//      0x0: Input clock is 12 MHz
//      0x1: Input clock is 13 MHz
//      0x2: Input clock is 19.2 MHz
//      0x3: Input clock is 26 MHz
//      0x4: Input clock is 38.4 MHz
//
#if (DIV_SYS_CLK  == 2)
#define BSP_PRM_CLKSEL                  (1)
#else
#define BSP_PRM_CLKSEL                  (3)
#endif


//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL_CORE
//
//  Determines CORE clock selection and dividers.  Used to update CM_CLKSEL_CORE
//
//  Allowed values:
//
#define BSP_CLKSEL_L3                  (2 << 0)    // L3 = CORE_CLK/2
#define BSP_CLKSEL_L4                  (2 << 2)    // L4 = L3 / 2
#define BSP_CLKSEL_GPT10               (0 << 6)    // GPT10 clk src = 32khz
#define BSP_CLKSEL_GPT11               (1 << 7)    // GPT11 clk src = sys_clk (26Mhz)

#define BSP_CM_CLKSEL_CORE             (BSP_CLKSEL_L3 | \
                                        BSP_CLKSEL_L4 | \
                                        BSP_CLKSEL_GPT10 | \
                                        BSP_CLKSEL_GPT11)


//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKEN_PLL
//
//  Determines the DPLL3 and DPLL4 internal frequency based on DPLL reference
//  clock and divider (n).  Used to update CM_CLKEN_PLL
//
//  Allowed values:
//
//      0x3: 0.75 MHz to 1.0 MHz
//      0x4: 1.0 MHz to 1.25 MHz
//      0x5: 1.25 MHz to 1.5 MHz
//      0x6: 1.5 MHz to 1.75 MHz
//      0x7: 1.75 MHz to 2.1 MHz
//      0xB: 7.5 MHz to 10 MHz
//      0xC: 10 MHz to 12.5 MHz
//      0xD: 12.5 MHz to 15 MHz
//      0xE: 15 MHz to 17.5 MHz
//      0xF: 17.5 MHz to 21 MHz
//
#define BSP_PWRDN_EMU_PERIPH            (0 << 31)   // enable DPLL4_M6X2
#define BSP_PWRDN_CAM                   (0 << 30)   // enable DPLL4_M5X2
#define BSP_PWRDN_DSS1                  (0 << 29)   // enable DPLL4_M4X2
#define BSP_PWRDN_TV                    (0 << 28)   // enable DPLL4_M3X2
#define BSP_PWRDN_96M                   (0 << 27)   // enable DPLL4_M2X2
#define BSP_EN_PERIPH_DPLL_LPMODE       (0 << 26)   // disable DPLL4 LP mode
#define BSP_PERIPH_DPLL_RAMPTIME        (0 << 24)   // disable DPLL4 ramptime
#define BSP_PERIPH_DPLL_FREQSEL         (7 << 20)   // freqsel = 1.75-2.1 mhz
#define BSP_EN_PERIPH_DPLL_DRIFTGUARD   (1 << 19)   // enable DPLL4 driftguard
#define BSP_EN_PERIPH_DPLL              (7 << 16)   // lock DPLL4
#define BSP_PWRDN_EMU_CORE              (0 << 12)   // enable DPLL3_M3X2
#define BSP_EN_CORE_DPLL_LPMODE         (0 << 10)   // disable DPLL3 LP mode
#define BSP_CORE_DPLL_RAMPTIME          (0 << 8)    // disable ramp time
#define BSP_EN_CORE_DPLL_DRIFTGUARD     (1 << 3)    // enable DPLL3 driftguard
#define BSP_EN_CORE_DPLL                (7 << 0)    // lock DPLL3

#define BSP_CORE_DPLL_FREQSEL           (7 << 4)    // freqsel=1.75 MHz to 2.1 MHz

#define BSP_CM_CLKEN_PLL                (BSP_PWRDN_EMU_PERIPH |         \
                                         BSP_PWRDN_CAM |                \
                                         BSP_PWRDN_DSS1 |               \
                                         BSP_PWRDN_TV |                 \
                                         BSP_PWRDN_96M |                \
                                         BSP_EN_PERIPH_DPLL_LPMODE |    \
                                         BSP_PERIPH_DPLL_RAMPTIME |     \
                                         BSP_PERIPH_DPLL_FREQSEL |      \
                                         BSP_EN_PERIPH_DPLL_DRIFTGUARD |\
                                         BSP_EN_PERIPH_DPLL |           \
                                         BSP_PWRDN_EMU_CORE |           \
                                         BSP_EN_CORE_DPLL_LPMODE |      \
                                         BSP_CORE_DPLL_RAMPTIME |       \
                                         BSP_CORE_DPLL_FREQSEL |        \
                                         BSP_EN_CORE_DPLL_DRIFTGUARD |  \
                                         BSP_EN_CORE_DPLL)


//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKEN2_PLL
//
//  Determines the DPLL5 internal frequency based on DPLL reference
//  clock and divider (n).  Used to update CM_CLKEN2_PLL
//
//  Allowed values:
//
//      0x3: 0.75 MHz to 1.0 MHz
//      0x4: 1.0 MHz to 1.25 MHz
//      0x5: 1.25 MHz to 1.5 MHz
//      0x6: 1.5 MHz to 1.75 MHz
//      0x7: 1.75 MHz to 2.1 MHz
//      0xB: 7.5 MHz to 10 MHz
//      0xC: 10 MHz to 12.5 MHz
//      0xD: 12.5 MHz to 15 MHz
//      0xE: 15 MHz to 17.5 MHz
//      0xF: 17.5 MHz to 21 MHz
//
#define BSP_EN_PERIPH2_DPLL_LPMODE      (0 << 10)   // disable DPLL5 LP mode
#define BSP_PERIPH2_DPLL_RAMPTIME       (0 << 8)    // disable ramp time
#define BSP_PERIPH2_DPLL_FREQSEL        (7 << 4)    // freqsel=1.75 - 2.1 MHz
#define BSP_EN_PERIPH2_DPLL_DRIFTGUARD  (1 << 3)    // enable DPLL4 driftguard
#define BSP_EN_PERIPH2_DPLL             (7 << 0)    // lock DPLL5


#define BSP_CM_CLKEN2_PLL               (BSP_EN_PERIPH2_DPLL_LPMODE |      \
                                         BSP_PERIPH2_DPLL_RAMPTIME |       \
                                         BSP_PERIPH2_DPLL_FREQSEL |        \
                                         BSP_EN_PERIPH2_DPLL_DRIFTGUARD |  \
                                         BSP_EN_PERIPH2_DPLL)


//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL1_PLL
//
//  Determines master clock frequency.  Used to update CM_CLKSEL1_PLL
//
//  Allowed values:
//
#define BSP_CORE_DPLL_CLKOUT_DIV       (1 << 27)    // DPLL3 output is CORE_CLK/1
#define BSP_SOURCE_54M                 (0 << 5)     // 54Mhz clk src = DPLL4
#define BSP_SOURCE_48M                 (0 << 3)     // 48Mhz clk src = DPLL4

// Set Core DPLL based on attached DDR memory specification
// NOTE - Be sure to set BSP_CORE_DPLL_FREQSEL correctly based on the divider value
    #define CORE_DPLL_MULT_260              ((DIV_SYS_CLK*130) << 16)
    #define CORE_DPLL_DIV_260               (12 << 8)
    #define CORE_DPLL_MULT_330              ((DIV_SYS_CLK*166) << 16)
    #define CORE_DPLL_DIV_330               (12 << 8)
#define BSP_CORE_DPLL_MULT              CORE_DPLL_MULT_330  // Multiplier
#define BSP_CORE_DPLL_DIV               CORE_DPLL_DIV_330    // Divider
#define BSP_SOURCE_96M                  (0 << 6)     // 96Mhz clk src=CM_96M_FCLK

#define BSP_CM_CLKSEL1_PLL             (BSP_CORE_DPLL_CLKOUT_DIV | \
                                        BSP_CORE_DPLL_MULT | \
                                        BSP_CORE_DPLL_DIV | \
                                        BSP_SOURCE_54M | \
                                        BSP_SOURCE_48M | \
                                        BSP_SOURCE_96M)


//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL2_PLL
//
//  Determines PER clock settings.  Used to update CM_CLKSEL2_PLL (DPLL4)
//
//  Allowed values:
//
#define BSP_PERIPH_DPLL_MULT           ((DIV_SYS_CLK*216) << 8)    // freq = 864MHz
#define BSP_PERIPH_DPLL_DIV            (12 << 0)     //

#define BSP_CM_CLKSEL2_PLL             (BSP_PERIPH_DPLL_MULT | \
                                        BSP_PERIPH_DPLL_DIV)


//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL3_PLL
//
//  Determines divisor from DPLL4 for 96M.  Used to update CM_CLKSEL3_PLL
//
//  Allowed values:
//
// Note that 96MHz clock comes from the M2X2 port
#define BSP_DIV_96M                    (9 << 0)     // DPLL4 864MHz/96MHz = 9

#define BSP_CM_CLKSEL3_PLL             (BSP_DIV_96M)



//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL4_PLL
//
//  Determines master clock frequency.  Used to update CM_CLKSEL4_PLL
//
//  Allowed values:
//
#define BSP_PERIPH2_DPLL_MULT          ((DIV_SYS_CLK*60) << 8)    // Multiplier
#define BSP_PERIPH2_DPLL_DIV           (12  << 0)    // Divider

#define BSP_CM_CLKSEL4_PLL             (BSP_PERIPH2_DPLL_MULT | \
                                        BSP_PERIPH2_DPLL_DIV)


//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL5_PLL
//
//  Determines divisor from DPLL5 for 120M.  Used to update CM_CLKSEL5_PLL
//
//  Allowed values:
//
#define BSP_DIV_120M                   (1 << 0)     // DPLL5/1 = 120Mhz

#define BSP_CM_CLKSEL5_PLL             (BSP_DIV_120M)


//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL_CAM
//
//  Determines CAM clock settings.  Used to update CM_CLKSEL_CAM
//
//  Allowed values:
//
#define BSP_CAM_CLKSEL_CAM             (4 << 0)     // DPLL4/4=216mhz

#define BSP_CM_CLKSEL_CAM              (BSP_CAM_CLKSEL_CAM)

//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL1_EMU
//
//  Determines EMU clock settings.  Used to update CM_CLKSEL1_EMU
//
//  Allowed values:
//
#define BSP_EMU_DIV_DPLL4              (3 << 24)    // DPLL4/3=288mhz
#define BSP_EMU_DIV_DPLL3              (2 << 16)    // DPLL3/2
#define BSP_EMU_CLKSEL_TRACECLK        (1 << 11)    // TRACECLK/1       (default)
#define BSP_EMU_CLKSEL_PCLK            (2 << 8)     // PCLK.FCLK/2      (default)
#define BSP_EMU_PCLKX2                 (1 << 6)     // PCLKx2.FCLK/1    (default)
#define BSP_EMU_CLKSEL_ATCLK           (1 << 4)     // ATCLK.FCLK/1     (default)
#define BSP_EMU_TRACE_MUX_CTRL         (0 << 2)     // TRACE src=sysclk (default)
#define BSP_EMU_MUX_CTRL               (0 << 0)     // ATCLK.PCLK=sysclk(default)

#define BSP_CM_CLKSEL1_EMU             (BSP_EMU_DIV_DPLL4 | \
                                        BSP_EMU_DIV_DPLL3 | \
                                        BSP_EMU_CLKSEL_TRACECLK | \
                                        BSP_EMU_CLKSEL_PCLK | \
                                        BSP_EMU_PCLKX2 | \
                                        BSP_EMU_CLKSEL_ATCLK | \
                                        BSP_EMU_TRACE_MUX_CTRL | \
                                        BSP_EMU_MUX_CTRL)


//------------------------------------------------------------------------------
//
//  Define: BSP_MPU_DPLL_FREQSEL
//
//  Determines the DPLL1 internal frequency based on DPLL reference clock
//  and divider (n).  Used to update CM_CLKEN_PLL_MPU
//
//  Allowed values:
//
//      0x3: 0.75 MHz to 1.0 MHz
//      0x4: 1.0 MHz to 1.25 MHz
//      0x5: 1.25 MHz to 1.5 MHz
//      0x6: 1.5 MHz to 1.75 MHz
//      0x7: 1.75 MHz to 2.1 MHz
//      0xB: 7.5 MHz to 10 MHz
//      0xC: 10 MHz to 12.5 MHz
//      0xD: 12.5 MHz to 15 MHz
//      0xE: 15 MHz to 17.5 MHz
//      0xF: 17.5 MHz to 21 MHz
//
#define BSP_EN_MPU_DPLL_LPMODE         (0 << 10)   // disable DPLL1 LP mode
#define BSP_MPU_DPLL_RAMPTIME          (2 << 8)    // ramp time = 20us
#define BSP_EN_MPU_DPLL_DRIFTGUARD     (1 << 3)    // enable DPLL1 driftguard
#define BSP_EN_MPU_DPLL                (7 << 0)    // lock DPLL1
#define MPU_DPLL_FREQSEL_500           (7 << 4)
#define BSP_MPU_DPLL_FREQSEL       MPU_DPLL_FREQSEL_500

#define BSP_CM_CLKEN_PLL_MPU           (BSP_EN_MPU_DPLL_LPMODE |      \
                                        BSP_MPU_DPLL_RAMPTIME |       \
                                        BSP_MPU_DPLL_FREQSEL |        \
                                        BSP_EN_MPU_DPLL_DRIFTGUARD |  \
                                        BSP_EN_MPU_DPLL)


//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL1_PLL_MPU
//
//  Determines master clock frequency.  Used to update CM_CLKSEL1_PLL_MPU
//
//  Allowed values:
//
#define BSP_MPU_CLK_SRC                (2 << 19)    // DPLL1 bypass = CORE.CLK/2

// BSP_SPEED_CPUMHZ is set by .BAT file, default value is 500
#define BSP_MPU_DPLL_MULT           (((BSP_SPEED_CPUMHZ*DIV_SYS_CLK) / 2) << 8)   // Multiplier
#define BSP_MPU_DPLL_DIV            (12 << 0)                       // Divider

#define BSP_CM_CLKSEL1_PLL_MPU         (BSP_MPU_CLK_SRC |   \
                                        BSP_MPU_DPLL_MULT | \
                                        BSP_MPU_DPLL_DIV)

//------------------------------------------------------------------------------
//
//  Define: BSP_CM_CLKSEL2_PLL_MPU
//
//  Determines the output clock divider for DPLL1.  Used to update
//  CM_CLKSEL2_PLL_MPU
//
//  Allowed values:
//
//      0x1: DPLL1 output clock is divided by 1
//      0x2: DPLL1 output clock is divided by 2
//      0x3: DPLL1 output clock is divided by 3
//      0x4: DPLL1 output clock is divided by 4
//      0x5: DPLL1 output clock is divided by 5
//      0x6: DPLL1 output clock is divided by 6
//      0x7: DPLL1 output clock is divided by 7
//      0x8: DPLL1 output clock is divided by 8
//      0x9: DPLL1 output clock is divided by 9
//      0xA: DPLL1 output clock is divided by 10
//      0xB: DPLL1 output clock is divided by 11
//      0xC: DPLL1 output clock is divided by 12
//      0xD: DPLL1 output clock is divided by 13
//      0xE: DPLL1 output clock is divided by 14
//      0xF: DPLL1 output clock is divided by 15
//      0x10: DPLL1 output clock is divided by 16
//
#define BSP_MPU_DPLL_CLKOUT_DIV         (1 << 0)    // CLKOUTX2 = DPLL1 freq

#define BSP_CM_CLKSEL2_PLL_MPU          (BSP_MPU_DPLL_CLKOUT_DIV)

//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_MCFG_0
//
//  Determines memory configuration registers.  Used to update SDRC_MCFG_0
//
//  Allowed values:
//
#define BSP_RASWIDTH_0                 (2 << 24)
#define BSP_CASWIDTH_0                 (5 << 20)
#define BSP_ADDRMUXLEGACY_0            (1 << 19)    // flexible address mux
#define BSP_RAMSIZE_0                  (64 << 8)    // 128mb SDRAM
#define BSP_BANKALLOCATION_0           (2 << 6)     // bank-row-column
#define BSP_B32NOT16_0                 (1 << 4)     // Ext. SDRAM is x32 bit.
#define BSP_DEEPPD_0                   (1 << 3)     // supports deep-power down
#define BSP_DDRTYPE_0                  (0 << 2)     // SDRAM is MobileDDR
#define BSP_RAMTYPE_0                  (1 << 0)     // SDRAM is DDR

#define BSP_SDRC_MCFG_0                (BSP_RASWIDTH_0 | \
                                        BSP_CASWIDTH_0 | \
                                        BSP_ADDRMUXLEGACY_0 | \
                                        BSP_RAMSIZE_0 | \
                                        BSP_BANKALLOCATION_0 | \
                                        BSP_B32NOT16_0 | \
                                        BSP_DEEPPD_0 | \
                                        BSP_DDRTYPE_0 | \
                                        BSP_RAMTYPE_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_MCFG_1
//
//  Determines memory configuration registers.  Used to update SDRC_MCFG_1
//
//  Allowed values:
//
#define BSP_RASWIDTH_1                 (2 << 24)
#define BSP_CASWIDTH_1                 (5 << 20)
#define BSP_ADDRMUXLEGACY_1            (1 << 19)    // flexible address mux
#if BSP_SDRAM_BANK1_ENABLE == 1
    #define BSP_RAMSIZE_1              (64 << 8)    // 128mb SDRAM
#else
    #define BSP_RAMSIZE_1              (0 << 8)     // 0mb SDRAM
#endif
#define BSP_BANKALLOCATION_1           (2 << 6)     // bank-row-column
#define BSP_B32NOT16_1                 (1 << 4)     // Ext. SDRAM is x32 bit.
#define BSP_DEEPPD_1                   (1 << 3)     // supports deep-power down
#define BSP_DDRTYPE_1                  (0 << 2)     // SDRAM is MobileDDR
#define BSP_RAMTYPE_1                  (1 << 0)     // SDRAM is DDR

#define BSP_SDRC_MCFG_1                (BSP_RASWIDTH_1 | \
                                        BSP_CASWIDTH_1 | \
                                        BSP_ADDRMUXLEGACY_1 | \
                                        BSP_RAMSIZE_1 | \
                                        BSP_BANKALLOCATION_1 | \
                                        BSP_B32NOT16_1 | \
                                        BSP_DEEPPD_1 | \
                                        BSP_DDRTYPE_1 | \
                                        BSP_RAMTYPE_1)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_SHARING
//
//  Determines the SDRC module attached memory size and position on the SDRC
//  module I/Os..  Used to update SDRC_SHARING
//
//  Allowed values:
//
#define BSP_CS1MUXCFG                  (0 << 12)    // 32-bit SDRAM on [31:0]
#define BSP_CS0MUXCFG                  (0 << 9)     // 32-bit SDRAM on [31:0]
#define BSP_SDRCTRISTATE               (1 << 8)     // Normal mode

#define BSP_SDRC_SHARING               (BSP_CS1MUXCFG | \
                                        BSP_CS0MUXCFG | \
                                        BSP_SDRCTRISTATE)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_ACTIM_CTRLA_0
//
//  Determines ac timing control register A.  Used to update SDRC_ACTIM_CTRLA_0
//
//  Allowed values:
//
// NOTE - Settings below are based on CORE DPLL = 332MHz, L3 = CORE/2 (166MHz)

/* Samsung version [K5W1G1GACM-DL60](166MHz optimized) ~ 6.0ns
/* Micron version [MT29C2G24MAKLAJG-6](166MHz optimized) ~ 6.0ns
 *
 * ACTIM_CTRLA -
 *  TWR = 12/6  = 2 (samsung)
 *  TWR = 15/6  = 3 (micron)
 *  TDAL = Twr/Tck + Trp/tck = 12/6 + 18/6 = 2 + 3 = 5  (samsung)
 *  TDAL = Twr/Tck + Trp/tck = 15/6 + 18/6 = 3 + 3 = 6  (micron)
 *  TRRD = 12/6 = 2
 *  TRCD = 18/6 = 3
 *  TRP = 18/6  = 3
 *  TRAS = 42/6 = 7
 *  TRC = 60/6  = 10
 *  TRFC = 72/6 = 12 (samsung)
 *  TRFC = 125/6 = 21 (micron)
 *
 * ACTIM_CTRLB -
 *  TCKE            = 2 (samsung)
 *  TCKE            = 1 (micron)
 *  XSR = 120/6   = 20  (samsung)
 *  XSR = 138/6   = 23  (micron)
 */

// Choose more conservative of memory timings when they differ between vendors
#define BSP_TRFC_0                     (21 << 27)   // Autorefresh to active
#define BSP_TRC_0                      (10 << 22)   // Row cycle time
#define BSP_TRAS_0                     (7 << 18)    // Row active time
#define BSP_TRP_0                      (3 << 15)    // Row precharge time
#define BSP_TRCD_0                     (3 << 12)    // Row to column delay time
#define BSP_TRRD_0                     (2 << 9)     // Active to active cmd per.
#define BSP_TWR_0                      (3 << 6)     // Data-in to precharge cmd
#define BSP_TDAL_0                     (6 << 0)     // Data-in to active command

#define BSP_SDRC_ACTIM_CTRLA_0         (BSP_TRFC_0 | \
                                        BSP_TRC_0 | \
                                        BSP_TRAS_0 | \
                                        BSP_TRP_0 | \
                                        BSP_TRCD_0 | \
                                        BSP_TRRD_0 | \
                                        BSP_TWR_0 | \
                                        BSP_TDAL_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_ACTIM_CTRLA_1
//
//  Determines ac timing control register A.  Used to update SDRC_ACTIM_CTRLA_1
//
//  Allowed values:
//
#define BSP_SDRC_ACTIM_CTRLA_1          BSP_SDRC_ACTIM_CTRLA_0


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_ACTIM_CTRLB_0
//
//  Determines ac timing control register B.  Used to update SDRC_ACTIM_CTRLB_0
//
//  Allowed values:
//
#define BSP_TWTR_0                     (0x1 << 16)  // 1-cycle write to read delay
#define BSP_TCKE_0                     (2 << 12)    // CKE minimum pulse width
#define BSP_TXP_0                      (0x5 << 8)   // 5 minimum cycles
#define BSP_TXSR_0                     (20 << 0)    // Self Refresh Exit to Active period

#define BSP_SDRC_ACTIM_CTRLB_0         (BSP_TCKE_0 | \
                                        BSP_TXSR_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_ACTIM_CTRLB_1
//
//  Determines ac timing control register A.  Used to update SDRC_ACTIM_CTRLB_1
//
//  Allowed values:
//
#define BSP_SDRC_ACTIM_CTRLB_1          BSP_SDRC_ACTIM_CTRLB_0


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_RFR_CTRL_0
//
//  SDRAM memory autorefresh control.  Used to update SDRC_RFR_CTRL_0
//
//  Allowed values:
//
#define BSP_ARCV                       (0x4E2)
#define BSP_ARCV_0                     (BSP_ARCV << 8)  // Autorefresh counter val
#define BSP_ARE_0                      (1 << 0)         // Autorefresh on counter x1

#define BSP_SDRC_RFR_CTRL_0            (BSP_ARCV_0 | \
                                        BSP_ARE_0)

//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_RFR_CTRL_1
//
//  SDRAM memory autorefresh control.  Used to update SDRC_RFR_CTRL_1
//
//  Allowed values:
//
#define BSP_SDRC_RFR_CTRL_1             BSP_SDRC_RFR_CTRL_0


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_MR_0
//
//  Corresponds to the JEDEC SDRAM MR register.  Used to update SDRC_MR_0
//
//  Allowed values:
//
#define BSP_CASL_0                     (3 << 4)    // CAS latency = 3
#define BSP_SIL_0                      (0 << 3)    // Serial mode
#define BSP_BL_0                       (2 << 0)    // Burst Length = 4(DDR only)

#define BSP_SDRC_MR_0                  (BSP_CASL_0 | \
                                        BSP_SIL_0 | \
                                        BSP_BL_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_MR_0
//
//  Corresponds to the JEDEC SDRAM MR register.  Used to update SDRC_MR_1
//
//  Allowed values:
//
#define BSP_SDRC_MR_1                  (BSP_SDRC_MR_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_EMR2_0
//
//  Corresponds to the low-power EMR register, as defined in the mobile DDR
//  JEDEC standard.  Used to update SDRC_EMR2_0
//
//  Allowed values:
//
#define BSP_DS_0                       (0 << 5)    // Strong-strength driver
#define BSP_TCSR_0                     (0 << 3)    // 70 deg max temp
#define BSP_PASR_0                     (0 << 0)    // All banks

#define BSP_SDRC_EMR2_0                (BSP_DS_0 | \
                                        BSP_TCSR_0 | \
                                        BSP_PASR_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_EMR2_1
//
//  Corresponds to the low-power EMR register, as defined in the mobile DDR
//  JEDEC standard.  Used to update SDRC_EMR2_1
//
//  Allowed values:
//
#define BSP_SDRC_EMR2_1                (BSP_SDRC_EMR2_0)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_DLLA_CTRL
//
//  Used to fine-tune DDR timings.  Used to update SDRC_DLLA_CTRL
//
//  Allowed values:
//
#define BSP_FIXEDELAY                  (38 << 24)
#define BSP_MODEFIXEDDELAYINITLAT      (0 << 16)
#define BSP_DLLMODEONIDLEREQ           (0 << 5)
#define BSP_ENADLL                     (1 << 3)     // enable DLLs
#define BSP_LOCKDLL                    (0 << 2)     // run in unlock mode
#define BSP_DLLPHASE                   (1 << 1)     // 72 deg phase
#define BSP_SDRC_DLLA_CTRL             (BSP_FIXEDELAY | \
                                        BSP_MODEFIXEDDELAYINITLAT | \
                                        BSP_DLLMODEONIDLEREQ | \
                                        BSP_ENADLL | \
                                        BSP_LOCKDLL | \
                                        BSP_DLLPHASE)

//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_POWER_REG
//
//  Set SDRAM power management mode.  Used to update SDRC_POWER_REG
//
//  Allowed values:
//
#define BSP_WAKEUPPROC                 (0 << 26)    // don't stall 500 cycles on 1st access
#define BSP_AUTOCOUNT                  (4370 << 8)  // SDRAM idle count down
//#define BSP_SRFRONRESET                (0 << 7)     // disable idle on reset
#define BSP_SRFRONRESET                (1 << 7)     // enable self refresh on warm reset
#define BSP_SRFRONIDLEREQ              (1 << 6)     // hw idle on request
#define BSP_CLKCTRL                    (2 << 4)     // self-refresh on auto_cnt
#define BSP_EXTCLKDIS                  (1 << 3)     // disable ext clock
#define BSP_PWDENA                     (1 << 2)     // active power-down mode
#define BSP_PAGEPOLICY                 (1 << 0)     // must be 1
#define BSP_SDRC_POWER_REG             (BSP_WAKEUPPROC | \
                                        BSP_AUTOCOUNT | \
                                        BSP_SRFRONRESET | \
                                        BSP_SRFRONIDLEREQ | \
                                        BSP_CLKCTRL | \
                                        BSP_EXTCLKDIS | \
                                        BSP_PWDENA | \
                                        BSP_PAGEPOLICY)


//------------------------------------------------------------------------------
//
//  Define: BSP_SDRC_DLLB_CTRL
//
//  Used to fine-tune DDR timings.  Used to update SDRC_DLLB_CTRL
//
//  Allowed values:
//
#define BSP_SDRC_DLLB_CTRL             (BSP_SDRC_DLLA_CTRL)


//------------------------------------------------------------------------------
//
//  Define:  BSP_GPMC_xxx
//
//  These constants are used to initialize general purpose memory configuration
//  registers
//
// NOTE - Settings below are based on CORE DPLL = 332MHz, L3 = CORE/2 (166MHz)

//  NAND settings, not optimized
#define BSP_GPMC_NAND_CONFIG1       0x00001800      // 16 bit NAND interface
#define BSP_GPMC_NAND_CONFIG2       0x00060600      // 0x00141400
#define BSP_GPMC_NAND_CONFIG3       0x00060401      // 0x00141400
#define BSP_GPMC_NAND_CONFIG4       0x05010801      // 0x0F010F01
#define BSP_GPMC_NAND_CONFIG5       0x00080909      // 0x010C1414
#define BSP_GPMC_NAND_CONFIG6       0x050001C0      // 0x00000A80
#define BSP_GPMC_NAND_CONFIG7       ((BSP_NAND_REGS_PA >> 24) | BSP_NAND_MASKADDRESS | GPMC_CSVALID)

//  NOR settings
#define BSP_GPMC_NOR_CONFIG1        0x00011210      // 16 bit NOR interface
#define BSP_GPMC_NOR_CONFIG2        0x00101001
#define BSP_GPMC_NOR_CONFIG3        0x00020201
#define BSP_GPMC_NOR_CONFIG4        0x0F031003
#define BSP_GPMC_NOR_CONFIG5        0x000F1111
#define BSP_GPMC_NOR_CONFIG6        0x0F030080
#define BSP_GPMC_NOR_CONFIG7        ((BSP_NOR_REGS_PA >> 24) | BSP_NOR_MASKADDRESS | GPMC_CSVALID)

//  LAN9311 settings
//  165ns minimum cycle time for back to back accesses
//      32ns min CS, OE, WE assertion
//      13ns min deassertion
//  supports paged bursts, disabled for now
#define BSP_GPMC_LAN_CONFIG1       0x00001000       // no wait, 16 bit, non multiplexed
#define BSP_GPMC_LAN_CONFIG2       0x00080800       // CS OffTime 48ns
#define BSP_GPMC_LAN_CONFIG3       0x00020201       // we don't use ADV
#define BSP_GPMC_LAN_CONFIG4       0x08000800       // Deassert #WE, #OE at 48ns
#define BSP_GPMC_LAN_CONFIG5       0x01060D0D       // Cycle time 78ns, access time 36ns
#define BSP_GPMC_LAN_CONFIG6       0x00000F80       // Delay 90ns between successive accesses to meet minimum cycle time
#define BSP_GPMC_LAN_CONFIG7       ((BSP_LAN9311_REGS_PA >> 24) | BSP_LAN9311_MASKADDRESS | GPMC_CSVALID)


//------------------------------------------------------------------------------
//
//  Define:  BSP_UART_DSIUDLL & BSP_UART_DSIUDLH
//
//  This constants are used to initialize serial debugger output UART.
//  Serial debugger uses 115200-8-N-1
//
#define BSP_UART_LCR                   (0x03)
#define BSP_UART_DSIUDLL               (26)
#define BSP_UART_DSIUDLH               (0)


//------------------------------------------------------------------------------
//
//  Define:  CM_AUTOIDLE1_xxxx_INIT
//
//  initial autoidle settings for a given power domain
//
// Note: Some bits reserved for non-GP devices are being set
#define CM_AUTOIDLE1_CORE_INIT          (0x7FFFFED1)
// Note: This register is reserved for non-GP devices
#define CM_AUTOIDLE2_CORE_INIT          (0x0000001F)
#define CM_AUTOIDLE3_CORE_INIT          (0x00000004)
#define CM_AUTOIDLE_WKUP_INIT           (0x0000003F)
#define CM_AUTOIDLE_PER_INIT            (0x0003FFFF)
#define CM_AUTOIDLE_CAM_INIT            (0x00000001)
#define CM_AUTOIDLE_DSS_INIT            (0x00000001)
#define CM_AUTOIDLE_USBHOST_INIT        (0x00000001)

//------------------------------------------------------------------------------
//
//  Define:  CM_SLEEPDEP_xxxx_INIT
//
//  initial sleep dependency settings for a given power domain
//
#define CM_SLEEPDEP_SGX_INIT            (0x00000000)
#define CM_SLEEPDEP_DSS_INIT            (0x00000000)
#define CM_SLEEPDEP_CAM_INIT            (0x00000000)
#define CM_SLEEPDEP_PER_INIT            (0x00000000)
#define CM_SLEEPDEP_USBHOST_INIT        (0x00000000)

//------------------------------------------------------------------------------
//
//  Define:  CM_WKDEP_xxxx_INIT
//
//  initial wake dependency settings for a given power domain
//
#define CM_WKDEP_IVA2_INIT              (0x00000000)
#define CM_WKDEP_MPU_INIT               (0x00000000)
#define CM_WKDEP_NEON_INIT              (0x00000000)
#define CM_WKDEP_SGX_INIT               (0x00000000)
#define CM_WKDEP_DSS_INIT               (0x00000000)
#define CM_WKDEP_CAM_INIT               (0x00000000)
#define CM_WKDEP_PER_INIT               (0x00000000)
#define CM_WKDEP_USBHOST_INIT           (0x00000000)

//------------------------------------------------------------------------------
//
//  Define:  CM_WKEN_xxxx_INIT
//
//  initial wake enable settings for a given power domain
//
#define CM_WKEN_WKUP_INIT               (0x00000100)
#define CM_WKEN1_CORE_INIT              (0x00000000)
#define CM_WKEN_DSS_INIT                (0x00000000)
#define CM_WKEN_PER_INIT                (0x00000000)
#define CM_WKEN_USBHOST_INIT            (0x00000000)


//------------------------------------------------------------------------------
//
//  Define:  BSP_I2Cx_OA_INIT
//
//  own address settings for i2c device
//
#define BSP_I2C1_OA_INIT                (0x0E)
#define BSP_I2C2_OA_INIT                (0x0E)
#define BSP_I2C3_OA_INIT                (0x0E)


//------------------------------------------------------------------------------
//
//  Define:  BSP_I2C_TIMEOUT_INIT
//
//  default timeout in tick count units (milli-seconds)
//
#define BSP_I2C_TIMEOUT_INIT            (500)


//------------------------------------------------------------------------------
//
//  Define:  BSP_I2Cx_BAUDRATE_INIT
//
//  default baud rate
//
#define BSP_I2C1_BAUDRATE_INIT          (1)
#define BSP_I2C2_BAUDRATE_INIT          (1)
#define BSP_I2C3_BAUDRATE_INIT          (1)


//------------------------------------------------------------------------------
//
//  Define:  BSP_I2Cx_MAXRETRY_INIT
//
//  maximum number of attempts before failure
//
#define BSP_I2C1_MAXRETRY_INIT          (5)
#define BSP_I2C2_MAXRETRY_INIT          (5)
#define BSP_I2C3_MAXRETRY_INIT          (5)


//------------------------------------------------------------------------------
//
//  Define:  BSP_I2Cx_RX_THRESHOLD_INIT
//
//  recieve fifo threshold
//
#define BSP_I2C1_RX_THRESHOLD_INIT      (5)
#define BSP_I2C2_RX_THRESHOLD_INIT      (5)
#define BSP_I2C3_RX_THRESHOLD_INIT      (60)


//------------------------------------------------------------------------------
//
//  Define:  BSP_I2Cx_TX_THRESHOLD_INIT
//
//  transmit fifo threshold
//
#define BSP_I2C1_TX_THRESHOLD_INIT      (5)
#define BSP_I2C2_TX_THRESHOLD_INIT      (5)
#define BSP_I2C3_TX_THRESHOLD_INIT      (60)


//------------------------------------------------------------------------------
//
//  Define:  BSP_VC_SMPS_SA_INIT
//
//  slave address for VP1 & VP2
//
#define VC_SMPS_SA1                     (0x12 << 0)
#define VC_SMPS_SA2                     (0x12 << 16)

#define BSP_VC_SMPS_SA_INIT             (VC_SMPS_SA1 | \
                                         VC_SMPS_SA2)

//------------------------------------------------------------------------------
//
//  Define:  BSP_VP1_CMDADDRESS_INIT & BSP_VP2_CMDADDRESS_INIT
//
//  cmd address for VP1 & VP2
//
#define VC_SMPS_CMD_RA1                 (0 << 0)
#define VC_SMPS_CMD_RA2                 (0 << 16)

#define BSP_VC_SMPS_CMD_RA_INIT         (VC_SMPS_CMD_RA1 | \
                                         VC_SMPS_CMD_RA2)

//------------------------------------------------------------------------------
//
//  Define:  BSP_VP1_VOLTADDRESS_INIT & BSP_VP2_VOLTADDRESS_INIT
//
//  volt address for VP1 & VP2
//
#define VC_SMPS_VOL_RA1                 (0 << 0)
#define VC_SMPS_VOL_RA2                 (1 << 16)

#define BSP_VC_SMPS_VOL_RA_INIT         (VC_SMPS_VOL_RA1 | \
                                         VC_SMPS_VOL_RA2)

//------------------------------------------------------------------------------
//
//  Define:  BSP_VC_CH_CONF_INIT
//
//  flag to determine which subaddress to use to control voltage for VP1 & VP2
//
#define VC_CH_CONF_SA0                  (0 << 0)
#define VC_CH_CONF_RAV0                 (0 << 1)
#define VC_CH_CONF_RAC0                 (0 << 2)
#define VC_CH_CONF_RACEN0               (0 << 3)
#define VC_CH_CONF_CMD0                 (0 << 4)
#define VC_CH_CONF_SA1                  (1 << 16)
#define VC_CH_CONF_RAV1                 (1 << 17)
#define VC_CH_CONF_RAC1                 (1 << 18)
#define VC_CH_CONF_RACEN1               (0 << 19)
#define VC_CH_CONF_CMD1                 (1 << 20)

#define BSP_VC_CH_CONF_INIT             (VC_CH_CONF_CMD1 |   \
                                         VC_CH_CONF_RACEN1 | \
                                         VC_CH_CONF_RAC1 |   \
                                         VC_CH_CONF_RAV1 |   \
                                         VC_CH_CONF_SA1 |    \
                                         VC_CH_CONF_CMD0 |   \
                                         VC_CH_CONF_RACEN0 | \
                                         VC_CH_CONF_RAC0 |   \
                                         VC_CH_CONF_RAV0 |   \
                                         VC_CH_CONF_SA0)

//------------------------------------------------------------------------------
//
//  Define:  BSP_VP1_USECMDADDRESS_INIT & BSP_VP2_USECMDADDRESS_INIT
//
//  flag to determine which subaddress to use to control voltage for VP1 & VP2
//
#define VC_I2C_CFG_HSMASTER             (0 << 5)
#define VC_I2C_CFG_SREN                 (0 << 4)
#define VC_I2C_CFG_HSEN                 (0 << 3)
#define VC_I2C_CFG_MCODE                (0x5 << 0)

#define BSP_PRM_VC_I2C_CFG_INIT         (VC_I2C_CFG_HSMASTER |\
                                         VC_I2C_CFG_SREN |    \
                                         VC_I2C_CFG_HSEN |    \
                                         VC_I2C_CFG_MCODE)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VP1_CONFIG_INIT
//
//  flag to determine which subaddress to use to control voltage for VP1 & VP2
//
#define VP1_CONFIG_ERROROFFSET          (0 << 24)
#define VP1_CONFIG_ERRORGAIN            (0x20 << 16)
#define VP1_CONFIG_INITVOLTAGE          (VDD1_INIT_VOLTAGE_VALUE << 8) // should same as VC_CMD_0_VOLT_ON
#define VP1_CONFIG_TIMEOUTEN            (1 << 3)
#define VP1_CONFIG_INITVDD              (0 << 2)
#define VP1_CONFIG_FORCEUPDATE          (0 << 1)
#define VP1_CONFIG_VPENABLE             (0 << 0)

#define BSP_PRM_VP1_CONFIG_INIT         (VP1_CONFIG_ERROROFFSET |   \
                                         VP1_CONFIG_ERRORGAIN |     \
                                         VP1_CONFIG_INITVOLTAGE |   \
                                         VP1_CONFIG_TIMEOUTEN |     \
                                         VP1_CONFIG_INITVDD |       \
                                         VP1_CONFIG_FORCEUPDATE |   \
                                         VP1_CONFIG_VPENABLE)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VP2_CONFIG_INIT
//
//  flag to determine which subaddress to use to control voltage for VP2 & VP2
//
#define VP2_CONFIG_ERROROFFSET          (0 << 24)
#define VP2_CONFIG_ERRORGAIN            (0x20 << 16)
#define VP2_CONFIG_INITVOLTAGE          (0x2C << 8) // should same as VC_CMD_1_VOLT_ON
#define VP2_CONFIG_TIMEOUTEN            (1 << 3)
#define VP2_CONFIG_INITVDD              (0 << 2)
#define VP2_CONFIG_FORCEUPDATE          (0 << 1)
#define VP2_CONFIG_VPENABLE             (0 << 0)

#define BSP_PRM_VP2_CONFIG_INIT         (VP2_CONFIG_ERROROFFSET |   \
                                         VP2_CONFIG_ERRORGAIN |     \
                                         VP2_CONFIG_INITVOLTAGE |   \
                                         VP2_CONFIG_TIMEOUTEN |     \
                                         VP2_CONFIG_INITVDD |       \
                                         VP2_CONFIG_FORCEUPDATE |   \
                                         VP2_CONFIG_VPENABLE)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VC_CMD_VAL_0_INIT
//
//  initial voltage for ON, LP, RET, OFF states
//  using the following eq. {volt = 0.0125(val) + 0.6} setup voltage levels
//  Set Vdd1 voltages: ON=1.35v, ON_LP=1.0v, VDD1_RET=0.9v, VDD1_OFF=0.0v
//
#define VC_CMD_0_VOLT_ON                (VDD1_INIT_VOLTAGE_VALUE << 24)  // should be the same as VP1_CONFIG_INITVOLTAGE
#define VC_CMD_0_VOLT_LP                (0x20 << 16)
#define VC_CMD_0_VOLT_RET               (0x18 << 8)
#define VC_CMD_0_VOLT_OFF               (0x00 << 0)

#define BSP_PRM_VC_CMD_VAL_0_INIT       (VC_CMD_0_VOLT_ON |  \
                                         VC_CMD_0_VOLT_LP |  \
                                         VC_CMD_0_VOLT_RET | \
                                         VC_CMD_0_VOLT_OFF)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VC_CMD_VAL_1_INIT
//
//  initial voltage for ON, LP, RET, OFF states
//  using the following eq. {volt = 0.0125(val) + 0.6} setup voltage levels
//  Set Vdd2 voltages: ON=1.15v, ON_LP=1.0v, VDD2_RET=1.0v, VDD2_OFF=0.0v
//
#define VC_CMD_1_VOLT_ON                (0x2C << 24)  // should be the same as VP2_CONFIG_INITVOLTAGE
#define VC_CMD_1_VOLT_LP                (0x20 << 16)
#define VC_CMD_1_VOLT_RET               (0x20 << 8)
#define VC_CMD_1_VOLT_OFF               (0x00 << 0)

#define BSP_PRM_VC_CMD_VAL_1_INIT       (VC_CMD_1_VOLT_ON |  \
                                         VC_CMD_1_VOLT_LP |  \
                                         VC_CMD_1_VOLT_RET | \
                                         VC_CMD_1_VOLT_OFF)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VP1_VSTEPMIN_INIT
//
#define VP1_SMPSWAITTIMEMIN             (0x1F4 << 8)
#define VP1_VSTEPMIN                    (0x01 << 0)

#define BSP_PRM_VP1_VSTEPMIN_INIT       (VP1_VSTEPMIN |  \
                                         VP1_SMPSWAITTIMEMIN)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VP1_VSTEPMIN_INIT
//
#define VP1_SMPSWAITTIMEMAX             (0x1F4 << 8)
#define VP1_VSTEPMAX                    (0x10 << 0)

#define BSP_PRM_VP1_VSTEPMAX_INIT       (VP1_VSTEPMAX |  \
                                         VP1_SMPSWAITTIMEMAX)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VP2_VSTEPMIN_INIT
//
#define VP2_SMPSWAITTIMEMIN             (0x1F4 << 8)
#define VP2_VSTEPMIN                    (0x01 << 0)

#define BSP_PRM_VP2_VSTEPMIN_INIT       (VP2_VSTEPMIN |  \
                                         VP2_SMPSWAITTIMEMIN)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VP2_VSTEPMIN_INIT
//
#define VP2_SMPSWAITTIMEMAX             (0x1F4 << 8)
#define VP2_VSTEPMAX                    (0x10 << 0)

#define BSP_PRM_VP2_VSTEPMAX_INIT       (VP2_VSTEPMAX |  \
                                         VP2_SMPSWAITTIMEMAX)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VP1_VLIMITTO_INIT
//
#define VP1_VDDMAX                      (0x3F << 24)
#define VP1_VDDMMIN                     (0x00 << 16)
#define VP1_TIMEOUT                     (0xFFFF << 0)

#define BSP_PRM_VP1_VLIMITTO_INIT       (VP1_VDDMAX |   \
                                         VP1_VDDMMIN |  \
                                         VP1_TIMEOUT)

//------------------------------------------------------------------------------
//
//  Define:  BSP_PRM_VP2_VLIMITTO_INIT
//
#define VP2_VDDMAX                      (0x3C << 24)
#define VP2_VDDMMIN                     (0x00 << 16)
#define VP2_TIMEOUT                     (0xFFFF << 0)

#define BSP_PRM_VP2_VLIMITTO_INIT       (VP2_VDDMAX |   \
                                         VP2_VDDMMIN |  \
                                         VP2_TIMEOUT)

//------------------------------------------------------------------------------
//
//  Define:  TPS659XX_I2C_BUS_ID
//
//  i2c bus twl is on
//      OMAP_DEVICE_I2C1
//      OMAP_DEVICE_I2C2
//      OMAP_DEVICE_I2C3
//
#define TPS659XX_I2C_BUS_ID              (OMAP_DEVICE_I2C1)
#define TPS659XX_I2C_SLAVE_ADDRESS		 (0x0048)


//------------------------------------------------------------------------------
//
//  Define:  BSP_WATCHDOG_REFRESH_PERIOD_MILLISECONDS
//
//  Configure the watchdog period.  This value is in milliseconds. The refresh period is set to half this value
//
#define BSP_WATCHDOG_PERIOD_MILLISECONDS    (10000)

//------------------------------------------------------------------------------
//
//  Define:  BSP_WATCHDOG_THREAD_PRIORITY
//
//  Configure the kernel watchdog thread priority.
//
#define BSP_WATCHDOG_THREAD_PRIORITY                (100)

//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// nand pin connection information
#define BSP_GPMC_NAND_CS            (0)      // NAND is on CHIP SELECT 0
#define BSP_GPMC_IRQ_WAIT_EDGE      (GPMC_IRQENABLE_WAIT0_EDGEDETECT)


//-----------------------------------------------------------------------------
// GPIO id start for GPIO expander 1
#define GPIO_EXPANDER_1_PINID_START  (256)
//-----------------------------------------------------------------------------
// GPIO id start for GPIO expander 2
#define GPIO_EXPANDER_2_PINID_START  (288)
//-----------------------------------------------------------------------------
// GPIO id start for GPIO expander 3
#define GPIO_EXPANDER_3_PINID_START  (320)

//-----------------------------------------------------------------------------
// IRQ for GPIOs mapping
#define IRQ_GPIO_0  128

//-----------------------------------------------------------------------------
// BSP gpio table initialization
BOOL BSPInsertGpioDevice(UINT range,void* fnTbl,WCHAR* name);


//-----------------------------------------------------------------------------
// GPIOs
// Note : This must be in sync with the PAD configuration as well (in bsp_padcfg.h)
#define LAN9311_IRQ_GPIO            (157)
#define LCD_PANEL_PWR_GPIO          (176)
#define LCD_BACKLIGHT_PWR_GPIO      (182)
#define LCD_BACKLIGHT_PWM_GPIO      (181)
#define RTC_IRQ_GPIO                (55)
#define MMC1_CARDDET_GPIO           (127)
#define IOEXPANDER2_IRQ_GPIO        (160)   //on App board
#define IOEXPANDER3_IRQ_GPIO        (0)     //on experimenter board
#define TOUCH_PEN_IRQ_GPIO          (65)
//-----------------------------------------------------------------------------
// SYSINTR for the external LAN (required because NDIS doesn't support IRQ number greater than 255)
#define SYSINTR_LAN9311   SYSINTR_FIRMWARE

//-----------------------------------------------------------------------------
// Default Mac address (used when no settings found in flash and no MAC programmed into the ethernet device
#define DEFAULT_MAC_ADDRESS	{0x2020,0x3040,0x5060}


//-----------------------------------------------------------------------------
// S35390 RTC : I2C device address and baud rate setting
#define RTC_S35390_I2C_DEVICE       (OMAP_DEVICE_I2C1)
#define RTC_S35390_BAUD_INDEX       (0)
#define RTC_S35390_ADDRESS          (0x30)

#ifdef __cplusplus
}
#endif

#endif
