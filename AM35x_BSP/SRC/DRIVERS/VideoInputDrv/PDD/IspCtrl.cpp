// All rights reserved ADENEO EMBEDDED 2010
// Portions Copyright (c) 2009 BSQUARE Corporation. All rights reserved.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//
#include <windows.h>
#include <bsp.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <sdk_padcfg.h>
#include "ispreg.h"
#include "tvp5146.h"
#include "util.h"
#include "params.h"
#include "ISPctrl.h"
#undef ZONE_INIT
#undef ZONE_ERROR
#include "dbgsettings.h"

//-----------------------------------------------------------------------------
// Global variable

//------------------------------------------------------------------------------
//  Variable define
#define GPIO_MODE_MASK_EVENPIN  0xfffffff8
#define GPIO_MODE_MASK_ODDPIN   0xfff8ffff

//------------------------------------------------------------------------------
//  Buffer size define
#ifdef ENABLE_PACK8
    #define IMAGE_CAMBUFF_SIZE              (X_RES*Y_RES*2)//video input buffer size
    #define NUM_BYTES_LINE                  (X_RES*2)   
#else
    #define IMAGE_CAMBUFF_SIZE              (X_RES*Y_RES*4)//video input buffer size
    #define NUM_BYTES_LINE                  (X_RES*4)
#endif //ENABLE_PACK8


CIspCtrl::CIspCtrl()
{   
    m_pCCDCRegs			= NULL;
	m_pSysConfRegs		= NULL;
    m_pYUVDMAAddr		= NULL;
    m_pYUVVirtualAddr   = NULL;
    m_pYUVPhysicalAddr  = NULL;
	m_bCCDCInitialized  = FALSE;
}

CIspCtrl::~CIspCtrl()
{
	if (m_pSysConfRegs != NULL)
	{
		MmUnmapIoSpace((PVOID)m_pSysConfRegs, sizeof(OMAP_SYSC_GENERAL_REGS));
	}

	if (m_pSysConfRegs != NULL)
	{
		MmUnmapIoSpace((PVOID)m_pCCDCRegs, sizeof(CAM_ISP_CCDC_REGS));
	}
}

//-----------------------------------------------------------------------------
//
//  Function:       GetPhysFromVirt
//
//  Maps the Virtual address passed to a physical address.
//
//  returns a physical address with a page boundary size.
//
LPVOID
CIspCtrl::GetPhysFromVirt(
    ULONG ulVirtAddr
    )
{
    ULONG aPFNTab[1];
    ULONG ulPhysAddr;

    if (LockPages((LPVOID)ulVirtAddr, UserKInfo[KINX_PAGESIZE], aPFNTab,
                   LOCKFLAG_QUERY_ONLY))
    {
		// Merge PFN with address offset to get physical address         
		ulPhysAddr= ((*aPFNTab << UserKInfo[KINX_PFN_SHIFT]) & UserKInfo[KINX_PFN_MASK])|(ulVirtAddr & 0xFFF);
	} 
	else 
	{
		ulPhysAddr = 0;
	}

    return ((LPVOID)ulPhysAddr);
}

//------------------------------------------------------------------------------
//
//  Function:  MapCameraReg
//
//  Read data from register
//      
BOOL CIspCtrl::MapCameraReg()
{
    PHYSICAL_ADDRESS pa;
	BOOL bRet		 = TRUE;

	DEBUGMSG(ZONE_FUNCTION,(TEXT("+MapCameraReg\r\n")));
    
    // Map Camera ISP CCDC register
    pa.QuadPart = GetAddressByDevice(OMAP_DEVICE_VPFE);
    m_pCCDCRegs = (CAM_ISP_CCDC_REGS *)MmMapIoSpace(pa, sizeof(CAM_ISP_CCDC_REGS), FALSE);
    if (m_pCCDCRegs == NULL)
    {
        ERRORMSG(TRUE,(TEXT("Failed map Camera ISP CCDC physical address to virtual address!\r\n")));
		bRet = FALSE;
        goto exit;
    }

	pa.QuadPart = (LONGLONG)OMAP_SYSC_GENERAL_REGS_PA;
	m_pSysConfRegs = (OMAP_SYSC_GENERAL_REGS*)MmMapIoSpace(pa, sizeof(OMAP_SYSC_GENERAL_REGS), FALSE);
    if (m_pSysConfRegs == NULL)
    {
        ERRORMSG(TRUE,(TEXT("Failed to map SysConfig registers!\r\n")));
		bRet = FALSE;
        goto exit;
    }    

exit:

	DEBUGMSG(ZONE_FUNCTION,(TEXT("-MapCameraReg\r\n")));

    return bRet;
}    

