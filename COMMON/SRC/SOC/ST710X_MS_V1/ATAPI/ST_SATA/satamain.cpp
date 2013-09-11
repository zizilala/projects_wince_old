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

#include <satamain.h>
#include <ST202T_sata.h>

// DLL instance handle; differentiate this driver from other ATAPI instances
// (what other buses?)
HINSTANCE g_hInstance;

// List of active devices/disks
CDisk *g_pDiskRoot = NULL;

// Protect global variables
CRITICAL_SECTION g_csMain;

// Debug

extern "C" BOOL RegisterDbgZones(HMODULE hMod, LPDBGPARAM lpdbgparam);

// Definitions

typedef CDisk *(* POBJECTFUNCTION)(HKEY hKey);

// IDE/ATA bus implementation

// Constructor
CSATABUS::CSATABUS(
    )
{
    // initialize device handle table, device registry key name table,
    // and port structures
    for (int i = 0; i < MAX_SATA_DEVICES_PER_CONTROLLER; i++) {
        m_hDevice[i] = NULL;
        m_szDevice[i] = NULL;
        m_pPort[i] = NULL;
    }

    // initialize addresses
    m_dwControllerBase = NULL;
    m_dwControllerRegSize = 0;

    // initialize IRQ data
    m_dwSysIntr = SYSINTR_NOP;
    m_dwIrq     = IRQ_UNSPECIFIED;

    // initialize DDKREGWINDOW structure
    memset(&m_dwi, 0, sizeof(m_dwi));

    // initialize port structures
    m_pIdeReg = NULL;
}

// Destructor
CSATABUS::~CSATABUS(
    )
{
    // disable interrupt
    if (m_dwSysIntr != SYSINTR_NOP) {
        InterruptDisable(m_dwSysIntr);
    }

    // deinitialize device handle table and device registry key name table
    for (int i = 0; i < MAX_SATA_DEVICES_PER_CONTROLLER; i++) {
        if (m_hDevice[i]) {
            BOOL fOk = DeactivateDevice(m_hDevice[i]);
            DEBUGCHK(fOk);
            DEBUGCHK(m_szDevice[i] != NULL);
        }
        if (m_szDevice[i] != NULL) {
            delete m_szDevice[i];
        }

        // deinitialize port structures
        if (m_pPort[i]) {
            delete m_pPort[i];
        }
    }
    // delete IDE_ registry value set
    if (m_pIdeReg) {
        if (m_pIdeReg->pszSpawnFunction) {
            LocalFree(m_pIdeReg->pszSpawnFunction);
        }
        if (m_pIdeReg->pszIsrDll) {
            LocalFree(m_pIdeReg->pszIsrDll);
        }
        if (m_pIdeReg->pszIsrHandler) {
            LocalFree(m_pIdeReg->pszIsrHandler);
        }
        LocalFree(m_pIdeReg);
    }

    // unmap SATA controller's I/O windows
    if (m_dwControllerBase && m_dwControllerRegSize != 0) {
        MmUnmapIoSpace((LPVOID)m_dwControllerBase, m_dwControllerRegSize);
    }
}

// Constructor
CSATAPort::CSATAPort(
    CSATABUS *pParent
    )
{
    DEBUGCHK(pParent);
    InitializeCriticalSection(&m_csPort);

    // initialize flags
    m_fInitialized = 0;
    m_dwFlag = 0;

    // hook up bus
    m_pController = pParent;
    // initialize I/O ports
    m_dwRegBase = 0;
    m_dwRegAlt = 0;
    m_dwBMR = 0;
    m_bStatus = 0;
    // initialize registry value
    m_pDskReg[0] = NULL;
    m_pDskReg[1] = NULL;
    // initialize interrupt event
    m_hIRQEvent = NULL;
    m_hSATAEvent = NULL;
    m_hDMAEvent = NULL;
    m_hErrorEvent = NULL;
    m_dwSysIntr = SYSINTR_UNDEFINED;
}

// Destructor
CSATAPort::~CSATAPort(
    )
{
    DeleteCriticalSection(&m_csPort);
    // close interrupt event handle
    if (m_hSATAEvent) {
        CloseHandle(m_hSATAEvent);
    }
    if (m_hDMAEvent) {
        CloseHandle(m_hDMAEvent);
    }
    if (m_hErrorEvent) {
        CloseHandle(m_hErrorEvent);
    }
    // close interrupt event handle
    if (m_hIRQEvent) {
        CloseHandle(m_hIRQEvent);
    }

    // free DSK_ registry value set
    if (m_pDskReg[0]) {
        LocalFree(m_pDskReg[0]);
    }
    if (m_pDskReg[1]) {
        LocalFree(m_pDskReg[1]);
    }
}

// Acquire exclusive access to IDE/ATA channel's I/O window
VOID
CSATAPort::TakeCS(
    )
{
    EnterCriticalSection(&m_csPort);
}

// Release exclusive access to IDE/ATA channel's I/O window
VOID
CSATAPort::ReleaseCS(
    )
{
    LeaveCriticalSection(&m_csPort);
}

// Write I/O window data to debug output
VOID
CSATAPort::PrintInfo(
    )
{
    DEBUGMSG(ZONE_INIT, (TEXT("dwRegBase            = %08X\r\n"), m_dwRegBase));
    DEBUGMSG(ZONE_INIT, (TEXT("dwRegAlt             = %08X\r\n"), m_dwRegAlt));
    DEBUGMSG(ZONE_INIT, (TEXT("dwBMR                = %08X\r\n"), m_dwBMR));
}

// Helper functions

