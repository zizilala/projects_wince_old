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
// FILENAME: Touch.cpp
//
// DESCRIPTION:
//   Implementation of the WinCE touch screen PDD.
// 	NOTE - This driver is written for the Texas Instruments TSC2046 touch screen controller
//
// Functions:
//
//  TouchDriverCalibrationPointGet
//  DdsiTouchPanelGetDeviceCaps
//  DdsiTouchPanelSetMode
//  DdsiTouchPanelEnable
//  DdsiTouchPanelDisable
//  DdsiTouchPanelAttach
//  DdsiTouchPanelDetach
//  DdsiTouchPanelGetPoint
//  DdsiTouchPanelPowerHandler
//
////////////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------------
// Public
//
#include <windows.h>
#include <types.h>
#include <nkintr.h>
#include <tchddsi.h>

//------------------------------------------------------------------------------
// Platform
//
#include "omap.h"
#include "ceddkex.h"
#include "soc_cfg.h"
#include "sdk_gpio.h"
#include "sdk_i2c.h"
//------------------------------------------------------------------------------
// Local
//
#include "touchscreen.h"


//------------------------------------------------------------------------------
// Defines
//

#define COMMAND_CNV_SELECTXY			0x00
#define COMMAND_ENABLE_READ             0x3
#define COMMAND_ENABLE_WRITE			0x2
#define COMMAND_XPOS                    0xD4
#define COMMAND_YPOS                    0xC4

#define CFG0_REG						0xC
#define CFG2_REG						0xE


// CFG2 bit definitions
//MSB
#define PINTS1							(1 << 7)
#define PINTS0							(1 << 6)
#define MAV_M0M1_FLTR_SIZE_15			(3 << 4)
#define MAV_W0W1_FLTR_SIZE_7_8			(2 << 2)
//LSB
#define MAV_FLTR_EN_X					(1<<4)
#define MAV_FLTR_EN_Y					(1<<3)

#define MAX_PTS							4
#define ALLOWED_X_STANDARD_DEVIATION	12
#define ALLOWED_Y_STANDARD_DEVIATION	12

//------------------------------------------------------------------------------
// local data structures
//

typedef struct  
{
    BOOL    bInitialized;

    DWORD   nSampleRate;    
    LONG    nPenGPIO;    
	ULONG	nI2CAddr;
	ULONG	nI2CBaudrateIndex;
    DWORD   nI2CBusIndex;

    LONG    nProcessAttached;
    HANDLE  hGPIO;
    HANDLE  hI2C;
    LONG    nPenIRQ;
}
TOUCH_DEVICE;


//------------------------------------------------------------------------------
//  Device registry parameters

static WCHAR const* s_szRegistryPath = L"\\HARDWARE\\DEVICEMAP\\TOUCH";
static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"SampleRate", PARAM_DWORD, FALSE, offset(TOUCH_DEVICE, nSampleRate),
        fieldsize(TOUCH_DEVICE, nSampleRate), (VOID*)DEFAULT_SAMPLE_RATE_HZ
    }, {
        L"PenGPIO", PARAM_DWORD, TRUE, offset(TOUCH_DEVICE, nPenGPIO),
        fieldsize(TOUCH_DEVICE, nPenGPIO), 0
    }, {
        L"I2CBus", PARAM_DWORD, TRUE, offset(TOUCH_DEVICE, nI2CBusIndex),
        fieldsize(TOUCH_DEVICE, nI2CBusIndex), 0
	}, {
        L"I2CAddr", PARAM_DWORD, TRUE, offset(TOUCH_DEVICE, nI2CAddr),
        fieldsize(TOUCH_DEVICE, nI2CAddr), 0
	}, {
        L"I2CBaudrateIndex", PARAM_DWORD, FALSE, offset(TOUCH_DEVICE, nI2CBaudrateIndex),
        fieldsize(TOUCH_DEVICE, nI2CBaudrateIndex), (void*)0
    }
};


//------------------------------------------------------------------------------
// global variables
//

static TOUCH_DEVICE s_TouchDevice =
    {
    FALSE,
    0,
    0,
    0,
    0
    };


// Referenced in MDD. TS controller only asserts SYSINTR_TOUCH.
//
DWORD gIntrTouchChanged = SYSINTR_NOP;   // Not used here.
DWORD gIntrTouch        = SYSINTR_NOP;

void StartCalibrationThread();