//------------------------------------------------------------------------------
//
//  Function:  CCDCInitSYNC
//
//  Init. ISPCCDC_SYN_MODE register 
//
//
BOOL CIspCtrl::CCDCInitSYNC()
{
	DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCInitSYNC\r\n")));

	UINT32 syn_mode = 0 ;
	syn_mode |= ISPCCDC_SYN_MODE_WEN;// Video data to memory 
	syn_mode |= ISPCCDC_SYN_MODE_DATSIZ_10;// cam_d is 10 bits
	syn_mode |= ISPCCDC_SYN_MODE_VDHDEN;// Enable timing generator
	syn_mode = (syn_mode & ISPCCDC_SYN_MODE_INPMOD_MASK)|ISPCCDC_SYN_MODE_FLDMODE | ISPCCDC_SYN_MODE_INPMOD_RAW;//Set input mode:Interlanced, RAW
#ifdef ENABLE_PACK8     
	syn_mode |=ISPCCDC_SYN_MODE_PACK8; //pack 8-bit in memory
#endif //   ENABLE_PACK8    
	ISP_OutReg32(&m_pCCDCRegs->CCDC_SYN_MODE, syn_mode);

#ifdef ENABLE_BT656     
	ISP_OutReg32(&m_pCCDCRegs->CCDC_REC656IF, ISPCCDC_REC656IF_R656ON); //enable BT656
#endif //ENABLE_BT656

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-CCDCInitSYNC\r\n")));

	return TRUE;                
}       

//------------------------------------------------------------------------------
//
//  Function:  CCDCInitCFG
//
//  Init. Camera CCDC   
//
BOOL CIspCtrl::CCDCInitCFG()
{
	UINT32 setting = 0;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCInitCFG\r\n")));

	// Request Init
	ISP_InReg32(&m_pCCDCRegs->CCDC_CFG, &setting);

	setting |= (ISPCCDC_CFG_VDLC | (1 << ISPCCDC_CFG_FIDMD_SHIFT));

#ifdef ENABLE_PACK8
	setting |=ISPCCDC_CFG_BSWD; //swap byte
#endif //ENABLE_PACK8

#ifdef ENABLE_BT656
	#ifndef ENABLE_PACK8
	setting |=ISPCCDC_CFG_BW656; //using 10-bit BT656
	#endif //ENABLE_BT656
#endif //ENABLE_BT656

	ISP_OutReg32(&m_pCCDCRegs->CCDC_CFG, setting);

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-CCDCInitCFG\r\n")));

	return TRUE;        
}       

//------------------------------------------------------------------------------
//
//  Function:  ConfigOutlineOffset
//
//  Configures the output line offset when stored in memory.
//  Configures the num of even and odd line fields in case of rearranging
//  the lines
//  offset: twice the Output width and aligned on 32byte boundary.
//  oddeven: odd/even line pattern to be chosen to store the output
//  numlines: Configure the value 0-3 for +1-4lines, 4-7 for -1-4lines
//
BOOL CIspCtrl::ConfigOutlineOffset(UINT32 offset, UINT8 oddeven, UINT8 numlines)
{       
	DEBUGMSG(ZONE_FUNCTION, (TEXT("+ConfigOutlineOffset\r\n")));
	UINT32 setting = 0;    

	// Make sure offset is multiple of 32bytes. ie last 5bits should be zero 
	setting = offset & ISP_32B_BOUNDARY_OFFSET;
	ISP_OutReg32(&m_pCCDCRegs->CCDC_HSIZE_OFF, setting);

	// By default Donot inverse the field identification 
	ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
	setting &= (~ISPCCDC_SDOFST_FINV);
	ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);

	// By default one line offset
	ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
	setting &= ISPCCDC_SDOFST_FOFST_1L;
	ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);

	switch (oddeven) {
	case EVENEVEN:      /*even lines even fields*/
		ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
		setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST0_SHIFT);
		ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
	break;
	case ODDEVEN:       /*odd lines even fields*/
		ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
		setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST1_SHIFT);
		ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
	break;
	case EVENODD:       /*even lines odd fields*/
		ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
		setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST2_SHIFT);
		ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
	break;
	case ODDODD:        /*odd lines odd fields*/
		ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
		setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST3_SHIFT);
		ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
	break;
	default:
	break;
	}

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-ConfigOutlineOffset\r\n")));

	return TRUE;
}       

