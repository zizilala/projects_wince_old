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
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  memory.c

Abstract:  
   This file implements x86 memory related functions.
 
exported function:
        void x86InitMemory (void);

globals defined:
        x86CacheInfo

globals affected:
        g_pOemGlobal->dwMainMemoryAddress
        g_pOemGlobal->pfnSetMemoryAttributes


Notes: 

--*/

#include <windows.h>
#include <pc.h>
#include <msr.h>
#include <oal.h>

static DWORD g_dwCPUFeatures;
static int g_nMtrrCnt;
static int g_nMtrrInuse;

extern const DWORD g_dwRAMAutoDetectStart;
extern const DWORD g_dwRAMAutoDetectSize;

// -----------------------------------------------------------------------------
//
//  IsDRAM
//
//  Is there DRAM available at the specified location?  This test must be non-
//  destructive... can't alter the contents except temporarily.
//
// -----------------------------------------------------------------------------
static DWORD
IsDRAM(
    DWORD dwMemStartAddr, 
    DWORD dwMaxMemLength
    )
{

#define EXTENSION_CHECK_SIZE 4
#define DRAM_TEST_INTERVAL 0x100000

    DWORD testvect [EXTENSION_CHECK_SIZE] ={0x55aaaa55,0xaa5555aa,12345678,0xf00f4466 };
    DWORD backup   [EXTENSION_CHECK_SIZE];
    DWORD *target;
    BOOL bMismatch= FALSE;
    DWORD length;

    //
    // Make sure we aren't caching this data. That wouldn't be too useful...
    //
    dwMemStartAddr |= 0x20000000;
 
    //
    // Probe each page until we find one that fails
    //
    for (length = 0; (length < dwMaxMemLength) && !bMismatch ; )
    {
        target = (PDWORD) (length + dwMemStartAddr);
        //
        // Save the original data in backup buffer
        //
        memcpy( backup, target, sizeof(testvect) );
        //  
        // Now fill memory from the test vector
        //
        memcpy( target, testvect, sizeof(testvect) );
        //  
        // Now compare memory to the test vector
        //
        if ( memcmp( target, testvect, sizeof(testvect) ) )
            bMismatch = TRUE;
        else
            length += DRAM_TEST_INTERVAL;   // OK, advance our probe pointer        
        //
        // Don't forget to restore the saved data.
        //
        memcpy( target, backup, sizeof(backup) );  
    }
    //
    // Check every 1MB but report 4MB aligned only. All 4MB must be good to pass.
    //
    length &= ~(0x00400000 - 1);
    
    DEBUGMSG(length, (TEXT("OEM Extra DRAM Detected @ base = x%X, size=%d MB \r\n"),
                  dwMemStartAddr, (length >> 20)));
    
    return (length);
}

static void WriteMtrrRegPair (int ixReg, const LARGE_INTEGER *pliPhysBase, const LARGE_INTEGER *pliPhysMask)
{
    DEBUGMSG (1, (L"Setting MTRR reg-pair:%d with 0x%8.8lx%8.8lx, 0x%8.8lx%8.8lx\r\n", ixReg,
        pliPhysBase->HighPart, pliPhysBase->LowPart| MTRR_TypeWriteCombining, 
        pliPhysMask->HighPart, pliPhysMask->LowPart));

// Per Intel MTRR document, we need to disable/enable interrupt, flush cache and TLB, and disable MTRR before/after
// changing MTRR registers. However, since we use this only for non-memory area (video frame buffer) during 
// initialization, both cache and TLB shouldn't have any refernece to them. We'll need to revisit this function
// if the assumption is no longer true.
//
//    pre_mtrr_change();
    NKwrmsr(MSRAddr_MTRRphysBase0 + (2 * ixReg), pliPhysBase->HighPart, pliPhysBase->LowPart | MTRR_TypeWriteCombining);
    NKwrmsr(MSRAddr_MTRRphysMask0 + (2 * ixReg), pliPhysMask->HighPart, pliPhysMask->LowPart);
//    post_mtrr_change();

}

#ifdef DEBUG
static void DumpMTRR (void)
{
    int i;
    DWORD dwHi, dwLo;
    for (i = 0; i < g_nMtrrCnt; i ++) {
        NKrdmsr (MSRAddr_MTRRphysMask0 + 2*i, &dwHi, &dwLo);
        NKDbgPrintfW (L"MTRR PhysMask%d: %8.8lx%8.8lx\r\n", i, dwHi, dwLo);
        NKrdmsr (MSRAddr_MTRRphysBase0 + 2*i, &dwHi, &dwLo);
        NKDbgPrintfW (L"MTRR PhysBase%d: %8.8lx%8.8lx\r\n", i, dwHi, dwLo);
    }
}
#endif


// return the least significant non-zero bit
__inline DWORD LSB (DWORD x)
{
    return x & (0-x);
}

