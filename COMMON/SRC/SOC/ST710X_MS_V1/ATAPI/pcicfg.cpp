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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//=======================================================================
//  COPYRIGHT (C) STMicroelectronics 2007.  ALL RIGHTS RESERVED
//
//  Use of this source code is subject to the terms of your STMicroelectronics
//  development license agreement. If you did not accept the terms of such a license,
//  you are not authorized to use this source code.
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//========================================================================
#include <windows.h>
#include <nkintr.h>
#include <ceddk.h>
#include <cardserv.h>
#include <cardapi.h>
#include <tuple.h>
#include <devload.h>
#include <PCIbus.h>
#include <debug.h>

typedef PPCI_RSRC (*PPCIRSRC_NEW)(
    DWORD Bus,
    DWORD Device,
    DWORD Function,
    DWORD Offset,
    DWORD Base,
    DWORD Size,
    BOOL  Bridge,
    DWORD SecBus,
    BOOL  Placed,
    PPCI_CFG_INFO ConfigInfo
    );

typedef VOID (*PPCIRSRC_ADD)(
    PPCI_RSRC Head,
    PPCI_RSRC Rsrc
    );

//
// Function prototypes
//
static BOOL ConfigRsrc(
    PPCI_DEV_INFO pInfo,
    PPCI_RSRC pMemHead,
    PPCI_RSRC pIoHead,
    DWORD *pMemSize,
    DWORD *pIoSize
    );

static BOOL ConfigSize(
    PPCI_DEV_INFO pInfo
    );

static BOOL ConfigInit(
    PPCI_DEV_INFO pInfo
    );


__inline static DWORD
PCIConfig_Read(
    ULONG BusNumber,
    ULONG Device,
    ULONG Function,
    ULONG Offset
    );


__inline static void
PCIConfig_Write(
    ULONG BusNumber,
    ULONG Device,
    ULONG Function,
    ULONG Offset,
    ULONG Value,
    ULONG Size = sizeof(DWORD)
    );



//
// DeviceConfig
//
EXTERN_C DWORD GenericConfig(
    DWORD Command,
    PPCI_DEV_INFO pInfo,
    PPCI_RSRC pRsrc1,
    PPCI_RSRC pRsrc2,
    DWORD *pMemSize,
    DWORD *pIoSize
    )
{
    DEBUGMSG(1, (L"ATAPI:PCIConfig!DeviceConfig+(%d)\r\n", Command));

    switch (Command) {
    case PCIBUS_CONFIG_RSRC:
        if (ConfigRsrc(pInfo, pRsrc1, pRsrc2, pMemSize, pIoSize)) {
            return ERROR_SUCCESS;
        } else {
            return ERROR_GEN_FAILURE;
        }

    case PCIBUS_CONFIG_SET:
        return ERROR_NOT_SUPPORTED;

    case PCIBUS_CONFIG_SIZE:
        if (ConfigSize(pInfo)) {
            return ERROR_SUCCESS;
        } else {
            return ERROR_GEN_FAILURE;
        }

    case PCIBUS_CONFIG_INIT:
        if (ConfigInit(pInfo)) {
            return ERROR_SUCCESS;
        } else {
            return ERROR_GEN_FAILURE;
        }

    default:
        break;
    }

    DEBUGMSG(1, (L"ATAPI:PCIConfig!DeviceConfig-: ERROR: Command %d not recognized\r\n", Command));

    return ERROR_BAD_COMMAND;
}


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
    PCI_SLOT_NUMBER SlotNumber;

    SlotNumber.u.AsULONG = 0;
    SlotNumber.u.bits.DeviceNumber = Device;
    SlotNumber.u.bits.FunctionNumber = Function;

    HalGetBusDataByOffset(PCIConfiguration, BusNumber, SlotNumber.u.AsULONG, &RetVal, Offset, sizeof(RetVal));

    return RetVal;
}

