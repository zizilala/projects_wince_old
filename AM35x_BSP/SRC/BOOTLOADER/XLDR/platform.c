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
//
//  File:  startup.c
//
//  This file contains X-Loader startup code for OMAP35XX
//
#include "bsp.h"
#include "omap_32ksyncnt_regs.h"

#include "sdk_i2c.h"
#include "sdk_padcfg.h"
#include "oal_padcfg.h"
#include "oal_i2c.h"

#include "bsp_padcfg.h"

// Bits
#define BIT0    0x00000001
#define BIT1    0x00000002
#define BIT2    0x00000004
#define BIT3    0x00000008
#define BIT4    0x00000010
#define BIT5    0x00000020
#define BIT6    0x00000040
#define BIT7    0x00000080
#define BIT8    0x00000100
#define BIT9    0x00000200
#define BIT10   0x00000400
#define BIT11   0x00000800
#define BIT12   0x00001000
#define BIT13   0x00002000
#define BIT14   0x00004000
#define BIT15   0x00008000
#define BIT16   0x00010000
#define BIT17   0x00020000
#define BIT18   0x00040000
#define BIT19   0x00080000
#define BIT20   0x00100000
#define BIT21   0x00200000
#define BIT22   0x00400000
#define BIT23   0x00800000
#define BIT24   0x01000000
#define BIT25   0x02000000
#define BIT26   0x04000000
#define BIT27   0x08000000
#define BIT28   0x10000000
#define BIT29   0x20000000
#define BIT30   0x40000000
#define BIT31   0x80000000

//------------------------------------------------------------------------------
//  Defines
//

// Clock related definitions
#define LDELAY						12000000
#define SILICON_REV					1
#define GPT_EN						((0 << 2) | BIT1 | BIT0)
#define S12M						12000000
#define S13M						13000000
#define S19_2M						19200000
#define S26M						26000000
#define S38_4M						38400000
#define PLL_STOP					1 
#define PLL_LOW_POWER_BYPASS		5
#define PLL_FAST_RELOCK_BYPASS		6
#define PLL_LOCK					7
#define CORE_M3X2					2
#define CORE_L3_DIV					2
#define CORE_L4_DIV					2
#define PER_M6X2					3
#define PER_M5X2					4
#define PER_M4X2					9
#define PER_M3X2					16
#define WKUP_RSM					2
#define SYSCLKDIV_SHIFT				6
#define SYSCLKDIV_MASK				(0x03 << SYSCLKDIV_SHIFT)
#define EN_MPU_DPLL_SHIFT			0
#define EN_MPU_DPLL_MASK			(0x07 << EN_MPU_DPLL_SHIFT)
#define EN_CORE_DPLL_SHIFT			0
#define EN_CORE_DPLL_MASK			(0x07 << EN_CORE_DPLL_SHIFT)
#define DIV_DPLL3_SHIFT				16
#define DIV_DPLL3_MASK				(0x1F << DIV_DPLL3_SHIFT)
#define DIV_DPLL4_SHIFT				24
#define DIV_DPLL4_MASK				(0x1F << DIV_DPLL4_SHIFT)
#define CORE_DPLL_CLKOUT_DIV_SHIFT	27
#define CORE_DPLL_CLKOUT_DIV_MASK	(0x1F << CORE_DPLL_CLKOUT_DIV_SHIFT)
#define CLKSEL_RM_SHIFT				1
#define CLKSEL_RM_MASK				(0x03 << CLKSEL_RM_SHIFT)
#define CLKSEL_DSS1_SHIFT			0
#define CLKSEL_TV_SHIFT				8
#define CORE_DPLL_FREQSEL_SHIFT		4
#define CORE_DPLL_FREQSEL_MASK		(0x0F << CORE_DPLL_FREQSEL_SHIFT)
#define EN_PERIPH_DPLL_SHIFT		16
#define EN_PERIPH_DPLL_MASK			(0x07 << EN_PERIPH_DPLL_SHIFT)
#define PERIPH_DPLL_MULT_SHIFT		8
#define PERIPH_DPLL_MULT_MASK		(0x07FF << PERIPH_DPLL_MULT_SHIFT)
#define PERIPH_DPLL_DIV_SHIFT		0
#define PERIPH_DPLL_DIV_MASK		(0x7F << PERIPH_DPLL_DIV_SHIFT)
#define PERIPH_DPLL_FREQSEL_SHIFT	20
#define PERIPH_DPLL_FREQSEL_MASK	(0x0F << PERIPH_DPLL_FREQSEL_SHIFT)
#define MPU_DPLL_CLKOUT_DIV_SHIFT	0
#define MPU_DPLL_MULT_SHIFT			8
#define MPU_DPLL_MULT_MASK			(0x07FF << MPU_DPLL_MULT_SHIFT)
#define MPU_DPLL_DIV_SHIFT			0
#define MPU_DPLL_DIV_MASK			(0x7F << MPU_DPLL_DIV_SHIFT)
#define MPU_DPLL_FREQSEL_SHIFT		4
#define MPU_DPLL_FREQSEL_MASK		(0x0F << MPU_DPLL_FREQSEL_SHIFT)
#define CLKSEL_GPT_SHIFT			0
#define CLKSEL_GPT_MASK				(0xFF << CLKSEL_GPT_SHIFT)
#define CLKSEL_GPT1_SHIFT			0
#define CLKSEL_GPT1_MASK			(0x01 << CLKSEL_GPT1_SHIFT)
#define SYS_CLKIN_SEL_SHIFT			0
#define SYS_CLKIN_SEL_MASK			(0x07 << SYS_CLKIN_SEL_SHIFT)
#define CLKSEL_GPT2_SHIFT			0
#define CLKSEL_GPT2_MASK			(0x01 << CLKSEL_GPT2_SHIFT)
#define EN_GPT2_SHIFT				3
#define EN_GPT2_MASK				(0x01 << EN_GPT2_SHIFT)
#define EN_UART1_SHIFT				13
#define EN_UART1_MASK				(0x01 << EN_UART1_SHIFT)
#define EN_UART2_SHIFT				14
#define EN_UART2_MASK				(0x01 << EN_UART2_SHIFT)
#define EN_UART3_SHIFT				11
#define EN_UART3_MASK				(0x01 << EN_UART3_SHIFT)
#define EN_MMC1_SHIFT				24
#define EN_MMC1_MASK				(0x01 << EN_MMC1_SHIFT)
#define EN_MMC2_SHIFT				25
#define EN_MMC2_MASK				(0x01 << EN_MMC2_SHIFT)
#define EN_I2C1_SHIFT				15
#define EN_I2C1_MASK				(0x01 << EN_I2C1_SHIFT)
#define EN_I2C2_SHIFT				16
#define EN_I2C2_MASK				(0x01 << EN_I2C1_SHIFT)
#define EN_I2C3_SHIFT				17
#define EN_I2C3_MASK				(0x01 << EN_I2C1_SHIFT)
#define EN_GPIO1_SHIFT				3
#define EN_GPIO1_MASK				(0x01 << EN_GPIO1_SHIFT)

