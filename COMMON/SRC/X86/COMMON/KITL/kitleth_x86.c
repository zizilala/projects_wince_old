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
#include <nkintr.h>
#include <oal.h>
#include <pci.h>
#include <x86kitl.h>


static KTIL_NIC_INFO gKitlNic;
PCKITL_NIC_INFO      g_pKitlPCINic;         // non-null only if it's PCI. Used in InitKitlRegistryInfo

PCSUPPORTED_NIC FindNICByType (UCHAR ucType)
{
    int i;
    for (i = 0; i < g_nNumNicSupported; i ++) {
        if (g_NicSupported[i].Type == ucType) {            
            return &g_NicSupported[i];
        }
    }
    return NULL;
}

LPCSTR FindNICAbbrev (DWORD dwUpperMAC)
{
    int i;
    for (i = 0; i < g_nNumNicSupported; i ++) {
        if (g_NicSupported[i].dwUpperMAC == dwUpperMAC) {            
            return g_NicSupported[i].szAbbrev;
        }
    }
    return "";
}

PCSUPPORTED_NIC FindNICByVendor (PCI_COMMON_CONFIG *pPciCfg)
{
    int i;
    for (i = 0; i < g_nNumNicSupported; i ++) {
        if ((g_NicSupported[i].wVenId == pPciCfg->VendorID) &&
            (g_NicSupported[i].wDevId == pPciCfg->DeviceID)) {
            return &g_NicSupported[i];
        }
    }
    return NULL;
}

void printPCIConfig(PCI_COMMON_CONFIG* config)
{
    KITLOutputDebugString("+printPCIConfig\r\n");
    KITLOutputDebugString("config.VendorID           = 0x%x\r\n", config->VendorID);
    KITLOutputDebugString("config.DeviceID           = 0x%x\r\n", config->DeviceID);
    KITLOutputDebugString("config.Command            = 0x%x\r\n", config->Command);
    KITLOutputDebugString("config.Status             = 0x%x\r\n", config->Status);
    KITLOutputDebugString("config.RevisionID         = 0x%x\r\n", config->RevisionID);
    KITLOutputDebugString("config.ProgIf             = 0x%x\r\n", config->ProgIf);
    KITLOutputDebugString("config.SubClass           = 0x%x\r\n", config->SubClass);
    KITLOutputDebugString("config.BaseClass          = 0x%x\r\n", config->BaseClass);
    KITLOutputDebugString("config.CacheLineSize      = 0x%x\r\n", config->CacheLineSize);
    KITLOutputDebugString("config.LatencyTimer       = 0x%x\r\n", config->LatencyTimer);
    KITLOutputDebugString("config.HeaderType         = 0x%x\r\n", config->HeaderType);
    KITLOutputDebugString("config.BIST               = 0x%x\r\n", config->BIST);
    KITLOutputDebugString("config.BaseAddresses[0]   = 0x%x\r\n", config->u.type1.BaseAddresses[0]);
    KITLOutputDebugString("config.BaseAddresses[1]   = 0x%x\r\n", config->u.type1.BaseAddresses[1]);
    KITLOutputDebugString("config.PrimaryBusNumber   = 0x%x\r\n", config->u.type1.PrimaryBusNumber);
    KITLOutputDebugString("config.SecondaryBusNumber = 0x%x\r\n", config->u.type1.SecondaryBusNumber);
    KITLOutputDebugString("config.SubordinateBusNumber  = 0x%x\r\n", config->u.type1.SubordinateBusNumber);
    KITLOutputDebugString("config.SecondaryLatencyTimer = 0x%x\r\n", config->u.type1.SecondaryLatencyTimer);
    KITLOutputDebugString("config.IOBase             = 0x%x\r\n", config->u.type1.IOBase);
    KITLOutputDebugString("config.IOLimit            = 0x%x\r\n", config->u.type1.IOLimit);
    KITLOutputDebugString("config.SecondaryStatus    = 0x%x\r\n", config->u.type1.SecondaryStatus);
    KITLOutputDebugString("config.MemoryBase         = 0x%x\r\n", config->u.type1.MemoryBase);
    KITLOutputDebugString("config.MemoryLimit        = 0x%x\r\n", config->u.type1.MemoryLimit);
    KITLOutputDebugString("config.PrefetchableMemoryBase         = 0x%x\r\n", config->u.type1.PrefetchableMemoryBase);
    KITLOutputDebugString("config.PrefetchableMemoryLimit        = 0x%x\r\n", config->u.type1.PrefetchableMemoryLimit);
    KITLOutputDebugString("config.PrefetchableMemoryBaseUpper32  = 0x%x\r\n", config->u.type1.PrefetchableMemoryBaseUpper32);
    KITLOutputDebugString("config.PrefetchableMemoryLimitUpper32 = 0x%x\r\n", config->u.type1.PrefetchableMemoryLimitUpper32);
    KITLOutputDebugString("config.IOBaseUpper        = 0x%x\r\n", config->u.type1.IOBaseUpper);
    KITLOutputDebugString("config.IOLimitUpper       = 0x%x\r\n", config->u.type1.IOLimitUpper);
    KITLOutputDebugString("config.Reserved2          = 0x%x\r\n", config->u.type1.Reserved2);
    KITLOutputDebugString("config.ExpansionROMBase   = 0x%x\r\n", config->u.type1.ExpansionROMBase);
    KITLOutputDebugString("config.InterruptLine      = 0x%x\r\n", config->u.type1.InterruptLine);
    KITLOutputDebugString("config.InterruptPin       = 0x%x\r\n", config->u.type1.InterruptPin);
    KITLOutputDebugString("config.BridgeControl      = 0x%x\r\n", config->u.type1.BridgeControl);
    KITLOutputDebugString("-printPCIConfig\r\n");
}

