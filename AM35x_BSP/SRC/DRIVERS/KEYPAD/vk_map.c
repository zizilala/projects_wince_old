// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
//------------------------------------------------------------------------------
//
//  File: vk_map.c
//
//  This file containts definition for keyboard to virtual key
//  mapping.
//
#include "bsp.h"
#include <winuserm.h>
#include "gpio_keypad.h"

#ifndef dimof
#define dimof(x)            (sizeof(x)/sizeof(x[0]))
#endif

#define NB_KEYS 10


#define EXP1PINID(x)    (GPIO_EXPANDER_1_PINID_START+(x))
//------------------------------------------------------------------------------

const GPIO_KEY g_keypadVK[NB_KEYS] = { 
    {EXP1PINID(6),  VK_DOWN},               //DOWN key    
    {EXP1PINID(7),  VK_UP},                 //UP key
    {EXP1PINID(8),  VK_MENU},               //MENU key
    {EXP1PINID(9),  VK_CONTROL},            //MODE key
    {EXP1PINID(10), VK_LSHIFT},             //SHIFT key
    {EXP1PINID(11), VK_LEFT},               //LEFT key
    {EXP1PINID(12), VK_RIGHT},              //RIGHT key
    {EXP1PINID(13), VK_PERIOD},             //STOP key
    {EXP1PINID(14), VK_SPACE},              //PLAY/PAUSE key
    {EXP1PINID(15), VK_TRECORD},            //REC key
        
};

const int g_nbKeys = NB_KEYS;

//------------------------------------------------------------------------------

const UCHAR g_wakeupVKeys[]     = { VK_PERIOD };
const int g_nbWakeupVKeys = dimof(g_wakeupVKeys);


//------------------------------------------------------------------------------

static const UCHAR off[]     = { VK_PERIOD };
static const KEYPAD_REMAP_ITEM remapItems[] = {
    { VK_OFF, dimof(off),     3000, off     },
};

const KEYPAD_REMAP g_keypadRemap = { dimof(remapItems), remapItems };

//------------------------------------------------------------------------------

static const UCHAR softkeys[] = { VK_CONTROL };

static const KEYPAD_REPEAT_BLOCK softkeyBlock = { dimof(softkeys), softkeys };

static const KEYPAD_REPEAT_ITEM repeatItems[] = {
    {VK_DOWN,             500, 500, TRUE,  &softkeyBlock},
    {VK_UP,               500, 500, TRUE,  &softkeyBlock },
    {VK_MENU,             500, 500, TRUE,  NULL },
    {VK_CONTROL,          500, 500, TRUE,  NULL },
    {VK_LSHIFT,           500, 500, TRUE,  NULL },
    {VK_LEFT,             500, 500, TRUE,  &softkeyBlock },
    {VK_RIGHT,            500, 500, TRUE,  &softkeyBlock },
    {VK_PERIOD,			  500, 500, TRUE,  NULL },
    {VK_SPACE,			  500, 500, TRUE,  NULL },
    {VK_TRECORD,          500, 500, TRUE,  NULL },                    
};



const KEYPAD_REPEAT g_keypadRepeat = { dimof(repeatItems), repeatItems };

//------------------------------------------------------------------------------