// EMIF4 register
#define	EMIF4_BASE				OMAP_SDRC_REGS_PA
#define	EMIF4_MOD_ID			(EMIF4_BASE + 0x00)
#define	EMIF4_SDRAM_STS			(EMIF4_BASE + 0x04)
#define	EMIF4_SDRAM_CFG			(EMIF4_BASE + 0x08)
#define	EMIF4_SDRAM_RFCR		(EMIF4_BASE + 0x10)
#define	EMIF4_SDRAM_RFCR_SHDW	(EMIF4_BASE + 0x14)
#define	EMIF4_SDRAM_TIM1		(EMIF4_BASE + 0x18)
#define	EMIF4_SDRAM_TIM1_SHDW	(EMIF4_BASE + 0x1C)
#define	EMIF4_SDRAM_TIM2		(EMIF4_BASE + 0x20)
#define	EMIF4_SDRAM_TIM2_SHDW	(EMIF4_BASE + 0x24)
#define	EMIF4_SDRAM_TIM3		(EMIF4_BASE + 0x28)
#define	EMIF4_SDRAM_TIM3_SHDW	(EMIF4_BASE + 0x2c)
#define	EMIF4_PWR_MGT_CTRL		(EMIF4_BASE + 0x38)
#define	EMIF4_PWR_MGT_CTRL_SHDW	(EMIF4_BASE + 0x3C)
#define	EMIF4_IODFT_TLGC		(EMIF4_BASE + 0x60)
#define	EMIF4_DDR_PHYCTL1		(EMIF4_BASE + 0xE4)
#define	EMIF4_DDR_PHYCTL1_SHDW	(EMIF4_BASE + 0xE8)
#define	EMIF4_DDR_PHYCTL2		(EMIF4_BASE + 0xEC)

// Definitions for EMIF4 configuration values
#define	EMIF4_TIM1_T_RP			0x3
#define	EMIF4_TIM1_T_RCD		0x3
#define	EMIF4_TIM1_T_WR			0x3
#define	EMIF4_TIM1_T_RAS		0x8
#define	EMIF4_TIM1_T_RC			0xA
#define	EMIF4_TIM1_T_RRD		0x2
#define	EMIF4_TIM1_T_WTR		0x2
#define	EMIF4_TIM2_T_XP			0x2
#define	EMIF4_TIM2_T_ODT		0x0
#define	EMIF4_TIM2_T_XSNR		0x1C
#define	EMIF4_TIM2_T_XSRD		0xC8
#define	EMIF4_TIM2_T_RTP		0x1
#define	EMIF4_TIM2_T_CKE		0x2
#define	EMIF4_TIM3_T_TDQSCKMAX	0x0
#define	EMIF4_TIM3_T_RFC		0x25
#define	EMIF4_TIM3_T_RAS_MAX	0x7
#define	EMIF4_PWR_IDLE			0x2
#define	EMIF4_PWR_DPD_EN		0x0
#define	EMIF4_PWR_PM_EN			0x0
#define	EMIF4_PWR_PM_TIM		0x0
#define	EMIF4_INITREF_DIS		0x0
#define	EMIF4_PASR				0x0
#define	EMIF4_REFRESH_RATE		0x050F

// SDRAM Config register
#define	EMIF4_CFG_SDRAM_TYP		0x2
#define	EMIF4_CFG_IBANK_POS		0x0
#define	EMIF4_CFG_DDR_TERM		0x0
#define	EMIF4_CFG_DDR2_DDQS		0x1
#define	EMIF4_CFG_DYN_ODT		0x0
#define	EMIF4_CFG_DDR_DIS_DLL	0x0
#define	EMIF4_CFG_SDR_DRV		0x0
#define	EMIF4_CFG_CWL			0x0
#define	EMIF4_CFG_NARROW_MD		0x0
#define	EMIF4_CFG_CL			0x5
#define	EMIF4_CFG_ROWSIZE		0x0
#define	EMIF4_CFG_IBANK			0x3
#define	EMIF4_CFG_EBANK			0x0
#define	EMIF4_CFG_PGSIZE		0x2

// EMIF4 PHY Control 1 register configuration
#define EMIF4_DDR1_RD_LAT		0x6
#define	EMIF4_DDR1_PWRDN_DIS	0x0
#define EMIF4_DDR1_STRBEN_EXT	0x0
#define EMIF4_DDR1_DLL_MODE		0x0
#define EMIF4_DDR1_VTP_DYN		0x0
#define EMIF4_DDR1_LB_CK_SEL	0x0

// EMIF4 PHY Control 2 register configuration
#define EMIF4_DDR2_TX_DATA_ALIGN	0x0
#define EMIF4_DDR2_RX_DLL_BYPASS	0x0

//------------------------------------------------------------------------------
//  Typedefs
//
typedef struct
{
	UINT32 m;
	UINT32 n;
	UINT32 fsel;
	UINT32 m2;
} DpllParam;

//------------------------------------------------------------------------------
//  Function Prototypes
//
static UINT32 WaitOnValue(UINT32 readBitMask, UINT32 matchValue, volatile UINT32 *readAddr, UINT32 bound);
static UINT32 GetCpuRev();

static VOID WatchdogSetup();
static VOID PinMuxSetup();
static VOID GpioSetup();
static VOID ClockSetup();
static VOID MemorySetup();
static VOID SDRAMSetup();
void do_emif4_init();

//------------------------------------------------------------------------------
//
//  Function:  WaitOnValue
//
//  Common routine to allow waiting for changes in volatile regs.
//
UINT32 WaitOnValue(UINT32 readBitMask, UINT32 matchValue, volatile UINT32 *readAddr, UINT32 bound)
{
	UINT32 i = 0, val;
	for(;;)
	{
		i++;
		val = INREG32(readAddr) & readBitMask;
		if (val == matchValue)
			return 1;
		if (i == bound)
			return 0;
	} 
}

//------------------------------------------------------------------------------
//
//  Function:  GetCpuRev
//
//  Extracts CPU version info.
//
UINT32 GetCpuRev()
{
	// TODO: Get real silicon revision
	return SILICON_REV;
}