//------------------------------------------------------------------------------
//
//  Function:  CCDCSetOutputAddress
//
//  Configures the memory address where the output should be stored.
//
BOOL CIspCtrl::CCDCSetOutputAddress(ULONG SDA_Address)
{       
	ISP_OutReg32(&m_pCCDCRegs->CCDC_SDR_ADDR, SDA_Address & ISP_32B_BOUNDARY_BUF);

	return TRUE;            
}   

//------------------------------------------------------------------------------
//
//  Function:  CCDCEnable
//
//  Enables the CCDC module.
//
BOOL CIspCtrl::CCDCEnable(BOOL bEnable)
{       
	BOOL rc			= FALSE;
	UINT32 setting  = 0;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCEnable\r\n"))); 

	ISP_InReg32(&m_pCCDCRegs->CCDC_PCR, &setting);  
	if (bEnable == 1)
	{
		setting |= (ISPCCDC_PCR_EN);
	}
	else
	{
		setting &= ~(ISPCCDC_PCR_EN);
	}
	    
	rc = ISP_OutReg32(&m_pCCDCRegs->CCDC_PCR, setting);

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-CCDCEnable\r\n"))); 

	return rc;          
}   

//------------------------------------------------------------------------------
//
//  Function:  AllocBuffer
//
//  AllocBuffer for video input and format transfer out 
//
BOOL CIspCtrl::AllocBuffer()
{
	BOOL bRet    = TRUE;
	DWORD dwSize = IMAGE_CAMBUFF_SIZE;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+AllocBuffer\r\n")));

	if(m_pYUVDMAAddr != NULL)
	{
		goto exit;
	}

	m_camBuffer.ObjectSize = sizeof(m_camBuffer);
	m_camBuffer.InterfaceType = Internal;
	m_camBuffer.BusNumber = 0;        

	m_pYUVDMAAddr = (PBYTE)HalAllocateCommonBuffer(&m_camBuffer, dwSize, &m_CamBufferPhys, TRUE );

	if (m_pYUVDMAAddr == NULL)
	{   
		ERRORMSG(TRUE, (TEXT("HalAllocateCommonBuffer failed !!!\r\n")));
		bRet = FALSE;
		goto exit;
	}

	m_pYUVVirtualAddr = (PBYTE)VirtualAlloc(NULL, dwSize, MEM_RESERVE, PAGE_NOACCESS);        

	if (m_pYUVVirtualAddr == NULL)
	{    
		ERRORMSG(TRUE, (TEXT("Sensor buffer memory alloc failed !!!\r\n")));
		bRet = FALSE;
		goto exit;
	}

	VirtualCopy(m_pYUVVirtualAddr, (VOID *) (m_CamBufferPhys.LowPart >> 8), dwSize, PAGE_READWRITE | PAGE_PHYSICAL );

	m_pYUVPhysicalAddr = GetPhysFromVirt((ULONG)m_pYUVVirtualAddr);
	if(!m_pYUVPhysicalAddr)
	{
		ERRORMSG(TRUE,(_T("GetPhysFromVirt 0x%08X failed: \r\n"), m_pYUVVirtualAddr));
		bRet = FALSE;
		goto exit;
	}

	DEBUGMSG(ZONE_VERBOSE, (TEXT("m_pYUVVirtualAddr=0x%x\r\n"),m_pYUVVirtualAddr));   
	DEBUGMSG(ZONE_VERBOSE, (TEXT("m_pYUVPhysicalAddr=0x%x\r\n"),m_pYUVPhysicalAddr)); 
	DEBUGMSG(ZONE_VERBOSE, (TEXT("m_pYUVDMAAddr=0x%x\r\n"),m_pYUVDMAAddr));   

exit:

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-AllocBuffer\r\n")));

	return bRet;      
}   

//------------------------------------------------------------------------------
//
//  Function:  DeAllocBuffer
//
//  DeAllocBuffer for video input and format transfer out   
//
BOOL CIspCtrl::DeAllocBuffer()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+DeAllocBuffer\r\n")));

    if (m_pYUVDMAAddr == NULL)
	{
		goto exit;
	}

	HalFreeCommonBuffer(&m_camBuffer, 
						IMAGE_CAMBUFF_SIZE, 
						m_CamBufferPhys, 
						m_pYUVVirtualAddr, 
						FALSE);

    VirtualFree(m_pYUVVirtualAddr, 0, MEM_RELEASE);

    m_pYUVVirtualAddr=NULL;
    m_pYUVPhysicalAddr=NULL;
    m_pYUVDMAAddr=NULL;

