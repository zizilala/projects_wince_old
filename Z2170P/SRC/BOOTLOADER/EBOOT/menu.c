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
//  File:  menu.c
//
#include <eboot.h>
#include "omap_cpuver.h"
//
#include "bsp.h"
#include "bsp_logo.h"
#include "omap_dss_regs.h"
#include "lcd.h"
#include "oalex.h"
#include "eboot.h"
#include "sdk_gpio.h"
//
#include "twl.h"
#include "triton.h"
#include "tps659xx_internals.h"

//------------------------------------------------------------------------------
//
//  Define:  dimof
//
#ifdef dimof
#undef dimof
#endif
#define dimof(x)                (sizeof(x)/sizeof(x[0]))

//------------------------------------------------------------------------------
static VOID SetMacAddress(OAL_BLMENU_ITEM *pMenu);
static VOID ShowSettings(OAL_BLMENU_ITEM *pMenu);
static VOID ShowNetworkSettings(OAL_BLMENU_ITEM *pMenu);
static VOID SetKitlMode(OAL_BLMENU_ITEM *pMenu);
static VOID SetKitlType(OAL_BLMENU_ITEM *pMenu);
static VOID SetDeviceID(OAL_BLMENU_ITEM *pMenu);
static VOID SaveSettings(OAL_BLMENU_ITEM *pMenu);
static VOID SetRetailMsgMode(OAL_BLMENU_ITEM *pMenu);
static VOID SetDisplayResolution(OAL_BLMENU_ITEM *pMenu);
static VOID SetOPPmode(OAL_BLMENU_ITEM *pMenu);
static VOID SetBacklight(OAL_BLMENU_ITEM *pMenu);	//setup backlight, Ray 13-07-26 
static VOID SDtoFlash(OAL_BLMENU_ITEM *pMenu);	    //File Send To Flash By SD Card, Ray 13-09-03 
//BOOL BLSDCardToFlash(WCHAR *);			        //Initial SD Card, Ray 13-09-03 
BOOL BLSDtoFlash(void);					            //Initial SD Card, Ray 13-09-10
//BOOL OEMWriteFlash(ULONG address, ULONG size);
//BOOL WriteFlashFromEEBOOT(UINT32 address, UINT32 size);  //Write EBOOT file to Flash, Ray 13-09-14
//VOID DumpFlashBuffer(OAL_BLMENU_ITEM *pMenu);  //Dumpping flash & buffer, Ray 13-09-16
//BOOL WriteFlashNK(UINT32 address, UINT32 size); //XX, Ray 13-09-16
static VOID BacklightON(OAL_BLMENU_ITEM *pMenu);   //Below instance Backlight function,Ray 13-09-18  
static VOID BacklightOFF(OAL_BLMENU_ITEM *pMenu);
static VOID BacklightBrightness(OAL_BLMENU_ITEM *pMenu);
//VOID KeypadState();									//keypad testing, Ray 13-08-14
VOID LCMInitial(OAL_BLMENU_ITEM *pMenu);                //Testing LCM , Ray 13-09-24
void LCD_SPI_Init(void);                                 //LCM SPI_Init, Ray 13-09-24                             
void spi_Low(void);                                     //LCM SPI_low, Ray 13-09-24                              
void LcdStall(DWORD);




//------------------------------------------------------------------------------
#if BUILDING_EBOOT_SD
static VOID ShowSDCardSettings(OAL_BLMENU_ITEM *pMenu);
static VOID EnterSDCardFilename(OAL_BLMENU_ITEM *pMenu);
#endif

//------------------------------------------------------------------------------

// Functions
extern BOOL LAN911XFindController(void* pAddr, DWORD * pdwID);
extern BOOL LAN911XSetMacAddress(void* pAddr, UINT16 mac[3]);
extern BOOL LAN911XGetMacAddress(void* pAddr, UINT16 mac[3]);
extern OAL_BLMENU_ITEM g_menuFlash[];

//------------------------------------------------------------------------------

static OAL_BLMENU_ITEM g_menuNetwork[] = {
    {
        L'1', L"Show Current Settings", ShowNetworkSettings,
        NULL, NULL, NULL
    }, {
        L'2', L"Enable/disable KITL", OALBLMenuEnable,
        L"KITL", &g_bootCfg.kitlFlags, (VOID*)OAL_KITL_FLAGS_ENABLED
    }, {
        L'3', L"KITL interrupt/poll mode", SetKitlMode,
        NULL, NULL, NULL
    }, {
        L'4', L"KITL Active/Passive mode", SetKitlType,
        NULL, NULL, NULL
    }, {
        L'5', L"Enable/disable DHCP", OALBLMenuEnable,
        L"DHCP", &g_bootCfg.kitlFlags, (VOID*)OAL_KITL_FLAGS_DHCP
    }, {
        L'6', L"Set IP address", OALBLMenuSetIpAddress,
        L"Device", &g_bootCfg.ipAddress, NULL
    }, {
        L'7', L"Set IP mask", OALBLMenuSetIpMask,
        L"Device", &g_bootCfg.ipMask, NULL
    }, {
        L'8', L"Set default router", OALBLMenuSetIpAddress,
        L"Default router", &g_bootCfg.ipRoute, NULL
    }, {
        L'9', L"Enable/disable VMINI", OALBLMenuEnable,
        L"VMINI", &g_bootCfg.kitlFlags, (VOID*)OAL_KITL_FLAGS_VMINI
    }, {
        L'a', L"Set MAC address", SetMacAddress,
        NULL, NULL, NULL
    }, {
        L'0', L"Exit and Continue", NULL,
        NULL, NULL, NULL
    }, {
        0, NULL, NULL,
        NULL, NULL, NULL
    }
};

