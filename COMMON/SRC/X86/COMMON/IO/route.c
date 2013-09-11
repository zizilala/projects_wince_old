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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//  
// -----------------------------------------------------------------------------
#include <windows.h>
#include <oal.h>
#include <nkintr.h>
#include <x86boot.h>
#include <PCIReg.h>
#include "pci.h"


// Inline functions
__inline static DWORD
PCIConfig_Read(
    ULONG BusNumber,
    ULONG Device,
    ULONG Function,
    ULONG Offset
    )
{
    ULONG RetVal = FALSE;
    
    PCIReadBusData(BusNumber, Device, Function, &RetVal, Offset, sizeof(RetVal));

    return RetVal;
}


__inline static void
PCIConfig_Write(
    ULONG BusNumber,
    ULONG Device,
    ULONG Function,
    ULONG Offset,
    ULONG Value
    )
{
    PCIWriteBusData(BusNumber, Device, Function, &Value, Offset, sizeof(Value));
}


BOOL
PCIReadBARs(
    PPCI_REG_INFO pInfo
    )
{   
    DWORD NumberOfRegs;
    DWORD Offset;
    DWORD i;
    DWORD BaseAddress;
    DWORD Size;
    DWORD Reg;
    DWORD IoIndex = 0;
    DWORD MemIndex = 0;

    // Determine number of BARs to examine from header type
    switch (pInfo->Cfg.HeaderType & ~PCI_MULTIFUNCTION) {
    case PCI_DEVICE_TYPE:
        NumberOfRegs = PCI_TYPE0_ADDRESSES;
        break;

    case PCI_BRIDGE_TYPE:
        NumberOfRegs = PCI_TYPE1_ADDRESSES;
        break;

    case PCI_CARDBUS_TYPE:
        NumberOfRegs = PCI_TYPE2_ADDRESSES;
        break;

    default:
        return FALSE;
    }
        
    for (i = 0, Offset = 0x10; i < NumberOfRegs; i++, Offset += 4) {
        // Get base address register value
        Reg = pInfo->Cfg.u.type0.BaseAddresses[i];

        // Get size info
        PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, Offset, 0xFFFFFFFF);
        BaseAddress = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, Offset);
        PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, Offset, Reg);

        // Re-adjust BaseAddress if upper 16-bits are 0 (this happens on some devices that don't follow
        // the PCI spec, like the Intel UHCI controllers)
        if (((BaseAddress & 0xFFFFFFFC) != 0) && ((BaseAddress & 0xFFFF0000) == 0)) {
            BaseAddress |= 0xFFFF0000;
        }
        
        if (Reg & 1) {
            // IO space
            Size = ~(BaseAddress & 0xFFFFFFFC);

            if ((BaseAddress != 0) && (BaseAddress != 0xFFFFFFFF) && (((Size + 1) & Size) == 0)) {
                // BAR has valid format (consecutive high 1's and consecutive low 0's)
                pInfo->IoLen.Reg[IoIndex] = Size + 1;
                pInfo->IoLen.Num++;
                pInfo->IoBase.Reg[IoIndex++] = Reg & 0xFFFFFFFC;
                pInfo->IoBase.Num++;
            } else {
                // BAR invalid => skip to next one
                continue;
            }
        } else {
            // Memory space
            // TODO: don't properly handle the MEM20 case
            Size = ~(BaseAddress & 0xFFFFFFF0);
            
            if ((BaseAddress != 0) && (BaseAddress != 0xFFFFFFFF) && (((Size + 1) & Size) == 0)) {
                // BAR has valid format (consecutive high 1's and consecutive low 0's)
                pInfo->MemLen.Reg[MemIndex] = Size + 1;
                pInfo->MemLen.Num++;
                pInfo->MemBase.Reg[MemIndex++] = Reg & 0xFFFFFFF0;
                pInfo->MemBase.Num++;
            } else {
                // BAR invalid => skip to next one
                continue;
            }
        }

        //
        // check for 64 bit device - BAR is twice as big
        //
        if ((pInfo->Cfg.u.type0.BaseAddresses[i] & 0x7) == 0x4) {
            // 64 bit device - BAR is twice as wide - zero out high part
            Offset += 4;
            i++;
        }
    }

    return TRUE;
}
//----------------------------------------------------------------------------------
// PCI BIOS (Routing Table Support)
#define BIOS_START  0xe0000
#define BIOS_LENGTH 0x20000
#pragma pack(1)
typedef struct {
    BYTE PCIBusNumber;
    BYTE PCIDeviceNumber;// In upper 5bits
    
    BYTE INTA_LinkValue;
    unsigned short INTA_IrqBitMap;
    
    BYTE INTB_LinkValue;
    unsigned short INTB_IrqBitMap;
    
    BYTE INTC_LinkValue;
    unsigned short INTC_IrqBitMap;
    
    BYTE INTD_LinkValue;
    unsigned short INTD_IrqBitMap;

    BYTE SlotNumber;
    BYTE Reserved;
} RoutingOptionTable;