exit:

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-DeAllocBuffer\r\n")));

    return TRUE;    
}

//------------------------------------------------------------------------------
//
//  Function:  CCDCInit
//
//  Init. Camera CCDC   
//
BOOL CIspCtrl::CCDCInit()
{
	BOOL bRet = TRUE;

	if (m_bCCDCInitialized)
	{
		goto exit;
	}

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCInit\r\n")));

	SETREG32(&m_pSysConfRegs->CONTROL_IPSS_CLK_CTRL, VPFE_FUNC_CLK_EN);
	SETREG32(&m_pSysConfRegs->CONTROL_IPSS_CLK_CTRL, VPFE_VBUSP_CLK_EN);

	Sleep(10);

	SETREG32(&m_pSysConfRegs->CONTROL_IP_SW_RESET, VPFE_PCLK_SW_RST);
	Sleep(10);
	CLRREG32(&m_pSysConfRegs->CONTROL_IP_SW_RESET, VPFE_PCLK_SW_RST);

	Sleep(10);

	SETREG32(&m_pSysConfRegs->CONTROL_IP_SW_RESET, VPFE_VBUSP_SW_RST);
	Sleep(10);
	CLRREG32(&m_pSysConfRegs->CONTROL_IP_SW_RESET, VPFE_VBUSP_SW_RST);
	Sleep(10);

	if (!RequestDevicePads(OMAP_DEVICE_VPFE))
	{
		DEBUGMSG(ZONE_ERROR, (TEXT(" CCDCInit : Failed to request Pads\r\n")));
		bRet = FALSE;
		goto exit;
	}

	CCDCInitCFG();
	CCDCInitSYNC();

	// Allocate buffer for YUV
	if (!AllocBuffer())
	{
		bRet = FALSE;
		goto exit;
	}

	// Set CCDC_SDR address
	if (!CCDCSetOutputAddress((ULONG) m_pYUVPhysicalAddr))
	{
		bRet = FALSE;
		goto exit;
	}

	m_bCCDCInitialized = TRUE;

exit:

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-CCDCInit\r\n")));

	return bRet;        
}       

//------------------------------------------------------------------------------
//
//  Function:  CCDCDeInit
//
//  Deinitialize Camera CCDC   
//
BOOL CIspCtrl::CCDCDeInit()
{
	BOOL bRet = TRUE;

	if (!m_bCCDCInitialized)
	{
		goto exit;
	}

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCDeInit\r\n")));

	// Release pads
	if (!ReleaseDevicePads(OMAP_DEVICE_VPFE))
	{
		DEBUGMSG(ZONE_ERROR, (TEXT(" CCDCDeinit : Failed to release pads\r\n")));
		bRet = FALSE;
		goto exit;
	}

	// Free camera buffer
	if (!DeAllocBuffer())
	{
		DEBUGMSG(ZONE_ERROR, (TEXT(" CCDCDeinit : Failed to deallocate buffer\r\n")));
		bRet = FALSE;
		goto exit;
	}

	// Clear CCDC_SDR address
	if (!CCDCSetOutputAddress(NULL))
	{
		bRet = FALSE;
		goto exit;
	}

	m_bCCDCInitialized = FALSE;

exit:

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-CCDCDeInit\r\n")));

	return bRet;
}