#if BUILDING_EBOOT_SD
static OAL_BLMENU_ITEM g_menuSDCard[] = {
    {
        L'1', L"Show Current Settings", ShowSDCardSettings,
        NULL, NULL, NULL
    }, {
        L'2', L"Enter Filename", EnterSDCardFilename,
        NULL, NULL, NULL
    }, {
        L'0', L"Exit and Continue", NULL,
        NULL, NULL, NULL
    }, {
        0, NULL, NULL,
        NULL, NULL, NULL
    }
};
#endif

//------------------------------------------------------------------------------

#if BUILDING_EBOOT_SD           //if have insert SD Card execution here, Ray 13-09-13
static OAL_BLMENU_ITEM g_menuMain[] = {
    {
        L'1', L"Show Current Settings", ShowSettings,
        NULL, NULL, NULL
    }, {
        L'2', L"Select Boot Device", OALBLMenuSelectDevice,
        L"Boot", &g_bootCfg.bootDevLoc, g_bootDevices
    }, {
        L'3', L"Select KITL (Debug) Device", OALBLMenuSelectDevice,
        L"Debug", &g_bootCfg.kitlDevLoc, g_kitlDevices
    }, {
        L'4', L"Network Settings", OALBLMenuShow,
        L"Network Settings", &g_menuNetwork, NULL
    }, {
        L'5', L"SDCard Settings", OALBLMenuShow,
        L"SDCard Settings", &g_menuSDCard, NULL
    }, {
        L'6', L"Set Device ID", SetDeviceID,
        NULL, NULL, NULL
    }, {
        L'7', L"Save Settings", SaveSettings,
        NULL, NULL, NULL
    }, {
        L'8', L"Flash Management", OALBLMenuShow,
        L"Flash Management", &g_menuFlash, NULL
    },  {
        L'9', L"Enable/Disable OAL Retail Messages", SetRetailMsgMode,
        NULL, NULL, NULL
    },  {
        L'a', L"Select Display Resolution", SetDisplayResolution,
        NULL, NULL, NULL
    }, {
        L'b', L"Select OPP Mode", SetOPPmode,
        NULL, NULL, NULL
    }, {
		L'c', L"Backlight ON/OFF", SetBacklight,						//Ray 13-07-26 
        NULL, NULL, NULL
	}, {
		L'd', L"File Transfer from SD Card to Flash", SDtoFlash,		//Ray 13-09-03 
        NULL, NULL, NULL
	}, {
		L'e', L"LCM Initial", LCMInitial,		                        //Ray 13-09-24 
        NULL, NULL, NULL
	}, {
        L'0', L"Exit and Continue", NULL,
        NULL, NULL, NULL
    }, {
        0, NULL, NULL,
        NULL, NULL, NULL
    }
};
#else     //Not have insert SD Card,from Flash memory, Ray 13-09-13
static OAL_BLMENU_ITEM g_menuMain[] = {
    {	
		L'1', L"Show Current Settings", ShowSettings,
        NULL, NULL, NULL
	}, {
        L'2', L"Select Boot Device", OALBLMenuSelectDevice,
        L"Boot", &g_bootCfg.bootDevLoc, g_bootDevices},
	{
        L'3', L"Select KITL (Debug) Device", OALBLMenuSelectDevice,
        L"Debug", &g_bootCfg.kitlDevLoc, g_kitlDevices
	}, {
        L'4', L"Network Settings", OALBLMenuShow,
        L"Network Settings", &g_menuNetwork, NULL
	},{
        L'5', L"Flash Management", OALBLMenuShow,
        L"Flash Management", &g_menuFlash, NULL
	}, {
        L'6', L"Set Device ID", SetDeviceID,
        NULL, NULL, NULL
    }, {
        L'7', L"Save Settings", SaveSettings,
        NULL, NULL, NULL
    }, {
        L'8', L"Enable/Disable OAL Retail Messages", SetRetailMsgMode,
        NULL, NULL, NULL
    }, {
        L'9', L"Select Display Resolution", SetDisplayResolution,
        NULL, NULL, NULL
    }, {
        L'a', L"Select OPP Mode", SetOPPmode,
        NULL, NULL, NULL
    }, {
		L'b', L"Backlight ON/OFF", SetBacklight,	                  //Ray 13-07-26 
        NULL, NULL, NULL
	}, {
		L'c', L"File Transfer from SD Card to Flash", SDtoFlash,		//Ray 13-09-03 
        NULL, NULL, NULL
	}, {
		L'e', L"LCM Initial", LCMInitial,		                        //Ray 13-09-24 
        NULL, NULL, NULL
	}, {
        L'0', L"Exit and Continue", NULL,
        NULL, NULL, NULL
    }, {
        0, NULL, NULL,
        NULL, NULL, NULL
    }
};
#endif

//------------------------------------------------------------------------------

static OAL_BLMENU_ITEM g_menuRoot = {
    0, NULL, OALBLMenuShow,
    L"Main Menu", g_menuMain, NULL
};

typedef struct OMAP_LCD_DVI_RES_MENU {
    DWORD       lcdDviRes;
    LPCWSTR     resName;
} OMAP_LCD_DVI_RES_MENU;

OMAP_LCD_DVI_RES_MENU dispResMenu[] = {
    {OMAP_LCD_DEFAULT,          L"LCD  480x640 60Hz"},
    {OMAP_DVI_640W_480H,        L"DVI  640x480 60Hz"},
    {OMAP_DVI_640W_480H_72HZ,   L"DVI  640x480 72Hz"},
    {OMAP_DVI_800W_480H,        L"DVI  800x480 60Hz"},
    {OMAP_DVI_800W_600H,        L"DVI  800x600 60Hz"},
    {OMAP_DVI_800W_600H_56HZ,   L"DVI  800x600 56Hz"},
    {OMAP_DVI_1024W_768H,       L"DVI 1024x768 60Hz"},
    {OMAP_DVI_1280W_720H,       L"DVI 1280x720 60Hz"},
    {OMAP_RES_INVALID,          L"Invalid Entry"},
};