typedef struct {
    DWORD   Signature;
    WORD    Version;
    WORD    Table_Size;
    BYTE    Bus;
    BYTE    DeviceFunc;
    WORD    ExclusiveIrqs;
    DWORD   CompatibleRouter;
    DWORD   MiniportData;
    BYTE    Reseved[11];
    BYTE    Checksum;
} PCI_ROUTING_TABLE;


typedef struct 
{
    ULONG   magic;
    ULONG   phys_bsd_entry;
    UCHAR   vers,prg_lens,crc;
} BIOS32;

typedef struct {
    DWORD RegEax;
    DWORD RegEbx;
    DWORD RegEcx;
    DWORD RegEdx;
    DWORD RegEsi;
    DWORD RegEdi;
} Reg32;
#define MAX_DEVICE 0x20
typedef struct {
    unsigned short BufferSize;
    BYTE * pDataBuffer;
    DWORD DS;
    RoutingOptionTable routingOptionTable[MAX_DEVICE];
} IRQRountingOptionsBuffer;

typedef struct _irqLink {
    BYTE linkValue;
    BYTE bus;
    BYTE device;
    struct _irqLink* pNext;
} irqLink, *pIrqLink;

#pragma pack() 

extern BOOL CallBios32(Reg32 * pReg,PVOID pCallAddr);

BOOL search_pci_bios(PBYTE pBiosAddr,ULONG * pphOffset)
{
    DWORD       p=0;
    BIOS32      *x;
    UCHAR       flag=0;
    UCHAR       crc;
    int         i;

    BIOS32 *master_bios32=NULL;
    DEBUGMSG(1,(L"search_pci_bios start\r\n"));
    while(pphOffset && flag==0 && p<BIOS_LENGTH)
    {
        x=(BIOS32*)(pBiosAddr+p);
        if (x->magic==0x5F32335F)   /* _32_ */
        {
            for(i=0, crc=0; i<(x->prg_lens*16); i++)
            crc+=*(pBiosAddr+p+i);
            if(crc==0)
            {
                flag=1;
                master_bios32=x;
                *pphOffset=master_bios32->phys_bsd_entry-BIOS_START;
                DEBUGMSG(1,(L"CE Ethernet Bootloader found 32Bit BIOS Entry master_bios32=%x bios32_call_offset=%x for CE/PC \n",
                    master_bios32,*pphOffset));
                return TRUE;
            }
        }
        p+=0x10;
    }
    DEBUGMSG(1,(L"search_pci_bios end fails\r\n"));
    return FALSE;
}
#define PCI_FUNCTION_ID 0xB1
#define PCI_BIOS_PRESENT 0x01
#define FIND_PCI_DEVICE 0x02
#define FIND_PCI_CLASS_CODE 0x03
#define GENERATE_SPECIAL_CYCLE 0x06
#define READ_CONFIG_BYTE 0x8
#define READ_CONFIG_WORD 0x9
#define READ_CONFIG_DWORD 0xA
#define WRITE_CONFIG_BYTE 0xB
#define WRITE_CONFIG_WORD 0xC
#define WRITE_CONFIG_DWORD 0xD
#define GET_IRQ_ROUTING_OPTIONS 0xE
#define SET_PCI_IRQ 0xF

#define NUM_IRQS 0x10
#define IRQ_LINK_POOL_SIZE ((VM_PAGE_SIZE / (sizeof(irqLink))) - NUM_IRQS) // let's limit our pool to 1 page in size

// Simple allocation pool for the irqToLinkValue table
irqLink irqLinkPool[IRQ_LINK_POOL_SIZE];
pIrqLink irqToLinkValue[NUM_IRQS];

// allocates an unused link from the allocation pool.  Note that there is no way to dealloc as it is
// expected that this table will only need to be set up once.
static pIrqLink allocFreeIrqLink()
{
    static unsigned int irqLinkPoolCounter = 0;
    if(irqLinkPoolCounter >= (IRQ_LINK_POOL_SIZE - 1)) {
        // We've run out of memory in our pool
        OALMSG(OAL_ERROR, (L"irqLinkPoolCounter (%d) exceeded allocation pool size!\r\n", irqLinkPoolCounter));
        return 0;
    }
    else {
        // Allocate the next free block
        irqLinkPoolCounter++;
        return &(irqLinkPool[irqLinkPoolCounter]);
    }
}