//------------------------------------------------------------------------------
// exports
//
extern "C" DWORD gdwTouchIstTimeout;    // MDD thread wait timeout.

// The MDD requires a minimum of MIN_CAL_COUNT consecutive samples before
// it will return a calibration coordinate to GWE.
extern "C" const int MIN_CAL_COUNT = 20;

//------------------------------------------------------------------------------
//
//  TouchDriverCalibrationPointGet
//
extern "C" BOOL 
TouchDriverCalibrationPointGet(
    TPDC_CALIBRATION_POINT *pTCP 
    )
{
    BOOL rc = FALSE;

    INT32 cDisplayWidth  = pTCP->cDisplayWidth;
    INT32 cDisplayHeight = pTCP->cDisplayHeight;

    int CalibrationRadiusX = cDisplayWidth / 20;
    int CalibrationRadiusY = cDisplayHeight / 20;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("TouchDriverCalibrationPointGet+\r\n")));    
    
    // Check which of the 5 calibration point is requested.
    switch ( pTCP->PointNumber )
        {
        case 0:
            pTCP->CalibrationX = cDisplayWidth / 2;
            pTCP->CalibrationY = cDisplayHeight / 2;
            rc = TRUE;
            break;

        case 1:
            pTCP->CalibrationX = CalibrationRadiusX * 2;
            pTCP->CalibrationY = CalibrationRadiusY * 2;
            rc = TRUE;
            break;

        case 2:
            pTCP->CalibrationX = CalibrationRadiusX * 2;
            pTCP->CalibrationY = cDisplayHeight - ( CalibrationRadiusY * 2 );
            rc = TRUE;
            break;

        case 3:
            pTCP->CalibrationX = cDisplayWidth - ( CalibrationRadiusX * 2 );
            pTCP->CalibrationY = cDisplayHeight - ( CalibrationRadiusY * 2 );
            rc = TRUE;
            break;

        case 4:
            pTCP->CalibrationX = cDisplayWidth - ( CalibrationRadiusX * 2 );
            pTCP->CalibrationY = CalibrationRadiusY * 2;
            rc = TRUE;
            break;

        default:
            pTCP->CalibrationX = cDisplayWidth / 2;
            pTCP->CalibrationY = cDisplayHeight / 2;

            SetLastError( ERROR_INVALID_PARAMETER );
            break;
        }

    DEBUGMSG(ZONE_FUNCTION, (TEXT("cDisplayWidth        : %4X\r\n"), cDisplayWidth     ));
    DEBUGMSG(ZONE_FUNCTION, (TEXT("cDisplayHeight       : %4X\r\n"), cDisplayHeight    ));
    DEBUGMSG(ZONE_FUNCTION, (TEXT("CalibrationRadiusX   : %4d\r\n"), CalibrationRadiusX));
    DEBUGMSG(ZONE_FUNCTION, (TEXT("CalibrationRadiusY   : %4d\r\n"), CalibrationRadiusY));
    DEBUGMSG(ZONE_FUNCTION, (TEXT("pTCP -> PointNumber  : %4d\r\n"), pTCP->PointNumber));
    DEBUGMSG(ZONE_FUNCTION, (TEXT("pTCP -> CalibrationX : %4d\r\n"), pTCP->CalibrationX));
    DEBUGMSG(ZONE_FUNCTION, (TEXT("pTCP -> CalibrationY : %4d\r\n"), pTCP->CalibrationY));

    DEBUGMSG(ZONE_FUNCTION, (TEXT("TouchDriverCalibrationPointGet-\r\n")));    
    return ( rc );
}

//------------------------------------------------------------------------------
//
//  DdsiTouchPanelGetDeviceCaps
//
//