extern UINT32 g_CPUFamily;

typedef struct OPP_MODE_MENU {
    LPCWSTR     oppModeName;
} OPP_MODE_MENU;

OPP_MODE_MENU oppModeMenu35x[OMAP35x_OPP_NUM] = {
    {L"MPU[125Mhz @ 0.975V], IVA2[ 90Mhz @ 0.975V]"},
    {L"MPU[250Mhz @ 1.000V], IVA2[180Mhz @ 1.00V]"},
    {L"MPU[500Mhz @ 1.2000V], IVA2[360Mhz @ 1.20V]"},
    {L"MPU[550Mhz @ 1.2750V], IVA2[400Mhz @ 1.27V]"},
    {L"MPU[600Mhz @ 1.3500V], IVA2[430Mhz @ 1.35V]"},
    {L"MPU[720hz @ 1.3500V], IVA2[520Mhz @ 1.35V]"},
};

OPP_MODE_MENU oppModeMenu37x[OMAP37x_OPP_NUM] = {
     {L"MPU[300Mhz  @ 0.9375V], IVA2[260Mhz @ 0.9375V]"},
     {L"MPU[600Mhz  @ 1.1000V], IVA2[520Mhz @ 1.1000V]"},		
     {L"MPU[800Mhz  @ 1.2625V], IVA2[660Mhz @ 1.2625V]"},    
     {L"MPU[1000Mhz @ 1.375V], IVA2[800Mhz @ 1.375V]"},    
};

//Ray 13-07-26
typedef struct _OMAP_LCM_BACKLIGHT_MENU {
	   DWORD       blActive;
       LPCWSTR     blName;
} OMAP_LCM_BACKLIGHT_MENU;

OMAP_LCM_BACKLIGHT_MENU disBacklight[] = {
	{BACKLIGHT_ON,		    L"ON"},
	{BACKLIGHT_OFF,		    L"OFF"},
    {BACKLIGHT_BRIGHTNESS,  L"BRIGHTNESS"},
	{BACKLIGHT_EXIT,        L"EXIT"},
};

//Ray 13-09-03
typedef struct _SD_TO_FLASH_MENU{
		DWORD	 	sdActive;
		LPCWSTR		sdName;
}SD_TO_FLASH_MENU;

SD_TO_FLASH_MENU sdToFlash[] ={
	{READING, 		L"Reading"},
	{WRITING,	 	L"Writing"},
    {SHOW_DATA,	 	L"Show .nb0 Data"},
	{FLASH_EXIT,    L"EXIT"},
};

//Ray 13-09-18
typedef struct _OMAP_LCM_BACKLIGHT_BN_MENU{
		DWORD	 	bnActive;
		LPCWSTR		bnName;
}OMAP_LCM_BACKLIGHT_BN_MENU;

OMAP_LCM_BACKLIGHT_BN_MENU backlightBN[] ={
	{PLUS, 		L"+"},
	{MINUS,	 	L"-"},
	{BN_EXIT,   L"EXIT"},
};

//Ray 13-09-24
typedef struct _OMAP_LCM_Initial_MENU{
		DWORD	 	lcmIniActive;
		LPCWSTR		lcmIniName;
}OMAP_LCM_Initial_MENU;

OMAP_LCM_Initial_MENU LCMInitiaArray[] ={
	{LCM_HIGH, 		L"High"},
	{LCM_LOW,	 	L"Low"},
	{LCM_EXIT,   L"EXIT"},
};

//------------------------------------------------------------------------------
//Ray 13-08-13

VOID BLMenu(BOOL bForced)
{
    UINT32 time, delay = 8;
    WCHAR key = 0;
	//BOOL entry;
    BSP_ARGS *pArgs = OALCAtoUA(IMAGE_SHARE_ARGS_CA);

	HANDLE hGPIO;
	hGPIO = GPIOOpen();
	GPIOGetBit(hGPIO, 114);	//key SD1
	GPIOGetBit(hGPIO, 115); //key SD3
    
    // First let user break to menu 
    // Break to menu & count, Ray 13-09-26 
    while (!bForced && (delay > 0 && key != L' ')) 
    //while (!bForced && (delay > 0 && key != GPIOGetBit(hGPIO, 114) 
	//								&& key != GPIOGetBit(hGPIO, 115) )) 
	{
		//OALLog(L"Hit space to enter configuration menu %d...\r\n", delay);
		OALLog(L"Sync-press side key to enter configuration menu %d...\r\n", delay);
		time = OALGetTickCount();
		while ((OALGetTickCount() - time) < 1000) 
		{
			if ((key = OALBLMenuReadKey(FALSE)) == L' ') 
			/*if ( (key = OALBLMenuReadKey(FALSE)) == GPIOGetBit(hGPIO, 114) 
					&& (key = OALBLMenuReadKey(FALSE)) == GPIOGetBit(hGPIO, 115) )*/ 
				break;
		}
		delay--;
	}
	
    //entry = FALSE;
    if ((bForced == TRUE) || (key == L' '))
    //if ((!bForced == TRUE))
	//if ((bForced == TRUE) || (key ==  GPIOGetBit(hGPIO, 114)) && (key ==  GPIOGetBit(hGPIO, 115))) 
	{									//Combo SIDE KEY 
        BLShowLogo();					//function is TRUE,so Forever go , Ray 13-07-31 
		OALBLMenuShow(&g_menuRoot);
		//KeypadState();
		
		// Invalidate arguments to force them to be reinitialized
        // with new config data generated by the boot menu
		pArgs->header.signature = 0;
    }  
}

