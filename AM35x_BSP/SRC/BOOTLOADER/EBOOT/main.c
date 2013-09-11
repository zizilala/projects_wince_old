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
//  File:  main.c
//
//  This file implements main bootloader functions called from blcommon
//  library.
//
#include "bsp.h"
#include <eboot.h>
#include "sdk_i2c.h"
#include "sdk_gpio.h"
#include "oal_i2c.h"
#include "kitl_cfg.h"
#include "boot_cfg.h"
#include "oal_alloc.h"
#include "ceddkex.h"

#include "bsp_cfg.h"
#include "bsp_padcfg.h"

UINT16 DefaultMacAddress[] = DEFAULT_MAC_ADDRESS;

// The following constants are used to distinguish between ES2.0 and ES2.1 CPU 
// revisions.  This mechanism is necessary because the ID fuses are incorrectly
// set to ES2.0 in some ES2.1 devices.
#define PUBLIC_ROM_CRC_PA       0x14020
#define PUBLIC_ROM_CRC_ES2_0    0x5a540331
#define PUBLIC_ROM_CRC_ES2_1    0x6880d8d6

//------------------------------------------------------------------------------
//
//  Global: g_bootCfg
//
//  This global variable is used to save boot configuration. It is read from
//  flash memory or initialized to default values if flash memory doesn't
//  contain valid structure. It can be modified by user in bootloader
//  configuration menu invoked by BLMenu.
//
BOOT_CFG g_bootCfg;
const volatile DWORD dwEbootECCtype = (DWORD)-1;
UCHAR g_ecctype;

//------------------------------------------------------------------------------
//
//  Global: g_eboot
//
//  This global variable is used to save information about downloaded regions.
//
EBOOT_CONTEXT g_eboot;


//------------------------------------------------------------------------------
// External Variables
extern DEVICE_IFC_GPIO Omap_Gpio;

//------------------------------------------------------------------------------
// External Functions

VOID Launch(UINT32 address);
VOID JumpTo(UINT32 address);
VOID OEMDeinitDebugSerial();
extern BOOL EnableDeviceClocks(UINT devId, BOOL bEnable);
extern void ReadMacAddressFromFuse(UCHAR mac[6]);
extern BOOL WriteFlashNK(UINT32 address, UINT32 size);

//------------------------------------------------------------------------------
//  Local Functions


void BSPGpioInit()
{
   BSPInsertGpioDevice(0,&Omap_Gpio,NULL);
}
void main()
{
    EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,TRUE);
    BootloaderMain();
}