// Inline functions
__inline static BYTE
PCIConfig_ReadByte(
    ULONG BusNumber,
    ULONG Device,
    ULONG Function,
    ULONG Offset
    )
{
    BYTE  RetVal = FALSE;
    PCI_SLOT_NUMBER SlotNumber;

    SlotNumber.u.AsULONG = 0;
    SlotNumber.u.bits.DeviceNumber = Device;
    SlotNumber.u.bits.FunctionNumber = Function;

    HalGetBusDataByOffset(PCIConfiguration, BusNumber, SlotNumber.u.AsULONG, &RetVal, Offset, sizeof(RetVal));

    return RetVal;
}



__inline static void
PCIConfig_Write(
    ULONG BusNumber,
    ULONG Device,
    ULONG Function,
    ULONG Offset,
    ULONG Value,
    ULONG Size
    )
{
    PCI_SLOT_NUMBER SlotNumber;

    SlotNumber.u.AsULONG = 0;
    SlotNumber.u.bits.DeviceNumber = Device;
    SlotNumber.u.bits.FunctionNumber = Function;

    HalSetBusDataByOffset(PCIConfiguration, BusNumber, SlotNumber.u.AsULONG, &Value, Offset, Size);
}



//
// ConfigRsrc
//
static BOOL ConfigRsrc(
    PPCI_DEV_INFO pInfo,
    PPCI_RSRC pMemHead,
    PPCI_RSRC pIoHead,
    DWORD *pMemSize,
    DWORD *pIoSize
    )
{
    DWORD NumberOfRegs;
    ULONG Offset;
    ULONG i;
    ULONG BaseAddress;
    ULONG Size;
    ULONG Type;
    DWORD Reg;
    BOOL SizeFound;
    DWORD IoIndex = 0;
    DWORD MemIndex = 0;
    DWORD Bus = pInfo->Bus;
    DWORD Device = pInfo->Device;
    DWORD Function = pInfo->Function;
    PPCI_COMMON_CONFIG pCfg = pInfo->Cfg;

    HINSTANCE    hPciBusInstance = NULL;
    PPCIRSRC_NEW pfnPCIRsrc_New = NULL;
    PPCIRSRC_ADD pfnPCIRsrc_Add = NULL;
    BOOL         fResult = FALSE;

    hPciBusInstance = LoadLibrary(L"PCIBUS.DLL");
    if (NULL == hPciBusInstance) {
        goto exit;
    }
    pfnPCIRsrc_New = (PPCIRSRC_NEW)GetProcAddress(hPciBusInstance, L"PCIRsrc_New");
    if (pfnPCIRsrc_New == NULL) {
        goto exit;
    }
    pfnPCIRsrc_Add = (PPCIRSRC_ADD)GetProcAddress(hPciBusInstance, L"PCIRsrc_Add");
    if (pfnPCIRsrc_Add == NULL) {
        goto exit;
    }

    DEBUGMSG(ZONE_PCI | ZONE_INIT, (L"ATAPI:PCIConfig!ConfigRsrc+(Bus %d, Device %d, Function %d)\r\n",
        Bus, Device, Function));

    // Determine number of BARs to examine from header type
    switch (pCfg->HeaderType & ~PCI_MULTIFUNCTION) {
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
        goto exit;
    }

    for (i = 0, Offset = 0x10; i < NumberOfRegs; i++, Offset += 4) {
        // Get base address register value
        Reg = pCfg->u.type0.BaseAddresses[i];
        Type = Reg & PCI_ADDRESS_IO_SPACE;

        // Probe hardware for size
        PCIConfig_Write(Bus,Device,Function,Offset,0xFFFFFFFF);
        BaseAddress = PCIConfig_Read(Bus,Device,Function,Offset);
        PCIConfig_Write(Bus, Device, Function, Offset, Reg);

        if (Type) {
            // IO space
            // Re-adjust BaseAddress if upper 16-bits are 0 (allowable in PCI 2.2 spec)
            if (((BaseAddress & PCI_ADDRESS_IO_ADDRESS_MASK) != 0) && ((BaseAddress & 0xFFFF0000) == 0)) {
                BaseAddress |= 0xFFFF0000;
            }

            Size = ~(BaseAddress & PCI_ADDRESS_IO_ADDRESS_MASK);
            Reg &= PCI_ADDRESS_IO_ADDRESS_MASK;
        } else {
            // Memory space
            if ((Reg & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_20BIT) {
                // PCI 2.2 spec no longer supports this type of memory addressing
                DEBUGMSG(ZONE_ERROR, (L"ATAPI:PCIConfig!ConfigRsrc: 20-bit addressing not supported\r\n"));
                goto exit;
            }

            // Re-adjust BaseAddress if upper 16-bits are 0 (allowed by PCI 2.2 spec)
            if (((BaseAddress & PCI_ADDRESS_MEMORY_ADDRESS_MASK) != 0) && ((BaseAddress & 0xFFFF0000) == 0)) {
                BaseAddress |= 0xFFFF0000;
            }

            Size = ~(BaseAddress & PCI_ADDRESS_MEMORY_ADDRESS_MASK);
            Reg &= PCI_ADDRESS_MEMORY_ADDRESS_MASK;
        }

        // Check that the register has a valid format; it should have consecutive high 1's and consecutive low 0's
        SizeFound = (BaseAddress != 0) && (BaseAddress != 0xFFFFFFFF) && (((Size + 1) & Size) == 0);

        Size +=1;

        if (SizeFound) {
            PPCI_RSRC Rsrc = (*pfnPCIRsrc_New)(Bus, Device, Function, Offset, Reg, Size, FALSE, 0, FALSE, pInfo->ConfigInfo);

            if (!Rsrc) {
                DEBUGMSG(ZONE_PCI | ZONE_ERROR, (L"ATAPI:PCIConfig!ConfigRsrc: Failed local alloc of Rsrc\r\n"));
                goto exit;
            }

            if (Type == PCI_ADDRESS_IO_SPACE) {
                *pIoSize += Size;
                (*pfnPCIRsrc_Add)(pIoHead, Rsrc);
            } else {
                *pMemSize += Size;
                (*pfnPCIRsrc_Add)(pMemHead, Rsrc);
            }

            DEBUGMSG(ZONE_PCI | ZONE_INIT, (L"ATAPI:PCIConfig!ConfigRsrc: BAR(%d/%d/%d): Offset 0x%x, Type %s, Size 0x%X\r\n",
                Bus, Device, Function, Offset, (Type == PCI_ADDRESS_IO_SPACE) ? TEXT("I/O") : TEXT("Memory"), Size));
        } else {
            // Some devices have invalid BARs before valid ones (even though the spec says you can't).  Skip invalid BARs.
            continue;
        }

        // check for 64 bit device (memory only)
        if ((Type == PCI_ADDRESS_MEMORY_SPACE) && ((Reg & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)) {
            // 64 bit device - BAR is twice as wide - zero out high part
            Offset += 4;
            i++;
            PCIConfig_Write(Bus, Device, Function, Offset, 0x0);
        }
    }

    //
    // Add resource for expansion ROM, offset 0x30
    //
    Offset = 0x30;
    PCIConfig_Write(Bus, Device, Function, Offset ,0xFFFFFFFF);
    BaseAddress = PCIConfig_Read(Bus, Device, Function, Offset);

    // Memory space
    Size = ~(BaseAddress & 0xFFFFFFF0);
    Reg &= 0xFFFFFFF0;

    // Check that the register has a valid format; it should have consecutive high 1's and consecutive low 0's
    SizeFound = (BaseAddress != 0) && (BaseAddress != 0xFFFFFFFF) && (((Size + 1) & Size) == 0);

    Size +=1;

    if (SizeFound) {
        PPCI_RSRC Rsrc = (*pfnPCIRsrc_New)(Bus, Device, Function, Offset, Reg, Size, FALSE, 0, FALSE, pInfo->ConfigInfo);

        if (!Rsrc) {
            DEBUGMSG(ZONE_PCI | ZONE_ERROR, (L"ATAPI:PCIConfig!ConfigRsrc: Failed local alloc of Rsrc\r\n"));
            goto exit;
        }

        *pMemSize += Size;
        (*pfnPCIRsrc_Add)(pMemHead, Rsrc);


        DEBUGMSG(ZONE_PCI | ZONE_INIT, (L"ATAPI:PCIConfig!ConfigRsrc: ROM(%d/%d/%d): Offset 0x%x, Type Memory, Size 0x%X\r\n",
            Bus, Device, Function, Offset, Size));
    }

    fResult = TRUE;

exit:;

    if (NULL != hPciBusInstance) {
        FreeLibrary(hPciBusInstance);
    }

    DEBUGMSG(ZONE_PCI | ZONE_INIT, (L"ATAPI:PCIConfig!ConfigRsrc-\r\n", Bus, Device, Function));

    return fResult;
}


//
// ConfigSize
//
static BOOL ConfigSize(
    PPCI_DEV_INFO pInfo
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

    DEBUGMSG(ZONE_INIT | ZONE_PCI, (L"ATAPI:PCIConfig!ConfigSize+(Bus %d, Device %d, Function %d)\r\n",
        pInfo->Bus, pInfo->Device, pInfo->Function));

    // Determine number of BARs to examine from header type
    switch (pInfo->Cfg->HeaderType & ~PCI_MULTIFUNCTION) {
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
        Reg = pInfo->Cfg->u.type0.BaseAddresses[i];

        // Legacy I/O
        if (i <= 3) {
            switch(i) {
                case 0:
                    if (!Reg)
                        Reg = 0x1F1;
                    Size = 0x8;
                    break;
                case 1:
                    if (!Reg)
                        Reg = 0x3F5;
                    Size = 0x4;
                    break;
                case 2:
                    if (!Reg)
                        Reg = 0x171;
                    Size = 0x8;
                    break;
                case 3:
                    if (!Reg)
                        Reg = 0x375;
                    Size = 0x4;
                    break;
            }

            pInfo->IoLen.Reg[IoIndex] = Size;
            pInfo->IoLen.Num++;
            pInfo->IoBase.Reg[IoIndex++] = Reg & PCI_ADDRESS_IO_ADDRESS_MASK;
            pInfo->IoBase.Num++;
        } else {
            // Get size info
            PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, Offset, 0xFFFFFFFF);
            BaseAddress = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, Offset);
            PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, Offset, Reg);

            if (Reg & PCI_ADDRESS_IO_SPACE) {
                // IO space
                // Re-adjust BaseAddress if upper 16-bits are 0 (this is allowable in PCI 2.2 spec)
                if (((BaseAddress & PCI_ADDRESS_IO_ADDRESS_MASK) != 0) && ((BaseAddress & 0xFFFF0000) == 0)) {
                    BaseAddress |= 0xFFFF0000;
                }

                Size = ~(BaseAddress & PCI_ADDRESS_IO_ADDRESS_MASK);

                if ((BaseAddress != 0) && (BaseAddress != 0xFFFFFFFF) && (((Size + 1) & Size) == 0)) {
                    // BAR has valid format (consecutive high 1's and consecutive low 0's)
                    pInfo->IoLen.Reg[IoIndex] = Size + 1;
                    pInfo->IoLen.Num++;
                    pInfo->IoBase.Reg[IoIndex++] = Reg & PCI_ADDRESS_IO_ADDRESS_MASK;
                    pInfo->IoBase.Num++;
                } else {
                    // BAR invalid => skip to next one
                    continue;
                }
            } else {
                // Memory space
                if ((Reg & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_20BIT) {
                    // PCI 2.2 spec no longer supports this type of memory addressing
                    DEBUGMSG(ZONE_ERROR, (L"ATAPI:PCIConfig!ConfigSize: 20-bit addressing not supported\r\n"));
                    return FALSE;
                }

                // Re-adjust BaseAddress if upper 16-bits are 0 (this is allowable in PCI 2.2 spec)
                if (((BaseAddress & PCI_ADDRESS_MEMORY_ADDRESS_MASK) != 0) && ((BaseAddress & 0xFFFF0000) == 0)) {
                    BaseAddress |= 0xFFFF0000;
                }

                Size = ~(BaseAddress & PCI_ADDRESS_MEMORY_ADDRESS_MASK);

                if ((BaseAddress != 0) && (BaseAddress != 0xFFFFFFFF) && (((Size + 1) & Size) == 0)) {
                    // BAR has valid format (consecutive high 1's and consecutive low 0's)
                    pInfo->MemLen.Reg[MemIndex] = Size + 1;
                    pInfo->MemLen.Num++;
                    pInfo->MemBase.Reg[MemIndex++] = Reg & PCI_ADDRESS_MEMORY_ADDRESS_MASK;
                    pInfo->MemBase.Num++;
                } else {
                    // BAR invalid => skip to next one
                    continue;
                }

                if ((Reg & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT) {
                    // 64 bit device - BAR is twice as wide, skip upper 32-bits
                    Offset += 4;
                    i++;
                }
            }
        }
    }

    DEBUGMSG(ZONE_INIT | ZONE_PCI, (L"ATAPI:PCIConfig!ConfigSize-\r\n"));

    return TRUE;
}


//
// ConfigInit
//
static BOOL ConfigInit(
    PPCI_DEV_INFO pInfo
    )
{
    return TRUE;
}

//
// PromiseInit
//
static BOOL PromiseInit(
    PPCI_DEV_INFO pInfo
    )
{
    DWORD dwInfo;
    dwInfo = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x60);
    DEBUGMSG( ZONE_INIT, (L"ATAPI:PromiseInit Value at offset 0x60 is %08X\r\n", dwInfo));
    dwInfo = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x64);
    DEBUGMSG( ZONE_INIT, (L"ATAPI:PromiseInit Value at offset 0x64 is %08X\r\n", dwInfo));
    dwInfo = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x68);
    DEBUGMSG( ZONE_INIT, (L"ATAPI:PromiseInit Value at offset 0x68 is %08X\r\n", dwInfo));
    dwInfo = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x6C);
    DEBUGMSG( ZONE_INIT, (L"ATAPI:PromiseInit Value at offset 0x6C is %08X\r\n", dwInfo));
    PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, 0x60, 0x004FF304);
    PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, 0x64, 0x004FF304);
    PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, 0x68, 0x004FF304);
    PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, 0x6C, 0x004FF304);
    dwInfo = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x60);
    DEBUGMSG( ZONE_INIT, (L"ATAPI:PromiseInit Value at offset 0x60 is %08X\r\n", dwInfo));
    dwInfo = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x64);
    DEBUGMSG( ZONE_INIT, (L"ATAPI:PromiseInit Value at offset 0x64 is %08X\r\n", dwInfo));
    dwInfo = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x68);
    DEBUGMSG( ZONE_INIT, (L"ATAPI:PromiseInit Value at offset 0x68 is %08X\r\n", dwInfo));
    dwInfo = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x6C);
    DEBUGMSG( ZONE_INIT, (L"ATAPI:PromiseInit Value at offset 0x6C is %08X\r\n", dwInfo));
     return TRUE;
}