// allocates an irqLink at inIrqLink and fills the value with the link number
static BOOL addIrqLink(pIrqLink* inIrqLink, DWORD inIrq, BYTE inLinkNumber, BYTE inBus, BYTE inDevice)
{
    if(!((*inIrqLink) = allocFreeIrqLink())) {
        OALMSG(OAL_ERROR, (L"Failed allocation for IrqToLink, link %x will not be associated with Irq %d\r\n", inLinkNumber, inIrq));
    } else {
        // Allocation successful, update the entry with the new value
        (*inIrqLink)->linkValue = inLinkNumber;
        (*inIrqLink)->bus = inBus;
        (*inIrqLink)->device = inDevice;        
        DEBUGMSG(1,(L"addIrqLink: LinkNumber=%x,bus=%d,device=%d associated with irq=%d\r\n",inLinkNumber,inBus,inDevice,inIrq));
        return TRUE;
    }
    return FALSE;
}

PVOID pBiosAddr=NULL;
IRQRountingOptionsBuffer irqRoutingOptionBuffer;
WORD wBestPCIIrq=0;


extern DWORD GetDS();
void DumpRoutingOption(IRQRountingOptionsBuffer *pBuffer,WORD wExClusive)
{
    DEBUGMSG(1,(L"DumpRoutingOption with PCI Exclusive Irq Bit (wExClusive)  =%x \r\n",wExClusive));
    if (pBuffer) {
        RoutingOptionTable * pRoute;
        DWORD dwCurPos=0;
        DEBUGMSG(1,(L"DumpRoutingOption BufferSize = %d @ address %x \r\n", pBuffer->BufferSize,pBuffer->pDataBuffer));
        pRoute =(RoutingOptionTable * )(pBuffer->routingOptionTable);
        while (pRoute && dwCurPos + sizeof(RoutingOptionTable)<=pBuffer->BufferSize) {
            DEBUGMSG(1,(L"DumpRouting for Bus=%d ,Device=%d SlotNumber=%d\r\n",
                pRoute->PCIBusNumber,(pRoute->PCIDeviceNumber)>>3,pRoute->SlotNumber));
            DEBUGMSG(1,(L"     INTA_LinkValue=%x,INTA_IrqBitMap=%x\r\n",pRoute->INTA_LinkValue,pRoute->INTA_IrqBitMap));
            DEBUGMSG(1,(L"     INTB_LinkValue=%x,INTB_IrqBitMap=%x\r\n",pRoute->INTB_LinkValue,pRoute->INTB_IrqBitMap));
            DEBUGMSG(1,(L"     INTC_LinkValue=%x,INTC_IrqBitMap=%x\r\n",pRoute->INTC_LinkValue,pRoute->INTC_IrqBitMap));
            DEBUGMSG(1,(L"     INTD_LinkValue=%x,INTC_IrqBitMap=%x\r\n",pRoute->INTD_LinkValue,pRoute->INTD_IrqBitMap));
            dwCurPos +=sizeof(RoutingOptionTable);
            pRoute ++;
        }

    }
}
PBYTE pBiosVirtAddr=NULL;
BOOL search_pci_routing(PCI_ROUTING_TABLE ** pphAddr)
{
    DWORD       p=0;
    PCI_ROUTING_TABLE      *x;
    UCHAR       flag=0;
    UCHAR       crc;
    int         i;

    DEBUGMSG(1,(L"search_pci_routing\r\n"));
    while(pBiosVirtAddr && pphAddr && flag==0 && (ULONG)p<BIOS_LENGTH)
    {
        x=(PCI_ROUTING_TABLE *)(pBiosVirtAddr+p);
        if (x->Signature==*((DWORD *)"$PIR"))
        {
            for(i=0, crc=0; i<x->Table_Size; i++)
                crc+=*(pBiosVirtAddr+p+i);
            if(crc==0)
            {
                flag=1;
                *pphAddr=x;
                DEBUGMSG(1,(L"search_pci_routing found entry =%x  CE/PC \n",*pphAddr));
                return TRUE;
            }
            else
                DEBUGMSG(1,(L"search_pci_routing Entry Checksum Error @%x",x));
        }
        p+=0x10;
    }
    DEBUGMSG(1,(L"search_pci_routing end fails\r\n"));
    return FALSE;
}
BOOL BiosMapIqr(PDEVICE_LOCATION pDevLoc,BYTE bIrq)
{
    Reg32 mReg32;
    BOOL  bRet;
    if (pDevLoc && pBiosAddr) {
        mReg32.RegEax=PCI_FUNCTION_ID*0x100 + SET_PCI_IRQ;
        mReg32.RegEcx=((DWORD)bIrq)*0x100 + (pDevLoc->Pin & 0xff );
        mReg32.RegEbx=
            (pDevLoc->BusNumber & 0xff) * 0x100 +  // Bus Number
            ((pDevLoc->LogicalLoc >> 5) & 0xF8) +  // Device Number
            ((pDevLoc->LogicalLoc ) & 7 );  //Function Number.
        DEBUGMSG(1,(L"BiosMapIqr(EAX=%x,EBX=%x,ECX=%x)\r\n",mReg32.RegEax,mReg32.RegEbx,mReg32.RegEcx));
        bRet=CallBios32(&mReg32,pBiosAddr);
        DEBUGMSG(1,(L"BiosMapIqr return %d and EAX=%x\r\n",bRet,mReg32.RegEax));
        return bRet;
    }
    return FALSE;
}
void ScanConfiguredIrq(IRQRountingOptionsBuffer *pBuffer,WORD wExClusive);