//------------------------------------------------------------------------------
//
//  Function:  ISPConfigSize
//
//
// Configures CCDC HORZ/VERT_INFO registers to decide the start line
// stored in memory.
//
// output_w : output width from the CCDC in number of pixels per line
// output_h : output height for the CCDC in number of lines
//
//
BOOL CIspCtrl::ISPConfigSize()
{
	UINT32 setting = 0;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+ISPConfigSize\r\n")));

	// Set output_w         
	setting = (HORZ_INFO_SPH_VAL << ISPCCDC_HORZ_INFO_SPH_SHIFT) | ((X_RES*2 - 1)<< ISPCCDC_HORZ_INFO_NPH_SHIFT);
	ISP_OutReg32(&m_pCCDCRegs->CCDC_HORZ_INFO, setting);

	//vertical shift
	setting = ((VERT_START_VAL) << ISPCCDC_VERT_START_SLV0_SHIFT | (VERT_START_VAL) << ISPCCDC_VERT_START_SLV1_SHIFT);
	ISP_OutReg32(&m_pCCDCRegs->CCDC_VERT_START, setting);
	     
	// Set output_h
	setting = (Y_RES/2 - 1) << ISPCCDC_VERT_LINES_NLV_SHIFT;
	ISP_OutReg32(&m_pCCDCRegs->CCDC_VERT_LINES, setting);

	ConfigOutlineOffset(NUM_BYTES_LINE, 0, 0);
#ifdef ENABLE_DEINTERLANCED    //There is no field pin connected to OMAP3 from tvp5416, so only for BT656.
	//de-interlance
	ConfigOutlineOffset(NUM_BYTES_LINE, EVENEVEN, 1);
	ConfigOutlineOffset(NUM_BYTES_LINE, ODDEVEN, 1);
	ConfigOutlineOffset(NUM_BYTES_LINE, EVENODD, 1);
	ConfigOutlineOffset(NUM_BYTES_LINE, ODDODD, 1);   
#endif //ENABLE_DEINTERLANCED       

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-ISPConfigSize\r\n")));

	return TRUE;        
}

//------------------------------------------------------------------------------
//
//  Function:  IsCCDCBusy
//
//  To check CCDC busy bit
//
BOOL CIspCtrl::IsCCDCBusy()
{
	UINT32 setting = 0;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+IsCCDCBusy\r\n")));
	        
	ISP_InReg32(&m_pCCDCRegs->CCDC_PCR, &setting); 
	setting &= ISPCCDC_PCR_BUSY;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-IsCCDCBusy\r\n")));

	return (setting != 0) ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  InitializeCamera
//
//  To initialize Camera .
//
BOOL CIspCtrl::InitializeCamera()
{   
	BOOL bRet = TRUE;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+InitializeCamera\r\n")));    
            
	// Map camera registers
    bRet = MapCameraReg(); 
        
    DEBUGMSG(ZONE_FUNCTION, (TEXT("-InitializeCamera\r\n")));    

    return bRet;
}

//------------------------------------------------------------------------------
//
//  Function:  EnableCamera
//
//  To enable Camera.
//
BOOL CIspCtrl::EnableCamera()
{    
	BOOL bRet = TRUE;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+EnableCamera\r\n")));    
        
	// Initialize CCDC
    if (!CCDCInit())               
	{
		bRet = FALSE;
		goto exit;
	}

	// Initialize video size 
	if (!ISPConfigSize())
	{
		bRet = FALSE;
		goto exit;
	}

	// Clear the frame buffer
    memset(m_pYUVVirtualAddr,0,IMAGE_CAMBUFF_SIZE); 

    if (!CCDCEnable(TRUE))
	{
		bRet = FALSE;
		goto exit;
	}

exit:

    DEBUGMSG(ZONE_FUNCTION, (TEXT("-EnableCamera\r\n")));    

    return bRet;
}

//------------------------------------------------------------------------------
//
//  Function:  DisableCamera
//
//  To disable Camera.
//
BOOL CIspCtrl::DisableCamera()
{
	BOOL bRet = TRUE;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+DisableCamera\r\n")));  

	// Disable CCDC
	if (!CCDCEnable(FALSE))
	{
		bRet = FALSE;
		goto exit;
	}

	// Deinitialize CCDC
    if (!CCDCDeInit())
	{
		bRet = FALSE;
		goto exit;
	}

exit:

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-DisableCamera\r\n")));  

	return bRet;
}

//------------------------------------------------------------------------------
//
//  Function:  ChangeFrameBuffer
//
//  To Change the frame buffer address to CCDC_SDR.
//
BOOL CIspCtrl::ChangeFrameBuffer(ULONG ulVirtAddr)
{
	BOOL bRet = TRUE;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("+ChangeFrameBuffer\r\n")));  

	m_pYUVVirtualAddr= (LPVOID) ulVirtAddr;
	m_pYUVPhysicalAddr = GetPhysFromVirt((ULONG)m_pYUVVirtualAddr);
	if(!m_pYUVPhysicalAddr)
	{
		bRet = FALSE;
		goto exit;
	}

	//Set CCDC_SDR address
	if (!CCDCSetOutputAddress((ULONG) m_pYUVPhysicalAddr))
	{
		bRet = FALSE;
		goto exit;
	}

exit:

	DEBUGMSG(ZONE_FUNCTION, (TEXT("-ChangeFrameBuffer\r\n")));  

	return bRet;
}

