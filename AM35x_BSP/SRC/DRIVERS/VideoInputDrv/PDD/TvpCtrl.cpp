// All rights reserved ADENEO EMBEDDED 2010

#include <windows.h>
#include <bsp.h>
#include <ceddk.h>
#include <ceddkex.h>
#include "tvp5146.h"
#include "util.h"
#define INSTANT_TVP_SETTINGS
#include "params.h"
#include "TvpCtrl.h"
#include "soc_cfg.h"
#include "sdk_i2c.h"

//------------------------------------------------------------------------------
//  I2C Address define
#define TVP5146_I2C_DEVICE			3
#define TVP5146_I2C_DEVICE_ADDR     (0xB8 >> 1)
    
CTvpCtrl::CTvpCtrl()
{   
    m_hI2C = NULL;
}

CTvpCtrl::~CTvpCtrl()
{
}

//------------------------------------------------------------------------------
//
//  Function:  TVPInit
//
//  Init. TVP5146   video decoder
//
BOOL CTvpCtrl::Init()
{
	I2CInit();

	if (m_hI2C == NULL)
	{
		goto cleanup;
	}

	// Write all setting registers
	for(UINT i=0;i<NUM_TVP_SETTINGS;i++)
	{
		WriteReg(tvpSettings[i].reg, tvpSettings[i].val);
	}

cleanup:

	I2CDeinit();    

	return TRUE;                
}   

//------------------------------------------------------------------------------
//
//  Function:  TVPReadReg
//
//  Read data from TVP5146 register
//      
BOOL CTvpCtrl::ReadReg(UINT8 slaveaddress, UINT8* data)
{
    BOOL rc   = FALSE;
	DWORD len = 0;

    if (m_hI2C)
    {
        len = I2CRead(m_hI2C, slaveaddress, data, sizeof(UINT8));
        if ( len != sizeof(UINT8))
		{
            ERRORMSG(ZONE_ERROR,(TEXT("TVPReadReg Failed!!\r\n")));
		}
        else
		{
            rc = TRUE;
		}
	}
    
	return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TVPWriteReg
//
//  Read data from TVP5146 register
//      
BOOL CTvpCtrl::WriteReg(UINT8 slaveaddress, UINT8 value)
{
    BOOL rc = FALSE;
	DWORD len = 0;

    if (m_hI2C)
    {
        len = I2CWrite(m_hI2C, slaveaddress, &value, sizeof(UINT8));
        if ( len != sizeof(UINT8))
		{
            ERRORMSG(ZONE_ERROR,(TEXT("TVPWriteReg Failed!!\r\n")));
		}
        else
		{
               rc = TRUE;
		}
	}

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  I2CInit
//
//  Init TVP5146 I2C interface
//      
BOOL CTvpCtrl::I2CInit()
{
    m_hI2C = I2COpen(SOCGetI2CDeviceByBus(TVP5146_I2C_DEVICE));
	if (m_hI2C == NULL)
	{
		return FALSE;
	}

    I2CSetSlaveAddress(m_hI2C,  TVP5146_I2C_DEVICE_ADDR); 
    I2CSetSubAddressMode(m_hI2C, I2C_SUBADDRESS_MODE_8);    
    
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  I2CDeinit
//
//  Deinit TVP5146 I2C interface
//      
BOOL CTvpCtrl::I2CDeinit()
{
    if (m_hI2C != NULL)
    {
        I2CClose(m_hI2C);
        m_hI2C = NULL;
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SelectComposite
//
//  To select composite path
//
BOOL CTvpCtrl::SelectComposite()
{       
	BOOL bRet = TRUE;

	I2CInit();      

	bRet = WriteReg(REG_INPUT_SEL, 0x00); // Composite path

	I2CDeinit();        

	return bRet;
}

//------------------------------------------------------------------------------
//
//  Function:  SelectSVideo
//
//  To select s-video path
//
BOOL CTvpCtrl::SelectSVideo()
{
	BOOL bRet = TRUE;

    I2CInit();

    bRet = WriteReg(REG_INPUT_SEL, 0x46); // s-video path

    I2CDeinit();        

    return bRet;
}

//------------------------------------------------------------------------------
//
//  Function:  SelectComponent
//
//  To select component path
//
BOOL CTvpCtrl::SelectComponent()
{
	BOOL bRet = TRUE;

    I2CInit();

    bRet = WriteReg(REG_INPUT_SEL, 0x85);// Component path

    I2CDeinit();

    return bRet;
}

//------------------------------------------------------------------------------
//
//  Function:  SetPowerState
//
//  Set the TVP's power state
//
BOOL CTvpCtrl::SetPowerState(BOOL PowerOn)
{
	BOOL bRet = TRUE;

    I2CInit();

    bRet = WriteReg(REG_OPERATION_MODE, PowerOn ? 0x00 : 0x01);

    I2CDeinit();

    return bRet;
}
