// All rights reserved ADENEO EMBEDDED 2010

#ifndef _SENSOR_CTRL_H
#define _SENSOR_CTRL_H

#include "am3517.h"
#include "ispreg.h"

#ifdef __cplusplus
extern "C" {
#endif

class CIspCtrl
{
public:
    
    CIspCtrl();
    ~CIspCtrl();
    
    BOOL					InitializeCamera();    
    BOOL					EnableCamera();
    BOOL					DisableCamera();
    CAM_ISP_CCDC_REGS*		GetCCDCRegs() { return m_pCCDCRegs; }
    OMAP_SYSC_GENERAL_REGS* GetSyscRegs() { return m_pSysConfRegs; }
    LPVOID					GetFrameBuffer() { return m_pYUVVirtualAddr; }
    BOOL					ChangeFrameBuffer(ULONG ulVirtAddr);
    
private:
	OMAP_SYSC_GENERAL_REGS  *m_pSysConfRegs;
    CAM_ISP_CCDC_REGS       *m_pCCDCRegs; 
	DMA_ADAPTER_OBJECT		m_camBuffer;
	PHYSICAL_ADDRESS		m_CamBufferPhys; 
    LPVOID                  m_pYUVVirtualAddr;      //YUV virtual address
    LPVOID                  m_pYUVPhysicalAddr;     //YUV physical address
    LPVOID                  m_pYUVDMAAddr;          //YUV DMA address
	BOOL					m_bCCDCInitialized;

    LPVOID GetPhysFromVirt(
        ULONG ulVirtAddr
    );
    
    BOOL MapCameraReg();
       
    BOOL ConfigGPIO4MDC();
    
    BOOL CCDCInitCFG();
    
    BOOL ConfigOutlineOffset(
        UINT32 offset, 
        UINT8 oddeven, 
        UINT8 numlines);
        
    BOOL CCDCSetOutputAddress(
        ULONG SDA_Address);
        
    BOOL CCDCEnable(
        BOOL bEnable);
    
    BOOL AllocBuffer();
        
    BOOL DeAllocBuffer();
        
    BOOL CCDCInit();
    
	BOOL CCDCDeInit();

    BOOL ISPInit();
    
    BOOL ISPEnable(
        BOOL bEnable);

    BOOL ISPConfigSize();

    BOOL IsCCDCBusy();
    
    BOOL Check_IRQ0STATUS();
    
    BOOL CCDCInitSYNC();    
  
};

#ifdef __cplusplus
}
#endif

#endif