BOOL GetRoutingOption(IRQRountingOptionsBuffer * pBuffer,PVOID phPciBiosAddr)
{
    Reg32 mReg32;
    PCI_ROUTING_TABLE * pPCIRoutingTable=NULL;
    DEBUGMSG(1,(L"+GetRoutingOption\r\n"));
    if (pBiosVirtAddr!=NULL && search_pci_routing(&pPCIRoutingTable) && pPCIRoutingTable) {
        DWORD dwCurPos=sizeof(PCI_ROUTING_TABLE);
        RoutingOptionTable * curTable=(RoutingOptionTable *)(pPCIRoutingTable+1);
        DWORD dwIndex=0;
        pBuffer->BufferSize=0;
        pBuffer->pDataBuffer = (PBYTE)(pBuffer->routingOptionTable);
        DEBUGMSG(1,(L"GetRoutingOption, found ROM version for Routing table.\r\n"));
        while (dwCurPos + sizeof(RoutingOptionTable)<=pPCIRoutingTable->Table_Size && dwIndex < MAX_DEVICE ) {
            memcpy(pBuffer->routingOptionTable+dwIndex,curTable,sizeof(RoutingOptionTable));
            dwIndex++;
            curTable++;
            dwCurPos +=sizeof(RoutingOptionTable);
            pBuffer->BufferSize +=sizeof(RoutingOptionTable);
        }
        DEBUGMSG(1,(L"GetRoutingOption return SUCCESS .AH=%x \r\n",pPCIRoutingTable->ExclusiveIrqs));
        ScanConfiguredIrq(pBuffer,(WORD)pPCIRoutingTable->ExclusiveIrqs);
        return TRUE;
    }
    else
    if (pBuffer && phPciBiosAddr) {
        pBuffer->BufferSize = sizeof(pBuffer->routingOptionTable);
        pBuffer->pDataBuffer = (PBYTE)(pBuffer->routingOptionTable);
        pBuffer->DS=GetDS();
        DEBUGMSG(1,(L"GetRoutingOption with buffer Size %d bytes buffer DS%x:addr =%x \r\n",pBuffer->BufferSize,pBuffer->DS,pBuffer->pDataBuffer));
        mReg32.RegEax=PCI_FUNCTION_ID*0x100 + GET_IRQ_ROUTING_OPTIONS;
        mReg32.RegEbx=0;
        mReg32.RegEdi=(DWORD)pBuffer;
        if (CallBios32(&mReg32,(PVOID) phPciBiosAddr)) { 
            // Success
            DEBUGMSG(1,(L"GetRoutingOption return SUCCESS .AH=%x \r\n",(mReg32.RegEax & 0xff00)>>8));
            ScanConfiguredIrq(pBuffer,(WORD)mReg32.RegEbx);
            return TRUE;
        }
        else {
            pBuffer->pDataBuffer = NULL;// Routing does not exist.
            DEBUGMSG(1,(L"GetRoutingOption return FAILS error code AH=%x \r\n",(mReg32.RegEax & 0xff00)>>8));
        }
    }
    return FALSE;
}
void GetPciRoutingIrqTable()
{
    ULONG phBiosOffset=0;
    // Initial the talbe.
    memset(&irqRoutingOptionBuffer,0,sizeof(irqRoutingOptionBuffer));
    pBiosAddr=NULL;
    // Maping BIOS address 
    if (!pBiosVirtAddr)
        pBiosVirtAddr=(PBYTE)NKCreateStaticMapping((DWORD)BIOS_START>>8,BIOS_LENGTH);// 64k From E0000-FFFFF
   DEBUGMSG(1,(L"PCIBIOS:: BIOS Address static map to addr=%x\r\n",pBiosVirtAddr));
    if (!pBiosVirtAddr)
        return;
    DEBUGMSG(1,(L"GetPicRoutingIrqTable: Start\n"));
    if (search_pci_bios(pBiosVirtAddr,&phBiosOffset) && phBiosOffset!=0) {
        // Find out the address off $PCI service.
        ULONG phPciServiceAddr=0;
        Reg32 mReg32;
        mReg32.RegEax=0x49435024;//"$PCI"
        mReg32.RegEbx=0;
        CallBios32(&mReg32,pBiosVirtAddr+phBiosOffset);
        DEBUGMSG(1,(L"Return from First BIOS EAX=%x EBX=%x,ECX=%x EDX=%x\n",
                mReg32.RegEax,mReg32.RegEbx,mReg32.RegEcx,mReg32.RegEdx));
        if ((mReg32.RegEax & 0xff)==0) { // Success to load and PCI calls
            phBiosOffset=mReg32.RegEbx+mReg32.RegEdx-BIOS_START;
            DEBUGMSG(1,(L"32 PCI BIOS offset located.addr=%x\n",phBiosOffset));
            mReg32.RegEax=PCI_FUNCTION_ID*0x100+PCI_BIOS_PRESENT;
            if (CallBios32(&mReg32,pBiosVirtAddr+phBiosOffset) &&  (mReg32.RegEbx & 0xffff)>=0x210 ) { // IF PCI 2.10 exist.
                DWORD dwNumOfBus=mReg32.RegEcx & 0xff;
                DEBUGMSG(1,(L"32 PCI BIOS Present EDX=%x,EAX=%x EBX=%x,ECX=%x\n",
                    mReg32.RegEdx,mReg32.RegEax,mReg32.RegEbx,mReg32.RegEcx));
                pBiosAddr=pBiosVirtAddr+phBiosOffset;
                GetRoutingOption(&irqRoutingOptionBuffer,pBiosAddr);
                return;
            }
        }
        
    }
    
    DEBUGMSG(1,(L"GetPicRoutingIrqTable: FAILS!!!\n"));
}
void ScanConfiguredIrq(IRQRountingOptionsBuffer *pBuffer,WORD wExClusive)
{
    DEBUGMSG(1,(L"ScanConfiguredIrq with PCI Exclusive Irq Bit (wExClusive)  =%x \r\n",wExClusive));
    memset(irqToLinkValue, 0, sizeof(irqToLinkValue));
    memset(irqLinkPool, 0, sizeof(irqLinkPool));
    wBestPCIIrq=wExClusive;
    
    if (pBuffer) {
        RoutingOptionTable * pRoute;
        DWORD dwCurPos=0;
        DEBUGMSG(1,(L"ScanConfigureIrq: BufferSize = %d @ address %x \r\n", pBuffer->BufferSize,pBuffer->pDataBuffer));
        pRoute =(RoutingOptionTable * )(pBuffer->pDataBuffer);
        while (pRoute && dwCurPos + sizeof(RoutingOptionTable)<=pBuffer->BufferSize) {
            DWORD dwFunc;
            DEBUGMSG(1,(L"ScanConfigureIrq: for Bus=%d ,Device=%d SlotNumber=%d\r\n",
                pRoute->PCIBusNumber,(pRoute->PCIDeviceNumber)>>3,pRoute->SlotNumber));
            DEBUGMSG(1,(L"     INTA_LinkValue=%x,INTA_IrqBitMap=%x\r\n",pRoute->INTA_LinkValue,pRoute->INTA_IrqBitMap));
            DEBUGMSG(1,(L"     INTB_LinkValue=%x,INTB_IrqBitMap=%x\r\n",pRoute->INTB_LinkValue,pRoute->INTB_IrqBitMap));
            DEBUGMSG(1,(L"     INTC_LinkValue=%x,INTC_IrqBitMap=%x\r\n",pRoute->INTC_LinkValue,pRoute->INTC_IrqBitMap));
            DEBUGMSG(1,(L"     INTD_LinkValue=%x,INTD_IrqBitMap=%x\r\n",pRoute->INTD_LinkValue,pRoute->INTD_IrqBitMap));
            for (dwFunc=0;dwFunc<8;dwFunc++) {
                PCI_COMMON_CONFIG pciConfig;
                DWORD dwLength;
                pciConfig.VendorID = 0xFFFF;
                pciConfig.HeaderType = 0;
                dwLength=PCIReadBusData(pRoute->PCIBusNumber,(pRoute->PCIDeviceNumber)>>3,dwFunc,
                          &pciConfig,0,sizeof(pciConfig) - sizeof(pciConfig.DeviceSpecific));
                if (dwLength != (sizeof(pciConfig) - sizeof(pciConfig.DeviceSpecific)) ||
                        (pciConfig.DeviceID == PCI_INVALID_DEVICEID) || (pciConfig.VendorID == PCI_INVALID_VENDORID) || (pciConfig.VendorID == 0)) {
                    if (dwFunc != 0) {
                        // If a multi-function device, continue to next function
                        continue;
                    } else {
                        // If not a multi-function device, continue to next device
                        break;
                    }
                }

                // If device not already placed, configure interrupt
                //if ((pciConfig.Command & (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE))) 
                {
                    DWORD Irq=(DWORD)-1;
                    DWORD Pin=0;
                    switch( pciConfig.HeaderType & ~PCI_MULTIFUNCTION) {
                        case PCI_DEVICE_TYPE: // Devices
                            Irq = pciConfig.u.type0.InterruptLine;
                            Pin = pciConfig.u.type0.InterruptPin;
                            break;
                        case PCI_BRIDGE_TYPE: // PCI-PCI bridge
                            Irq = pciConfig.u.type1.InterruptLine;
                            Pin = pciConfig.u.type1.InterruptPin;
                            break;

                        case PCI_CARDBUS_TYPE: // PCI-Cardbus bridge
                            Irq = pciConfig.u.type2.InterruptLine;
                            Pin = pciConfig.u.type2.InterruptPin;
                            break;
                    }
                    if (Irq<=0xf) {
                        BYTE bLinkNumber=0;
                        switch (Pin) {
                            case 1:
                                bLinkNumber=pRoute->INTA_LinkValue;
                                break;
                            case 2:
                                bLinkNumber=pRoute->INTB_LinkValue;
                                break;
                            case 4:
                                bLinkNumber=pRoute->INTC_LinkValue;
                                break;
                            case 8:
                                bLinkNumber=pRoute->INTD_LinkValue;
                                break;
                        }
                        if (bLinkNumber!=0) {

                            if (irqToLinkValue[Irq]) {
                                // Already some links associated with this irq

                                // Search the list for a duplicate entry
                                pIrqLink currentIrqToLink = irqToLinkValue[Irq];
                                BOOL matchFound = FALSE;
                                do {
                                    if(currentIrqToLink->linkValue == bLinkNumber &&
                                       currentIrqToLink->bus == pRoute->PCIBusNumber &&
                                       currentIrqToLink->device == ((pRoute->PCIDeviceNumber)>>3)) {
                                        matchFound = TRUE;                                        
                                    }
                                    if(currentIrqToLink->pNext) {
                                        currentIrqToLink = currentIrqToLink->pNext;
                                    } else {
                                        break;
                                    }
                                } while (!matchFound);

                                if(!matchFound) {
                                    // No association for this link with this irq, so add a new entry to pNext
                                    addIrqLink(&(currentIrqToLink->pNext), Irq, bLinkNumber, pRoute->PCIBusNumber, ((pRoute->PCIDeviceNumber)>>3));
                                }                                
                            } else {
                                // No links associated with this irq, add a new entry to the list
                                addIrqLink(&(irqToLinkValue[Irq]), Irq, bLinkNumber, pRoute->PCIBusNumber, ((pRoute->PCIDeviceNumber)>>3));
                            }
                        }
                    }
                }
                if ((pciConfig.HeaderType & PCI_MULTIFUNCTION)==0) { // Not multi-function card.
                    break;
                }                
            }
            dwCurPos +=sizeof(RoutingOptionTable);
            pRoute ++;
        }

    }
}