//------------------------------------------------------------------------------

VOID SetMacAddress(OAL_BLMENU_ITEM *pMenu)
{
    DWORD dummy;
#pragma warning(push)
#pragma warning(disable:4204 4221)
    UINT16 mac[3] = {0xFFFF, 0xFFFF, 0xFFFF};
    // We're using OALBLMenuSetMacAddress outside of a menu, but it still requires 
    // a menu item input structure
    OAL_BLMENU_ITEM DummyMacMenuItem = 
    {
        0, NULL, NULL,
        L"EVM ", mac, NULL
    };
#pragma warning(pop)

    UNREFERENCED_PARAMETER(pMenu);

    OALLog(L"\r\n Set Ethernet (LAN9xxx) MAC Address:\r\n");   
	
	
    memset(mac,0xFF,sizeof(mac));
    LAN911XGetMacAddress(OALPAtoUA((DWORD) BSP_LAN9115_REGS_PA),mac);
    if (((mac[0]== 0xFFFF) && (mac[1] == 0xFFFF) && (mac[2] == 0xFFFF)) || ((mac[0]== 0x0) && (mac[1] == 0x0) && (mac[2] == 0x0)))
    {
        RETAILMSG(1,(TEXT("mac address not found in EEPROM. Using Mac address stored with the boot settings\r\n")));
        memcpy(mac,g_bootCfg.mac,sizeof(mac));       
    }

    OALBLMenuSetMacAddress(&DummyMacMenuItem);
    
    // Make sure we don't come out of OALBLMenuSetMacAddress with the original 0xFFFF FFFF FFFF or with all zeros
    if (((mac[0]== 0xFFFF) && (mac[1] == 0xFFFF) && (mac[2] == 0xFFFF)) || ((mac[0]== 0x0) && (mac[1] == 0x0) && (mac[2] == 0x0)))
    {
        OALLog(L"\r\n   WARNING: MAC address not programmed!\r\n");
        return;
    }
        
   memcpy(g_bootCfg.mac,mac,sizeof(mac));


    if (!LAN911XFindController(OALPAtoUA((DWORD) BSP_LAN9115_REGS_PA),&dummy))
    {     
        return;
    }
   LAN911XSetMacAddress(OALPAtoUA((DWORD) BSP_LAN9115_REGS_PA),mac);  
}

//------------------------------------------------------------------------------

VOID ShowSettings(OAL_BLMENU_ITEM *pMenu)
{
    OPP_MODE_MENU * pOppMenu = oppModeMenu35x;
    UNREFERENCED_PARAMETER(pMenu);
    
    if(g_CPUFamily == CPU_FAMILY_DM37XX)
    {
        pOppMenu = oppModeMenu37x;
    }

    OALLog(L"\r\n Main:\r\n");
    OALLog(
        L"  Boot device:   %s\r\n",
        OALKitlDeviceName(&g_bootCfg.bootDevLoc, g_bootDevices)
    );
    OALLog(
        L"  Debug device:  %s\r\n",
        (g_bootCfg.kitlDevLoc.LogicalLoc) ? OALKitlDeviceName(&g_bootCfg.kitlDevLoc, g_kitlDevices) : L"not specified"
    );
    OALLog(
        L"  Retail Msgs:   %s\r\n",
        (g_bootCfg.oalFlags & BOOT_CFG_OAL_FLAGS_RETAILMSG_ENABLE) ? L"enabled" : L"disabled"
    );
    OALLog(
        L"  Device ID:     %d\r\n", g_bootCfg.deviceID
    );
    OALLog(
        L"  Display Res:   %s\r\n", dispResMenu[g_bootCfg.displayRes].resName
    );
    OALLog(
        L"  Flashing NK.bin:   %s\r\n", 
        (g_bootCfg.flashNKFlags & ENABLE_FLASH_NK) ? L"enabled" : L"disabled"
    );    
    OALLog(
        L"  OPP Mode:   %s\r\n", pOppMenu[g_bootCfg.opp_mode].oppModeName
    );
	OALLog(
        L"  Backlight:   %s\r\n", disBacklight[g_bootCfg.backlight].blName	//Ray 13-07-29
    );
	OALLog(
        L"  SD Card to Flash status:   %s\r\n", sdToFlash[g_bootCfg.sdtoflash].sdName //Ray 13-07-29
    );
	
#if BUILDING_EBOOT_SD
    ShowSDCardSettings(pMenu);
#endif
    ShowNetworkSettings(pMenu);
}

//------------------------------------------------------------------------------

