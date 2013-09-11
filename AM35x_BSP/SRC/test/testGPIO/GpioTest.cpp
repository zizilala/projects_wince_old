// All rights reserved ADENEO EMBEDDED 2010
// GpioTest.cpp : Defines the entry point for the console application.
//

#include "omap.h"
#include "sdk_gpio.h"
#include "gpio_ioctls.h"

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
    DWORD id = 168;
	HANDLE h = INVALID_HANDLE_VALUE;

    _tprintf(_T("GPIOTest\n"));

    if (argc == 2)
    {
        id = _wtoi(argv[1]);
    }

    h = GPIOOpen();
	if (h == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("Failed to open the GPIO driver\n"));
		return 0;
	}

	// Configure the GPIO as an output
    GPIOSetMode(h,id,GPIO_DIR_OUTPUT);

    for (;;)
    {
        GPIOSetBit(h,id);
        Sleep(1000);
        GPIOClrBit(h,id);
        Sleep(1000);
    }

    GPIOClose(h);

    return 0;
}

