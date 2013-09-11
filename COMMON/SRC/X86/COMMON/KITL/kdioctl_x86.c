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

//
// KD Ioctl routines
//

#include <windows.h>
#include <oal.h>

//+++
// Hardware Debug Register support 
//
__inline void CLEARDR0()
{
    __asm { 
        push eax
            xor eax,eax
            mov dr0, eax
            mov eax, dr7
            and eax,0xFFFFFFFC;
        mov dr7, eax
            pop eax
    }
}

__inline void CLEARDR1()
{
    __asm { 
        push eax
            xor eax,eax
            mov dr1, eax
            mov eax, dr7
            and eax,0xFFFFFFF3;
        mov dr7, eax
            pop eax
    }
}

__inline void CLEARDR2()
{
    __asm { 
        push eax
            xor eax,eax
            mov dr2, eax
            mov eax, dr7
            and eax,0xFFFFFFCF;
        mov dr7, eax
            pop eax
    }
}

__inline void CLEARDR3()
{
    __asm { 
        push eax
            xor eax,eax
            mov dr3, eax
            mov eax, dr7
            and eax,0xFFFFFF3F;
        mov dr7, eax
            pop eax
    }
}


static int SetDataBP(ULONG ulAddress)
{
    ULONG ulHandle = 0;
    ULONG ulFlags = 0;
    __asm {
            mov eax,dr3
            cmp eax, 0
            jne  checkdr2
            mov eax,ulAddress
            mov dr3, eax
            mov eax, dr7
            or   eax, 0xD00003C0 // 1101 0000 0000 0000 0000 0011 1100 0000
            mov ulFlags, eax
            mov dr7, eax
            mov ulHandle, 1
            jmp  done
         checkdr2:       
            mov eax,dr2
            cmp eax, 0
            jne  checkdr1
            mov eax, ulAddress
            mov dr2, eax
            mov eax, dr7
            or   eax, 0x0D000330 // 0000 1101 0000 0000 0000 0011 0011 0000
            mov ulFlags, eax
            mov dr7, eax
            mov ulHandle, 2
            jmp  done
         checkdr1:
            mov eax,dr1
            cmp eax, 0
            jne  checkdr0
            mov eax, ulAddress
            mov dr1, eax
            mov eax, dr7
            or   eax, 0x00D0030C // 0000 0000 1101 0000 0000 0011 0000 1100
            mov ulFlags, eax
            mov dr7, eax
            mov ulHandle, 3
            jmp  done
         checkdr0:
            mov eax,dr0
            cmp eax, 0
            jne  done
            mov eax, ulAddress
            mov dr0, eax
            mov eax, dr7
            or   eax, 0x000D0303 // 0000 0000 0000 1101 0000 0011 0000 00011
            mov ulFlags, eax
            mov dr7, eax
            mov ulHandle, 4
            jmp  done
    }
    done:
    return ulHandle;
}

BOOL OEMKDIoControl( DWORD dwIoControlCode, LPVOID lpBuf, DWORD nBufSize)
{
    switch (dwIoControlCode) {
    case KD_IOCTL_INIT:
        CLEARDR0();
        CLEARDR1();
        CLEARDR2();
        CLEARDR3();
        return TRUE;
    case KD_IOCTL_SET_CBP:
    case KD_IOCTL_CLEAR_CBP:
    case KD_IOCTL_ENUM_CBP:
        break;
    case KD_IOCTL_QUERY_CBP:
        ((PKD_BPINFO)lpBuf)->ulCount = 0;
        return TRUE;
    case KD_IOCTL_SET_DBP:
        if (((PKD_BPINFO)lpBuf)->ulHandle = SetDataBP( ((PKD_BPINFO)lpBuf)->ulAddress))
            return TRUE;
        break;
    case KD_IOCTL_CLEAR_DBP:
        if (((PKD_BPINFO)lpBuf)->ulHandle == 1) {
            CLEARDR3();
            return TRUE;
        }
        if (((PKD_BPINFO)lpBuf)->ulHandle == 2) {
            CLEARDR2();
            return TRUE;
        }
        if (((PKD_BPINFO)lpBuf)->ulHandle == 3) {
            CLEARDR1();
            return TRUE;
        }
        if (((PKD_BPINFO)lpBuf)->ulHandle == 4) {
            CLEARDR0();
            return TRUE;
        }
        break;
    case KD_IOCTL_QUERY_DBP:
        ((PKD_BPINFO)lpBuf)->ulCount = 4;
        return TRUE;
    case KD_IOCTL_ENUM_DBP:
    case KD_IOCTL_MAP_EXCEPTION:
        // The SINGLE_STEP exception is overloaded in Platform Builder with HW data breakpoints
        if (((PKD_EXCEPTION_INFO)lpBuf)->ulExceptionCode == 0) {
            ((PKD_EXCEPTION_INFO)lpBuf)->ulExceptionCode = STATUS_SINGLE_STEP;
            return TRUE;
        }
        break;      
    case KD_IOCTL_RESET:
    default:
        break;
    }
    return FALSE;
}