extern "C" BOOL 
DdsiTouchPanelGetDeviceCaps( 
    INT iIndex, 
    LPVOID lpOutput 
    )
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelGetDeviceCaps+\r\n")));
 
    BOOL rc = FALSE;
    if ( lpOutput == NULL )
        {
        DEBUGMSG(ZONE_ERROR, (TEXT("TouchPanelGetDeviceCaps: Invalid parameter.\r\n")));
        SetLastError( ERROR_INVALID_PARAMETER );
        }
    else
        {
        TPDC_SAMPLE_RATE *pTSR = (TPDC_SAMPLE_RATE*)lpOutput;
        TPDC_CALIBRATION_POINT_COUNT *pTCPC = (TPDC_CALIBRATION_POINT_COUNT*)lpOutput;

        // Check which of the device capabilities are requested.
        switch ( iIndex )
            {
            // Return the sample rate.
            case TPDC_SAMPLE_RATE_ID:

                pTSR->SamplesPerSecondLow      = TOUCHPANEL_SAMPLE_RATE_LOW;
                pTSR->SamplesPerSecondHigh     = TOUCHPANEL_SAMPLE_RATE_HIGH;
                pTSR->CurrentSampleRateSetting = s_TouchDevice.nSampleRate;

                rc = TRUE;
                break;

            // Return the number of calibration points used to calibrate the touch screen.
            case TPDC_CALIBRATION_POINT_COUNT_ID:

                pTCPC->flags              = 0;
                pTCPC->cCalibrationPoints = 5;

                rc = TRUE;
                break;

            // Return the x and y coordinates of the requested calibration point.
            // The index of the calibration point is set in lpOutput->PointNumber.
            case TPDC_CALIBRATION_POINT_ID:
                rc = TouchDriverCalibrationPointGet( (TPDC_CALIBRATION_POINT*)lpOutput );
                break;

            default:
                DEBUGMSG( ZONE_ERROR, 
                           (TEXT("TouchPanelGetDeviceCaps: Invalid parameter.\r\n")));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
        }
    }
    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelGetDeviceCaps-\r\n")));

    return ( rc );
}


//------------------------------------------------------------------------------
//
//  DdsiTouchPanelSetMode
//
//  Comments:
//    Only one sample rate supported.
//

BOOL 
DdsiTouchPanelSetMode( 
    INT iIndex, 
    LPVOID lpInput 
    )
{
    BOOL rc = FALSE;

UNREFERENCED_PARAMETER(lpInput);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelSetMode+\r\n")));
    
    switch ( iIndex )
        {
        case TPSM_SAMPLERATE_LOW_ID:
        case TPSM_SAMPLERATE_HIGH_ID:
            SetLastError( ERROR_SUCCESS );
            rc = TRUE;
            break;

        default:
            DEBUGMSG( ZONE_ERROR, 
                       (TEXT("DdsiTouchPanelSetMode: Invalid parameter.\r\n")));
            SetLastError( ERROR_INVALID_PARAMETER );
            break;
        }

    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelSetMode-\r\n")));

    return rc;
}

//------------------------------------------------------------------------------
//
//  DdsiTouchPanelEnable
//

BOOL 
DdsiTouchPanelEnable()
{
    BOOL rc = FALSE;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelEnable+\r\n")));
    
    // Initialize once only
    if (s_TouchDevice.bInitialized)
        {
        rc = TRUE;
        goto cleanup;
        }

    // Initialize HW
    if (!PddInitializeHardware())
	{
		DEBUGMSG(
			ZONE_ERROR, 
			(TEXT("ERROR: TOUCH: Failed to initialize touch PDD.\r\n"))
			);
		goto cleanup;
	}

	// get Logical interrupt # from GPIO manager
	DWORD dwLogIntr = GPIOGetSystemIrq(s_TouchDevice.hGPIO,s_TouchDevice.nPenGPIO);

    //Get a valid sysintr for this interrupt
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,&dwLogIntr,sizeof(dwLogIntr),&gIntrTouch,sizeof(gIntrTouch),NULL))
    {
        goto cleanup;
    }

	// Set the interrupt as a wake-up source
	KernelIoControl(IOCTL_HAL_ENABLE_WAKE, &gIntrTouch,
			sizeof(gIntrTouch), NULL, 0, NULL);

	// assign to global IRQ used by MDD
	s_TouchDevice.nPenIRQ = dwLogIntr;


	// Create a calibration thread. This thread will check to see if calibration is needed.
	StartCalibrationThread();

    //Done
    s_TouchDevice.bInitialized = TRUE;
    rc = TRUE;
    
cleanup:
    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelEnable-\r\n")));
    return rc;
}

//------------------------------------------------------------------------------
//
//  DdsiTouchPanelDisable
//
//

VOID 
DdsiTouchPanelDisable()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelDisable+\r\n")));

    // Close pen event and kill thread.
    PddDeinitializeHardware();
        
    // Release interrupt
    if (gIntrTouch != 0) 
        {
        KernelIoControl(
            IOCTL_HAL_RELEASE_SYSINTR, 
            &gIntrTouch, 
            sizeof(gIntrTouch), 
            NULL, 
            0, 
            NULL
            );
        }

    s_TouchDevice.bInitialized = FALSE;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelDisable-\r\n")));
}