// This function is used by an Xxx_Init function to fetch the name of and return
// a handle to the instance/device ("Key") key from an Active key
HKEY
AtaLoadRegKey(
    HKEY hActiveKey,
    TCHAR **pszDevKey
    )
{
    DWORD dwValueType;        // registry value type
    DWORD dwValueLength = 0;  // registry value length
    PTSTR szDeviceKey = NULL; // name of device key; value associated with "Key"
    HKEY hDeviceKey = NULL;   // handle to device key; handle to "Key"

    // query the value of "Key" with @dwValueLength=0, to determine the actual
    // length of the value (so as to allocate the exact amount of memory)

    if (ERROR_SUCCESS == RegQueryValueEx(hActiveKey, DEVLOAD_DEVKEY_VALNAME, NULL, &dwValueType, NULL, &dwValueLength)) {

        // allocate just enough memory to store the value of "Key"
        szDeviceKey = (PTSTR)LocalAlloc(LPTR, dwValueLength);
        if (szDeviceKey) {

            // read the actual value of "Key" and null terminate the target buffer
            RegQueryValueEx(hActiveKey, DEVLOAD_DEVKEY_VALNAME, NULL, &dwValueType, (PBYTE)szDeviceKey, &dwValueLength);
            DEBUGCHK(dwValueLength != 0);
            szDeviceKey[(dwValueLength / sizeof(TCHAR)) - 1] = 0;

            // open the device key
            if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDeviceKey, 0, 0, &hDeviceKey)) {
                DEBUGMSG(ZONE_INIT, (_T(
                    "AtaLoadRegyKey> Failed to open %s\r\n"
                    ), szDeviceKey));
                hDeviceKey = NULL;
            }
        }
    }
    if (!hDeviceKey) {
        if (szDeviceKey) {
            LocalFree(szDeviceKey);
        }
        *pszDevKey = NULL;
    }
    else {
        *pszDevKey = szDeviceKey;
    }
    return hDeviceKey;
}

// This function is used to determine whether a target disk instance is valid
BOOL
AtaIsValidDisk(
    CDisk *pDisk
    )
{
    CDisk *pTemp = g_pDiskRoot;
    while (pTemp) {
        if (pTemp == pDisk) {
            return TRUE;
        }
        pTemp = pTemp->m_pNextDisk;
    }
    return FALSE;
}

// This function is used to fetch a SATA channel's I/O window from its instance
// key; this function recovers gracefully if an OEM has a proprietary registry
// configuration that doesn't specify bus type or bus number
BOOL
AtaGetRegistryResources(
    HKEY hDevKey,
    PDDKWINDOWINFO pdwi
    )
{
    DEBUGCHK(pdwi != NULL);

    if (!pdwi) {
        return FALSE;
    }

    // fetch I/O window information
    pdwi->cbSize = sizeof(*pdwi);
    if (ERROR_SUCCESS != ::DDKReg_GetWindowInfo(hDevKey, pdwi)) {
        return FALSE;
    }

    // if interface not specified, then assume PCI
    if (pdwi->dwInterfaceType == InterfaceTypeUndefined) {
        DEBUGMSG(ZONE_WARNING, (_T(
            "Atapi!AtaGetRegistryResources> bus type not specified, using PCI as default\r\n"
            )));
        pdwi->dwInterfaceType = PCIBus;
    }

    return TRUE;
}

// This function translates a bus address in an I/O window to a virtual address
DWORD
DoIoTranslation(
    PDDKWINDOWINFO pdwi,
    DWORD dwIoWindowIndex
    )
{
    PHYSICAL_ADDRESS PhysicalAddress; // bus address
    DWORD AddressSpace = 1;           // mark bus address as being in an I/O window
    LPVOID pAddress;                  // return

    DEBUGCHK(pdwi != NULL);
    DEBUGCHK(dwIoWindowIndex < MAX_DEVICE_WINDOWS);
    if (!pdwi) {
        return NULL;
    }

    // extract the target bus address
    PhysicalAddress.HighPart = 0;
    PhysicalAddress.LowPart = pdwi->ioWindows[dwIoWindowIndex].dwBase;

    // translate the target bus address to a virtual address
    if (!TransBusAddrToVirtual(
        (INTERFACE_TYPE)pdwi->dwInterfaceType,
        pdwi->dwBusNumber,
        PhysicalAddress,
        pdwi->ioWindows[dwIoWindowIndex].dwLen,
        &AddressSpace,
        &pAddress
     )) {
        return NULL;
    }

    return (DWORD)pAddress;
}

// This function translates a bus address to a statically physical address
DWORD
DoStaticTranslation(
    PDDKWINDOWINFO pdwi,
    DWORD dwIoWindowIndex
    )
{
    PHYSICAL_ADDRESS PhysicalAddress; // bus address
    DWORD AddressSpace = 1;           // mark bus address as being in an I/O window
    LPVOID pAddress;                  // return

    DEBUGCHK(pdwi != NULL);
    DEBUGCHK(dwIoWindowIndex < MAX_DEVICE_WINDOWS);

    if (!pdwi) {
        return NULL;
    }

    // extract bus address
    PhysicalAddress.HighPart = 0;
    PhysicalAddress.LowPart = pdwi->ioWindows[dwIoWindowIndex].dwBase;

    // translate the target bus address to a statically mapped physical address
    if (!TransBusAddrToStatic(
        (INTERFACE_TYPE)pdwi->dwInterfaceType,
        pdwi->dwBusNumber,
        PhysicalAddress,
        pdwi->ioWindows[dwIoWindowIndex].dwLen,
        &AddressSpace,
        &pAddress
    )) {
        return NULL;
    }

    return (DWORD)pAddress;
}