//------------------------------------------------------------------------------
//
//  Function:  OEMPlatformInit
//
//  This function provide platform initialization functions. It is called
//  from boot loader after OEMDebugInit is called.  Note that boot loader
//  BootloaderMain is called from  s/init.s code which is run after reset.
//
BOOL 
OEMPlatformInit(
    )
{
    UINT32 dwIdReg;
    OMAP_GPTIMER_REGS *pTimerRegs = OALPAtoUA(OMAP_GPTIMER1_REGS_PA);
    UINT32 * pIDAddr = OALPAtoUA(OMAP_IDCODE_REGS_PA);    
    HANDLE hTwl,hGPIO;
    static UCHAR allocationPool[512];
    static const PAD_INFO ebootPinMux[] = {
            DSS_PADS
            ETHERNET_PADS
            GPIO_PADS
            END_OF_PAD_ARRAY
    };
    OALLocalAllocInit(allocationPool,sizeof(allocationPool));

    ConfigurePadArray(ebootPinMux);
    //OALLogSetZones( 
    //           (1<<OAL_LOG_VERBOSE)  |
    //           (1<<OAL_LOG_INFO)     |
    //           (1<<OAL_LOG_ERROR)    |
    //           (1<<OAL_LOG_WARN)     |
    //           (1<<OAL_LOG_FUNC)     |
    //           (1<<OAL_LOG_IO)     |
    //           0);

    OALLog(
        L"\r\nTexas Instruments Windows CE EBOOT for AM35x, "
        L"Built %S at %S\r\n", __DATE__, __TIME__
        );
    OALLog(
        L"EBOOT Version %d.%d, BSP %d.%02d.%02d.%02d\r\n", 
        EBOOT_VERSION_MAJOR, EBOOT_VERSION_MINOR, 
        BSP_VERSION_MAJOR, BSP_VERSION_MINOR, BSP_VERSION_QFES, BSP_VERSION_INCREMENTAL
        );

    // Soft reset GPTIMER1
    OUTREG32(&pTimerRegs->TIOCP, SYSCONFIG_SOFTRESET);
    // While until done
    while ((INREG32(&pTimerRegs->TISTAT) & GPTIMER_TISTAT_RESETDONE) == 0);
    // Enable posted mode
    OUTREG32(&pTimerRegs->TSICR, GPTIMER_TSICR_POSTED);
    // Start timer
    OUTREG32(&pTimerRegs->TCLR, GPTIMER_TCLR_AR|GPTIMER_TCLR_ST);
    
	// Enable device clocks used by the bootloader
    EnableDeviceClocks(OMAP_DEVICE_GPIO1,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO2,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO3,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO4,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO5,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO6,TRUE);

    // configure i2c devices
    OALI2CInit(OMAP_DEVICE_I2C1);
    OALI2CInit(OMAP_DEVICE_I2C2);
    OALI2CInit(OMAP_DEVICE_I2C3);

    GPIOInit();
    // Note that T2 accesses must occur after I2C initialization
    hTwl = NULL;
    hGPIO = GPIOOpen();

	
    GPIOSetMode(hGPIO, IOEXPANDER3_IRQ_GPIO,GPIO_DIR_INPUT | GPIO_INT_LOW);
    GPIOSetMode(hGPIO, IOEXPANDER2_IRQ_GPIO,GPIO_DIR_INPUT | GPIO_INT_LOW);
    GPIOSetMode(hGPIO, RTC_IRQ_GPIO,GPIO_DIR_INPUT | GPIO_INT_LOW);
    GPIOSetMode(hGPIO, LAN9311_IRQ_GPIO,GPIO_DIR_INPUT | GPIO_INT_LOW);
	

    dwIdReg = INREG32(pIDAddr);
    OALLog(L"\r\nTI AM3517 Version 0x%x (Hawkeye 0x%x / manufacturer ID  0x%x) \r\n",
        (dwIdReg & 0xF0000000) >> 28,
        (dwIdReg & 0x0FFFF000) >> 12,
        (dwIdReg & 0x00000FFE) >> 1
        );
    g_ecctype = (UCHAR)dwEbootECCtype;
	
    // Done
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMPlatformDeinit
//
static
VOID
OEMPlatformDeinit(
    )
{
    OMAP_GPTIMER_REGS *pTimerRegs = OALPAtoUA(OMAP_GPTIMER1_REGS_PA);

    // Soft reset GPTIMER
    OUTREG32(&pTimerRegs->TIOCP, SYSCONFIG_SOFTRESET);
    // While until done
    while ((INREG32(&pTimerRegs->TISTAT) & GPTIMER_TISTAT_RESETDONE) == 0);

	// Disable device clocks that were used by the bootloader
    EnableDeviceClocks(OMAP_DEVICE_GPIO1,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO2,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO3,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO4,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO5,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO6,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_GPTIMER2,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_UART1,FALSE);
	EnableDeviceClocks(OMAP_DEVICE_UART2,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_UART3,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_MMC1,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_MMC2,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_I2C1,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_I2C2,FALSE);
    EnableDeviceClocks(OMAP_DEVICE_I2C3,FALSE);
}
/*
//------------------------------------------------------------------------------
//
//  Function:  OALTritonSet
//
static void OALTritonSet(HANDLE hTwl, DWORD Register, BYTE Mask)
{
    BYTE regval;
    
    OALTritonRead(hTwl, Register, &regval);
    regval |= Mask;
    OALTritonWrite(hTwl, Register, regval);
}

//------------------------------------------------------------------------------
//
//  Function:  OALTritonClear
//
static void OALTritonClear(HANDLE hTwl, DWORD Register, BYTE Mask)
{
    BYTE regval;
    
    OALTritonRead(hTwl, Register, &regval);
    regval &= ~Mask;
    OALTritonWrite(hTwl, Register, regval);
}
*/
//------------------------------------------------------------------------------
//
//  Function:  CpuGpioOutput
//
/*
static void CpuGpioOutput(DWORD GpioNumber, DWORD Value)
{
    UINT32 fWkup, iWkup;
    UINT32 fPer, iPer;
    OMAP_GPIO_REGS* pGpio;
    OMAP_PRCM_PER_CM_REGS* pPrcmPerCM = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
    OMAP_PRCM_WKUP_CM_REGS* pPrcmWkupCM = OALPAtoUA(OMAP_PRCM_WKUP_CM_REGS_PA);

    // Enable clocks to GPIO modules
    fWkup = INREG32(&pPrcmWkupCM->CM_FCLKEN_WKUP);
    iWkup = INREG32(&pPrcmWkupCM->CM_ICLKEN_WKUP);
    fPer = INREG32(&pPrcmPerCM->CM_FCLKEN_PER);
    iPer = INREG32(&pPrcmPerCM->CM_ICLKEN_PER);
    SETREG32(&pPrcmWkupCM->CM_FCLKEN_WKUP, CM_CLKEN_GPIO1);
    SETREG32(&pPrcmWkupCM->CM_ICLKEN_WKUP, CM_CLKEN_GPIO1);
    SETREG32(&pPrcmPerCM->CM_FCLKEN_PER, CM_CLKEN_GPIO2|CM_CLKEN_GPIO3|CM_CLKEN_GPIO4|CM_CLKEN_GPIO5|CM_CLKEN_GPIO6);
    SETREG32(&pPrcmPerCM->CM_ICLKEN_PER, CM_CLKEN_GPIO2|CM_CLKEN_GPIO3|CM_CLKEN_GPIO4|CM_CLKEN_GPIO5|CM_CLKEN_GPIO6);

    switch (GpioNumber >> 5)
    {
        case 0:
            pGpio = OALPAtoUA(OMAP_GPIO1_REGS_PA);
            break;
            
        case 1:
            pGpio = OALPAtoUA(OMAP_GPIO2_REGS_PA);
            GpioNumber -= 32;
            break;

        case 2:
            pGpio = OALPAtoUA(OMAP_GPIO3_REGS_PA);
            GpioNumber -= 64;
            break;

        case 3:
            pGpio = OALPAtoUA(OMAP_GPIO4_REGS_PA);
            GpioNumber -= 96;
            break;

        case 4:
            pGpio = OALPAtoUA(OMAP_GPIO5_REGS_PA);
            GpioNumber -= 128;
            break;

        case 5:
            pGpio = OALPAtoUA(OMAP_GPIO6_REGS_PA);
            GpioNumber -= 160;
            break;

        default:
            return;
    }

    // set output state of GPIO pin
    if (Value)
        SETREG32(&pGpio->DATAOUT, 1 << GpioNumber);
    else
        CLRREG32(&pGpio->DATAOUT, 1 << GpioNumber);

    // make GPIO pin an output
    CLRREG32(&pGpio->OE, 1 << GpioNumber);

    // Put clocks back
    OUTREG32(&pPrcmWkupCM->CM_FCLKEN_WKUP, fWkup);
    OUTREG32(&pPrcmWkupCM->CM_ICLKEN_WKUP, iWkup);
    OUTREG32(&pPrcmPerCM->CM_FCLKEN_PER, fPer);
    OUTREG32(&pPrcmPerCM->CM_ICLKEN_PER, iPer);
}*/

//------------------------------------------------------------------------------
//
//  Function:  OEMPreDownload
//
//  This function is called before downloading an image. There is place
//  where user can be asked about device setup.
//
ULONG
OEMPreDownload(
    )
{
    ULONG rc = (ULONG) BL_ERROR;
    BSP_ARGS *pArgs = OALCAtoUA(IMAGE_SHARE_ARGS_CA);
    BOOL bForceBootMenu;
    OMAP_PRCM_GLOBAL_PRM_REGS * pPrmGlobal = OALPAtoUA(OMAP_PRCM_GLOBAL_PRM_REGS_PA);
    ULONG dwTemp;
    UINT32 *pStatusControlAddr = OALPAtoUA(OMAP_STATUS_CONTROL_REGS_PA);
    UINT32 dwSysBootCfg;

    OALLog(L"INFO: Predownload....\r\n");

    // We need to support multi bin notify
    g_pOEMMultiBINNotify = OEMMultiBinNotify;

    // Ensure bootloader blocks are marked as reserved
    BLReserveBootBlocks();

    // Read saved configration
    if (BLReadBootCfg(&g_bootCfg) &&
        (g_bootCfg.signature == BOOT_CFG_SIGNATURE) &&
        (g_bootCfg.version == BOOT_CFG_VERSION))
        {
        g_bootCfg.ECCtype = (UCHAR)dwEbootECCtype;
        
        OALLog(L"INFO: Boot configuration found\r\n");
        }
    else 
        {
        OALLog(L"WARN: Boot config wasn't found, using defaults\r\n");
        memset(&g_bootCfg, 0, sizeof(g_bootCfg));
#ifndef BSP_READ_MAC_FROM_FUSE 
		memcpy(&g_bootCfg.mac,DefaultMacAddress,sizeof(g_bootCfg.mac));
#else
        EnableDeviceClocks(OMAP_DEVICE_EFUSE,TRUE);
        ReadMacAddressFromFuse((UCHAR*)g_bootCfg.mac);
        EnableDeviceClocks(OMAP_DEVICE_EFUSE,FALSE);
#endif
        g_bootCfg.signature = BOOT_CFG_SIGNATURE;
        g_bootCfg.version = BOOT_CFG_VERSION;

        g_bootCfg.oalFlags = 0;
        g_bootCfg.flashNKFlags = 0;
        g_bootCfg.ECCtype = (UCHAR)dwEbootECCtype;
        // To make it easier to select USB or EBOOT from menus when booting from SD card,
        // preset the kitlFlags. This has no effect if booting from SD card.
        g_bootCfg.kitlFlags = OAL_KITL_FLAGS_DHCP|OAL_KITL_FLAGS_ENABLED;
        g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_VMINI;
        g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_EXTNAME;

        g_bootCfg.displayRes = OMAP_LCD_DEFAULT;

        g_bootCfg.opp_mode = BSP_OPM_SELECT-1;

        // select default boot device based on boot select switch setting
        dwSysBootCfg = INREG32(pStatusControlAddr);
        OALLog(L"INFO: SW4 boot setting: 0x%02x\r\n", dwSysBootCfg & 0x3f);

        switch (dwSysBootCfg & 0x3f)
        {
        /*
        case 0x24:
        case 0x26:
        case 0x3b:
            // 1st boot device is USB
            g_bootCfg.bootDevLoc.LogicalLoc = OMAP_USBHS_REGS_PA;
            g_bootCfg.kitlDevLoc.LogicalLoc = OMAP_USBHS_REGS_PA;
            break;
        */
        case 0x2c:
        case 0x2d:
            // 1st boot device is MMC1 (SD Card Boot)
            g_bootCfg.bootDevLoc.LogicalLoc = OMAP_MMCHS1_REGS_PA;
            break;          

        case 0x0c: 
            // 1st boot device is NAND
            g_bootCfg.bootDevLoc.LogicalLoc = BSP_NAND_REGS_PA + 0x20;
            break;

        default:
            // UART,  Ethernet Boot
            g_bootCfg.bootDevLoc.LogicalLoc = OMAP_CPGMAC_REGS_PA;
            g_bootCfg.kitlDevLoc.LogicalLoc = OMAP_CPGMAC_REGS_PA;
            break;
        }            
        if (g_bootCfg.kitlDevLoc.LogicalLoc == 0)
        {
            g_bootCfg.kitlDevLoc.LogicalLoc = BSP_LAN9311_REGS_PA;
        };
        g_bootCfg.deviceID = 0;
        g_bootCfg.osPartitionSize = IMAGE_WINCE_CODE_SIZE;
        wcscpy(g_bootCfg.filename, L"nk.bin");
        }

    // Initialize flash partitions if needed
    BLConfigureFlashPartitions(FALSE);

    // Initialize ARGS structure
    if ((pArgs->header.signature != OAL_ARGS_SIGNATURE) ||
        (pArgs->header.oalVersion != OAL_ARGS_VERSION) ||
        (pArgs->header.bspVersion != BSP_ARGS_VERSION))
        {
        memset(pArgs, 0, IMAGE_SHARE_ARGS_SIZE);
        }        
    
    // Save reset type
    dwTemp = INREG32(&pPrmGlobal->PRM_RSTST);
    if (dwTemp & (GLOBALWARM_RST /* actually SW reset */ | EXTERNALWARM_RST))
    {
        pArgs->coldBoot = FALSE;
    }
    else
    {
        pArgs->coldBoot = TRUE;
        OALLog(L"\r\n>>> Forcing cold boot (non-persistent registry and other data will be wiped) <<< \r\n");
    }
    
    // Don't force the boot menu, use default action unless user breaks
    // into menu
    bForceBootMenu = FALSE;
    
retryBootMenu:
    // Call configuration menu
    BLMenu(bForceBootMenu);
    
    // Update ARGS structure if necessary
    if ((pArgs->header.signature != OAL_ARGS_SIGNATURE) ||
        (pArgs->header.oalVersion != OAL_ARGS_VERSION) ||
        (pArgs->header.bspVersion != BSP_ARGS_VERSION))
        {
        pArgs->header.signature = OAL_ARGS_SIGNATURE;
        pArgs->header.oalVersion = OAL_ARGS_VERSION;
        pArgs->header.bspVersion = BSP_ARGS_VERSION;
        pArgs->kitl.flags = g_bootCfg.kitlFlags;
        pArgs->kitl.devLoc = g_bootCfg.kitlDevLoc;
        pArgs->kitl.ipAddress = g_bootCfg.ipAddress;
        pArgs->kitl.ipMask = g_bootCfg.ipMask;
        pArgs->kitl.ipRoute = g_bootCfg.ipRoute;
		memcpy(pArgs->kitl.mac,g_bootCfg.mac,sizeof(pArgs->kitl.mac));
		memcpy(pArgs->mac,g_bootCfg.mac,sizeof(pArgs->mac)); 
 	    pArgs->updateMode = FALSE;
        pArgs->deviceID = g_bootCfg.deviceID;
        pArgs->oalFlags = g_bootCfg.oalFlags;
        pArgs->dispRes = g_bootCfg.displayRes;
        pArgs->ECCtype = g_bootCfg.ECCtype; 
        pArgs->opp_mode = g_bootCfg.opp_mode; 
        }        
    
    // Initialize display
    BLShowLogo();

    // Image download depends on protocol
    g_eboot.bootDeviceType = OALKitlDeviceType(
        &g_bootCfg.bootDevLoc, g_bootDevices
        );

    switch (g_eboot.bootDeviceType)
        {
        case BOOT_SDCARD_TYPE:
            rc = BLSDCardDownload(g_bootCfg.filename);
            break;
        case OAL_KITL_TYPE_FLASH:
            rc = BLFlashDownload(&g_bootCfg, g_bootDevices);
            break;
        case OAL_KITL_TYPE_ETH:
            rc = BLEthDownload(&g_bootCfg, g_bootDevices);
            break;
        }
        
    if (rc == BL_ERROR)
    {
        // No automatic mode now, force the boot menu to appear
        bForceBootMenu = TRUE;
        goto retryBootMenu; 
    }   
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMLaunch
//
//  This function is the last one called by the boot framework and it is
//  responsible for to launching the image.
//
VOID
OEMLaunch(
   ULONG start, 
   ULONG size, 
   ULONG launch, 
   const ROMHDR *pRomHeader
    )
{
    BSP_ARGS *pArgs = OALCAtoUA(IMAGE_SHARE_ARGS_CA);

	UNREFERENCED_PARAMETER(pRomHeader);
	UNREFERENCED_PARAMETER(size);

	// Save Mac Address
	memcpy(pArgs->mac,g_bootCfg.mac,sizeof(pArgs->mac));

    OALMSG(OAL_FUNC, (
        L"+OEMLaunch(0x%08x, 0x%08x, 0x%08x, 0x%08x - %d/%d)\r\n", start, size,
        launch, pRomHeader, g_eboot.bootDeviceType, g_eboot.type
        ));

    // Depending on protocol there can be some action required
    switch (g_eboot.bootDeviceType)
        {
#if BUILDING_EBOOT_SD
        case BOOT_SDCARD_TYPE:            
            switch (g_eboot.type)
                {
#if 0
/*
                case DOWNLOAD_TYPE_FLASHRAM:
                    if (BLFlashDownload(&g_bootCfg, g_kitlDevices) != BL_JUMP)
                        {
                        OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: "
                            L"Image load from flash memory failed\r\n"
                            ));
                        goto cleanUp;
                        }
                    launch = g_eboot.launchAddress;
                    break;
*/
#endif
                case DOWNLOAD_TYPE_RAM:
                    launch = (UINT32)OEMMapMemAddr(start, launch);
                    break;
					
                case DOWNLOAD_TYPE_FLASHNAND:
                    if (BLFlashDownload(&g_bootCfg, g_kitlDevices) != BL_JUMP)
                        {
                        OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: "
                            L"Image load from flash memory failed\r\n"
                            ));
                        goto cleanUp;
                        }
                    launch = g_eboot.launchAddress;
                    break;
								
#if 0
/*
                case DOWNLOAD_TYPE_EBOOT:
                case DOWNLOAD_TYPE_XLDR:
                    OALMSG(OAL_INFO, (L"INFO: "
                        L"XLDR/EBOOT/IPL downloaded, spin forever\r\n"
                        ));
                    while (TRUE);
                    break;
*/
#endif
                default:
                    OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: Unknown download type, spin forever\r\n"));
                    for(;;);
                    break;
                }
            break;

#endif

        case OAL_KITL_TYPE_ETH:
            BLEthConfig(pArgs);
            switch (g_eboot.type)
                {
#ifdef IMGMULTIXIP
                case DOWNLOAD_TYPE_EXT:
#endif					
                case DOWNLOAD_TYPE_FLASHNAND:
				case DOWNLOAD_TYPE_FLASHNOR:
                    if (BLFlashDownload(&g_bootCfg, g_kitlDevices) != BL_JUMP)
                        {
                        OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: "
                            L"Image load from flash memory failed\r\n"
                            ));
                        goto cleanUp;
                        }
                    launch = g_eboot.launchAddress;
                    break;

                case DOWNLOAD_TYPE_RAM:
                    launch = (UINT32)OEMMapMemAddr(start, launch);
                    break;

                case DOWNLOAD_TYPE_EBOOT:
                case DOWNLOAD_TYPE_XLDR:
                    OALMSG(OAL_INFO, (L"INFO: "
                        L"XLDR/EBOOT/IPL downloaded, spin forever\r\n"
                        ));
                    for(;;);
                    break;

				case DOWNLOAD_TYPE_LOGO:
                    OALMSG(OAL_INFO, (L"INFO: "
                        L"Splashcreen logo downloaded, spin forever\r\n"
                        ));
                    for(;;);
                    break;

                default:
                    OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: Unknown download type, spin forever\r\n"));
                    for(;;);
                    break;
                }
            break;

        default:        
            launch = g_eboot.launchAddress;
            break;
        }
	