//------------------------------------------------------------------------------
//
//  DdsiTouchPanelAttach
//
//
LONG 
DdsiTouchPanelAttach()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelAttach+\r\n")));

    // Increment the number of process attach calls.
    s_TouchDevice.nProcessAttached++;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelAttach-\r\n")));
        
    // Return the number.
    return ( s_TouchDevice.nProcessAttached );
}

//------------------------------------------------------------------------------
//
//  DdsiTouchPanelDetach
//
//
LONG 
DdsiTouchPanelDetach()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelDetach+\r\n")));
    
    // Decrement the number of process attach calls.
    s_TouchDevice.nProcessAttached--;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelDetach\r\n")));
    
    // Return the number.
    return ( s_TouchDevice.nProcessAttached );
}

//------------------------------------------------------------------------------
//
//  DdsiTouchPanelGetPoint
//
//
void 
DdsiTouchPanelGetPoint( 
    TOUCH_PANEL_SAMPLE_FLAGS *pTipStateFlags,
    INT *pUncalX, 
    INT *pUncalY 
    )
{
    // MDD needs us to hold on to last valid sample and previous pen state.
    static ULONG usLastFilteredX    	= 0;	// This holds the previous X sample
    static ULONG usLastFilteredY    	= 0;	// This holds the previous Y sample
    static ULONG usSavedFilteredX  	= 0;	// This holds the last reported X sample
    static ULONG usSavedFilteredY  	= 0;	// This holds the last reported Y sample
    static BOOL bPrevReportedPenDown	= FALSE;
    static DWORD DelayedSampleCount		= 0;
	
    BOOL bActualPenDown         		= FALSE; // This indicates if the pen is actually down, whether we report or not
    UINT32 xpos = 0;
    UINT32 ypos = 0;

    DEBUGMSG(ZONE_FUNCTION&&ZONE_SAMPLES, (TEXT("DdsiTouchPanelGetPoint+\r\n")));



    // Check if pen data are available If so, get the data.
	// Note that we always return data from the previous sample to avoid returning data nearest
	// the point where the pen is going up.  Data during light touches is not accurate and should
	// normally be rejected.  Without the ability to do pressure measurement, we are limited to 
	// rejecting points near the beginning and end of the pen down period.
	bActualPenDown = PddGetTouchIntPinState();
    if (bActualPenDown)
    {
	    PddGetTouchData(&xpos, &ypos);
    }

    // Check if we have valid data to report to the MDD.
    if (bActualPenDown)
    {
        // Return the valid pen data.  Note that this data was actually obtained on the
		// previous sample
        *pUncalX = xpos;
        *pUncalY = ypos;
		
		// Save the data that we returned.  
		// This will also be returned on penup to help avoid returning data obtained during
		// a light press
		usSavedFilteredX = xpos;
		usSavedFilteredY = ypos;
		
        DEBUGMSG(ZONE_SAMPLES, ( TEXT( "X(0x%X) Y(0x%X)\r\n" ), *pUncalX, *pUncalY ) );
        
        // Store reported pen state.
        *pTipStateFlags = TouchSampleDownFlag | TouchSampleValidFlag;
    }
    else    // Otherwise, assume pen is up.
	{
 
        DEBUGMSG(ZONE_TIPSTATE, ( TEXT( "Pen Up!\r\n" ) ) );

        // Use the last valid sample. MDD needs an up with valid data.
        *pUncalX = usSavedFilteredX;
        *pUncalY = usSavedFilteredY;
        *pTipStateFlags = TouchSampleValidFlag;

        DEBUGMSG(ZONE_SAMPLES, ( TEXT( "Point: (%d,%d)\r\n" ), *pUncalX, *pUncalY ) );
    }
		
	// Set up interrupt/timer for next sample
	if (bActualPenDown)
	{
        // Pen down so set MDD timeout for polling mode.
        gdwTouchIstTimeout = 1000/s_TouchDevice.nSampleRate;
		// Save point measured during this sample so it can be reported on the next sample
		usLastFilteredX = xpos;
		usLastFilteredY = ypos;
	}
	else
	{
		// Reset the delayed sample counter
		DelayedSampleCount = 0;
		usLastFilteredX = 0;
		usLastFilteredY = 0;
        
		// Pen up so set MDD timeout for interrupt mode.
        gdwTouchIstTimeout = INFINITE;

        // Set the proper state for the next interrupt.
        InterruptDone( gIntrTouch );
	}

    DEBUGMSG(ZONE_FUNCTION&&ZONE_SAMPLES, (TEXT("DdsiTouchPanelGetPoint+\r\n")));
    
    return;
}