// This function reads the IDE registry value set from the SATA controller's
// registry key.
BOOL
GetIDERegistryValueSet(
    HKEY hIDEInstanceKey,
    PIDEREG pIdeReg
    )
{
    BOOL fRet;

    DEBUGCHK(NULL != pIdeReg);

    // fetch legacy boolean
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_LEGACY, &pIdeReg->dwLegacy);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_LEGACY));
        return FALSE;
    }
    if (pIdeReg->dwLegacy >= 2) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Bad value(%d) for %s in IDE instance key; valid: {0, 1}\r\n"
            ), pIdeReg->dwLegacy, REG_VAL_IDE_LEGACY));
        return FALSE;
    }

    // fetch IRQ; this value is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_IRQ, &pIdeReg->dwIrq);
    if (!fRet) {
        pIdeReg->dwIrq = IRQ_UNSPECIFIED;
    }

    // fetch SysIntr; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_SYSINTR, &pIdeReg->dwSysIntr);
    if (!fRet) {
        pIdeReg->dwSysIntr = SYSINTR_NOP;
    }

    // fetch vendor id; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_VENDORID, &pIdeReg->dwVendorId);
    if (!fRet) {
        pIdeReg->dwVendorId = 0;
    }

    // fetch DMA alignment; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_DMAALIGNMENT, &pIdeReg->dwDMAAlignment);
    if (!fRet) {
        pIdeReg->dwDMAAlignment = 0;
    }

    // fetch soft reset timeout
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_SOFTRESETTIMEOUT, &pIdeReg->dwSoftResetTimeout);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_SOFTRESETTIMEOUT));
        return FALSE;
    }
    else {
        pIdeReg->dwSoftResetTimeout *= 100; // We query the drive on 10ms increments.
    }


    // fetch Status register poll cycles
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_STATUSPOLLCYCLES, &pIdeReg->dwStatusPollCycles);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_STATUSPOLLCYCLES));
        return FALSE;
    }

    // fetch Status register polls per cycle
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_STATUSPOLLSPERCYCLE, &pIdeReg->dwStatusPollsPerCycle);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_STATUSPOLLSPERCYCLE));
        return FALSE;
    }

    // fetch Status register poll cycle pause
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_STATUSPOLLCYCLEPAUSE, &pIdeReg->dwStatusPollCyclePause);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_STATUSPOLLCYCLEPAUSE));
        return FALSE;
    }

    // fetch spawn function
    fRet = AtaGetRegistryString(hIDEInstanceKey, REG_VAL_IDE_SPAWNFUNCTION, &pIdeReg->pszSpawnFunction);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_SPAWNFUNCTION));
        return FALSE;
    }

    // fetch ISR dll; this is not mandatory; allocate pszIsrDll
    fRet = AtaGetRegistryString(hIDEInstanceKey, REG_VAL_IDE_ISRDLL, &pIdeReg->pszIsrDll, 0);
    if (!fRet) {
        pIdeReg->pszIsrDll = NULL;
    }

    // fetch ISR handler; this is not mandatory; allocate pszIsrHandler
    fRet = AtaGetRegistryString(hIDEInstanceKey, REG_VAL_IDE_ISRHANDLER, &pIdeReg->pszIsrHandler, 0);
    if (!fRet) {
        pIdeReg->pszIsrHandler = NULL;
    }

    // fetch device control offset; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_DEVICECONTROLOFFSET, &pIdeReg->dwDeviceControlOffset);
    if (!fRet) {
        // this value is only used by atapipcmcia
        pIdeReg->dwDeviceControlOffset = ATA_REG_ALT_STATUS;
    }

    // fetch alternate status offset; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_ALTERNATESTATUSOFFSET, &pIdeReg->dwAlternateStatusOffset);
    if (!fRet) {
        // this value is only used by atapipcmcia
        pIdeReg->dwAlternateStatusOffset = ATA_REG_DRV_CTRL;
    }

    // fetch register stride
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_REGISTERSTRIDE, &pIdeReg->dwRegisterStride);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_REGISTERSTRIDE));
        return FALSE;
    }
    if (0 == pIdeReg->dwRegisterStride) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Bad value(%d) for %s in IDE instance key; valid: > 0\r\n"
            ), pIdeReg->dwRegisterStride, REG_VAL_IDE_REGISTERSTRIDE));
        return FALSE;
    }

    // fetch number of times to retry a command when a transient non-recovered error is detected
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_RETRIESONERROR, &pIdeReg->dwNumRetriesOnSATAError);
    if (!fRet) {
        // Default to 3 retries if value not specified in registry.
        pIdeReg->dwNumRetriesOnSATAError = 3;
    }

    return TRUE;
}