#ifndef BSP_NO_NAND_IN_SDBOOT
    if ((g_bootCfg.flashNKFlags & ENABLE_FLASH_NK) && 
        /* if loading from NAND then do not need to flash NAND again */      
        (g_eboot.bootDeviceType != OAL_KITL_TYPE_FLASH) &&
	    (start != (IMAGE_WINCE_CODE_CA + NAND_ROMOFFSET)) &&
	    (start != (IMAGE_WINCE_CODE_CA + NOR_ROMOFFSET))) {
            if( !WriteFlashNK(start, size))
	            OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: "
	                L"Flash NK.bin failed, start=%x\r\n", start
	                ));
    	    }
#endif

    // Check if we get launch address
    if (launch == (UINT32)INVALID_HANDLE_VALUE)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: "
            L"Unknown image launch address, spin forever\r\n"
            ));
        for(;;);
        }        

    // Print message, flush caches and jump to image
    OALLog(
        L"Launch Windows CE image by jumping to 0x%08x...\r\n\r\n", launch
        );

	OEMDeinitDebugSerial();
    OEMPlatformDeinit();
    JumpTo(OALVAtoPA((UCHAR*)launch));

cleanUp:
    return;
}

//------------------------------------------------------------------------------
//
//  Function:   OEMMultiBinNotify
//
VOID
OEMMultiBinNotify(
    MultiBINInfo *pInfo
    )
{
    BOOL rc = FALSE;
    UINT32 base = OALVAtoPA((UCHAR*)IMAGE_WINCE_CODE_CA);
    UINT32 start, length;
    UINT32 ix;


    OALMSGS(OAL_FUNC, (
        L"+OEMMultiBinNotify(0x%08x -> %d)\r\n", pInfo, pInfo->dwNumRegions
        ));
    OALMSG(OAL_INFO, (
        L"Download file information:\r\n"
        ));
    OALMSG(OAL_INFO, (
        L"-----------------------------------------------------------\r\n"
        ));

    // Copy information to EBOOT structure and set also save address
    g_eboot.numRegions = pInfo->dwNumRegions;
    for (ix = 0; ix < pInfo->dwNumRegions; ix++)
        {
        g_eboot.region[ix].start = pInfo->Region[ix].dwRegionStart;
        g_eboot.region[ix].length = pInfo->Region[ix].dwRegionLength;
        g_eboot.region[ix].base = base;
        base += g_eboot.region[ix].length;
        OALMSG(OAL_INFO, (
            L"[%d]: Address=0x%08x  Length=0x%08x  Save=0x%08x\r\n",
            ix, g_eboot.region[ix].start, g_eboot.region[ix].length,
            g_eboot.region[ix].base
            ));
        }
    OALMSG(OAL_INFO, (
        L"-----------------------------------------------------------\r\n"
        ));

#ifndef IMGMULTIXIP

    //Determine type of image downloaded
    if (g_eboot.numRegions > 1) 
    {
        OALMSG(OAL_ERROR, (L"ERROR: MultiXIP image is not supported\r\n"));
        goto cleanUp;
    }
#endif

    base = g_eboot.region[0].base;
    start = g_eboot.region[0].start;
    length = g_eboot.region[0].length; 
    
    if (start == IMAGE_XLDR_CODE_PA)
        {
        g_eboot.type = DOWNLOAD_TYPE_XLDR;
        memset(OALPAtoCA(base), 0xFF, length);
        } 
    else if (start == IMAGE_EBOOT_CODE_CA)
        {
        g_eboot.type = DOWNLOAD_TYPE_EBOOT;
        memset(OALPAtoCA(base), 0xFF, length);
        }
    else if (start == (IMAGE_WINCE_CODE_CA + NAND_ROMOFFSET))
        {
        g_eboot.type = DOWNLOAD_TYPE_FLASHNAND;
        memset(OALPAtoCA(base), 0xFF, length);
        } 
#ifdef IMGMULTIXIP
    else if (start == (IMAGE_WINCE_EXT_CA))
        {
        g_eboot.type = DOWNLOAD_TYPE_EXT;
        memset(OALPAtoCA(base), 0xFF, length);
        } 
#endif	
    else if (start == (IMAGE_WINCE_CODE_CA + NOR_ROMOFFSET))
        {
        g_eboot.type = DOWNLOAD_TYPE_FLASHNOR;
        memset(OALPAtoCA(base), 0xFF, length);
        } 
	else if (start == 0) // Probably a NB0 file, let's fint out
		{
		// Convert the file name to lower case
		CHAR szFileName[MAX_PATH];
		int i = 0;
		int fileExtPos = 0;

		while ((pInfo->Region[0].szFileName[i] != '\0') && (i < MAX_PATH))
		{
			if((pInfo->Region[0].szFileName[i] >= 'A') && (pInfo->Region[0].szFileName[i] <= 'Z')) 
			{
				szFileName[i] = (pInfo->Region[0].szFileName[i] - 'A' + 'a'); 
			}
			else
			{
				szFileName[i] = pInfo->Region[0].szFileName[i];
			}

			// Keep track of file extension position
			if (szFileName[i] == '.')
			{
				fileExtPos = i;
			}
			i++;
		}

		// Copy string terminator as well
		szFileName[i] = pInfo->Region[0].szFileName[i];

		// Check if we support this file
		if (strncmp(szFileName, LOGO_NB0_FILE, LOGO_NB0_FILE_LEN) == 0)
		{
			// Remap the start address to the correct NAND location of the logo
			g_eboot.region[0].start = IMAGE_XLDR_BOOTSEC_NAND_SIZE + IMAGE_EBOOT_BOOTSEC_NAND_SIZE;
			g_eboot.type = DOWNLOAD_TYPE_LOGO;
		}
		else
		{
		    OALMSG(OAL_ERROR, (L"Unsupported downloaded file\r\n"));
			goto cleanUp;
		}
		}
    else 
        {
        g_eboot.type = DOWNLOAD_TYPE_RAM;
        }

    OALMSG(OAL_INFO, (
        L"Download file type: %d\r\n", g_eboot.type
    ));
    
    rc = TRUE;

cleanUp:
    if (!rc) 
        {
        OALMSG(OAL_ERROR, (L"Spin for ever...\r\n"));
        for(;;);
        }
    OALMSGS(OAL_FUNC, (L"-OEMMultiBinNotify\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  OEMMapMemAddr
//
//  This function maps image relative address to memory address. It is used
//  by boot loader to verify some parts of downloaded image.
//
//  EBOOT mapping depends on download type. Download type is
//  set in OMEMultiBinNotify.
//
UINT8* 
OEMMapMemAddr(
    DWORD image,
    DWORD address
    )
{
    UINT8 *pAddress = NULL;

    OALMSG(OAL_FUNC, (L"+OEMMapMemAddr(0x%08x, 0x%08x)\r\n", image, address));
    
    switch (g_eboot.type) {

        
    case DOWNLOAD_TYPE_XLDR:
    case DOWNLOAD_TYPE_EBOOT:   
	case DOWNLOAD_TYPE_LOGO:
        //  Map to scratch RAM prior to flashing
        pAddress = (UINT8*)g_eboot.region[0].base + (address - image);
        break;

    case DOWNLOAD_TYPE_RAM:            
        //  RAM based NK.BIN and EBOOT.BIN files are given in virtual memory addresses
        pAddress = (UINT8*)address;
        break;

    case DOWNLOAD_TYPE_FLASHNAND:
        pAddress = (UINT8*) (address - NAND_ROMOFFSET);
        break;

	case DOWNLOAD_TYPE_FLASHNOR:
        pAddress = (UINT8*) (address - NOR_ROMOFFSET);
        break;
		
#ifdef IMGMULTIXIP
        case DOWNLOAD_TYPE_EXT:
        pAddress = (UINT8*) (address);
        break;
#endif

    default:
        OALMSG(OAL_ERROR, (L"ERROR: OEMMapMemAddr: "
            L"Invalid download type %d\r\n", g_eboot.type
        ));

    }


    OALMSGS(OAL_FUNC, (L"-OEMMapMemAddr(pAddress = 0x%08x)\r\n", pAddress));
    return pAddress;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMIsFlashAddr
//
//  This function determines whether the address provided lies in a platform's
//  flash or RAM address range.
//
//  EBOOT decision depends on download type. Download type is
//  set in OMEMultiBinNotify.
//
BOOL
OEMIsFlashAddr(
    ULONG address
    )
{
    BOOL rc;

	UNREFERENCED_PARAMETER(address);

    OALMSG(OAL_FUNC, (L"+OEMIsFlashAddr(0x%08x)\r\n", address));

    // Depending on download type
    switch (g_eboot.type)
        {
        case DOWNLOAD_TYPE_XLDR:
        case DOWNLOAD_TYPE_EBOOT:
		case DOWNLOAD_TYPE_LOGO:
		case DOWNLOAD_TYPE_FLASHNAND:
        case DOWNLOAD_TYPE_FLASHNOR:
        case DOWNLOAD_TYPE_EXT:
            rc = TRUE;
            break;
        default:
            rc = FALSE;
            break;
        }

    OALMSG(OAL_FUNC, (L"-OEMIsFlashAddr(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:   OEMReadData
//
//  This function is called to read data from the transport during
//  the download process.
//
BOOL
OEMReadData(
    ULONG size, 
    UCHAR *pData
    )
{
    BOOL rc = FALSE;
    switch (g_eboot.bootDeviceType)
        {
        #if BUILDING_EBOOT_SD
        case BOOT_SDCARD_TYPE:
            rc = BLSDCardReadData(size, pData);
            break;
        #endif
        case OAL_KITL_TYPE_ETH:
            rc = BLEthReadData(size, pData);
            break;
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMShowProgress
//
//  This function is called during the download process to visualise
//  download progress.
//
VOID
OEMShowProgress(
    ULONG packetNumber
    )
{
    UNREFERENCED_PARAMETER(packetNumber);
    RETAILMSG(1,(TEXT(".")));
}

//------------------------------------------------------------------------------
//
//  Function:  OALGetTickCount
//
UINT32
OALGetTickCount(
    )
{
    OMAP_GPTIMER_REGS *pGPTimerRegs = OALPAtoUA(OMAP_GPTIMER1_REGS_PA);
    return INREG32(&pGPTimerRegs->TCRR) >> 5;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMEthGetSecs
//
//  This function returns relative time in seconds.
//
DWORD
OEMEthGetSecs(
    )
{
    return OALGetTickCount()/1000;
}


void
GetDisplayResolutionFromBootArgs(
    DWORD * pDispRes
    )
{
    *pDispRes=g_bootCfg.displayRes;
}

BOOL
IsDVIMode()
{
    DWORD dispRes;    
    GetDisplayResolutionFromBootArgs(&dispRes);
    if (dispRes==OMAP_LCD_DEFAULT)
        return FALSE;
    else
        return TRUE;        
}

DWORD 
ConvertCAtoPA(DWORD * va)
{     
    return OALVAtoPA(va);
}

//------------------------------------------------------------------------------