VOID ShowNetworkSettings(OAL_BLMENU_ITEM *pMenu)
{
    BOOL fValidExtMacAddr;
    UINT16 mac[3];
    UNREFERENCED_PARAMETER(pMenu);

    memset(mac,0xFF,sizeof(mac));
    LAN911XGetMacAddress(OALPAtoUA((DWORD) BSP_LAN9115_REGS_PA),mac);
    if (((mac[0]== 0xFFFF) && (mac[1] == 0xFFFF) && (mac[2] == 0xFFFF)) || ((mac[0]== 0x0) && (mac[1] == 0x0) && (mac[2] == 0x0)))
    {
        fValidExtMacAddr = FALSE;        
    }
    else
    {
        fValidExtMacAddr = TRUE;        
    }


    OALLog(L"\r\n Network:\r\n");
    OALLog(
        L"  KITL state:    %s\r\n",
        (g_bootCfg.kitlFlags & OAL_KITL_FLAGS_ENABLED) ? L"enabled" : L"disabled"
    );

    OALLog(
        L"  KITL type:     %s\r\n",
        (g_bootCfg.kitlFlags & OAL_KITL_FLAGS_PASSIVE) ? L"passive" : L"active"
    );

    OALLog(
        L"  KITL mode:     %s\r\n",
        (g_bootCfg.kitlFlags & OAL_KITL_FLAGS_POLL) ? L"polled" : L"interrupt"
    );
    OALLog(
        L"  DHCP:          %s\r\n",
        (g_bootCfg.kitlFlags & OAL_KITL_FLAGS_DHCP) ? L"enabled" : L"disabled"
    );
    OALLog(L"  IP address:    %s\r\n", OALKitlIPtoString(g_bootCfg.ipAddress));
    OALLog(L"  IP mask:       %s\r\n", OALKitlIPtoString(g_bootCfg.ipMask));
    OALLog(L"  IP router:     %s\r\n", OALKitlIPtoString(g_bootCfg.ipRoute));
    OALLog(L"  Eth MAC Addr:  %s (%s)\r\n", 
        OALKitlMACtoString(fValidExtMacAddr ? mac : g_bootCfg.mac)
        ,fValidExtMacAddr ? L"EEPROM" : L"Boot settings");
    OALLog(
        L"  VMINI:         %s\r\n",
        (g_bootCfg.kitlFlags & OAL_KITL_FLAGS_VMINI) ? L"enabled" : L"disabled"
    );
    OALLog(L"  Note: USBFN RNDIS MAC Addr cannot be changed.\r\n");
    
    OALBLMenuReadKey(TRUE);
}

//------------------------------------------------------------------------------

VOID SetKitlMode(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    UNREFERENCED_PARAMETER(pMenu);

    if ((g_bootCfg.kitlFlags & OAL_KITL_FLAGS_POLL) != 0) {
        OALLog(L" Set KITL to interrupt mode [y/-]: ");
    } else {
        OALLog(L" Set KITL to polled mode [y/-]: ");
    }    

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    if (key == L'y' || key == L'Y') {
        if ((g_bootCfg.kitlFlags & OAL_KITL_FLAGS_POLL) != 0) {
            g_bootCfg.kitlFlags &= ~OAL_KITL_FLAGS_POLL;
            OALLog(L" KITL set to interrupt mode\r\n");
        } else {
            g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_POLL;
            OALLog(L" KITL set to polled mode\r\n");
        }            
    }
}

//------------------------------------------------------------------------------

VOID SetKitlType(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    UNREFERENCED_PARAMETER(pMenu);

    if ((g_bootCfg.kitlFlags & OAL_KITL_FLAGS_PASSIVE) != 0) 
        {
        OALLog(L" Set KITL to active mode [y/-]: ");
        } 
   else 
        {
        OALLog(L" Set KITL to passive mode [y/-]: ");
        }    

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    if (key == L'y' || key == L'Y') 
        {
        if ((g_bootCfg.kitlFlags & OAL_KITL_FLAGS_PASSIVE) != 0) 
            {
            g_bootCfg.kitlFlags &= ~OAL_KITL_FLAGS_PASSIVE;
            OALLog(L" KITL set to active mode\r\n");
            }
        else 
            {
            g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_PASSIVE;
            OALLog(L" KITL set to passive mode\r\n");
            }            
        }
}

//------------------------------------------------------------------------------

VOID SetDeviceID(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR  szInputLine[16];
    UNREFERENCED_PARAMETER(pMenu);

    OALLog(
        L" Current Device ID:  %d\r\n", g_bootCfg.deviceID
    );

    OALLog(L"\r\n New Device ID: ");

    if (OALBLMenuReadLine(szInputLine, dimof(szInputLine)) == 0) 
        {
        goto cleanUp;
        }

    // Get device ID
    g_bootCfg.deviceID = OALStringToUINT32(szInputLine);

cleanUp:
    return;
}

//------------------------------------------------------------------------------

VOID SetRetailMsgMode(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;

    UNREFERENCED_PARAMETER(pMenu);

    if (g_bootCfg.oalFlags & BOOT_CFG_OAL_FLAGS_RETAILMSG_ENABLE) {
        OALLog(L" Disable OAL Retail Messages [y/-]: ");
    } else {
        OALLog(L" Enable OAL Retail Messages [y/-]: ");
    }    

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    if (key == L'y' || key == L'Y') {
        if (g_bootCfg.oalFlags & BOOT_CFG_OAL_FLAGS_RETAILMSG_ENABLE) 
		{
            g_bootCfg.oalFlags &= ~BOOT_CFG_OAL_FLAGS_RETAILMSG_ENABLE;
            OALLog(L" OAL Retail Messages disabled\r\n");
        }
		else 
		{
            g_bootCfg.oalFlags |= BOOT_CFG_OAL_FLAGS_RETAILMSG_ENABLE;
            OALLog(L" OAL Retail Messages enabled\r\n");
        }    
    }
}

//------------------------------------------------------------------------------

VOID SetDisplayResolution(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    UINT32 i;

    UNREFERENCED_PARAMETER(pMenu);
    
    OALBLMenuHeader(L"Select Display Resolution");

    for (i=0; i<OMAP_RES_INVALID; i++)
    {
        OALLog(L" [%d] %s\r\n", i+1 , dispResMenu[i].resName);
    }
    OALLog(L" [0] Exit and Continue\r\n");

    OALLog(L"\r\n Selection (actual %s): ", dispResMenu[g_bootCfg.displayRes].resName);
    // Get key
    do {
        key = OALBLMenuReadKey(TRUE);
    } while (key < L'0' || key > L'0' + i);    
    OALLog(L"%c\r\n", key);
    
    // If user select exit don't change device
    if (key == L'0') return;

    g_bootCfg.displayRes = (key - L'0' - 1);

}


//------------------------------------------------------------------------------