// return the most significant non-zero bit
static DWORD MSB (DWORD x)
{
    DWORD msb = x;
    while (x &= (x-1)) {
        msb = x;
    }
    return msb;
}

//
// CheckExistingMTRR: check if the requested range overlap with existing ranges
//
// returns: 0 - if already exist and is of type PAGE_WRITECOMBINE
//          1 - if not exist and no overlapping with any existing ranges
//         -1 - if overlapped with existing ranges (FAIL)
//
#define MTRR_ALREADY_EXIST  0
#define MTRR_NOT_EXIST      1
#define MTRR_OVERLAPPED     -1
static int CheckExistingMTRR (const LARGE_INTEGER *pliPhysBase, DWORD dwPhysSize)
{
    int i;
    LARGE_INTEGER liReg;

    DEBUGCHK (LSB (dwPhysSize) == dwPhysSize);       // dwPhysSize must be power of 2
    
    // loop through the MTTR register pairs
    for (i = 0; i < g_nMtrrCnt; i ++) {
        NKrdmsr (MSRAddr_MTRRphysMask0 + 2*i, &liReg.HighPart, &liReg.LowPart);
        
        if (liReg.LowPart & MTRRphysMask_ValidMask) {
            // register is in use, check to see if it covers our request
            // NOTE: we're assuming all MTRR ranges are of size 2^^n
            //       and physbase is also 2^^n aligned
            DWORD dwRegSize = LSB (liReg.LowPart & -PAGE_SIZE);
            DWORD dwRegType;
            NKrdmsr (MSRAddr_MTRRphysBase0 + 2*i, &liReg.HighPart, &liReg.LowPart);
            dwRegType = liReg.LowPart & MTRRphysBase_TypeMask;
            liReg.LowPart -= dwRegType;

            if ((pliPhysBase->QuadPart < liReg.QuadPart + dwRegSize) 
                && (pliPhysBase->QuadPart + dwPhysSize >= liReg.QuadPart)) {
                // range overlapped - return MTRR_ALREADY_EXIST if fully contained and of the same type.
                //                  - otherwise return MTRR_OVERLAPPED
                return ((MTRR_TypeWriteCombining == dwRegType)
                        &&(pliPhysBase->QuadPart >= liReg.QuadPart)
                        && (pliPhysBase->QuadPart + dwPhysSize <= liReg.QuadPart + dwRegSize))
                    ?   MTRR_ALREADY_EXIST
                    :   MTRR_OVERLAPPED;
            }
        }
    }
    return MTRR_NOT_EXIST;
}

//------------------------------------------------------------------------------
//
//  PATSetMemoryAttributes
//
//  PAT version of OEMSetMemoryAttributes
//
//------------------------------------------------------------------------------
static BOOL PATSetMemoryAttributes (
    LPVOID pVirtAddr,       // Virtual address of region
    LPVOID pPhysAddrShifted,// PhysicalAddress >> 8 (to support up to 40 bit address)
    DWORD  cbSize,          // Size of the region
    DWORD  dwAttributes     // attributes to be set
    )
{
    return (g_dwCPUFeatures & CPUID_PAT)
        ? NKVirtualSetAttributes (pVirtAddr, cbSize,
                                  x86_PAT_INDEX0,     // Index of first upper PAT entry
                                  x86_PAT_INDEX_MASK, // Mask of all PAT index bits
                                  &dwAttributes)
        : FALSE;

}