//------------------------------------------------------------------------------
//
//  Function:  OALGetTickCount()
//
//  Stub routine.
//
UINT32 OALGetTickCount()
{
    return 1;
}

//------------------------------------------------------------------------------
//
//  Function:  PlatformSetup
//
//  Initializes platform settings.  Stack based initialization only - no 
//  global variables allowed.
//
VOID PlatformSetup()
{
    // Initialize the platform
    WatchdogSetup();
    PinMuxSetup();
	GpioSetup();
	

	ClockSetup();

    // configure i2c devices
    OALI2CInit(OMAP_DEVICE_I2C1);
    OALI2CInit(OMAP_DEVICE_I2C2);
    OALI2CInit(OMAP_DEVICE_I2C3);

	SDRAMSetup();

    MemorySetup();
}

//------------------------------------------------------------------------------
//
//  Function:  WatchdogSetup
//
//  Initializes watchdog timer settings.
//
static VOID WatchdogSetup()
{
	/* There are 3 watch dogs WD1=Secure, WD2=MPU, WD3=IVA. WD1 is
	either taken care of by ROM (HS/EMU) or not accessible (GP).
	We need to take care of WD2-MPU or take a PRCM reset. WD3
	should not be running and does not generate a PRCM reset. */

	OMAP_PRCM_WKUP_CM_REGS *pPrcmWkupCM = OALPAtoUA(OMAP_PRCM_WKUP_CM_REGS_PA);
	OMAP_WDOG_REGS *pWdogTimer = OALPAtoUA(OMAP_WDOG2_REGS_PA);

	SETREG32(&pPrcmWkupCM->CM_FCLKEN_WKUP, CM_CLKEN_WDT2);
	SETREG32(&pPrcmWkupCM->CM_ICLKEN_WKUP, CM_CLKEN_WDT2);

	WaitOnValue(CM_IDLEST_ST_WDT2, CM_IDLEST_ST_WDT2, &pPrcmWkupCM->CM_IDLEST_WKUP, 5); // Some issue here

	OUTREG32(&pWdogTimer->WSPR, WDOG_DISABLE_SEQ1);
	while (INREG32(&pWdogTimer->WWPS));
	OUTREG32(&pWdogTimer->WSPR, WDOG_DISABLE_SEQ2);
}

//------------------------------------------------------------------------------
//
//  Function:  PinMuxSetup
//
//  Initializes pin/pad mux settings.
//
static VOID PinMuxSetup()
{
    static const PAD_INFO initialPinMux[] = {
            SDRC_PADS
            GPMC_PADS
            UART3_PADS
            MMC1_PADS
            I2C1_PADS
            I2C2_PADS
            I2C3_PADS
            WKUP_PAD_ENTRY(SYS_32K, INPUT_ENABLED | PULL_RESISTOR_DISABLED | MUXMODE(0))
            END_OF_PAD_ARRAY
    };

    ConfigurePadArray(initialPinMux);

}

//------------------------------------------------------------------------------
//
//  Function:  GpioSetup
//
//  Initializes GPIO pin direction/state.  
//
VOID GpioSetup()
{
	// TODO: Initialize GPIO pins here
}