// This function reads the DSK registry value set from the IDE/ATA controller's
// registry key
BOOL
GetDSKRegistryValueSet(
    HKEY hDSKInstanceKey,
    PDSKREG pDskReg
    )
{
    BOOL fRet;

    // fetch device ID
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_DEVICEID, &pDskReg->dwDeviceId);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_DEVICEID));
        return FALSE;
    }
    if (!((0 <= pDskReg->dwDeviceId) && (pDskReg->dwDeviceId < MAX_SATA_DEVICES_PER_CONTROLLER))) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: 0\r\n"
            ), pDskReg->dwDeviceId, REG_VAL_DSK_DEVICEID));
        return FALSE;
    }

    // fetch interrupt driven I/O boolean
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_INTERRUPTDRIVEN, &pDskReg->dwInterruptDriven);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_INTERRUPTDRIVEN));
        return FALSE;
    }
    if (pDskReg->dwInterruptDriven >= 2) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {0, 1}\r\n"
            ), pDskReg->dwInterruptDriven, REG_VAL_DSK_INTERRUPTDRIVEN));
        return FALSE;
    }

    // fetch DMA triple
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_DMA, &pDskReg->dwDMA);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_DMA));
        return FALSE;
    }
    if (pDskReg->dwDMA >= 3) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {0=no DMA, 1=DMA, 2=ATA DMA only}\r\n"
            ), pDskReg->dwDMA, REG_VAL_DSK_DMA));
        return FALSE;
    }
    if (pDskReg->dwDMA != 0 && pDskReg->dwInterruptDriven == 0) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Interrupts must be enabled when setting DMA mode!\r\n"
            ), pDskReg->dwDMA, REG_VAL_DSK_DMA));
        return FALSE;
    }

    // fetch double buffer size
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_DOUBLEBUFFERSIZE, &pDskReg->dwDoubleBufferSize);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_DOUBLEBUFFERSIZE));
        return FALSE;
    }
    if ((0 != pDskReg->dwDoubleBufferSize) && ((pDskReg->dwDoubleBufferSize < REG_VAL_DSK_DOUBLEBUFFERSIZE_MIN) || (pDskReg->dwDoubleBufferSize > REG_VAL_DSK_DOUBLEBUFFERSIZE_MAX))) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {%d, ..., %d}\r\n"
            ), pDskReg->dwDoubleBufferSize, REG_VAL_DSK_DOUBLEBUFFERSIZE, REG_VAL_DSK_DOUBLEBUFFERSIZE_MIN, REG_VAL_DSK_DOUBLEBUFFERSIZE_MAX));
        return FALSE;
    }
    if (0 != (pDskReg->dwDoubleBufferSize % SECTOR_SIZE)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; must be multiple of %d\r\n"
            ), pDskReg->dwDoubleBufferSize, REG_VAL_DSK_DOUBLEBUFFERSIZE, SECTOR_SIZE));
        return FALSE;
    }

    // fetch DRQ data block size
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_DRQDATABLOCKSIZE, &pDskReg->dwDrqDataBlockSize);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_DRQDATABLOCKSIZE));
        return FALSE;
    }
    if (pDskReg->dwDrqDataBlockSize > REG_VAL_DSK_DRQDATABLOCKSIZE_MAX) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {%d, ..., %d}\r\n"
            ), pDskReg->dwDrqDataBlockSize, REG_VAL_DSK_DRQDATABLOCKSIZE, REG_VAL_DSK_DRQDATABLOCKSIZE_MIN, REG_VAL_DSK_DRQDATABLOCKSIZE_MAX));
        return FALSE;
    }
    if (0 != (pDskReg->dwDrqDataBlockSize % SECTOR_SIZE)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; must be multiple of %d\r\n"
            ), pDskReg->dwDrqDataBlockSize, REG_VAL_DSK_DRQDATABLOCKSIZE, SECTOR_SIZE));
        return FALSE;
    }

    // fetch write cache boolean
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_WRITECACHE, &pDskReg->dwWriteCache);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_WRITECACHE));
        return FALSE;
    }
    if (pDskReg->dwWriteCache >= 2) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {0, 1}\r\n"
            ), pDskReg->dwWriteCache, REG_VAL_DSK_WRITECACHE));
        return FALSE;
    }

    // fetch look-ahead boolean
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_LOOKAHEAD, &pDskReg->dwLookAhead);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_LOOKAHEAD));
        return FALSE;
    }
    if (pDskReg->dwLookAhead >= 2) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {0, 1}\r\n"
            ), pDskReg->dwLookAhead, REG_VAL_DSK_LOOKAHEAD));
        return FALSE;
    }

    // fetch transfer mode
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_TRANSFERMODE, &pDskReg->dwTransferMode);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_TRANSFERMODE));
        return FALSE;
    }

    return TRUE;
}

// This function reads the I/O window data from the IDE instance key and builds
// the I/O ports for the controller channels
BOOL
GetIoPort(
    HKEY hDevKey,
    PTSTR szDevKey,
    CSATABUS *pBus
    )
{
    BOOL    fRet = FALSE;

    DEBUGCHK(pBus);

    // Fetch the SATA controller's I/O window.  This consists of
    // the I/O range which includes the ST Bus wrapper registers,
    // the DMA controller, and the individual device control registers.
    // We map them all as a single contiguous space and later segregate
    // regions depending on their function.

    if (
        (!AtaGetRegistryResources(hDevKey, &pBus->m_dwi)) ||
        (pBus->m_dwi.dwNumIoWindows != 1) ||
        (pBus->m_dwi.ioWindows[0].dwBase == 0) ||
        (pBus->m_dwi.ioWindows[0].dwLen == 0)
    ) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIoPort> Resource configuration missing or invalid in device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // Save the base virtual addresss of the controller.  This points to the
    // beginning of the ST Wrapper registers for the SATA block, and
    // also includes the DMA controller, channel control registers, and SATA
    // host registers.
    pBus->m_dwControllerBase = DoIoTranslation(&pBus->m_dwi, 0);

    // Save size of controller's register set so that it can later be unmapped
    pBus->m_dwControllerRegSize = pBus->m_dwi.ioWindows[0].dwLen;

    fRet = TRUE;

exit:;
    return fRet;
}

// SATA device stream interface

/*++

DSK_Init
    This function is called as a result of IDE_Init calling ActivateDevice on
    HKLM\Drivers\@BUS\@IDEAdapter\DeviceX, to initialize a single
    device on a particular SATA channel of a particular SATA controller.

    This function is responsible for creating a CDisk instance to associate
    with a device.  This function reads the "Object" value from its instance
    key to determine which CDisk (sub)type to instantiate and calls Init on the
    CDisk instance to initialize the device.  If the device is not present, then
    Init will fail.  The "Object" value maps to a function that creates an
    instance of the target CDisk (sub)type.

    Note that this driver model is convoluted.  A CDisk (sub)type instance
    corresponds to both a SATA controller and a SATA device.

Parameters:
    dwContext - pointer to string containing the registry path to the active key
    of the associated device; the active key contains a key to the device's instance
    key, which stores all of the device's configuration information

Return:
    On success, return handle to device (to identify device); this handle is
    passed to all subsequent DSK_Xxx calls.  Otherwise, return null.

--*/

#define DSKINIT_UNDO_CLS_KEY_ACTIVE 0x1
#define DSKINIT_UNDO_CLS_KEY_DEVICE 0x2

