//===================================================================
//
//  Module Name:    NLED.DLL
//
//  File Name:      nleddrv.c
//
//  Description:    Control of the notification LED(s)
//
//===================================================================
// Copyright (c) 2007- 2009 BSQUARE Corporation. All rights reserved.
//===================================================================

//-----------------------------------------------------------------------------
//
// header files
//
//-----------------------------------------------------------------------------

#include "bsp.h"
#include "ceddkex.h"
#include "sdk_gpio.h"
#include "oalex.h"


#define EXPANDER_3_GPIO(x) (GPIO_EXPANDER_3_PINID_START+(x))

DWORD g_GPIOId[] = {EXPANDER_3_GPIO(6),EXPANDER_3_GPIO(7)};
DWORD g_GPIOActiveState[dimof(g_GPIOId)] = {0,0};
DWORD g_dwNbLeds = dimof(g_GPIOId);
BOOL g_LastLEDIsVibrator = FALSE;

int NLedCpuFamily = CPU_FAMILY_AM35XX;

BOOL NLedBoardInit()
{        
    return TRUE;
}

BOOL NLedBoardDeinit()
{
    return TRUE;
}