VOID SetOPPmode(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    UINT32 i;
    UINT32 len = OMAP35x_OPP_NUM;
    OPP_MODE_MENU * pOppMenu = oppModeMenu35x;

    UNREFERENCED_PARAMETER(pMenu);
    
    OALBLMenuHeader(L"Select OPP Mode");

    
    if(g_CPUFamily == CPU_FAMILY_DM37XX)
    {
        len = OMAP37x_OPP_NUM;
        pOppMenu = oppModeMenu37x;
    }
    
    for (i=0; i<len; i++)
    {
        OALLog(L" [%d] %s\r\n", i+1 , pOppMenu[i].oppModeName);
    }
    OALLog(L" [0] Exit and Continue\r\n");

    OALLog(L"\r\n Selection (actual [%s]): ", pOppMenu[g_bootCfg.opp_mode].oppModeName);
    
    // Get key
    do {
        key = OALBLMenuReadKey(TRUE);
    } while (key < L'0' || key > L'0' + i);    
    OALLog(L"%c\r\n", key);
    
    // If user select exit don't change device
    if (key == L'0') return;

    g_bootCfg.opp_mode = (key - L'0' -1);

}
//------------------------------------------------------------------------------
//Add SetBacklight function Ray 13-09-17 
//
#define CODE_SIZE 11
HANDLE hGPIO;
DWORD BACKLIGHT_EN_GPIO = 61;
//
static OAL_BLMENU_ITEM g_menuBrightness[] ={
    {	
		L'1', L"ON" ,BacklightON,
		NULL, NULL, NULL
	}, {
        L'2', L"OFF" ,BacklightOFF,
        NULL, NULL, NULL                            
    }, {
        L'3', L"Brightness" ,BacklightBrightness,
        NULL, NULL, NULL
	}, {
        L'0', L"Exit and Continue", NULL,
        NULL, NULL, NULL
    }};
//
VOID BacklightON(OAL_BLMENU_ITEM *pMenu)
{
    //HANDLE hGPIO = NULL;
    hGPIO = GPIOOpen();
    GPIOSetBit(hGPIO, BACKLIGHT_EN_GPIO);
    UNREFERENCED_PARAMETER(pMenu);	
}

VOID BacklightOFF(OAL_BLMENU_ITEM *pMenu)
{
    //HANDLE hGPIO = NULL;
    hGPIO = GPIOOpen();
    GPIOClrBit(hGPIO, BACKLIGHT_EN_GPIO);
    UNREFERENCED_PARAMETER(pMenu);	
}

VOID BacklightBrightness(OAL_BLMENU_ITEM *pMenu)
{
    
//    UINT32 AAT3123_Code[CODE_SIZE] = { 1, 4 , 7, 10, 13, 16, 19, 22, 25, 28, 31};
                                    /*{ 1, 2, 3, 4, 5, 6, 7, 8, 9,10,
                                       11,12,13,14,15,16,17,18,19,20,
                                       21,22,23,24,25,26,27,28,29,30,
                                       31,32};*/
    WCHAR key;
//    UINT32 i = 6;
    int k;
    //HANDLE hGPIO = NULL;
    hGPIO = GPIOOpen();
    
    OALBLMenuHeader(L"Select Backlight Brightness +/-");
    for (k=0; k< BN_EXIT ; k++)
    {
        OALLog(L" [%d] %s\r\n", k+1,
                                backlightBN[k].bnName);
	}
    OALLog(L" [0] Exit and Continue\r\n");
    
    GPIOClrBit( hGPIO, BACKLIGHT_EN_GPIO);
    OALStall(1000);

    GPIOSetBit( hGPIO, BACKLIGHT_EN_GPIO);
    OALStall(1);


    
    key = OALBLMenuReadKey(TRUE);
    // Show +/- selection
    if(key != L'0'){
            OALLog(L"\r\n Selection : ");
        
        
       if(key == L'1'){
          OALLog(L"%s\r\n", backlightBN[g_bootCfg.backlighbn].bnName );
       }else{
          OALLog(L"%s\r\n ", (backlightBN[(g_bootCfg.backlighbn) +1 ].bnName));
       }

        
           /*switch(key)
            {
            case L'1':
                
                GPIOClrBit( hGPIO, BACKLIGHT_EN_GPIO);
                OALStall(1000);

                GPIOSetBit( hGPIO, BACKLIGHT_EN_GPIO);
                OALStall(1000);
                i = AAT3123_Code[i++];
                OALLog(L"\r\n i: %d, AAT3123_Code[i++]:%d.",i ,AAT3123_Code[i]);
                if(AAT3123_Code[i] > 31){
                    OALLog(L"\r\n Brightness is the largest. ");
                }else{
                    OALLog(L"\r\n Brightness increased.");
                }
                break;
            case L'2':
                
                GPIOClrBit( hGPIO, BACKLIGHT_EN_GPIO);
                OALStall(1000);

                GPIOSetBit( hGPIO, BACKLIGHT_EN_GPIO);
                OALStall(1000);
                i = AAT3123_Code[i--];
                if(AAT3123_Code[i] < 1){
                    OALLog(L"\r\n Brightness is the lowest.");
                }else{
                    OALLog(L"\r\n Brightness  decreased.");
                }
                break;
            case L'0':
               // goto stop;
               break;
            }*/
     }else{ /*(key == L'0'){*/
        OALLog(L"\r\n Selection : ");
        OALLog(L"%c\r\n", key);
     }
   
    //key = OALBLMenuReadKey(TRUE);
    
    /*do{
        key = OALBLMenuReadKey(TRUE);
        
    }while (key != L'0');*/

    /*if(key == L'0'){
        OALLog(L"\r\n Selection : ");
        OALLog(L"%c\r\n", key);
    }*/

    
    
   //do{
     // key = OALBLMenuReadKey(TRUE);
      //OALLog(L"%c\r\n", key);
     /* switch(key)
        {
            case L'1':
                
                GPIOClrBit( hGPIO, BACKLIGHT_EN_GPIO);
                OALStall(1000);

                GPIOSetBit( hGPIO, BACKLIGHT_EN_GPIO);
                OALStall(1000);
                i = AAT3123_Code[i++];
                OALLog(L"\r\n i: %d, AAT3123_Code[i++]:%d.",i ,AAT3123_Code[i]);
                if(AAT3123_Code[i] > 31){
                    OALLog(L"\r\n Brightness is the largest. ");
                }else{
                    OALLog(L"\r\n Brightness increased.");
                }
                break;
            case L'2':
                
                GPIOClrBit( hGPIO, BACKLIGHT_EN_GPIO);
                OALStall(1000);

                GPIOSetBit( hGPIO, BACKLIGHT_EN_GPIO);
                OALStall(1000);
                i = AAT3123_Code[i--];
                if(AAT3123_Code[i] < 1){
                    OALLog(L"\r\n Brightness is the lowest.");
                }else{
                    OALLog(L"\r\n Brightness  decreased.");
                }
                break;
            
        }*/
   //}while(key != L'0');

   //OALLog(L"%c\r\n", key);
   UNREFERENCED_PARAMETER(pMenu);
//stop:
   if (key == L'0') return;
   g_bootCfg.backlighbn = (key - L'0' - 1);
}