PCKITL_NIC_INFO InitKitlNIC (DWORD dwIrq, DWORD dwIoBase, DWORD dwDfltType)
{
    PCI_COMMON_CONFIG   pciConfig;
    int                 funcType, bus, device, function;
    PCSUPPORTED_NIC     pNicFound;
    int                 length = 0;
    enum {
        FIND_BY_VENDOR, // 0
        FIND_BY_TYPE    // 1
    };

    KITLOutputDebugString("InitKitlNIC: Searching for PCI Ethernet NIC (dwIrq = %x, dwIoBase = %x, dwDfltType = %x) ...\r\n",
        dwIrq, dwIoBase, dwDfltType);

    // Pass 1: iterate searching for vendor (this is the best match)
    // Pass 2: iterate searching for matching type
    for (funcType = FIND_BY_VENDOR; funcType <= FIND_BY_TYPE; funcType++)
    {
        // iterate through buses
        for (bus = 0; bus < PCI_MAX_BUS; bus++) {

            // iterate through devices
            for (device = 0; device < PCI_MAX_DEVICES; device++) {

                // iterate through functions
                for (function = 0; function < PCI_MAX_FUNCTION; function++) {
                
                    // read PCI config data
                    length = PCIReadBusData ( bus, 
                                              device,
                                              function,
                                              &pciConfig,
                                              0,
                                              (sizeof(pciConfig) - sizeof(pciConfig.DeviceSpecific)));

                    if (length == 0 || (pciConfig.VendorID == 0xFFFF))
                        break;

                    // network controller or USB?
                    if (    (  (pciConfig.BaseClass == PCI_CLASS_NETWORK_CTLR)
                            && (pciConfig.SubClass  == PCI_SUBCLASS_NET_ETHERNET_CTLR)) // Network device.
                        ||  (  (pciConfig.BaseClass == PCI_CLASS_BRIDGE_DEV)
                            && (pciConfig.SubClass  == PCI_SUBCLASS_BR_OTHER))) {       // Other Unknown Special Devices

                        DWORD dwFoundBase = pciConfig.u.type0.BaseAddresses[0] & 0xFFFFFFFC;
                        DWORD dwFoundIrq  = pciConfig.u.type0.InterruptLine;
                    
                        if (dwFoundIrq && dwFoundBase) {
                            if (!dwIrq                                                      // IRQ not specified -- use 1st found
                                || (!dwIoBase && (dwIrq == dwFoundIrq))                     // IRQ match, no IO base specified
                                || ((dwIoBase == dwFoundBase) && (dwIrq == dwFoundIrq))) {  // both IRQ and IOBase match

                                if(funcType == FIND_BY_VENDOR) {
                                    pNicFound = FindNICByVendor (&pciConfig);
                                }
                                else if(funcType == FIND_BY_TYPE) {
                                    pNicFound = FindNICByType ((UCHAR) dwDfltType);
                                }

                                if (pNicFound) {
                                    // found NIC card
                                    gKitlNic.dwIoBase   = dwFoundBase;
                                    gKitlNic.dwIrq      = dwFoundIrq;
                                    gKitlNic.dwBus      = bus;
                                    gKitlNic.dwDevice   = device;
                                    gKitlNic.dwFunction = function;
                                    gKitlNic.pDriver    = pNicFound->pDriver;
                                    gKitlNic.dwType     = pNicFound->Type;
                                    memcpy (&gKitlNic.pciConfig, &pciConfig, sizeof(pciConfig));

                                    KITLOutputDebugString ("InitKitlNIC: Found PCI Ethernet NIC (type = %x, IRQ=%d, IOBase=0x%x).\r\n",
                                        pNicFound->Type, dwFoundIrq, dwFoundBase);

                                    return g_pKitlPCINic = &gKitlNic;
                                }
                            }
                        }
                    }
                
                    if (function == 0 && !(pciConfig.HeaderType & 0x80)) 
                        break;
                
                }
                if (length == 0)
                    break;
            }

            if (length == 0 && device == 0)
                break;
        }

    }

    // can't find it on PCI bus, if IRQ and IoBase are specified, use it
    if (dwIrq && dwIoBase && (pNicFound = FindNICByType ((UCHAR) dwDfltType))) {
        gKitlNic.dwIoBase   = dwIoBase;
        gKitlNic.dwIrq      = dwIrq;
        gKitlNic.pDriver    = pNicFound->pDriver;
        gKitlNic.dwType     = dwDfltType;

        // Signal that we're using a device but it's not on the PCI bus
        memset(&gKitlNic.pciConfig, LEGACY_KITL_DEVICE_BYTEPATTERN, sizeof(pciConfig));

        KITLOutputDebugString ("InitKitlNIC: Can't find PCI Ethernet NIC, use specified data (type = %x, IRQ=%d, IOBase=0x%x).\r\n",
            pNicFound->Type, dwIrq, dwIoBase);
        return &gKitlNic;
    }
    
    return NULL;
}