//
// DeviceConfig
//
EXTERN_C BOOL PromiseConfig(
    DWORD Command,
    PPCI_DEV_INFO pInfo,
    PPCI_RSRC pRsrc1,
    PPCI_RSRC pRsrc2,
    DWORD *pMemSize,
    DWORD *pIoSize
    )
{
    DEBUGMSG(1, (L"ATAPI:PCIConfig!DeviceConfig+(%d)\r\n", Command));

    switch (Command) {
    case PCIBUS_CONFIG_RSRC:
        return ERROR_NOT_SUPPORTED;

    case PCIBUS_CONFIG_SET:
        return ERROR_NOT_SUPPORTED;

    case PCIBUS_CONFIG_SIZE:
        return ERROR_NOT_SUPPORTED;

    case PCIBUS_CONFIG_INIT:
        if (PromiseInit(pInfo)) {
            return ERROR_SUCCESS;
        } else {
            return ERROR_GEN_FAILURE;
        }

    default:
        break;
    }

    DEBUGMSG(1, (L"ATAPI:PCIConfig!DeviceConfig-: ERROR: Command %d not recognized\r\n", Command));

    return ERROR_BAD_COMMAND;
}



/*
0x83fde63c: ATAPI:AliInit Value at offset 00 is 522910B9
0x83fde63c: ATAPI:AliInit Value at offset 04 is 02800005
0x83fde63c: ATAPI:AliInit Value at offset 08 is 01018AC1
0x83fde63c: ATAPI:AliInit Value at offset 0C is 00002000
0x83fde63c: ATAPI:AliInit Value at offset 10 is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 14 is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 18 is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 1C is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 20 is 0000D001
0x83fde63c: ATAPI:AliInit Value at offset 24 is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 28 is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 2C is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 30 is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 34 is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 38 is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 3C is 04020100
0x83fde63c: ATAPI:AliInit Value at offset 40 is 7A000000
0x83fde63c: ATAPI:AliInit Value at offset 44 is 00000000
0x83fde63c: ATAPI:AliInit Value at offset 48 is 4A000000
0x83fde63c: ATAPI:AliInit Value at offset 4C is 3ABA8000
0x83fde63c: ATAPI:AliInit Value at offset 50 is 89000001
0x83fde63c: ATAPI:AliInit Value at offset 54 is 77777777
0x83fde63c: ATAPI:AliInit Value at offset 58 is 00310000
*/