EXTERN_C
DWORD
DSK_Init(
    DWORD dwContext
    )
{
    DWORD dwUndo = 0;                     // undo bitset

    PTSTR szActiveKey = (PTSTR)dwContext; // name of device's active key
    HKEY hActiveKey;                      // handle to device's active key
    PTSTR szDevKey = NULL;                // name of device's instance key
    HKEY hDevKey;                         // handle to device's instance key

    POBJECTFUNCTION pObject = NULL;       // pointer to spawn function

    CSATAPort *pPort = NULL;              // port
    DWORD dwDeviceId = 0;                 // device ID; 0 => master, 1 => slave

    CDisk *pDisk = NULL;                  // return

    // guard global data; i.e., g_pDiskRoot

    EnterCriticalSection(&g_csMain);

    // open device's active key

    if ((ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szActiveKey, 0, 0, &hActiveKey))) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to open device's active key(%s)\r\n"
            ), szActiveKey));
        goto exit;
    }
    dwUndo |= DSKINIT_UNDO_CLS_KEY_ACTIVE;
    DUMPREGKEY(ZONE_INIT, szActiveKey, hActiveKey);

    // read name of and open device's instance key from device's active key

    if (!(hDevKey = AtaLoadRegKey(hActiveKey, &szDevKey))) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to fetch/open device's instance key from device's active key(%s)\r\n"
            ), szActiveKey));
        goto exit;
    }
    dwUndo |= DSKINIT_UNDO_CLS_KEY_DEVICE;
    DUMPREGKEY(ZONE_INIT, szDevKey, hDevKey);

    // fetch heap address of port instance from device's instance key

    if (!AtaGetRegistryValue(hDevKey, REG_VALUE_PORT, (PDWORD)&pPort)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to read address of port instance from device's instance key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // fetch device ID from device's instance key

    if (!AtaGetRegistryValue(hDevKey, REG_VAL_DSK_DEVICEID, &dwDeviceId)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to read device ID device's instance key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // resolve address of spawn function

    pObject = (POBJECTFUNCTION)GetProcAddress(g_hInstance, pPort->m_pController->m_pIdeReg->pszSpawnFunction);
    if (!pObject) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to resolve address of device's spawn function(%s)\r\n"
            ), pPort->m_pController->m_pIdeReg->pszSpawnFunction));
        goto exit;
    }

    // instantiate CDisk object

    pDisk = pObject(hDevKey);

    // if successful, write the name of the device's active and instance keys to
    // its CDisk instance, and add the CDisk instance to the IDE/ATA bus driver's
    // list of active disk devices

    if (pDisk) {

        // allocate the sterile, maximal SG_REQ for safe I/O
        pDisk->m_pSterileIoRequest = (PSG_REQ)LocalAlloc(
            LPTR,
            (sizeof(SG_REQ) + ((MAX_SG_BUF) - 1) * sizeof(SG_BUF))
            );

        if (NULL == pDisk->m_pSterileIoRequest) {
            delete pDisk;
            pDisk = NULL;
            goto exit;
        }

        // this information is used for ATA/ATAPI power management

        pDisk->SetActiveKey(szActiveKey);
        pDisk->SetDeviceKey(szDevKey);

        // inform the CDisk instance as to which device it is

        pDisk->m_pPort = pPort;
        pDisk->m_dwDeviceId = 0;    // All SATA devices are masters
        pDisk->m_dwDevice = 0;

        // configure register block

        pDisk->ConfigureRegisterBlock(pPort->m_pController->m_pIdeReg->dwRegisterStride);

        // initialize device

        if (!pDisk->Init(hActiveKey)) {
            delete pDisk;
            pDisk = NULL;
            goto exit;
        }

        // add CDisk instance to IDE/ATA controller's list of active devices

        pDisk->m_pNextDisk = g_pDiskRoot;
        g_pDiskRoot = pDisk;

        pPort->m_fInitialized = TRUE;

        DEBUGMSG(ZONE_INIT, (_T(
            "Atapi!DSK_Init> Initialized Device %d %s\r\n"
            ), dwDeviceId, szDevKey));
    }

exit:;

    // clean up
    if (NULL == pDisk) {
        if (dwUndo & DSKINIT_UNDO_CLS_KEY_ACTIVE) {
            RegCloseKey(hActiveKey);
        }
        if (dwUndo & DSKINIT_UNDO_CLS_KEY_DEVICE) {
            RegCloseKey(hDevKey);
        }
        // pPort is deleted in IDE_Deinit
    }
    if (szDevKey) {
        LocalFree(szDevKey);
    }

    LeaveCriticalSection(&g_csMain);

    return (DWORD)pDisk;
}

/*++

DSK_Deinit
    This function deallocates the associated CDisk instance.

Parameters:
    dwHandle - pointer to associated CDisk instance (initially returned by
    DSK_Init)

Return:
    This function always succeeds.

--*/
EXTERN_C
BOOL
DSK_Deinit(
    DWORD dwHandle
    )
{
    CDisk *pDiskPrev = NULL;
    CDisk *pDiskCur = g_pDiskRoot;

    EnterCriticalSection(&g_csMain);

    // find the CDisk instance in global CDisk list

    while (pDiskCur) {
        if (pDiskCur == (CDisk *)dwHandle) {
            break;
        }
        pDiskPrev = pDiskCur;
        pDiskCur = pDiskCur->m_pNextDisk;
    }

    // remove CDisk instance from global CDisk list

    if (pDiskCur) {
        if (pDiskPrev) {
            pDiskPrev = pDiskCur->m_pNextDisk;
        }
        else {
            g_pDiskRoot = pDiskCur->m_pNextDisk;
        }
        delete pDiskCur;
    }

    LeaveCriticalSection(&g_csMain);

    return TRUE;
}

/*++

DSK_Open
    This function opens a CDisk instance for use.

Parameters:
    dwHandle - pointer to associated CDisk instance (initially returned by
    DSK_Init)
    dwAccess - specifes how the caller would like too use the device (read
    and/or write) [this argument is ignored]
    dwShareMode - specifies how the caller would like this device to be shared
    [this argument is ignored]

Return:
    On success, return handle to "open" CDisk instance; this handle is the
    same as dwHandle.  Otherwise, return null.

--*/
EXTERN_C
DWORD
DSK_Open(
    HANDLE dwHandle,
    DWORD dwAccess,
    DWORD dwShareMode
    )
{
    CDisk *pDisk = (CDisk *)dwHandle;

    EnterCriticalSection(&g_csMain);

    // validate the CDisk instance

    if (!AtaIsValidDisk(pDisk)) {
        pDisk = NULL;
    }

    LeaveCriticalSection(&g_csMain);

    // if the CDisk instance is valid, then open; open just increments the
    // instance's open count

    if (pDisk) {
        pDisk->Open();
    }

    return (DWORD)pDisk;
}