//------------------------------------------------------------------------------
static void
itoa10(
    int n,
    CHAR s[],
    int bufflen
    )
{
    int i = 0; 

    // Get absolute value of number
    unsigned int val = (unsigned int)((n < 0) ? -n : n);

    // Extract digits in reverse order
    while (val)
    {
        // Make sure we don't step off the end of the character array (leave
        // room for the possible '-' sign and the null terminator).
        if (i < (bufflen - 2))
        {
            s[i++] = (val % 10) + '0';
        }

        val /= 10;
    }

    // Add sign if number negative
    if (n < 0) s[i++] = '-';

    s[i--] = '\0';

    // Reverse string
    for (n = 0; n < i; n++, i--) {
        char swap = s[n];
        s[n] = s[i];
        s[i] = swap;
    }
}


static DWORD UpperDWFromMAC (UINT16 wMAC [])
{
    DWORD ret;

    //
    // The WORDs in wMAC field are in net order, so we need to do some
    // serious shifting around.
    // A hex ethernet address of 12 34 56 78 9a bc is stored in wMAC array as
    // wMAC[0] = 3412, wMAC[1] = 7856, wMAC[2] = bc9a.
    // The 4 byte return value should look like 0x00123456
    //
    ret = (wMAC[0] & 0x00ff) << 16;
    ret |= wMAC[0] & 0xff00;
    ret |= wMAC[1] & 0x00ff;
    return ret;
}


//------------------------------------------------------------------------------
//
//  Function:  x86KitlCreateName
//
//  This function create device name from prefix and mac address (usually last
//  two bytes of MAC address used for download).
//
BOOL x86KitlCreateName(CHAR *pPrefix, UINT16 mac[], CHAR *pBuffer)
{
    int    nLen;
    DWORD  dwUpperMAC = UpperDWFromMAC (mac);

    // calculate total length needed
    nLen = strlen (pPrefix) + 2 + 5;      // 2 for vendor id, 5 for itoa of 16 bit value

    if (OAL_KITL_ID_SIZE < nLen) {
        KITLOutputDebugString ("x86KitlCreateName: Device Name Too Long '%s' too long, can't create KITL name\r\n", pPrefix);
        return FALSE;
    }
    strcpy (pBuffer, pPrefix);
    strcat (pBuffer, FindNICAbbrev (dwUpperMAC));
    
    itoa10 (((mac[2]>>8) | ((mac[2] & 0x00ff) << 8)), (pBuffer + strlen (pBuffer)), (OAL_KITL_ID_SIZE - strlen(pBuffer)));
    KITLOutputDebugString ("x86KitlCreateName: Using Device Name '%s'\r\n", pBuffer);

    return TRUE;
}