//------------------------------------------------------------------------------
//
//  DdsiTouchPanelPowerHandler
//
//  Comments:
//    The controller powers down between samples.
//
//
void 
DdsiTouchPanelPowerHandler( 
    BOOL bOff 
    )
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelPowerHandler+\r\n")));

    if (gIntrTouch != SYSINTR_NOP)
        InterruptMask(gIntrTouch, bOff);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("DdsiTouchPanelPowerHandler+\r\n")));
}


		
//------------------------------------------------------------------------------
//
//  PddInitializeHardware()
//

BOOL 
PddInitializeHardware(VOID)
{
    BOOL   rc = FALSE; 
	UCHAR  ucCommand = 0;
	UCHAR  ucData[4];
    if (IsDVIMode())
        return FALSE;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("PddInitializeHardware+\r\n")));

    // Read parameters from registry
    if (GetDeviceRegistryParams(
            s_szRegistryPath, 
            &s_TouchDevice, 
            dimof(s_deviceRegParams), 
            s_deviceRegParams) != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: PddInitializeHardware: Error reading from Registry.\r\n")));
        goto cleanup;
        }

    // Open GPIO driver
    s_TouchDevice.hGPIO = GPIOOpen();
    if (s_TouchDevice.hGPIO == NULL) 
	{
		DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: PddInitializeHardware: Failed open GPIO device driver\r\n")));
		goto cleanup;
	}
	
    // Open I2C driver
    s_TouchDevice.hI2C = I2COpen(SOCGetI2CDeviceByBus(s_TouchDevice.nI2CBusIndex));
    I2CSetTimeout(s_TouchDevice.hI2C, 10);
    if (s_TouchDevice.hI2C == NULL) 
	{
		DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: PddInitializeHardware: Failed open I2C device driver\r\n")));
		goto cleanup;
	}
    I2CSetBaudIndex(s_TouchDevice.hI2C,s_TouchDevice.nI2CBaudrateIndex);
    I2CSetSlaveAddress(s_TouchDevice.hI2C, (UINT16)s_TouchDevice.nI2CAddr);
	I2CSetSubAddressMode(s_TouchDevice.hI2C, I2C_SUBADDRESS_MODE_0);

    // Setup nPENIRQ for input mode, low detect
    GPIOSetMode(s_TouchDevice.hGPIO,s_TouchDevice.nPenGPIO,GPIO_DIR_INPUT | GPIO_INT_LOW);


	// setup PENINTDAV to be output only on Pen Touch
	// read CFG2 register
	ucCommand = 0x80;		// MSB D7 is always set to 1
	I2CWrite(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,&ucCommand,sizeof(ucCommand));
	ucCommand = (CFG2_REG << 3)| COMMAND_ENABLE_READ;
	I2CWrite(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,&ucCommand,sizeof(ucCommand));
	I2CRead(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,ucData,2*sizeof(UCHAR));

	// enable PINTS1
	ucData[2] = (ucData[1] |
				MAV_FLTR_EN_X | 
				MAV_FLTR_EN_Y);
	
	ucData[1] = (ucData[0] |
				PINTS1 | 
				PINTS0 |
				MAV_M0M1_FLTR_SIZE_15 | 
				MAV_W0W1_FLTR_SIZE_7_8);

	// write to CFG2 register
	ucCommand = 0x80;		// MSB D7 is always set to 1
	I2CWrite(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,&ucCommand,sizeof(ucCommand));

	// enable read after conversion
	ucData[0] = (CFG2_REG << 3)| COMMAND_ENABLE_WRITE;
	I2CWrite(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,ucData,3*sizeof(UCHAR));

	Sleep(200);


    // Done    
    rc = TRUE;

cleanup:

    DEBUGMSG(ZONE_FUNCTION, (TEXT("PddInitializeHardware-\r\n")));    
    if( rc == FALSE )
	{
		PddDeinitializeHardware();
	}

    return rc;
}