/*++

DSK_Close
    This function closes a CDisk instance.

Parameters:
    dwHandle - pointer to associated CDisk instance (initially returned by
    DSK_Init)

Return:
    On success, return true.  Otherwise, return false.

--*/
EXTERN_C
BOOL
DSK_Close(
    DWORD dwHandle
    )
{
    CDisk *pDisk = (CDisk *)dwHandle;

    EnterCriticalSection(&g_csMain);

    // validate the CDisk instance

    if (!AtaIsValidDisk(pDisk)) {
        pDisk = NULL;
    }

    LeaveCriticalSection(&g_csMain);

    // if CDisk instance is valid, then close; close just decrements the
    // instance's open count

    if (pDisk) {
        pDisk->Close();
    }

    return (pDisk != NULL);
}

/*++

DSK_IOControl
    This function processes an IOCTL_DISK_Xxx/DISK_IOCTL_Xxx I/O control.

Parameters:
    dwHandle - pointer to associated CDisk instance (initially returned by
    DSK_Init)
    dwIOControlCode - I/O control to perform
    pInBuf - pointer to buffer containing the input data of the I/O control
    nInBufSize - size of pInBuf (bytes)
    pOutBuf - pointer to buffer that is to receive the output data of the
    I/O control
    nOutBufSize - size of pOutBuf (bytes)
    pBytesReturned - pointer to DWORD that is to receive the size (bytes) of the
    output data of the I/O control
    pOverlapped - ignored

Return:
    On success, return true.  Otherwise, return false.

--*/
EXTERN_C
BOOL
DSK_IOControl(
    DWORD dwHandle,
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned,
    PDWORD pOverlapped)
{
    CDisk *pDisk = (CDisk *)dwHandle;
    DWORD SafeBytesReturned = 0;
    BOOL fRet = FALSE;

    EnterCriticalSection(&g_csMain);

    // validate CDisk instance

    if (!AtaIsValidDisk(pDisk)) {
        pDisk = NULL;
    }

    LeaveCriticalSection(&g_csMain);

    if (!pDisk) {
        return FALSE;
    }

    // DISK_IOCTL_INITIALIZED is a deprecated IOCTL; what does PostInit do?

    if (dwIoControlCode == DISK_IOCTL_INITIALIZED) {
        fRet = pDisk->PostInit((PPOST_INIT_BUF)pInBuf);
    }
    else {

        IOREQ IOReq;

        // build I/O request structure

        memset(&IOReq, 0, sizeof(IOReq));
        IOReq.dwCode = dwIoControlCode;
        IOReq.pInBuf = pInBuf;
        IOReq.dwInBufSize = nInBufSize;
        IOReq.pOutBuf = pOutBuf;
        IOReq.dwOutBufSize = nOutBufSize;
        IOReq.pBytesReturned = &SafeBytesReturned;
        IOReq.hProcess = GetCallerProcess();

        // perform I/O control

        __try {
            fRet = pDisk->PerformIoctl(&IOReq);

            // if the caller supplied pBytesReturned, then write the value
            // from our safe copy
            if (pBytesReturned) {
                *pBytesReturned = SafeBytesReturned;
            }

        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fRet = FALSE;
            SetLastError(ERROR_GEN_FAILURE);
        }
    }

    return fRet;
}

/*++

DSK_PowerUp
    This function resumes the device.

Parameters:
    None

Return:
    On success, return true.  Otherwise, return false.

--*/
EXTERN_C
VOID
DSK_PowerUp(
    VOID
    )
{
    EnterCriticalSection(&g_csMain);

    CDisk *pDisk = g_pDiskRoot;

    // iterate through the global CDisk list and direct each CDisk instance to
    // power up its associated device

    while (pDisk) {
        pDisk->PowerUp();
        pDisk = pDisk->m_pNextDisk;
    }

    LeaveCriticalSection(&g_csMain);
}

/*++

DSK_PowerDown
    This function suspends a device.

Parameters:
    None

Return:
    On success, return true.  Otherwise, return false.

--*/
EXTERN_C
VOID
DSK_PowerDown(
    VOID
    )
{
    EnterCriticalSection(&g_csMain);

    CDisk *pDisk = g_pDiskRoot;

    // iterate through the global CDisk list and direct each CDist instance to
    // power down its associated device

    while (pDisk) {
        pDisk->PowerDown();
        pDisk = pDisk->m_pNextDisk;
    }

    LeaveCriticalSection(&g_csMain);
}

/*++

IDE_Init
    This function is called as a result of a bus driver enumerating the SATA
    controller.

    The SATA controller contains a number of "channels" which correspond to
    individual drives.  The SATA controller's instance key will typically
    contain the following subkeys: Device0, Device1, and so on.

    This function is responsible for searching the driver's instance key for
    DeviceX subkeys and calling ActivateDevice on each DeviceX subkey found.
    The call to ActivateDevice will eventually enter DSK_Init.  DSK_Init is
    responsible for creating a CDisk instance to associate with a device.  If a
    device is present and intialization succeeds, then DSK_Init will succeed.

Parameters:
    dwContext - pointer to string containing the registry path to the active key
    of the SATA controller; the active key contains a key to the SATA
    controller's instance key, which stores all of the SATA controller's
    configuration information

Return:
    On success, return handle to SATA controller (for identification); this
    handle is passed to all subsequent IDE_Xxx calls.  Otherwise, return null.

--*/

#define IDEINIT_UNDO_CLS_KEY_ACTIVE 0x01
#define IDEINIT_UNDO_CLS_KEY_DEVICE 0x02
#define IDEINIT_UNDO_DEL_BUS        0x04
#define IDEINIT_UNDO_DEL_PORT_PRI   0x08
#define IDEINIT_UNDO_DEL_PORT_SEC   0x10
#define IDEINIT_UNDO_DEL_REG_IDE    0x20
#define IDEINIT_UNDO_DEL_REG_DSK    0x40