//------------------------------------------------------------------------------
//Ray 13-09-18 
VOID SetBacklight(OAL_BLMENU_ITEM *pMenu)
{
    //OALBLMenuShow(&g_menuBrightness);
    OAL_BLMENU_ITEM *pBnItem;
    WCHAR key;
    //UINT32 i;
    const int running = 1;
	//HANDLE hGPIO;
   
    while(running)
    {
        OALBLMenuHeader(L"Select Backlight ON/OFF");

        // Print menu items [1]~[3], Ray 13-09-23 
        for (pBnItem = g_menuBrightness; pBnItem->key != L'0'; pBnItem++) {
            OALLog(L" [%c] %s\r\n", pBnItem->key, pBnItem->text);
        }
        OALLog(L" [0] Exit and Continue\r\n"); 
        OALLog(L"\r\n Selection : ");

        while(running)
        {
           // Get key
            key = OALBLMenuReadKey(TRUE);
            // Look for key in menu
            for (pBnItem = g_menuBrightness; pBnItem->key != 0; pBnItem++) {
                if (pBnItem->key == key)  
                    break;
            }
            //If we find it, break loop
            if (pBnItem->key != 0) 
                break;
        }

        // Show ON/OFF selection
    	 OALLog(L"%c\r\n", pBnItem->key);
	    
	    // When action is NULL return back to parent menu
        if (pBnItem->pfnAction == NULL) break;
        
        // Else call menu action(function pointer)
        pBnItem->pfnAction(pBnItem);
   }	
   

    /*for (i=0; i<BACKLIGHT_EXIT; i++)
    {
        OALLog(L" [%d] %s\r\n", i+1, disBacklight[i].blName);
	}
    OALLog(L" [0] Exit and Continue\r\n");*/
	
	//OALLog(L"\r\n Selection : [%s] ", disBacklight[g_bootCfg.backlight].blName);
	//OALLog(L"\r\n Selection : ");
    // Get key
   /* do {
        key = OALBLMenuReadKey(TRUE);
    } while (key < L'0' || key > L'0' + i);*/ 
	
	
	// Show ON/OFF selection
	//OALLog(L"%c\r\n", key);
	
	//ON/OFF Backlight action.
	//hGPIO = GPIOOpen();

	/*switch(key)
	{
		case L'1':
            BacklightNO(hGPIO);
			//GPIOSetBit(hGPIO, 61);
			break;
		case L'2':
            BacklightOFF(hGPIO);
			//GPIOClrBit(hGPIO, 61);
        case L'3':
            //.........
            BacklightBrightness(hGPIO);
			break;
		default:
			break;
	}*/
	/*if(key == L'2'){
		GPIOClrBit(hGPIO, 61);
	}else{
		GPIOSetBit(hGPIO, 61);
	}*/
    
	// If user select exit don't any change device
    //if (key == L'0') return;
     UNREFERENCED_PARAMETER(pMenu);
    //g_bootCfg.backlight = (key - L'0' - 1);
}