//------------------------------------------------------------------------------
//
//  Function:  ClockSetup
//
//  Initializes clocks and power. Stack based initialization only - no 
//  global variables allowed.
//
#if 0
void ClockSetup()
{
	DpllParam coreDpllParams[] =
	{
		// 12MHz
		{
			// ES1
			0x19F,
			0x0E,
			0x03,
			0x01
		},
		{
			// ES2
			0xA6,
			0x05,
			0x07,
			0x01
		},
		// 13MHz
		{
			// ES1
			0x01B2,
			0x10,
			0x03,
			0x01
		},
		{
			// ES2
			0x014C,
			0x0C,
			0x03,
			0x01
		},
		// 19.2MHz
		{
			// ES1
			0x19F,
			0x17,
			0x03,
			0x01
		},
		{
			// ES2
			0x19F,
			0x17,
			0x03,
			0x01
		}
	};

	DpllParam perDpllParams[] =
	{
		// 12MHz
		{
			0xD8,
			0x05,
			0x07,
			0x09
		},
		// 13MHz
		{
			0x01B0,
			0x0C,
			0x03,
			0x09
		},
		// 19.2MHz
		{
			0xE1,
			0x09,
			0x07,
			0x09
		}
	};

	DpllParam mpuDpllParams[] =
	{
		// 12MHz
		{
			// ES1
			0x00FE,
			0x07,
			0x05,
			0x01
		},
		{
			// ES2
			0xFA,
			0x05,
			0x07,
			0x01
		},
		// 13MHz
		{
			// ES1
			0x017D,
			0x0C,
			0x03,
			0x01
		},
		{
			// ES2
			0x1F4,
			0x0C,
			0x03,
			0x01
		},
		// 19.2MHz
		{
			// ES1
			0x0179,
			0x12,
			0x04,
			0x01
		},
		{
			// ES2
			0x0271,
			0x17,
			0x03,
			0x01
		}
	};

	OMAP_PRCM_GLOBAL_PRM_REGS *pPrcmGlobalPRM = OALPAtoUA(OMAP_PRCM_GLOBAL_PRM_REGS_PA);
	OMAP_PRCM_WKUP_CM_REGS *pPrcmWkupCM = OALPAtoUA(OMAP_PRCM_WKUP_CM_REGS_PA);
	OMAP_GPTIMER_REGS *pGptimer1 = OALPAtoUA(OMAP_GPTIMER1_REGS_PA);
	OMAP_32KSYNCNT_REGS *p32KSync = OALPAtoUA(OMAP_32KSYNC_REGS_PA);
	OMAP_PRCM_CLOCK_CONTROL_PRM_REGS *pPrcmClockControlPRM = OALPAtoUA(OMAP_PRCM_CLOCK_CONTROL_PRM_REGS_PA);
	OMAP_PRCM_CLOCK_CONTROL_CM_REGS *pPrcmClockControlCM = OALPAtoUA(OMAP_PRCM_CLOCK_CONTROL_CM_REGS_PA);
	OMAP_PRCM_MPU_CM_REGS *pPrcmMpuCM = OALPAtoUA(OMAP_PRCM_MPU_CM_REGS_PA);
	OMAP_PRCM_DSS_CM_REGS *pPrcmDssCM = OALPAtoUA(OMAP_PRCM_DSS_CM_REGS_PA);
	OMAP_PRCM_EMU_CM_REGS *pPrcmEmuCM = OALPAtoUA(OMAP_PRCM_EMU_CM_REGS_PA);
	OMAP_PRCM_CORE_CM_REGS *pPrcmCoreCM = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
	OMAP_PRCM_PER_CM_REGS *pPrcmPerCM = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);

	UINT32 oscClk = 0;
	UINT32 sysClkinSel;
	UINT32 start, cstart, cend, cdiff, val;
	UINT32 clkIndex, silIndex;
	DpllParam *pDpllParam;

	/* Gauge the input clock speed and find out the sysClkinSel
	value corresponding to the input clock. */

	val = INREG32(&pPrcmGlobalPRM->PRM_CLKSRC_CTRL);
	// If SYS_CLK is being divided by 2, remove for now
	val = (val & (~BIT7)) | BIT6;
	OUTREG32(&pPrcmGlobalPRM->PRM_CLKSRC_CTRL, val);

	// Enable timer
	val = INREG32(&pPrcmWkupCM->CM_CLKSEL_WKUP) | BIT0;
	OUTREG32(&pPrcmWkupCM->CM_CLKSEL_WKUP, val);

	// Enable I and F Clocks for GPT1
	val = INREG32(&pPrcmWkupCM->CM_ICLKEN_WKUP) | BIT0 | BIT2;
	OUTREG32(&pPrcmWkupCM->CM_ICLKEN_WKUP,  val);
	val = INREG32(&pPrcmWkupCM->CM_FCLKEN_WKUP) | BIT0;
	OUTREG32(&pPrcmWkupCM->CM_FCLKEN_WKUP, val);

	OUTREG32(&pGptimer1->TLDR, 0); // Start counting at 0
	OUTREG32(&pGptimer1->TCLR, GPT_EN); // Enable clock (sys_clk NO-prescale / 1)

	// Enable 32kHz source - enabled out of reset
	// Determine sys_clk via gauging */

	start = 20 + INREG32(&p32KSync->CR);			// Start time in 20 cycles
	while (INREG32(&p32KSync->CR) < start);			// Dead loop till start time
	cstart = INREG32(&pGptimer1->TCRR);				// Get start sys_clk count
	while (INREG32(&p32KSync->CR) < (start + 20));	// Wait for 40 cycles
	cend = INREG32(&pGptimer1->TCRR);				// Get end sys_clk count
	cdiff = cend - cstart;							// Get elapsed ticks

	// Based on number of ticks assign speed
	if (cdiff > 19000)
		oscClk = S38_4M;
	else if (cdiff > 15200)
		oscClk = S26M;
	else if (cdiff > 9000)
		oscClk = S19_2M;
	else if (cdiff > 7600)
		oscClk = S13M;
	else
		oscClk = S12M;

	// Get sysClkinSel
	if (oscClk == S38_4M)
		sysClkinSel = 4;
	else if (oscClk == S26M)
		sysClkinSel = 3;
	else if (oscClk == S19_2M)
		sysClkinSel = 2;
	else if (oscClk == S13M)
		sysClkinSel = 1;
	else if (oscClk == S12M)
		sysClkinSel = 0;
	else
	{for(;;);
	}

	MASKREG32(&pPrcmClockControlPRM->PRM_CLKSEL, SYS_CLKIN_SEL_MASK, sysClkinSel << SYS_CLKIN_SEL_SHIFT); // Set input crystal speed

	// If the input clock is greater than 19.2M always divide / 2
	if (sysClkinSel > 2)
	{
		MASKREG32(&pPrcmGlobalPRM->PRM_CLKSRC_CTRL, SYSCLKDIV_MASK, 2 << SYSCLKDIV_SHIFT);
		clkIndex = sysClkinSel / 2;
	}
	else
	{
		MASKREG32(&pPrcmGlobalPRM->PRM_CLKSRC_CTRL, SYSCLKDIV_MASK, 1 << SYSCLKDIV_SHIFT);
		clkIndex = sysClkinSel;
	}

	/* The DPLL tables are defined according to sysclk value and
	silicon revision. The clk_index value will be used to get
	the values for that input sysclk from the DPLL param table
	and sil_index will get the values for that SysClk for the
	appropriate silicon rev. */
	silIndex = GetCpuRev() - 1;

	// Unlock MPU DPLL (slows things down, and needed later)
	MASKREG32(&pPrcmMpuCM->CM_CLKEN_PLL_MPU, EN_MPU_DPLL_MASK, PLL_LOW_POWER_BYPASS << EN_MPU_DPLL_SHIFT);
	WaitOnValue(BIT0, 0, &pPrcmMpuCM->CM_IDLEST_PLL_MPU, LDELAY);

	// Getting the base address of Core DPLL param table
	pDpllParam = &coreDpllParams[2 * clkIndex + silIndex];

	// CORE DPLL
	MASKREG32(&pPrcmClockControlCM->CM_CLKEN_PLL, EN_CORE_DPLL_MASK, PLL_FAST_RELOCK_BYPASS << EN_CORE_DPLL_SHIFT);
	WaitOnValue(BIT0, 0, &pPrcmClockControlCM->CM_IDLEST_CKGEN, LDELAY);
	MASKREG32(&pPrcmEmuCM->CM_CLKSEL1_EMU, DIV_DPLL3_MASK, CORE_M3X2 << DIV_DPLL3_SHIFT); // M3x2
	MASKREG32(&pPrcmClockControlCM->CM_CLKSEL1_PLL, CORE_DPLL_CLKOUT_DIV_MASK, pDpllParam->m2 << CORE_DPLL_CLKOUT_DIV_SHIFT); // Set M2
	MASKREG32(&pPrcmClockControlCM->CM_CLKSEL1_PLL, CORE_DPLL_MULT_MASK, pDpllParam->m << CORE_DPLL_MULT_SHIFT); // Set M
	MASKREG32(&pPrcmClockControlCM->CM_CLKSEL1_PLL, CORE_DPLL_DIV_MASK, pDpllParam->n << CORE_DPLL_DIV_SHIFT); // Set N
	MASKREG32(&pPrcmClockControlCM->CM_CLKSEL1_PLL, SOURCE_96M, 0); // 96M src
    MASKREG32(&pPrcmCoreCM->CM_CLKSEL_CORE, CLKSEL_L4_MASK, CORE_L4_DIV << CLKSEL_L4_SHIFT); // L4
	MASKREG32(&pPrcmCoreCM->CM_CLKSEL_CORE, CLKSEL_L3_MASK, CORE_L3_DIV << CLKSEL_L3_SHIFT); // L3
	MASKREG32(&pPrcmWkupCM->CM_CLKSEL_WKUP, CLKSEL_RM_MASK, WKUP_RSM << CLKSEL_RM_SHIFT); // Reset mgr
	MASKREG32(&pPrcmClockControlCM->CM_CLKEN_PLL, CORE_DPLL_FREQSEL_MASK, pDpllParam->fsel << CORE_DPLL_FREQSEL_SHIFT); // FREQSEL
	MASKREG32(&pPrcmClockControlCM->CM_CLKEN_PLL, EN_CORE_DPLL_MASK, PLL_LOCK << EN_CORE_DPLL_SHIFT); // Lock mode
	WaitOnValue(BIT0, 1, &pPrcmClockControlCM->CM_IDLEST_CKGEN, LDELAY);

	// Getting the base address to PER DPLL param table
	pDpllParam = &perDpllParams[clkIndex];

	// PER DPLL
	MASKREG32(&pPrcmClockControlCM->CM_CLKEN_PLL, EN_PERIPH_DPLL_MASK, PLL_STOP << EN_PERIPH_DPLL_SHIFT);
	WaitOnValue(BIT1, 0, &pPrcmClockControlCM->CM_IDLEST_CKGEN, LDELAY);
	MASKREG32(&pPrcmEmuCM->CM_CLKSEL1_EMU, DIV_DPLL4_MASK, PER_M6X2 << DIV_DPLL4_SHIFT); // Set M6
	MASKREG32(&pPrcmDssCM->CM_CLKSEL_DSS, CLKSEL_DSS1_MASK, PER_M4X2 << CLKSEL_DSS1_SHIFT); // Set M4
	MASKREG32(&pPrcmDssCM->CM_CLKSEL_DSS, CLKSEL_TV_MASK, PER_M3X2 << CLKSEL_TV_SHIFT); // Set M3
	MASKREG32(&pPrcmClockControlCM->CM_CLKSEL3_PLL, DIV_96M_MASK, pDpllParam->m2 << DIV_96M_SHIFT); // Set M2
	MASKREG32(&pPrcmClockControlCM->CM_CLKSEL2_PLL, PERIPH_DPLL_MULT_MASK, pDpllParam->m << PERIPH_DPLL_MULT_SHIFT); // Set M
	MASKREG32(&pPrcmClockControlCM->CM_CLKSEL2_PLL, PERIPH_DPLL_DIV_MASK, pDpllParam->n << PERIPH_DPLL_DIV_SHIFT); // Set N
	MASKREG32(&pPrcmClockControlCM->CM_CLKEN_PLL, PERIPH_DPLL_FREQSEL_MASK, pDpllParam->fsel << PERIPH_DPLL_FREQSEL_SHIFT); // FREQSEL
	MASKREG32(&pPrcmClockControlCM->CM_CLKEN_PLL, EN_PERIPH_DPLL_MASK, PLL_LOCK << EN_PERIPH_DPLL_SHIFT); // Lock mode
	WaitOnValue(BIT1, 2, &pPrcmClockControlCM->CM_IDLEST_CKGEN, LDELAY);

	// Getting the base address to MPU DPLL param table
	pDpllParam = &mpuDpllParams[2 * clkIndex + silIndex];

	// MPU DPLL (unlocked already)
	MASKREG32(&pPrcmMpuCM->CM_CLKSEL2_PLL_MPU, MPU_DPLL_CLKOUT_DIV_MASK, pDpllParam->m2 << MPU_DPLL_CLKOUT_DIV_SHIFT); // Set M2
	MASKREG32(&pPrcmMpuCM->CM_CLKSEL1_PLL_MPU, MPU_DPLL_MULT_MASK, pDpllParam->m << MPU_DPLL_MULT_SHIFT); // Set M
	MASKREG32(&pPrcmMpuCM->CM_CLKSEL1_PLL_MPU, MPU_DPLL_DIV_MASK, pDpllParam->n << MPU_DPLL_DIV_SHIFT); // Set N
	MASKREG32(&pPrcmMpuCM->CM_CLKEN_PLL_MPU, MPU_DPLL_FREQSEL_MASK, pDpllParam->fsel << MPU_DPLL_FREQSEL_SHIFT); // FREQSEL
	MASKREG32(&pPrcmMpuCM->CM_CLKEN_PLL_MPU, EN_MPU_DPLL_MASK, PLL_LOCK << EN_MPU_DPLL_SHIFT); // Lock mode
	WaitOnValue(BIT0, 1, &pPrcmMpuCM->CM_IDLEST_PLL_MPU, LDELAY);

	// USB DPLL
    OUTREG32(&pPrcmClockControlCM->CM_CLKSEL5_PLL, BSP_CM_CLKSEL5_PLL);
    OUTREG32(&pPrcmClockControlCM->CM_CLKSEL4_PLL, BSP_CM_CLKSEL4_PLL);
    OUTREG32(&pPrcmClockControlCM->CM_CLKEN2_PLL, BSP_CM_CLKEN2_PLL);
	WaitOnValue(BIT0, 1, &pPrcmClockControlCM->CM_IDLEST2_CKGEN, LDELAY);

	// Set up GPTimers to 32k_clk source only
	MASKREG32(&pPrcmPerCM->CM_CLKSEL_PER, CLKSEL_GPT_MASK, 0xFF << CLKSEL_GPT_SHIFT);
	MASKREG32(&pPrcmWkupCM->CM_CLKSEL_WKUP, CLKSEL_GPT1_MASK, 0x00 << CLKSEL_GPT1_SHIFT);

	OALStall(5000);

	// Enable the clks & power for perifs

	// Enable GP2 timer
	MASKREG32(&pPrcmPerCM->CM_CLKSEL_PER, CLKSEL_GPT2_MASK, 1 << CLKSEL_GPT2_SHIFT); // GPT2 = sys clk
	MASKREG32(&pPrcmPerCM->CM_ICLKEN_PER, EN_GPT2_MASK, 1 << EN_GPT2_SHIFT); // ICKen GPT2
	MASKREG32(&pPrcmPerCM->CM_FCLKEN_PER, EN_GPT2_MASK, 1 << EN_GPT2_SHIFT); // FCKen GPT2

	// Enable UART1 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_UART1_MASK, 1 << EN_UART1_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_UART1_MASK, 1 << EN_UART1_SHIFT);

	// Enable UART2 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_UART2_MASK, 1 << EN_UART2_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_UART2_MASK, 1 << EN_UART2_SHIFT);

	// Enable UART3 clocks
	MASKREG32(&pPrcmPerCM->CM_FCLKEN_PER, EN_UART3_MASK, 1 << EN_UART3_SHIFT);
	MASKREG32(&pPrcmPerCM->CM_ICLKEN_PER, EN_UART3_MASK, 1 << EN_UART3_SHIFT);

	// Enable MMC1 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_MMC1_MASK, 1 << EN_MMC1_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_MMC1_MASK, 1 << EN_MMC1_SHIFT);

	// Enable MMC2 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_MMC2_MASK, 1 << EN_MMC2_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_MMC2_MASK, 1 << EN_MMC2_SHIFT);

	// Enable I2C1 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_I2C1_MASK, 1 << EN_I2C1_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_I2C1_MASK, 1 << EN_I2C1_SHIFT);

	// Enable I2C2 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_I2C2_MASK, 1 << EN_I2C2_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_I2C2_MASK, 1 << EN_I2C2_SHIFT);

	// Enable I2C3 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_I2C3_MASK, 1 << EN_I2C3_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_I2C3_MASK, 1 << EN_I2C3_SHIFT);

	// Enable GPIO1 clocks
	MASKREG32(&pPrcmWkupCM->CM_FCLKEN_WKUP, EN_GPIO1_MASK, 1 << EN_GPIO1_SHIFT);
	MASKREG32(&pPrcmWkupCM->CM_ICLKEN_WKUP, EN_GPIO1_MASK, 1 << EN_GPIO1_SHIFT);

	OALStall(1000);

	// TODO: add register definitions for the next lines

	// Enable the DDRPHY clk
	MASKREG32(0x48002584, 0x1 << 15, 0x1 << 15);
	// Enable the EMIF4 clk
	MASKREG32(0x48002584, 0x1 << 14, 0x1 << 14);

	// Enable the peripheral clocks
	MASKREG32(0x48002594, 0x7FF, 0x7FF);

	// Bring cpgmac out of reset
	MASKREG32(0x48002590, 0x1 << 1, 0x1 << 0x1);
	OALStall(10);
	MASKREG32(0x48002590, 0x1 << 1, 0x0 << 0x1);
}
#else
//------------------------------------------------------------------------------
//
//  Function:  ClockSetup
//
//  Initializes clocks and power.  Stack based initialization only - no 
//  global variables allowed.
//
void ClockSetup()
{    
    unsigned int val;
    OMAP_PRCM_EMU_CM_REGS* pPrcmEmuCM = OALPAtoUA(OMAP_PRCM_EMU_CM_REGS_PA);
//    OMAP_PRCM_DSS_CM_REGS* pPrcmDssCM = OALPAtoUA(OMAP_PRCM_DSS_CM_REGS_PA);
    OMAP_PRCM_PER_CM_REGS* pPrcmPerCM = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
#ifdef BSP_EVM2_3730
    OMAP_PRCM_SGX_CM_REGS* pPrcmSgxCM = OALPAtoUA(OMAP_PRCM_SGX_CM_REGS_PA);
#endif
    OMAP_PRCM_MPU_CM_REGS* pPrcmMpuCM = OALPAtoUA(OMAP_PRCM_MPU_CM_REGS_PA);
    OMAP_PRCM_CORE_CM_REGS* pPrcmCoreCM = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
    OMAP_PRCM_WKUP_CM_REGS* pPrcmWkupCM = OALPAtoUA(OMAP_PRCM_WKUP_CM_REGS_PA);
    OMAP_PRCM_CLOCK_CONTROL_CM_REGS* pPrcmClkCM = OALPAtoUA(OMAP_PRCM_CLOCK_CONTROL_CM_REGS_PA);
    OMAP_PRCM_CLOCK_CONTROL_PRM_REGS* pPrcmClkPRM = OALPAtoUA(OMAP_PRCM_CLOCK_CONTROL_PRM_REGS_PA);
    OMAP_PRCM_GLOBAL_PRM_REGS *pPrcmGlobalPRM = OALPAtoUA(OMAP_PRCM_GLOBAL_PRM_REGS_PA);

    // setup input system clock
    OUTREG32(&pPrcmClkPRM->PRM_CLKSEL, BSP_PRM_CLKSEL);
	// setup SYSCLK
	OUTREG32(&pPrcmGlobalPRM->PRM_CLKSRC_CTRL, BSP_PRM_CLKSRC_CTRL);

    //---------------------------------
    // setup dpll timings for core and peripheral dpll
    //
    
    // configure clock ratios for L3, L4
    // configure clock selection for gpt10, gpt11
    OUTREG32(&pPrcmCoreCM->CM_CLKSEL_CORE, BSP_CM_CLKSEL_CORE);
    
    // configure m:n clock ratios as well as frequency selection for core dpll
    OUTREG32(&pPrcmClkCM->CM_CLKSEL1_PLL, BSP_CM_CLKSEL1_PLL);

    // configure timings for all related peripherals
    OUTREG32(&pPrcmEmuCM->CM_CLKSEL1_EMU, BSP_CM_CLKSEL1_EMU);
    //OUTREG32(&pPrcmDssCM->CM_CLKSEL_DSS, BSP_CM_CLKSEL_DSS);
    OUTREG32(&pPrcmClkCM->CM_CLKSEL3_PLL, BSP_CM_CLKSEL3_PLL);
    OUTREG32(&pPrcmClkCM->CM_CLKSEL2_PLL, (INREG32(&pPrcmClkCM->CM_CLKSEL2_PLL) & 0xfff00000) | BSP_CM_CLKSEL2_PLL);
#ifdef BSP_EVM2_3730
    OUTREG32(&pPrcmSgxCM->CM_CLKSEL_SGX, BSP_CM_CLKSEL_SGX);
#endif

    // lock dpll with correct frequency selection
    OUTREG32(&pPrcmClkCM->CM_CLKEN_PLL, BSP_CM_CLKEN_PLL);
    while ((INREG32(&pPrcmClkCM->CM_IDLEST_CKGEN) & DPLL_STATUS_MASK) != DPLL_STATUS_LOCKED);

    //---------------------------------
    // setup dpll timings for mpu dpll
    //

    // put mpu dpll1 in bypass
    val = INREG32(&pPrcmMpuCM->CM_CLKEN_PLL_MPU);
    val &= ~DPLL_MODE_MASK;
    val |= DPLL_MODE_LOWPOWER_BYPASS;
    OUTREG32(&pPrcmMpuCM->CM_CLKEN_PLL_MPU, val);
    while ((INREG32(&pPrcmMpuCM->CM_IDLEST_PLL_MPU) & DPLL_STATUS_MASK) != DPLL_STATUS_BYPASSED);

    // setup DPLL1 divider
    OUTREG32(&pPrcmMpuCM->CM_CLKSEL2_PLL_MPU, BSP_CM_CLKSEL2_PLL_MPU);
    
    // configure m:n clock ratios as well as frequency selection for mpu dpll
    OUTREG32(&pPrcmMpuCM->CM_CLKSEL1_PLL_MPU, BSP_CM_CLKSEL1_PLL_MPU);

    // lock dpll1 with correct frequency selection
    OUTREG32(&pPrcmMpuCM->CM_CLKEN_PLL_MPU, BSP_CM_CLKEN_PLL_MPU);
    while ((INREG32(&pPrcmMpuCM->CM_IDLEST_PLL_MPU) & DPLL_STATUS_MASK) != DPLL_STATUS_LOCKED);
    
    //---------------------------------
    // setup dpll timings for core and peripheral dpll
    //
    
    // configure clock ratios for 120m
    OUTREG32(&pPrcmClkCM->CM_CLKSEL5_PLL, BSP_CM_CLKSEL5_PLL);
    
    // configure m:n clock ratios as well as frequency selection for core dpll
    OUTREG32(&pPrcmClkCM->CM_CLKSEL4_PLL, BSP_CM_CLKSEL4_PLL);

    // lock dpll with correct frequency selection
    OUTREG32(&pPrcmClkCM->CM_CLKEN2_PLL, BSP_CM_CLKEN2_PLL);
    while ((INREG32(&pPrcmClkCM->CM_IDLEST2_CKGEN) & DPLL_STATUS_MASK) != DPLL_STATUS_LOCKED);
    
    //--------------------------
    // Enable GPTIMER1, GPIO bank 1 (debug led)
    SETREG32(&pPrcmWkupCM->CM_FCLKEN_WKUP, (CM_CLKEN_GPT1|CM_CLKEN_GPIO1));
    SETREG32(&pPrcmWkupCM->CM_ICLKEN_WKUP, (CM_CLKEN_GPT1|CM_CLKEN_GPIO1));

    // Enable UART3 (debug port) and GPIO banks that are accessed in the bootloader
    SETREG32(&pPrcmPerCM->CM_FCLKEN_PER, (CM_CLKEN_UART3|CM_CLKEN_GPIO6|CM_CLKEN_GPIO5|CM_CLKEN_GPIO3));
    SETREG32(&pPrcmPerCM->CM_ICLKEN_PER, (CM_CLKEN_UART3|CM_CLKEN_GPIO6|CM_CLKEN_GPIO5|CM_CLKEN_GPIO3));

	// Enable GP2 timer
	MASKREG32(&pPrcmPerCM->CM_CLKSEL_PER, CLKSEL_GPT2_MASK, 1 << CLKSEL_GPT2_SHIFT); // GPT2 = sys clk
	MASKREG32(&pPrcmPerCM->CM_ICLKEN_PER, EN_GPT2_MASK, 1 << EN_GPT2_SHIFT); // ICKen GPT2
	MASKREG32(&pPrcmPerCM->CM_FCLKEN_PER, EN_GPT2_MASK, 1 << EN_GPT2_SHIFT); // FCKen GPT2

	// Enable UART1 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_UART1_MASK, 1 << EN_UART1_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_UART1_MASK, 1 << EN_UART1_SHIFT);

	// Enable UART2 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_UART2_MASK, 1 << EN_UART2_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_UART2_MASK, 1 << EN_UART2_SHIFT);

	// Enable UART3 clocks
	MASKREG32(&pPrcmPerCM->CM_FCLKEN_PER, EN_UART3_MASK, 1 << EN_UART3_SHIFT);
	MASKREG32(&pPrcmPerCM->CM_ICLKEN_PER, EN_UART3_MASK, 1 << EN_UART3_SHIFT);

	// Enable MMC1 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_MMC1_MASK, 1 << EN_MMC1_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_MMC1_MASK, 1 << EN_MMC1_SHIFT);

	// Enable MMC2 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_MMC2_MASK, 1 << EN_MMC2_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_MMC2_MASK, 1 << EN_MMC2_SHIFT);

	// Enable I2C1 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_I2C1_MASK, 1 << EN_I2C1_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_I2C1_MASK, 1 << EN_I2C1_SHIFT);

	// Enable I2C2 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_I2C2_MASK, 1 << EN_I2C2_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_I2C2_MASK, 1 << EN_I2C2_SHIFT);

	// Enable I2C3 clocks
	MASKREG32(&pPrcmCoreCM->CM_FCLKEN1_CORE, EN_I2C3_MASK, 1 << EN_I2C3_SHIFT);
	MASKREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, EN_I2C3_MASK, 1 << EN_I2C3_SHIFT);

	// Enable GPIO1 clocks
	MASKREG32(&pPrcmWkupCM->CM_FCLKEN_WKUP, EN_GPIO1_MASK, 1 << EN_GPIO1_SHIFT);
	MASKREG32(&pPrcmWkupCM->CM_ICLKEN_WKUP, EN_GPIO1_MASK, 1 << EN_GPIO1_SHIFT);

    // Disable HS USB OTG interface clock
    //CLRREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, CM_CLKEN_HSOTGUSB);

    // Disable D2D interface clock
    //CLRREG32(&pPrcmCoreCM->CM_ICLKEN1_CORE, CM_CLKEN_D2D);
}
#endif