EXTERN_C
DWORD
IDE_Init(
    DWORD dwContext
    )
{
    DWORD    dwUndo = 0;                     // undo bitset
    PTSTR    szActiveKey = (PTSTR)dwContext; // name of IDE/ATA controller's active key
    HKEY     hActiveKey;                     // handle to IDE/ATA controller's active key
    PTSTR    szDevKey = NULL;                // name of IDE/ATA controller's instance key
    HKEY     hDevKey;                        // handle to IDE/ATA controller's instance key
    PDSKREG  pDskReg;                        // ATA/ATAPI device's registry value set
    CSATABUS *pBus = NULL;                    // return

    // open the SATA controllers's active key

    if ((ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szActiveKey, 0, 0, &hActiveKey)) || (!hActiveKey)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to open SATA controller's active key(%s)\r\n"
            ), szActiveKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_CLS_KEY_ACTIVE;
    DUMPREGKEY(ZONE_INIT, szActiveKey, hActiveKey);

    // fetch the name of the SATA controller's instance key and open it

    if (!(hDevKey = AtaLoadRegKey(hActiveKey, &szDevKey)) || !szDevKey) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to fetch/open SATA controller's instance key from active key(%s)\r\n"
            ), szActiveKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_CLS_KEY_DEVICE;
    DUMPREGKEY(ZONE_INIT, szDevKey, hDevKey);

    // instantiate an SATA controller ("bus") object

    if (!(pBus = new CSATABUS)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to instantiate SATA controller bus object; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_DEL_BUS;

    // instantiate channel port objects.  If corresponding devices
    // do not exist in the registry or can't be initialized, then
    // the unused objects will be deleted at the end of the
    // initialization sequence.

    for (int i = 0; i < MAX_SATA_DEVICES_PER_CONTROLLER; i++) {
        pBus->m_pPort[i] = new CSATAPort(pBus);
        if (!pBus->m_pPort[i]) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to allocate port for SATA channel %d; device key(%s)\r\n"
                ), i, szDevKey));
            goto exit;
        }
        if (NULL == (pBus->m_pPort[i]->m_hSATAEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to create SATA interrupt event for port %d of device(%s)\r\n"
                ), i, szDevKey));
            goto exit;
        }
        if (NULL == (pBus->m_pPort[i]->m_hErrorEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to create Error interrupt event for port %d of device(%s)\r\n"
                ), i, szDevKey));
            goto exit;
        }
        if (NULL == (pBus->m_pPort[i]->m_hDMAEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to create DMA interrupt event for port %d of device(%s)\r\n"
                ), i, szDevKey));
            goto exit;
        }
    }
    dwUndo |= IDEINIT_UNDO_DEL_PORT_PRI;

    // configure controller from I/O window information in registry

    if (!GetIoPort(hDevKey, szDevKey, pBus)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Bad I/O window information; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // fetch SATA controller registry value set (i.e., registry configuration)

    pBus->m_pIdeReg = (PIDEREG)LocalAlloc(LPTR, sizeof(IDEREG));
    if (!pBus->m_pIdeReg) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to allocate IDE_ registry value set; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_DEL_REG_IDE;
    if (!GetIDERegistryValueSet(hDevKey, pBus->m_pIdeReg)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to read IDE_ registry value set from registry; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // no SysIntr provided; we have to map IRQ to SysIntr ourselves

    if (!pBus->m_pIdeReg->dwSysIntr) {

        DWORD dwReturned = 0;

        if (!KernelIoControl(
            IOCTL_HAL_REQUEST_SYSINTR,
            (LPVOID)&pBus->m_pIdeReg->dwIrq, sizeof(DWORD),
            (LPVOID)&pBus->m_dwSysIntr, sizeof(DWORD),
            &dwReturned
        )) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to map IRQ(%d) to SysIntr for device(%s)\r\n"
                ), pBus->m_pIdeReg->dwIrq, szDevKey));
            goto exit;
        }
    }

    // SATA "bus" enumeration; scan the current SATA controller's instance
    // key for DeviceX subkeys

    DEBUGMSG(ZONE_INIT, (_T(
        "Atapi!IDE_Init> Start of SATA device enumeration\r\n"
        )));

    DWORD dwIndex = 0;        // index of next DeviceX subkey to fetch/enumerate
    HKEY hKey;                // handle to DeviceX subkey
    TCHAR szNewKey[MAX_PATH]; // name of DeviceX subkey
    DWORD dwNewKeySize;       // size of name of DeviceX subkey
    DWORD dwDeviceId;         // "DeviceId" read from DeviceX subkey

    dwNewKeySize = (sizeof(szNewKey) / sizeof(TCHAR));

    while (
        ERROR_SUCCESS == RegEnumKeyEx(
            hDevKey,       // SATA controller's instance key
            dwIndex,       // index of the subkey to fetch
            szNewKey,      // name of subkey (e.g., "Device0")
            &dwNewKeySize, // size of name of subkey
            NULL,          // lpReserved; set to NULL
            NULL,          // lpClass; not required
            NULL,          // lpcbClass; lpClass is NULL; hence, NULL
            NULL           // lpftLastWriteTime; set to NULL
    )) {

        dwIndex += 1;
        dwNewKeySize = (sizeof(szNewKey) / sizeof(TCHAR));
        pDskReg = NULL;

        // open the DeviceX subkey; copy configuration information from the
        // SATA controller's instance key to the device's DeviceX key

        if (ERROR_SUCCESS != RegOpenKeyEx(hDevKey, szNewKey, 0, 0, &hKey)) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to open DeviceX subkey; device key(%s)\r\n"
                ), szDevKey));
            goto exit;
        }
        if (0 != wcscmp(szNewKey, REG_KEY_PRIMARY_MASTER)) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Found bad DeviceX subkey(%s) in device's(%s) key; ignoring\r\n"
                ), szNewKey, szDevKey));
            dwIndex -= 1;
            continue;
        }

        // fetch the device's registry value set

        pDskReg = (PDSKREG)LocalAlloc(LPTR, sizeof(DSKREG));
        if (!pDskReg) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to allocate DSK_ registry value set; device key(%s)\r\n"
                ), szNewKey));
            goto exit;
        }
        dwUndo |= IDEINIT_UNDO_DEL_REG_DSK;
        if (!GetDSKRegistryValueSet(hKey, pDskReg)) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to read DSK_ registry value set from registry; device key(%s)\r\n"
                ), szNewKey));
            goto exit;
        }

        // Make a local copy of this value so pDskReg is not continually dereferenced below
        dwDeviceId = pDskReg->dwDeviceId;

        // write the heap address of the port instance to the device's instance key
        pBus->m_pPort[dwDeviceId]->m_pDskReg[0] = pDskReg;

        if (!AtaSetRegistryValue(hKey, REG_VALUE_PORT, (DWORD)pBus->m_pPort[dwDeviceId])) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to write address of port instance to device's(%s) DeviceX subkey(%s)\r\n"
                ), szDevKey, szNewKey));
            goto exit;
        }

        if (!pBus->m_szDevice[dwDeviceId]) {

            // save name of device's full registry key path; when we've finished
            // enumerating the "bus", we'll call ActivateDevice against all of
            // these paths

            pBus->m_szDevice[dwDeviceId] = new TCHAR[wcslen(szDevKey) + wcslen(szNewKey) + 10];
            wcscpy(pBus->m_szDevice[dwDeviceId], szDevKey);
            wcscat(pBus->m_szDevice[dwDeviceId], L"\\");
            wcscat(pBus->m_szDevice[dwDeviceId], szNewKey);

            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!IDE_Init> Enumerated SATA device %s\r\n"
                ), pBus->m_szDevice[dwDeviceId]));
        }

    } // while

    DEBUGMSG(ZONE_INIT, (_T(
        "Atapi!IDE_Init> End of SATA device enumeration\r\n"
        )));

    // initialize enumerated devices

    for (dwDeviceId = 0; dwDeviceId < MAX_SATA_DEVICES_PER_CONTROLLER; dwDeviceId += 1) {
        if (pBus->m_szDevice[dwDeviceId]) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!IDE_Init> Activating IDE/ATA device %s\r\n"
                ), pBus->m_szDevice[dwDeviceId]));
            pBus->m_hDevice[dwDeviceId] = ActivateDeviceEx(pBus->m_szDevice[dwDeviceId], NULL, 0, NULL);
        }
    }

    // Delete ports which do not exist
    for (int i = 0; i < MAX_SATA_DEVICES_PER_CONTROLLER; i++) {
        if (pBus->m_pPort[i] && (pBus->m_pPort[i]->m_fInitialized == FALSE)) {
            delete pBus->m_pPort[i];
        }
    }

    dwUndo &= ~IDEINIT_UNDO_DEL_BUS;
    dwUndo &= ~IDEINIT_UNDO_DEL_PORT_PRI;
    dwUndo &= ~IDEINIT_UNDO_DEL_PORT_SEC;