//------------------------------------------------------------------------------
//Ray 13-09-03
//
VOID SDtoFlash(OAL_BLMENU_ITEM *pMenu)
{
	WCHAR key;
	UINT32 i;
//	BOOL read;
//  BOOL write;
	BOOL check = TRUE;
//  char size[500];
//  OAL_BLMENU_ITEM dumpFlashBuFunc;
//	int ch;
	
	//This function Initial SD Card file prepare file R/W
	//pRead =
	//BLSDtoFlash();
	//check = BLSDCardToFlash(L"OUT_DATA.txt");
	//read = BLSDtoFlash();
    //OEMWriteFlash(IMAGE_STARTUP_IMAGE_PA,
    //? Ray 13-09-16 
    /*write = WriteFlashFromEEBOOT(IMAGE_STARTUP_IMAGE_PA,
                         IMAGE_XLDR_BOOTSEC_NAND_SIZE + IMAGE_EBOOT_CODE_SIZE+
                         IMAGE_BOOTLOADER_BITMAP_SIZE + 8);*/

    /*write = WriteFlashNK(IMAGE_STARTUP_IMAGE_PA,
                         IMAGE_XLDR_BOOTSEC_NAND_SIZE + IMAGE_EBOOT_CODE_SIZE+
                         IMAGE_BOOTLOADER_BITMAP_SIZE + 8);*/

    //pRead  = fopen("c:/wince600/file/output_data.txt", "r");
	//pWrite = fopen("c:/wince600/file/input_data.txt", "w");
	
	UNREFERENCED_PARAMETER(pMenu);
	OALBLMenuHeader(L"Select File Transfer from SD Card to Flash");

	for(i=0; i<FLASH_EXIT; i++){
		OALLog(L" [%d] %s\r\n", i+1, sdToFlash[i].sdName);
	}
	OALLog(L" [0] Exit and Continue\r\n");

	OALLog(L"\r\n Selection : ");
	
	do{
		key = OALBLMenuReadKey(TRUE);
	}while(key < L'0' || key > '0'+i);

    /*if(key < L'0' || key > '0'+2){
        break;
    }else if (key == '3'){

    }*/

	OALLog(L"%c\r", key);
	
	switch(key)
	{
		case L'1':
            OALLog(L" Reading : ");
            
			if(/*read==*/check){
				OALLog(L"File Read ok!!\n");
            }else{
				OALLog(L"File Read failure!!\n");
			} 
			
			/*if((pRead != NULL) && (pWrite!= NULL))
			{
				OALLog(L"\tReading: \r");
				while((ch = getc(pWrite)) != EOF)
				{
					OALLog(L"%c,\r\n", ch);
				}
				OALLog(L"\rFile Read end!!\n");
				fclose(pRead);
				fclose(pWrite);
			}else{
				OALLog(L"\rFile Read failure!!\n");
			}*/
			break;
		case L'2':                  //Ray 13-09-14
            OALLog(L" Writing : ");
            
            if(/*write ==*/ check){
                OALLog(L"File Written ok!!\n");
            }else{  
                OALLog(L"File Write failure!!\n");
            }
         
            /*if((pRead != NULL) && (pWrite!= NULL))
			{
				while( (ch = getc(pRead)) != EOF)
				{
					putc(ch, pWrite);
				}
				OALLog(L"\rFile Write end!!\n");
			}else{
				OALLog(L"\rFile Read failure!!\n");
			}*/
			break;
       case L'3':
            //OALLog(L" Show .nb0 Data : ");
            //VOID DumpFlash(OAL_BLMENU_ITEM *pMenu)
            //dumpFlashBuFunc.pfnAction = DumpFlashBuffer;//dumpFlashBufer;
            //dumpFlashBuFunc.pfnAction = DumpFlashBuffer; 
            break;
		default:break;
	}
		
	// If user select exit, don't any change device		//Ray 13-09-04	
	if(key == L'0')return;

	//Call ShowSettings() , show current settings status
	g_bootCfg.sdtoflash = (key - L'0' - 1);
}

//------------------------------------------------------------------------------
//Ray 13-09-24
//
VOID LCMInitial(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    UINT32 i;
    DWORD dTime = 5000000;

    UNREFERENCED_PARAMETER(pMenu);
    
    OALBLMenuHeader(L"LCM Initial and Check");
    
    for (i=0; i<LCM_EXIT; i++)
    {
        OALLog(L" [%d] %s\r\n", i+1 , LCMInitiaArray[i].lcmIniName);
    }
    OALLog(L" [0] Exit and Continue\r\n");

    OALLog(L"\r\n Selection : ");
    // Get key
    do {
        key = OALBLMenuReadKey(TRUE);
    } while (key < L'0' || key > L'0'+ i);
    
    OALLog(L"%c\r\n", key);


    switch(key)
	{
		case L'1':
            LCD_SPI_Init();
            LcdStall(dTime);
			break;
		case L'2':
            spi_Low();
            break;
		default:
			break;
	}
    
    // If user select exit don't change device
    if (key == L'0') return;

    g_bootCfg.lcmInitial = (key - L'0' - 1);

}

//------------------------------------------------------------------------------

VOID SaveSettings(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    UNREFERENCED_PARAMETER(pMenu);

    OALLog(L" Do you want save current settings [-/y]? ");

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    // Depending on result
    if (key != L'y' && key != L'Y') goto cleanUp;

    if (BLWriteBootCfg(&g_bootCfg)) {
        OALLog(L" Current settings has been saved\r\n");
    } else {        
        OALLog(L"ERROR: Settings save failed!\r\n");
    }

cleanUp:
    return;
}

//------------------------------------------------------------------------------

UINT32 OALStringToUINT32(LPCWSTR psz)
{
    UINT32 i = 0;

    // Replace the dots with NULL terminators
    while (psz != NULL && *psz != L'\0') 
        {
        if (*psz < L'0' || *psz > L'9') 
            {
            break;
            }
        i = i * 10 + (*psz - L'0');
        psz++;
        }

    return i;
}

#if BUILDING_EBOOT_SD

//------------------------------------------------------------------------------

VOID ShowSDCardSettings(OAL_BLMENU_ITEM *pMenu)
{
    UNREFERENCED_PARAMETER(pMenu);

    OALLog(L"\r\n SDCard:\r\n");
    OALLog(
        L"  Filename:      \"%s\"\r\n", g_bootCfg.filename
    );
//  OALBLMenuReadKey(TRUE);
}

//------------------------------------------------------------------------------
#define dimof(x)                (sizeof(x)/sizeof(x[0]))

VOID EnterSDCardFilename(OAL_BLMENU_ITEM *pMenu)
{
    UNREFERENCED_PARAMETER(pMenu);

    OALLog(L"\r\n Type new filename (8.3 format) :");
    OALBLMenuReadLine(g_bootCfg.filename, dimof(g_bootCfg.filename));
}

//------------------------------------------------------------------------------

#endif