//------------------------------------------------------------------------------
//
//  Function:  MemorySetup
//
//  Initializes memory interfaces.
//
static VOID MemorySetup()
{
	OMAP_GPMC_REGS* pGpmc = OALPAtoUA(OMAP_GPMC_REGS_PA);
	

	// Global GPMC Configuration
    OUTREG32(&pGpmc->GPMC_SYSCONFIG,       0x00000008);   // No idle, L3 clock free running      
    OUTREG32(&pGpmc->GPMC_IRQENABLE,       0x00000000);   // All interrupts disabled    
    OUTREG32(&pGpmc->GPMC_TIMEOUT_CONTROL, 0x00000000);   // Time out disabled    
    OUTREG32(&pGpmc->GPMC_CONFIG,          0x00000011);   // WP high, force posted write for NAND    

    // Configure CS0 for NAND
    OUTREG32(&pGpmc->GPMC_CONFIG1_0, BSP_GPMC_NAND_CONFIG1);
    OUTREG32(&pGpmc->GPMC_CONFIG2_0, BSP_GPMC_NAND_CONFIG2);
    OUTREG32(&pGpmc->GPMC_CONFIG3_0, BSP_GPMC_NAND_CONFIG3);
    OUTREG32(&pGpmc->GPMC_CONFIG4_0, BSP_GPMC_NAND_CONFIG4);
    OUTREG32(&pGpmc->GPMC_CONFIG5_0, BSP_GPMC_NAND_CONFIG5);
    OUTREG32(&pGpmc->GPMC_CONFIG6_0, BSP_GPMC_NAND_CONFIG6);
    OUTREG32(&pGpmc->GPMC_CONFIG7_0, BSP_GPMC_NAND_CONFIG7);

	// Configure CS2 for NOR
    OUTREG32(&pGpmc->GPMC_CONFIG1_2, BSP_GPMC_NOR_CONFIG1);
    OUTREG32(&pGpmc->GPMC_CONFIG2_2, BSP_GPMC_NOR_CONFIG2);
    OUTREG32(&pGpmc->GPMC_CONFIG3_2, BSP_GPMC_NOR_CONFIG3);
    OUTREG32(&pGpmc->GPMC_CONFIG4_2, BSP_GPMC_NOR_CONFIG4);
    OUTREG32(&pGpmc->GPMC_CONFIG5_2, BSP_GPMC_NOR_CONFIG5);
    OUTREG32(&pGpmc->GPMC_CONFIG6_2, BSP_GPMC_NOR_CONFIG6);
    OUTREG32(&pGpmc->GPMC_CONFIG7_2, BSP_GPMC_NOR_CONFIG7);

    // Configure CS3 for LAN
    OUTREG32(&pGpmc->GPMC_CONFIG1_3, BSP_GPMC_LAN_CONFIG1);
    OUTREG32(&pGpmc->GPMC_CONFIG2_3, BSP_GPMC_LAN_CONFIG2);
    OUTREG32(&pGpmc->GPMC_CONFIG3_3, BSP_GPMC_LAN_CONFIG3);
    OUTREG32(&pGpmc->GPMC_CONFIG4_3, BSP_GPMC_LAN_CONFIG4);
    OUTREG32(&pGpmc->GPMC_CONFIG5_3, BSP_GPMC_LAN_CONFIG5);
    OUTREG32(&pGpmc->GPMC_CONFIG6_3, BSP_GPMC_LAN_CONFIG6);
    OUTREG32(&pGpmc->GPMC_CONFIG7_3, BSP_GPMC_LAN_CONFIG7);

	OALStall(2000);
}

