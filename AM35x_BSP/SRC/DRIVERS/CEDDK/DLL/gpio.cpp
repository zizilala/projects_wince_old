// All rights reserved ADENEO EMBEDDED 2010
#include "bsp.h"
#include <initguid.h>
#include "sdk_gpio.h"
#include "bsp_cfg.h"
#include "ceddkex.h"



void BSPGpioInit()
{
    BSPInsertGpioDevice(0,NULL,L"GIO1:");   // Omap GPIOs
    BSPInsertGpioDevice(GPIO_EXPANDER_1_PINID_START,NULL,L"GIO2:");   // GPIO expander 1
    BSPInsertGpioDevice(GPIO_EXPANDER_2_PINID_START,NULL,L"GIO3:");   // GPIO expander 2
    BSPInsertGpioDevice(GPIO_EXPANDER_3_PINID_START,NULL,L"GIO4:");   // GPIO expander 3
}