//------------------------------------------------------------------------------
//
//  MTRRSetMemoryAttributes
//
//  MTRR version of OEMSetMemoryAttributes
//
//------------------------------------------------------------------------------
static BOOL MTRRSetMemoryAttributes (
    LPVOID pVirtAddr,       // Virtual address of region
    LPVOID pPhysAddrShifted,// PhysicalAddress >> 8 (to support up to 40 bit address)
    DWORD  cbSize,          // Size of the region
    DWORD  dwAttributes     // attributes to be set
    )
{

    // try MTRR if we physical address is know and MTRR is available
    if (g_nMtrrCnt && (PHYSICAL_ADDRESS_UNKNOWN != pPhysAddrShifted)) {

        // MTRR supproted
        LARGE_INTEGER liPhysMask;
        LARGE_INTEGER liPhysBase;
        DWORD cbOrigSize = cbSize;      // save the original size since we need to do it in 2-passes
        DWORD cbAlignedSize, dwAlignedBase;
        int   nRegNeeded = 0, n;

        liPhysBase.QuadPart = ((ULONGLONG) pPhysAddrShifted) << 8;   // real physical address

        // first: calculate number of register pair needed
        // count MTRR registers needed
        for ( ; cbSize; cbSize -= cbAlignedSize, liPhysBase.QuadPart += cbAlignedSize) {
            cbAlignedSize = MSB(cbSize);            // maximun alignment that can be satisfied
            if (liPhysBase.LowPart) {
                dwAlignedBase = LSB (liPhysBase.LowPart); // least alignment required for base
                if (cbAlignedSize > dwAlignedBase) {
                    cbAlignedSize = dwAlignedBase;
                }
            }
            n = CheckExistingMTRR (&liPhysBase, cbAlignedSize);

            if (MTRR_OVERLAPPED == n) {
                // do not support overpped with existing ranges
                DEBUGMSG (1, (L"OEMSetMemoryAttributes: MTRR existing Range overallped with the new request\r\n"));
                return FALSE;
            }
            nRegNeeded += n;    // note: MTRR_ALREADY_EXIST == 0 and MTRR_NOT_EXIST == 1, so we can use this math
        }   

        DEBUGMSG (1, (L"OEMSetMemoryAttributes: nRegNeeded = %d\r\n", nRegNeeded));
        
        // do we have enough registers left?
        if (g_nMtrrInuse + nRegNeeded > g_nMtrrCnt) {
            // running out of MTRR registers
            DEBUGMSG (1, (L"OEMSetMemoryAttributes: Run out of MTRR registers\r\n"));
            return FALSE;
        }

        if (!nRegNeeded) {
            // no register needed, the range already exist with Write-Combine
            return TRUE;
        }

        // registers available, start allocating
        liPhysBase.QuadPart = ((ULONGLONG) pPhysAddrShifted) << 8;   // real physical address
        liPhysMask.HighPart = 0xF;          // size can never be > 4G, thus the high part of mask shold always be 0xf
        
        // update MTRR registers
        for (n = 0, cbSize = cbOrigSize; cbSize; cbSize -= cbAlignedSize, liPhysBase.QuadPart += cbAlignedSize) {
            cbAlignedSize = MSB(cbSize);    // maximun alignment that can be satisfied
            if (liPhysBase.LowPart) {
                dwAlignedBase = LSB (liPhysBase.LowPart); // least alignment required for base
                if (cbAlignedSize > dwAlignedBase) {
                    cbAlignedSize = dwAlignedBase;
                }
            }

            if (MTRR_ALREADY_EXIST != CheckExistingMTRR (&liPhysBase, cbAlignedSize)) {
                // find a free MTRR register
                do {
                    DWORD dummy, dwPhysMask;
                    NKrdmsr (MSRAddr_MTRRphysMask0 + 2*n, &dummy, &dwPhysMask);
                    if (!(MTRRphysMask_ValidMask & dwPhysMask)) {
                        // found
                        break;
                    }
                    n ++;
                } while (n < g_nMtrrCnt);

                DEBUGCHK (n < g_nMtrrCnt);
                liPhysMask.LowPart = (0-cbAlignedSize) | MTRRphysMask_ValidMask;
                WriteMtrrRegPair (n ++, &liPhysBase, &liPhysMask);
            }
            
        }

        g_nMtrrInuse += nRegNeeded;
        return TRUE;
    }

    return FALSE;
}

//------------------------------------------------------------------------------
//
//  OEMSetMemoryAttributes
//
//  OEM function to change memory attributes that isn't supported by kernel.
//  Current implementaion only supports PAGE_WRITECOMBINE.
//
//  This function first try to use PAT, and then try MTRR if PAT isn't available.
//
//------------------------------------------------------------------------------
static BOOL OEMSetMemoryAttributes (
    LPVOID pVirtAddr,       // Virtual address of region
    LPVOID pPhysAddrShifted,// PhysicalAddress >> 8 (to support up to 40 bit address)
    DWORD  cbSize,          // Size of the region
    DWORD  dwAttributes     // attributes to be set
    )
{
    if (PAGE_WRITECOMBINE ^ dwAttributes) {
        DEBUGMSG (1, (L"OEMSetMemoryAttributes: Only PAGE_WRITECOMBINE is supported\r\n"));
        return FALSE;
    }

    // I decided to try MTRR first since MTRR show better perf improvement on my CEPC (64M vs. 48M).
    // Depending on platform, you might want to reverse the order if PAT has a better perf improvement.
    return MTRRSetMemoryAttributes (pVirtAddr, pPhysAddrShifted, cbSize, dwAttributes)
        || PATSetMemoryAttributes (pVirtAddr, pPhysAddrShifted, cbSize, dwAttributes);
}