void SDRAMSetup()
{
    	UINT32 regval;

	// Init/Configure DDR first

	// Set the DDR PHY parameters in PHY ctrl registers
	regval = (EMIF4_DDR1_RD_LAT | (EMIF4_DDR1_PWRDN_DIS << 6) |
		(EMIF4_DDR1_STRBEN_EXT << 7) | (EMIF4_DDR1_DLL_MODE << 12) |
		(EMIF4_DDR1_VTP_DYN << 15) | (EMIF4_DDR1_LB_CK_SEL << 23));
	OUTREG32(EMIF4_DDR_PHYCTL1, regval);
	OUTREG32(EMIF4_DDR_PHYCTL1_SHDW, regval);

	regval = (EMIF4_DDR2_TX_DATA_ALIGN | (EMIF4_DDR2_RX_DLL_BYPASS << 1));
	OUTREG32(EMIF4_DDR_PHYCTL2, regval);

	// Reset the DDR PHY and wait till completed
	MASKREG32(EMIF4_IODFT_TLGC, 0x1 << 10, 0x1 << 10);
	// Wait till that bit clears
	while ((INREG32(EMIF4_IODFT_TLGC) & BIT10) == 0x1);
	// Re-verify the DDR PHY status
	while ((INREG32(EMIF4_SDRAM_STS) & BIT2) == 0x0);

	MASKREG32(EMIF4_IODFT_TLGC, 0x1 << 0, 0x1 << 0);
	// Set SDR timing registers
	regval = (EMIF4_TIM1_T_WTR | (EMIF4_TIM1_T_RRD << 3) |
		(EMIF4_TIM1_T_RC << 6) | (EMIF4_TIM1_T_RAS << 12) |
		(EMIF4_TIM1_T_WR << 17) | (EMIF4_TIM1_T_RCD << 21) |
		(EMIF4_TIM1_T_RP << 25));
	OUTREG32(EMIF4_SDRAM_TIM1, regval);
	OUTREG32(EMIF4_SDRAM_TIM1_SHDW, regval);

	regval = (EMIF4_TIM2_T_CKE | (EMIF4_TIM2_T_RTP << 3) |
		(EMIF4_TIM2_T_XSRD << 6) | (EMIF4_TIM2_T_XSNR << 16) |
		(EMIF4_TIM2_T_ODT << 25) | (EMIF4_TIM2_T_XP << 28));
	OUTREG32(EMIF4_SDRAM_TIM2, regval);
	OUTREG32(EMIF4_SDRAM_TIM2_SHDW, regval);

	regval = (EMIF4_TIM3_T_RAS_MAX | (EMIF4_TIM3_T_RFC << 4) |
		(EMIF4_TIM3_T_TDQSCKMAX << 13));
	OUTREG32(EMIF4_SDRAM_TIM3, regval);
	OUTREG32(EMIF4_SDRAM_TIM3_SHDW, regval);

	// Set the PWR control register
	regval = (UINT32) (EMIF4_PWR_PM_TIM | (EMIF4_PWR_PM_EN << 8) |
		(EMIF4_PWR_DPD_EN << 10) | (EMIF4_PWR_IDLE << 30));
	OUTREG32(EMIF4_PWR_MGT_CTRL, regval);
	OUTREG32(EMIF4_PWR_MGT_CTRL_SHDW, regval);

    // Set the DDR refresh rate control register
	regval = (EMIF4_REFRESH_RATE | (EMIF4_PASR << 24) |
		(EMIF4_INITREF_DIS << 31));
	OUTREG32(EMIF4_SDRAM_RFCR, regval);
	OUTREG32(EMIF4_SDRAM_RFCR_SHDW, regval);


	// Set the SDRAM configuration register
	regval = (EMIF4_CFG_PGSIZE | (EMIF4_CFG_EBANK << 3) |
		(EMIF4_CFG_IBANK << 4) | (EMIF4_CFG_ROWSIZE << 7) |
		(EMIF4_CFG_CL << 10) | (EMIF4_CFG_NARROW_MD << 14) |
		(EMIF4_CFG_CWL << 16) | (EMIF4_CFG_SDR_DRV << 18) |
		(EMIF4_CFG_DDR_DIS_DLL << 20) | (EMIF4_CFG_DYN_ODT << 21) |
		(EMIF4_CFG_DDR2_DDQS << 23) | (EMIF4_CFG_DDR_TERM << 24) |
		(EMIF4_CFG_IBANK_POS << 27) | (EMIF4_CFG_SDRAM_TYP << 29));
	OUTREG32(EMIF4_SDRAM_CFG, regval);
}