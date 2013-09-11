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
#include <PCIReg.h>
#include <ddkmacro.h>
#include <oal.h>

// -------------------------------------
UCHAR   ucPCIConfigMechanism = 0;

UCHAR   ucPCIMaxDeviceNumber = PCI_MAX_DEVICES;

#define PCI_TYPE1_ADDRESS       0x0CF8
#define PCI_TYPE1_DATA          0x0CFC

// -------------------------------------
typedef struct  _TYPE1_PCI_ADDRESS {
    union {
        struct {
            ULONG   Reserved:2;
            ULONG   Register:6;
            ULONG   Function:3;
            ULONG   Device:5;
            ULONG   Bus:8;
            ULONG   Reserved2:7;
            ULONG   ConfigurationAccess:1;
        } bits;
        ULONG   AsULONG;
    } u;
} TYPE1_PCI_ADDRESS, *PTYPE1_PCI_ADDRESS;

#define PCI_TYPE2_FORWARD       0x0CFA
#define PCI_TYPE2_CSE           0x0CF8

typedef struct  _TYPE2_PCI_CSE {
    union {
        struct {
            UCHAR   SCE:1;
            UCHAR   Function:3;
            UCHAR   Key:4;
        } bits;
        UCHAR   AsUCHAR;
    } u;
} TYPE2_PCI_CSE, *PTYPE2_PCI_CSE;

typedef struct  _TYPE2_PCI_ADDRESS {
    union {
        struct {
            USHORT  Reserved:2;
            USHORT  Register:6;
            USHORT  Device:4;
            USHORT  PCIConfigSpace:4;   // Must be 0xC
        } bits;
        USHORT  AsUSHORT;
    } u;
} TYPE2_PCI_ADDRESS, *PTYPE2_PCI_ADDRESS;

#define	HW_ACCESS_SIZE  (sizeof _inpd ((USHORT) PCI_TYPE1_DATA))	// 32-bit access only
#define	HW_ODD_BITS     (HW_ACCESS_SIZE - 1U)
#define	BITS_PER_BYTE   8U
#define	BYTE_MASK       0xffU

// -----------------------------------------------------------------------------
// Type 1 Configuration:
// Given the bus number, device number, and function number, read or write
// based on the "fWriting" flag "Length" bytes at "ByteOffset" in PCI space.
// Note that on this platform the PCI space exists in X86 IO space and can
// only be accessed as long-words. This routine is responsible for managing
// accesses that are not on long-word boundaries.
// -----------------------------------------------------------------------------
ULONG
PCI_Type1_Configuration(
    IN ULONG     BusNumber,
    IN ULONG     DeviceNumber,
    IN ULONG     FunctionNumber,
    IN OUT PVOID Buffer,
    IN ULONG     ByteOffset,
    IN ULONG     Length,        // Length in bytes to read or write
    IN BOOL      fWriting       // False for read, true for write
    )
{
    TYPE1_PCI_ADDRESS type1Address;
    ULONG             longOffset;	// Hardware can only access longs
    ULONG             value;
    int               bitPosition;
    union {
        ULONG *Long;	// Aligned accesses
        UCHAR *Char;	// Unaligned
    }                 ptr;

    ptr.Long = Buffer;
    longOffset = ByteOffset / HW_ACCESS_SIZE;

    // Construct a PCI address from the various fields.
    type1Address.u.AsULONG = 0UL;
    type1Address.u.bits.ConfigurationAccess = 1UL;
    type1Address.u.bits.Bus = BusNumber;
    type1Address.u.bits.Device = DeviceNumber;
    type1Address.u.bits.Function = FunctionNumber;
    type1Address.u.bits.Register = longOffset;

    //
    // If the offset is not on a long-word boundary,
    // operate on bytes until it is.
    //
    if (ByteOffset & HW_ODD_BITS)
    {
        // Set the PCI address latch in x86 IO space.
        _outpd ((USHORT) PCI_TYPE1_ADDRESS, type1Address.u.AsULONG);

        // Read the long-word from this aligned address.
        value = _inpd ((USHORT) PCI_TYPE1_DATA);

        if (!fWriting)
            value >>= (ByteOffset & HW_ODD_BITS) * BITS_PER_BYTE;	// Discard odd bytes read.

        // Operate on the unaligned bytes.
        while ((ByteOffset & HW_ODD_BITS) && (Length > 0UL))
        {
            if (fWriting)
            {
                bitPosition = (int)(ByteOffset & HW_ODD_BITS) * BITS_PER_BYTE;
                value &= ~(BYTE_MASK << bitPosition);
                value |= *ptr.Char++ << bitPosition;
            } else {
                *ptr.Char++ = (UCHAR)value;
                value >>= BITS_PER_BYTE;
            }
            ++ByteOffset;
            --Length;
        }
        if (fWriting)
        {
	    // Now write back the full long-word that has the odd bytes.
            _outpd ((USHORT) PCI_TYPE1_DATA, value);
        }
        ++longOffset;
    }

    //
    // Long alignment has been established. Operate on longs now.
    //
    while (Length >= sizeof *ptr.Long)
    {
        // Set the PCI address latch in x86 IO space.
        type1Address.u.bits.Register = longOffset++;
        _outpd ((USHORT) PCI_TYPE1_ADDRESS, type1Address.u.AsULONG);
        if (fWriting)
        {
            _outpd ((USHORT) PCI_TYPE1_DATA, *ptr.Long++);
        } else {
            *ptr.Long++ = _inpd ((USHORT) PCI_TYPE1_DATA);
        }
        Length -= sizeof *ptr.Long;
    }

    //
    // Operate on remaining bytes if the length was not long aligned.
    //
    if (Length > 0UL)
    {
        // Set the PCI address latch in x86 IO space.
        type1Address.u.bits.Register = longOffset;
        _outpd ((USHORT) PCI_TYPE1_ADDRESS, type1Address.u.AsULONG);

        // Read the aligned long-word to operate on the odd bytes.
        value = _inpd ((USHORT) PCI_TYPE1_DATA);

        for (ByteOffset = 0UL; Length > 0UL; ++ByteOffset)
        {
            if (fWriting)
            {
                bitPosition = (int)(ByteOffset & HW_ODD_BITS) * BITS_PER_BYTE;
                value &= ~(BYTE_MASK << bitPosition);
                value |= (ULONG)(*ptr.Char++ << bitPosition);
            } else {
                *ptr.Char++ = (UCHAR)(value & BYTE_MASK);
                value >>= BITS_PER_BYTE;
            }
            Length -= sizeof *ptr.Char;
        }
        if (fWriting)
        {
	    // Now write back the full long-word that has the remaining bytes.
            _outpd ((USHORT) PCI_TYPE1_DATA, value);
        }
    }

    return ((ULONG)(ptr.Char - (UCHAR *)Buffer));
}