//------------------------------------------------------------------------------
//
//  PddDeinitializeHardware()
//
VOID 
PddDeinitializeHardware()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("PddDeinitializeHardware+\r\n")));    

    // Close SPI device
    if (s_TouchDevice.hI2C != NULL)
    	{
    		I2CClose(s_TouchDevice.hI2C);
    		s_TouchDevice.hI2C = NULL;
    	}

    // Close GPIO device
    if (s_TouchDevice.hGPIO != NULL)
    	{
    		GPIOClose(s_TouchDevice.hGPIO);
    		s_TouchDevice.hGPIO = NULL;
    	}

    DEBUGMSG(ZONE_FUNCTION, (TEXT("PddDeinitializeHardware-\r\n")));    
}


//------------------------------------------------------------------------------
//
//  PddGetTouchIntPinState()
//
inline BOOL 
PddGetTouchIntPinState()
{
	UCHAR ucCommand = 0;
	UCHAR ucData[2];

	// read PSM bit in CFG0
	ucCommand = 0x80;		// MSB D7 is always set to 1
	I2CWrite(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,&ucCommand,sizeof(ucCommand));
	ucCommand = (CFG0_REG << 3)| COMMAND_ENABLE_READ;
	I2CWrite(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,&ucCommand,sizeof(ucCommand));
	I2CRead(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,ucData,2*sizeof(UCHAR));

	return (ucData[0] & PINTS1);
}

//------------------------------------------------------------------------------
//
//  PddGetControllerData 
//
void PddGetControllerData(UINT32 control, USHORT* read_data)
{
	UCHAR *p = (UCHAR *) read_data;
	UCHAR ucCommand = 0;
	UCHAR ucTemp = 0;

	// select conversion mode
	ucCommand = (UCHAR)(control | 0x80);		// MSB D7 is always set to 1
	I2CWrite(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,&ucCommand,sizeof(ucCommand));

	// enable read after conversion
	ucCommand = COMMAND_ENABLE_READ;
	I2CWrite(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,&ucCommand,sizeof(ucCommand));

	// read X & Y values
	I2CRead(s_TouchDevice.hI2C,I2C_SUBADDRESS_MODE_0,read_data,2*sizeof(USHORT));

	ucTemp = p[0];
	p[0] = p[1];
	p[1] = ucTemp;
	ucTemp = p[2];
	p[2] = p[3];
	p[3] = ucTemp;

	DEBUGMSG(ZONE_SAMPLES,(TEXT("X %x Y %x \r\n"),read_data[0], read_data[1]));

}

//------------------------------------------------------------------------------
//
//  PddGetTouchData
//
BOOL 
PddGetTouchData(
    UINT32 * xpos,
    UINT32 * ypos
    )
{
    USHORT tempdata[2];
	UINT16 window[MAX_PTS];
    UINT32 sum, pxAvg, pyAvg;
	UINT32 variance;
    int i;
	int index;

    DEBUGMSG(ZONE_FUNCTION, ( TEXT("PddGetTouchData+\r\n" )) );

	// Get initial X values
	sum = 0;
	for (i = 0; i < MAX_PTS; i++)
	{
		// Ask TSC for the X value
		PddGetControllerData(COMMAND_CNV_SELECTXY, tempdata);

		// Add the value to our array and update the sum
		window[i] = tempdata[0];
		sum += tempdata[0];
	}

	// Loop until we get a satisfying X value
	index = 0;
	for (;;)
	{
		// Calculate X average
		pxAvg = sum / MAX_PTS;

		// Calculate X variance (actually the variance multiplied by MAX_PTS)
		variance = 0;
		for (i = 0; i < MAX_PTS; i++)
			variance += (window[i] - pxAvg) * (window[i] - pxAvg);
				
		// Check if X variance is good enough
		if (variance < ALLOWED_X_STANDARD_DEVIATION * ALLOWED_X_STANDARD_DEVIATION * MAX_PTS)
			break;

		// Reset index if needed
		if (index == MAX_PTS)
			index = 0;

		// Ask TSC for a new X value
		PddGetControllerData(COMMAND_CNV_SELECTXY, tempdata);

		// Update the sum and add the value to our array
		sum -= window[index];
		window[index++] = tempdata[0];
		sum += tempdata[0];
	}

	// Get initial Y values
	sum = 0;
	for (i = 0; i < MAX_PTS; i++)
	{
		// Ask TSC for the Y value
		PddGetControllerData(COMMAND_CNV_SELECTXY, tempdata);

		// Add the value to our array and update the sum
		window[i] = tempdata[1];
		sum += tempdata[1];
	}

	// Loop until we get a satisfying Y value
	index = 0;
	for (;;)
	{
		// Calculate Y average
		pyAvg = sum / MAX_PTS;

		// Calculate Y variance (actually the variance multiplied by MAX_PTS)
		variance = 0;
		for (i = 0; i < MAX_PTS; i++)
			variance += (window[i] - pyAvg) * (window[i] - pyAvg);
				
		// Check if Y variance is good enough
		if (variance < ALLOWED_Y_STANDARD_DEVIATION * ALLOWED_Y_STANDARD_DEVIATION * MAX_PTS)
			break;

		// Reset index if needed
		if (index == MAX_PTS)
			index = 0;

		// Ask TSC for a new Y value
		PddGetControllerData(COMMAND_CNV_SELECTXY, tempdata);

		// Update the sum and add the value to our array
		sum -= window[index];
		window[index++] = tempdata[1];
		sum += tempdata[1];
    }

    *xpos = pxAvg;
    *ypos = pyAvg;
    

    DEBUGMSG(ZONE_FUNCTION, ( TEXT("PddGetTouchData-\r\n" )) );

    return TRUE;
}