BOOL OALIntrRequestIrqs(PDEVICE_LOCATION pDevLoc, UINT32 *pCount, UINT32 *pIrq)
{
    int Bus = (pDevLoc->LogicalLoc >> 16) & 0xFF;
    int Device = (pDevLoc->LogicalLoc >> 8) & 0xFF;

    // Figure out the Link number in routing table
    RoutingOptionTable * pRoute=(RoutingOptionTable * )irqRoutingOptionBuffer.pDataBuffer;

    DEBUGMSG(1,(L"OALIntrRequestIrqs:(Bus=%d,Device=%d,Pin=%d)\r\n",Bus,Device,pDevLoc->Pin));
    // This shouldn't happen
    if (*pCount < 1) return FALSE;
    if (pRoute) {
        // serch table.
        DWORD dwCurPos=0;
        while (dwCurPos + sizeof(RoutingOptionTable)<=irqRoutingOptionBuffer.BufferSize) {            
            DEBUGMSG(1,(L"OALIntrRequestIrqs: for Bus=%d ,Device=%d SlotNumber=%d\r\n",
                pRoute->PCIBusNumber,(pRoute->PCIDeviceNumber)>>3,pRoute->SlotNumber));
            DEBUGMSG(1,(L"     INTA_LinkValue=%x,INTA_IrqBitMap=%x\r\n",pRoute->INTA_LinkValue,pRoute->INTA_IrqBitMap));
            DEBUGMSG(1,(L"     INTB_LinkValue=%x,INTB_IrqBitMap=%x\r\n",pRoute->INTB_LinkValue,pRoute->INTB_IrqBitMap));
            DEBUGMSG(1,(L"     INTC_LinkValue=%x,INTC_IrqBitMap=%x\r\n",pRoute->INTC_LinkValue,pRoute->INTC_IrqBitMap));
            DEBUGMSG(1,(L"     INTD_LinkValue=%x,INTC_IrqBitMap=%x\r\n",pRoute->INTD_LinkValue,pRoute->INTD_IrqBitMap));
            if (pRoute->PCIBusNumber== Bus && ((pRoute->PCIDeviceNumber)>>3)==Device) { // found
                BYTE bLinkNumber=0;
                WORD wIntPossibleBit=0;
                switch (pDevLoc->Pin) {
                case 1:
                    bLinkNumber=pRoute->INTA_LinkValue;
                    wIntPossibleBit=pRoute->INTA_IrqBitMap;
                    break;
                case 2:
                    bLinkNumber=pRoute->INTB_LinkValue;
                    wIntPossibleBit=pRoute->INTB_IrqBitMap;
                    break;
                case 4:
                    bLinkNumber=pRoute->INTC_LinkValue;
                    wIntPossibleBit=pRoute->INTC_IrqBitMap;
                    break;
                case 8:
                    bLinkNumber=pRoute->INTD_LinkValue;
                    wIntPossibleBit=pRoute->INTD_IrqBitMap;
                    break;
                }
                if (bLinkNumber!=0) {
                    DWORD dwIndex;
                    BYTE  bIrq=(BYTE)-1;
                    WORD wIntrBit;
                    int iIndex;
                    for (dwIndex = 0; dwIndex < NUM_IRQS; dwIndex++) {
                        // Traverse the list for each irq and search for a matching
                        // bus, device, and linkNumber.
                        // If we find one this IRQ has already been mapped
                        pIrqLink currentIrqToLink = irqToLinkValue[dwIndex];
                        while (currentIrqToLink) {
                            OALMSG(OAL_INTR,(L"Traverse IrqLinks - IRQ %d maps to link %x bus %d device %d\r\n", dwIndex,
                                           currentIrqToLink->linkValue, currentIrqToLink->bus, currentIrqToLink->device));
                            if(currentIrqToLink->linkValue == bLinkNumber &&
                               currentIrqToLink->bus == pRoute->PCIBusNumber &&
                               currentIrqToLink->device == ((pRoute->PCIDeviceNumber)>>3)) {
                                if (pIrq) *pIrq=dwIndex;
                                if (pCount) *pCount = 1;
                                DEBUGMSG(1,(L"-OALIntrRequestIrqs: Found full IRQ match returning existing IRQ=%d\r\n",dwIndex));
                                return TRUE;
                            }
                            currentIrqToLink = currentIrqToLink->pNext;
                        }
                    }
                    for (dwIndex = 0; dwIndex < NUM_IRQS; dwIndex++) {
                        // Traverse the list for each irq and search for a matching
                        // linkNumber only.  This is our second-best metric for finding a match.
                        pIrqLink currentIrqToLink = irqToLinkValue[dwIndex];
                        while (currentIrqToLink) {
                            OALMSG(OAL_INTR,(L"Traverse IrqLinks - IRQ %d maps to link %x bus %d device %d\r\n", dwIndex,
                                           currentIrqToLink->linkValue, currentIrqToLink->bus, currentIrqToLink->device));
                            if(currentIrqToLink->linkValue == bLinkNumber) {
                                if (pIrq) *pIrq=dwIndex;
                                if (pCount) *pCount = 1;
                                DEBUGMSG(1,(L"-OALIntrRequestIrqs: Found linkNumber IRQ match returning existing IRQ=%d\r\n",dwIndex));
                                return TRUE;
                            }
                            currentIrqToLink = currentIrqToLink->pNext;
                        }
                    }
                    // We didn't match up with any IRQs, search for a useful Interrupt
                    wIntrBit=0x8000;
                    for (iIndex=(NUM_IRQS-1);iIndex>=0;iIndex--) {
                        if ((wBestPCIIrq & wIntrBit)!=0 && irqToLinkValue[iIndex]==0 ) { // Best interurpt is can be mapped but not mapped yet
                            bIrq=(BYTE)iIndex;
                            DEBUGMSG(1,(L"OALIntrRequestIrqs: Try mapping New IRQ(%d) to this device\r\n",bIrq));
                            if (bIrq<=(NUM_IRQS-1) && BiosMapIqr(pDevLoc,bIrq)) { // Mapped.

                                // Scan to the end and add an association for the irq and linkvalue
                                pIrqLink currentIrqToLink = irqToLinkValue[dwIndex];
                                while(currentIrqToLink) {
                                    currentIrqToLink = currentIrqToLink->pNext;
                                }
                                addIrqLink(&(irqToLinkValue[iIndex]), bIrq, bLinkNumber, pRoute->PCIBusNumber, ((pRoute->PCIDeviceNumber)>>3));
                                if (pIrq) *pIrq=(DWORD)bIrq;
                                if (pCount) *pCount = 1;
                                DEBUGMSG(1,(L"-OALIntrRequestIrqs: Mapped New IRQ return IRQ=%d\r\n",bIrq));
                                return TRUE;
                            }
                        }
                        wIntrBit>>=1;
                    }
                    wIntrBit=0x8000;
                    for (iIndex=(NUM_IRQS-1);iIndex>=0;iIndex--) {
                        if ((wIntPossibleBit & wIntrBit)!=0 && irqToLinkValue[iIndex]==0 ) { // Best interurpt is can be mapped but not mapped yet
                            bIrq=(BYTE)iIndex;
                            DEBUGMSG(1,(L"OALIntrRequestIrqs: Try mapping New IRQ(%d) to this device\r\n",bIrq));
                            if (bIrq<=(NUM_IRQS-1) && BiosMapIqr(pDevLoc,bIrq)) { // Mapped.

                                // Scan to the end and add an association for the irq and linkvalue
                                pIrqLink currentIrqToLink = irqToLinkValue[dwIndex];
                                while(currentIrqToLink) {
                                    currentIrqToLink = currentIrqToLink->pNext;
                                }
                                addIrqLink(&(irqToLinkValue[iIndex]), bIrq, bLinkNumber, pRoute->PCIBusNumber, ((pRoute->PCIDeviceNumber)>>3));
                                if (pIrq) *pIrq=(DWORD)bIrq;
                                if (pCount) *pCount = 1;
                                DEBUGMSG(1,(L"-OALIntrRequestIrqs: Mapped New IRQ return IRQ=%d\r\n",bIrq));
                                return TRUE;
                            }
                        }
                        wIntrBit>>=1;
                    }
                }
                break;// False
                
            }
            dwCurPos +=sizeof(RoutingOptionTable);
            pRoute ++;
        }
    }
    return FALSE;
}