static BOOL AliInit(
    PPCI_DEV_INFO pInfo
    )
{
    DWORD dwIndex, dwValue;

    for (dwIndex=0; dwIndex <= 0x5A; dwIndex += 4) {
        dwValue = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, dwIndex);
        DEBUGMSG( ZONE_INIT, (L"ATAPI:AliInit Value at offset %02X is %08X\r\n", dwIndex, dwValue));
    }
    DEBUGMSG( ZONE_INIT, (L"\r\n"));
    DEBUGMSG( ZONE_INIT, (L"\r\n"));
    DEBUGMSG( ZONE_INIT, (L"\r\n"));


    dwValue = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x50);
    dwValue |= 0x01000001;
    PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, 0x50, dwValue);

    dwValue = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x54);
    dwValue = 0x77777777;
    PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, 0x54, dwValue);

    dwValue = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, 0x58);
    dwValue = 0x00000000;
    PCIConfig_Write(pInfo->Bus, pInfo->Device, pInfo->Function, 0x58, dwValue);

    for (dwIndex=0; dwIndex <= 0x5A; dwIndex += 4) {
        dwValue = PCIConfig_Read(pInfo->Bus, pInfo->Device, pInfo->Function, dwIndex);
        DEBUGMSG( ZONE_INIT, (L"ATAPI:AliInit Value at offset %02X is %08X\r\n", dwIndex, dwValue));
    }
    return TRUE;
}