// -----------------------------------------------------------------------------
// Type 2 Configuration:
// Given the bus number, device number, and function number, read or write
// based on the "fWriting" flag "Length" bytes at "ByteOffset" in PCI space.
// Note that on this platform the PCI space exists in X86 IO space and can
// only be accessed as long-words. This routine is responsible for managing
// accesses that are not on long-word boundaries.
// -----------------------------------------------------------------------------
ULONG
PCI_Type2_Configuration (
    IN ULONG     BusNumber,
    IN ULONG     DeviceNumber,
    IN ULONG     FunctionNumber,
    IN OUT PVOID Buffer,
    IN ULONG     ByteOffset,
    IN ULONG     Length,        // Length in bytes to read or write
    IN BOOL      fWriting       // False for read, true for write
    )
{
    TYPE2_PCI_ADDRESS type2Address;
    TYPE2_PCI_CSE     type2CSE;
    USHORT            longOffset;	// Hardware can only access longs
    ULONG             value;
    int               bitPosition;
    union {
        ULONG *Long;	// Aligned accesses
        UCHAR *Char;	// Unaligned
    }                 ptr;

    ptr.Long = Buffer;
    longOffset = (USHORT)(ByteOffset / HW_ACCESS_SIZE);

    // Construct a PCI address from the various fields.
    type2CSE.u.bits.Key = type2CSE.u.bits.SCE = 0xffU;
    type2CSE.u.bits.Function = (UCHAR)FunctionNumber;

    type2Address.u.AsUSHORT = 0U;
    type2Address.u.bits.PCIConfigSpace = 0xcU;
    type2Address.u.bits.Device = (USHORT)DeviceNumber;
    type2Address.u.bits.Register = longOffset;

    _outp ((USHORT) PCI_TYPE2_FORWARD, (UCHAR) BusNumber);
    _outp ((USHORT) PCI_TYPE2_CSE, type2CSE.u.AsUCHAR);

    //
    // If the offset is not on a long-word boundary,
    // operate on bytes until it is.
    //
    if (ByteOffset & HW_ODD_BITS)
    {
        // Read the long-word from this aligned address.
        value = _inpd (type2Address.u.AsUSHORT);

        if (!fWriting)
            value >>= (ByteOffset & HW_ODD_BITS) * BITS_PER_BYTE;	// Discard odd bytes read.

        // Operate on the unaligned bytes.
        while ((ByteOffset & HW_ODD_BITS) && (Length > 0UL))
        {
            if (fWriting)
            {
                bitPosition = (int)(ByteOffset & HW_ODD_BITS) * BITS_PER_BYTE;
                value &= ~(BYTE_MASK << bitPosition);
                value |= *ptr.Char++ << bitPosition;
            } else {
                *ptr.Char++ = (UCHAR)value;
                value >>= BITS_PER_BYTE;
            }
            ++ByteOffset;
            --Length;
        }
        if (fWriting)
        {
	    // Now write back the full long-word that has the odd bytes.
          _outpd (type2Address.u.AsUSHORT, value);
        }
        ++longOffset;
    }

    //
    // Long alignment has been established. Operate on longs now.
    //
    while (Length >= sizeof *ptr.Long)
    {
        type2Address.u.bits.Register = longOffset++;
        if (fWriting)
        {
            _outpd (type2Address.u.AsUSHORT, *ptr.Long++);
        } else {
            *ptr.Long++ = _inpd (type2Address.u.AsUSHORT);
        }
        Length -= sizeof *ptr.Long;
    }

    //
    // Operate on remaining bytes if the length was not long aligned.
    //
    if (Length > 0UL)
    {
        // Read the aligned long-word to operate on the odd bytes.
        type2Address.u.bits.Register = longOffset;
        value = _inpd (type2Address.u.AsUSHORT);

        for (ByteOffset = 0UL; Length > 0UL; ++ByteOffset)
        {
            if (fWriting)
            {
                bitPosition = (int)(ByteOffset & HW_ODD_BITS) * BITS_PER_BYTE;
                value &= ~(BYTE_MASK << bitPosition);
                value |= (ULONG)(*ptr.Char++ << bitPosition);
            } else {
                *ptr.Char++ = (UCHAR)(value & BYTE_MASK);
                value >>= BITS_PER_BYTE;
            }
            Length -= sizeof *ptr.Char;
        }
        if (fWriting)
        {
	    // Now write back the full long-word that has the remaining bytes.
            _outpd (type2Address.u.AsUSHORT, value);
        }
    }

    _outp ((USHORT)PCI_TYPE2_CSE, 0);
    _outp ((USHORT)PCI_TYPE2_FORWARD, 0);

    return ((ULONG)(ptr.Char - (UCHAR *)Buffer));
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
ULONG
PCIReadBusData(
              IN ULONG BusNumber,
              IN ULONG DeviceNumber,
              IN ULONG FunctionNumber,
              OUT PVOID Buffer,
              IN ULONG Offset,
              IN ULONG Length
              )
{
    BOOL bIntEnable = INTERRUPTS_ENABLE (FALSE);
    switch (ucPCIConfigMechanism) {
    case 1:
        Length = PCI_Type1_Configuration (BusNumber,
            DeviceNumber, FunctionNumber, Buffer, Offset, Length, FALSE);
        break;
    case 2:
        Length = PCI_Type2_Configuration (BusNumber,
            DeviceNumber, FunctionNumber, Buffer, Offset, Length, FALSE);
        break;
    default:
        Length = 0;
    }
    INTERRUPTS_ENABLE(bIntEnable);

    return (Length);
}


ULONG
PCIWriteBusData(
              IN ULONG BusNumber,
              IN ULONG DeviceNumber,
              IN ULONG FunctionNumber,
              IN PVOID Buffer,
              IN ULONG Offset,
              IN ULONG Length
              )
{
    BOOL bIntEnable = INTERRUPTS_ENABLE (FALSE);
    switch (ucPCIConfigMechanism) {
    case 1:
        Length = PCI_Type1_Configuration (BusNumber,
            DeviceNumber, FunctionNumber, Buffer, Offset, Length, TRUE);
        break;
    case 2:
        Length = PCI_Type2_Configuration (BusNumber,
            DeviceNumber, FunctionNumber, Buffer, Offset, Length, TRUE);
        break;
    default:
        Length = 0;
    }
    INTERRUPTS_ENABLE(bIntEnable);

    return (Length);
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
ULONG
PCIGetBusDataByOffset(
                     IN ULONG BusNumber,
                     IN ULONG SlotNumber,
                     IN PVOID Buffer,
                     IN ULONG Offset,
                     IN ULONG Length
                     )
{
    PCI_SLOT_NUMBER     slotNumber;
    
    slotNumber.u.AsULONG = SlotNumber;

    // Since we turn interrupts off in PCIReadBusData/PCIWriteBusData, 
    // we don't need to use CS to serialize the call.

    return (slotNumber.u.bits.DeviceNumber >= ucPCIMaxDeviceNumber)
        ? 0
        : PCIReadBusData (BusNumber, slotNumber.u.bits.DeviceNumber,
                          slotNumber.u.bits.FunctionNumber, Buffer, Offset, Length);
}



// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
ULONG
PCISetBusDataByOffset(
                     IN ULONG BusNumber,
                     IN ULONG SlotNumber,
                     IN PVOID Buffer,
                     IN ULONG Offset,
                     IN ULONG Length
                     )
{
    PCI_SLOT_NUMBER     slotNumber;

    slotNumber.u.AsULONG = SlotNumber;

    // Since we turn interrupts off in PCIReadBusData/PCIWriteBusData, 
    // we don't need to use CS to serialize the call.

    return (slotNumber.u.bits.DeviceNumber >= ucPCIMaxDeviceNumber)
        ? 0
        : PCIWriteBusData (BusNumber, slotNumber.u.bits.DeviceNumber,
                          slotNumber.u.bits.FunctionNumber, Buffer, Offset, Length);
}

BOOL PCIInitConfigMechanism (UCHAR ucConfigMechanism)
{
    ucPCIConfigMechanism = ucConfigMechanism;

    if (ucPCIConfigMechanism == 2) {
        ucPCIMaxDeviceNumber = 16;
    }

    return 0 != ucPCIConfigMechanism;
}