exit:;

    if (dwUndo & IDEINIT_UNDO_CLS_KEY_ACTIVE) {
        RegCloseKey(hActiveKey);
    }
    if (dwUndo & IDEINIT_UNDO_CLS_KEY_DEVICE) {
        RegCloseKey(hDevKey);
    }
    if (szDevKey) {
        LocalFree(szDevKey);
    }
    if ((NULL != pBus) && (dwUndo & IDEINIT_UNDO_DEL_BUS)) {
        delete pBus;
        pBus = NULL;
    }

    return (DWORD)pBus;
}

/*++

IDE_Deinit
    This function deallocates the associated IDE/ATA controller ("bus") instance.

Parameters:
    dwHandle - pointer to associated bus instance (initially returned by
    IDE_Init)

Return:
    This function always succeeds.

--*/
EXTERN_C
BOOL
IDE_Deinit(
    DWORD dwHandle
    )
{
    CSATABUS *pBus = (CSATABUS *)dwHandle;

    DEBUGCHK(pBus != NULL);
    delete pBus;

    return TRUE;
}

/*++

IDE_Open
    This function is not supported.

Parameters:
    N/A

Return:
    This function always fails.

--*/
EXTERN_C
DWORD
IDE_Open(
    HANDLE dwHandle,
    DWORD dwAccess,
    DWORD dwShareMode
    )
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}


/*++

IDE_Close
    This function is not supported.

Parameters:
    N/A

Return:
    This function always fails.

--*/
EXTERN_C
BOOL
IDE_Close(
    DWORD dwHandle
    )
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

/*++

IDE_IOControl
    This function is not supported.

Parameters:
    N/A

Return:
    This function always fails.

--*/
EXTERN_C
BOOL
IDE_IOControl(
    DWORD dwHandle,
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned,
    PDWORD pOverlapped
    )
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

/*++

DllMain
    This function is the main ATAPI.DLL entry point.

Parameters:
    hInstance - a handle to the dll; this value is the base address of the DLL
    dwReason - the reason for the DLL is being entered
    lpReserved - not used

Return:
    On success, return true.  Otherwise, return false.

--*/
BOOL
WINAPI
DllMain(
    HANDLE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:

        // initialize global data
        g_hInstance = (HINSTANCE)hInstance;
        InitializeCriticalSection(&g_csMain);
        // register debug zones
        RegisterDbgZones((HMODULE)hInstance, &dpCurSettings);
        DisableThreadLibraryCalls((HMODULE)hInstance);
        DEBUGMSG(ZONE_INIT, (_T("ATAPI DLL_PROCESS_ATTACH\r\n")));

        break;

    case DLL_PROCESS_DETACH:

        // deinitialize global data
        DeleteCriticalSection(&g_csMain);
        DEBUGMSG(ZONE_INIT, (TEXT("ATAPI DLL_PROCESS_DETACH\r\n")));

        break;
    }

    return TRUE;
}