EXTERN_C BOOL AliConfig(
    DWORD Command,
    PPCI_DEV_INFO pInfo,
    PPCI_RSRC pRsrc1,
    PPCI_RSRC pRsrc2,
    DWORD *pMemSize,
    DWORD *pIoSize
    )
{
    DEBUGMSG(1, (L"ATAPI:PCIConfig!DeviceConfig+(%d)\r\n", Command));

    switch (Command) {
    case PCIBUS_CONFIG_RSRC:
        if (ConfigRsrc(pInfo, pRsrc1, pRsrc2, pMemSize, pIoSize)) {
            return ERROR_SUCCESS;
        } else {
            return ERROR_GEN_FAILURE;
        }

    case PCIBUS_CONFIG_SET:
        return ERROR_NOT_SUPPORTED;

    case PCIBUS_CONFIG_SIZE:
        if (ConfigSize(pInfo)) {
            return ERROR_SUCCESS;
        } else {
            return ERROR_GEN_FAILURE;
        }

    case PCIBUS_CONFIG_INIT:
        if (AliInit(pInfo)) {
            return ERROR_SUCCESS;
        } else {
            return ERROR_GEN_FAILURE;
        }

    default:
        break;
    }

    DEBUGMSG(1, (L"ATAPI:PCIConfig!DeviceConfig-: ERROR: Command %d not recognized\r\n", Command));

    return ERROR_BAD_COMMAND;
}