// stub for OALIoTransBusAddress, not supported in x86
BOOL OALIoTransBusAddress (INTERFACE_TYPE ifcType, UINT32 busNumber, UINT64 busAddress, UINT32 * pAddressSpace, UINT64 * pSystemAddress)
{
    return FALSE;
}


BOOL RegisterBINFS_NAND (PX86BootInfo pX86Info)
{
    if (pX86Info && pX86Info->NANDBootFlags) {
        PCI_SLOT_NUMBER SlotNumber;
        PCI_REG_INFO NANDPCIRegInfo ;
        PCI_COMMON_CONFIG Cfg;
        DWORD dwBus = pX86Info->NANDBusNumber;
        SlotNumber.u.AsULONG = pX86Info->NANDSlotNumber;
        PCIGetBusDataByOffset(dwBus, SlotNumber.u.AsULONG, &Cfg, 0, sizeof(PCI_COMMON_CONFIG));
        PCIInitInfo(L"Drivers\\BuiltIn\\PCI\\Instance\\NAND_Flash",
                dwBus, SlotNumber.u.bits.DeviceNumber, SlotNumber.u.bits.FunctionNumber, 0, &Cfg, &NANDPCIRegInfo);
        PCIReadBARs(&NANDPCIRegInfo);
        DEBUGMSG(1,(L"Num=%d,iobase=%x,ioLen=%x\r\n",NANDPCIRegInfo.IoBase.Num, NANDPCIRegInfo.IoBase.Reg[0], NANDPCIRegInfo.IoLen.Reg[0]));
        return PCIReg(&NANDPCIRegInfo);
    }
    return FALSE;
}

BOOL PCIInitConfigMechanism (UCHAR ucConfigMechanism);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VOID PCIInitBusInfo (void)
{
    if (PCIInitConfigMechanism (g_pX86Info->ucPCIConfigType & 0x03)) {
        GetPciRoutingIrqTable ();
    } else {
        RETAILMSG(1, (TEXT("No PCI bus\r\n")));
    }
}