//------------------------------------------------------------------------------

#define RK_HARDWARE_DEVICEMAP_TOUCH  	(TEXT("HARDWARE\\DEVICEMAP\\TOUCH"))
#define RV_CALIBRATION_DATA				(TEXT("CalibrationData"))

static DWORD CalibrationThread()
{
	HKEY hKey;
	DWORD dwType;
	LONG lResult;
	HANDLE hAPIs;

   	DEBUGMSG(ZONE_FUNCTION, (TEXT("CalibrationThread+\r\n")));    

	// try to open [HKLM\hardware\devicemap\touch] key
	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, RK_HARDWARE_DEVICEMAP_TOUCH, 0, KEY_ALL_ACCESS, &hKey))
	{
		DEBUGMSG(ZONE_CALIBRATE, (TEXT("touchp: calibration: Can't find [HKLM/%s]\r\n"), RK_HARDWARE_DEVICEMAP_TOUCH));
		return 0;
	}

	// check for calibration data (query the type of data only)
	lResult = RegQueryValueEx(hKey, RV_CALIBRATION_DATA, 0, &dwType, NULL, NULL);
    RegCloseKey(hKey);
	if (lResult == ERROR_SUCCESS)
	{
		// registry contains calibration data, return
		return 1;
	}

    hAPIs = OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("SYSTEM/GweApiSetReady"));
    if (hAPIs)
	{
        WaitForSingleObject(hAPIs, INFINITE);
	    CloseHandle(hAPIs);
    }

	// Perform calibration
	TouchCalibrate();

	// try to open [HKLM\hardware\devicemap\touch] key
	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, RK_HARDWARE_DEVICEMAP_TOUCH, 0, KEY_ALL_ACCESS, &hKey))
	{
		RETAILMSG(1, (TEXT("touchp: calibration: Can't find [HKLM/%s]\r\n"), RK_HARDWARE_DEVICEMAP_TOUCH));
		return 0;
	}

	// display new calibration data
	lResult = RegQueryValueEx(hKey, RV_CALIBRATION_DATA, 0, &dwType, NULL, NULL);
	if (lResult == ERROR_SUCCESS)
	{
		TCHAR szCalibrationData[100];
		DWORD Size = sizeof(szCalibrationData);

		RegQueryValueEx(hKey, RV_CALIBRATION_DATA, 0, &dwType, (BYTE *) szCalibrationData, (DWORD *) &Size);
		RETAILMSG(1, (TEXT("touchp: calibration: new calibration data is \"%s\"\r\n"), szCalibrationData));
	}
	RegCloseKey(hKey);
   	
	DEBUGMSG(ZONE_FUNCTION, (TEXT("CalibrationThread-\r\n")));    

	return 1;
}

void StartCalibrationThread()
{
	HANDLE hThread;
	
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CalibrationThread, NULL, 0, NULL);
	// We don't need the handle, close it here
	CloseHandle(hThread);
}