//------------------------------------------------------------------------------
//
//  InitializePATAndMTRR
//
//  Initialize memory type settings in the PAT (page attribute table).  The PAT
//  is a single 64-bit register which is a table of memory types.  Page table
//  entries index into the PAT via three bits: PCD, PWT, and PATi.  Each page 
//  uses the memory type stored in the PAT in that location.  By default the 
//  first four entries in the PAT (PATi=0) are initialized by the CPU on reboot 
//  to the legacy values that are implied by PCD and PWT for backward-
//  compatibility.  The remaining four PAT entries (PATi=1) may be programmed 
//  to any type desired.
//
//  In the i486 OAL we program the first upper PAT entry (PATi=1, PCD=0, PWT=0)
//  to be write-combined.  The remaining three upper PAT entries are currently
//  left as their default values and unused in the page table.
//
//  The PAT index is set on page table entries by OEMVirtualProtect, based on
//  the PAGE_NOCACHE, PAGE_x86_WRITETHRU, and PAGE_WRITECOMBINE flags passed
//  down by VirtualProtect.
//
//------------------------------------------------------------------------------
static void InitializePATAndMTRR (void)
{
    DWORD dwHighPart, dwLowPart;

    DEBUGMSG (1, (L"g_dwCPUFeatures = %8.8lx\r\n", g_dwCPUFeatures));

    // can't do anything if MSR is not support
    if (!(g_dwCPUFeatures & CPUID_MSR)) {
        return;
    }
    
    // Is PAT supported?
    if (g_dwCPUFeatures & CPUID_PAT) {
        // PAT is supported
        
        // Read the current PAT values
        NKrdmsr(MSRAddr_PAT, &dwHighPart, &dwLowPart);

        // MUST leave dwLowPart as-is for backward compatibility!
        // Set the first entry in the high part to be write-combining
        dwHighPart = (dwHighPart & ~PAT_Entry0Mask) | PAT_TypeWriteCombining;

        // Write the new PAT values
        NKwrmsr(MSRAddr_PAT, dwHighPart, dwLowPart);
        
    }

    if (g_dwCPUFeatures & CPUID_MTRR) {
        // MTRR is supported
        DWORD dummy, dwMTRR;

        // read the MTRRcap register and check if WC is supproted
        NKrdmsr(MSRAddr_MTRRcap, &dummy, &dwMTRR);
        if (dwMTRR & MTRRcap_WriteCombineMask) {

            int i;
            // WC is supported, continue to initialize the MTRR registers/globals
            g_nMtrrCnt = (int) (dwMTRR & MTRRcap_VarRangeRegisterCountMask);
            DEBUGMSG (1, (L"g_nMtrrCnt = %d\r\n", g_nMtrrCnt));
            DEBUGCHK (g_nMtrrCnt);
#ifdef DEBUG
            DumpMTRR ();
#endif
            // figure out the vairable entries that are already in use
            for (i = 0; i < g_nMtrrCnt; i ++) {
                NKrdmsr (MSRAddr_MTRRphysMask0 + 2*g_nMtrrInuse, &dummy, &dwMTRR);
                DEBUGMSG (1, (L"PhysMask%d: %8.8lx%8.8lx\r\n", i, dummy, dwMTRR));
                if (dwMTRR & MTRRphysMask_ValidMask)
                    g_nMtrrInuse ++;
            }
            DEBUGMSG (1, (L"g_nMtrrInuse = %d\r\n", g_nMtrrInuse));

            // enable MTRR if it's not already enabled
            NKrdmsr (MSRAddr_MTRRdefType, &dummy, &dwMTRR);
            if (!(dwMTRR & MTRRdefType_EnableMask)) {
                NKwrmsr (MSRAddr_MTRRdefType, 0, dwMTRR | MTRRdefType_EnableMask);
            }

        }
    }

    // initialize the pfnNKSetMemoryAttributes function pointer
    pfnOEMSetMemoryAttributes = OEMSetMemoryAttributes;
    
}

void x86InitMemory (void)
{
    // Use the PAT/MTRR to add write-combining support to VirtualProtect
    g_dwCPUFeatures = IdentifyCpu();
    InitializePATAndMTRR ();

    //
    // Detect extra RAM.
    //
    if (MainMemoryEndAddress == g_dwRAMAutoDetectStart)  {
       MainMemoryEndAddress += IsDRAM (MainMemoryEndAddress, g_dwRAMAutoDetectSize);
    }
}

void OEMCacheRangeFlush (LPVOID pAddr, DWORD dwLength, DWORD dwFlags)
{
    // cache flush?
    if (dwFlags & (CACHE_SYNC_WRITEBACK | CACHE_SYNC_DISCARD)) {
        // just perform an uncached access
        _asm {  mov eax, dword ptr ds:0xa0000000 }
    }

    if (dwFlags & CACHE_SYNC_FLUSH_TLB) {
        // flush TLB
        _asm {
            mov     eax, cr3
            mov     cr3, eax
        }
    }
}

VOID* OALPAtoVA(UINT32 pa, BOOL cached)
{
    return (VOID *) (pa | (cached? 0x80000000 : 0xa0000000));
}

UINT32 OALVAtoPA(VOID *va)
{
    return (DWORD) va & ~ 0xa0000000;
}


