// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

#include "bsp.h"
#include "sdk_gpio.h"

#include <..\lcd_cfg.h>

static HANDLE g_hGpio = NULL;
static DWORD g_gpioLcdPower;
static DWORD g_gpioLcdIni;
static DWORD g_gpioLcdResB;

BOOL LcdInitGpio(void)
{
    // Configure Backlight/Power pins as outputs
    g_hGpio = GPIOOpen();

    // Turning full backlight off
    GPIOClrBit(g_hGpio,LCD_BACKLIGHT_PWR_GPIO);
    GPIOClrBit(g_hGpio,LCD_BACKLIGHT_PWM_GPIO);
    // Turning off LCD power
    GPIOClrBit(g_hGpio,LCD_PANEL_PWR_GPIO);

    GPIOSetMode(g_hGpio,LCD_PANEL_PWR_GPIO,GPIO_DIR_OUTPUT);
    GPIOSetMode(g_hGpio,LCD_BACKLIGHT_PWR_GPIO,GPIO_DIR_OUTPUT);
    GPIOSetMode(g_hGpio,LCD_BACKLIGHT_PWM_GPIO,GPIO_DIR_OUTPUT);

    LcdSleep(100);
    return TRUE;
}

BOOL LcdDeinitGpio(void)
{
	// Close GPIO driver
    GPIOClose(g_hGpio);

	return TRUE;
}

void LcdPowerControl(BOOL bEnable)
{
	if (bEnable)
	{
		RETAILMSG(1,(TEXT("Turning on the LCD\r\n")));

		LcdSleep(100);

		// Turning on LCD power
		GPIOSetBit(g_hGpio,LCD_PANEL_PWR_GPIO);

		// Turning full backlight on
		GPIOSetBit(g_hGpio,LCD_BACKLIGHT_PWR_GPIO);
		GPIOSetBit(g_hGpio,LCD_BACKLIGHT_PWM_GPIO);
	}
	else
	{
		RETAILMSG(1,(TEXT("Turning off the LCD\r\n")));

		// Turning full backlight off
		GPIOClrBit(g_hGpio,LCD_BACKLIGHT_PWR_GPIO);        
		GPIOClrBit(g_hGpio,LCD_BACKLIGHT_PWM_GPIO);        

		// Turning off LCD power
		GPIOClrBit(g_hGpio,LCD_PANEL_PWR_GPIO);        
	}
}

// Screen should be in rotated to lanscape mode, 640x480 for use with DVI
void LcdDviEnableControl(BOOL bEnable)
{
  UNREFERENCED_PARAMETER(bEnable);
}

