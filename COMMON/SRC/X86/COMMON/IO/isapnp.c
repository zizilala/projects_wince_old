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
#include <ceddk.h>
#include <pc.h>
#include <oal.h>


int             __cdecl _inp  (unsigned short);
unsigned short  __cdecl _inpw (unsigned short);
unsigned long   __cdecl _inpd (unsigned short);

int             __cdecl _outp (unsigned short, int);
unsigned short  __cdecl _outpw(unsigned short, unsigned short);
unsigned long   __cdecl _outpd(unsigned short, unsigned long);


//
// Plug and Play Card Control Registers
//

#define SET_READ_DATA_PORT          0x00
#define SERIAL_ISOLATION_PORT       0x01
#define CONFIG_CONTROL_PORT         0x02
#define WAKE_CSN_PORT               0x03
#define SET_CSN_PORT                0x06

//
// Config Control command
//

#define CONTROL_WAIT_FOR_KEY        0x02
#define CONTROL_RESET_CSN           0x04

#define NUMBER_CARD_ID_BYTES        9
#define NUMBER_CARD_ID_BITS         (NUMBER_CARD_ID_BYTES * 8)
#define CHECKSUMED_BITS             64
#define LFSR_SEED                   0x6A
#define ISOLATION_TEST_BYTE_1       0x55
#define ISOLATION_TEST_BYTE_2       0xAA

#define PNP_ISA_IO_BASE             ( (PUCHAR)0 )
#define PNP_ADDRESS_PORT            0x279
#define PNP_WRITE_DATA_PORT         0xA79
#define PNP_READ_DATA_PORT          0x203

#define pnpWriteAddress(data)       _outp ((USHORT) PNP_ADDRESS_PORT, (UCHAR)(data))
#define pnpWriteData(data)          _outp ((USHORT) PNP_WRITE_DATA_PORT, (UCHAR)(data))
#define pnpReadData()               _inp  ((USHORT) PNP_READ_DATA_PORT)

CRITICAL_SECTION    g_csISAConfig;

BOOL                g_bISAInitialized;
USHORT              g_usISAReadPort;
UCHAR               g_ucISANumberCSNs;


UCHAR PnPReadRegister       (PUCHAR pIOSpace, UCHAR ucRegNo);
void  PnPWriteRegister      (PUCHAR pIOSpace, UCHAR ucRegNo, UCHAR ucValue);
void  PnPSendInitiationKey  (PUCHAR pIOSpace);
void  PnPSetWaitForKey      (PUCHAR pIOSpace);
void  PnPWake               (PUCHAR pIOSpace, UCHAR ucCSN);
void  PnPReadSerialId       (PUCHAR pIOSpace, PUCHAR ucSerialId);
int   PnPReadResourceData   (PUCHAR pIOSpace, PUCHAR ucResourceData, int iDataSize);

int   PnPGetLogicalDeviceInfo(PUCHAR ucResourceData, int iDataSize, 
                              PISA_PNP_LOGICAL_DEVICE_INFO pDeviceInfo);


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VOID
PnPIsolateCards (VOID)
{
    USHORT j;
    UCHAR  cardId[NUMBER_CARD_ID_BYTES];
    UCHAR  bit, bit7, checksum, byte1, byte2;
    ULONG  csn = 0;

    //
    // First send Initiation Key to all the PNP ISA cards to enable PnP auto-config
    // ports and put all cards in configuration mode.
    //

    PnPSendInitiationKey(NULL);

    //
    // Reset all Pnp ISA cards' CSN to 0 and return to wait-for-key state
    //

    pnpWriteAddress (CONFIG_CONTROL_PORT);
    pnpWriteData (CONTROL_WAIT_FOR_KEY | CONTROL_RESET_CSN);

    //
    // Delay 2 msec for cards to load initial configuration state.
    //

    NKSleep(2);

    //
    // Put cards into configuration mode to ready isolation process.
    // The hardware on each PnP Isa card expects 72 pairs of I/O read
    // access to the read data port.
    //

    PnPSendInitiationKey(NULL);

    //
    // Starting Pnp Isa card isolation process.
    //

    //
    // Send WAKE[CSN=0] to force all cards without CSN into isolation
    // state to set READ DATA PORT.
    //

    pnpWriteAddress(WAKE_CSN_PORT);
    pnpWriteData(0);

    //
    // Set read data port to current testing value.
    //

    pnpWriteAddress(SET_READ_DATA_PORT);
    pnpWriteData((UCHAR)(PNP_READ_DATA_PORT >> 2));

    //
    // Isolate one PnP ISA card until fail
    //

    while ( TRUE ) {

        //
        // Read serial isolation port to cause PnP cards in the isolation
        // state to compare one bit of the boards ID.
        //

        pnpWriteAddress(SERIAL_ISOLATION_PORT);

        //
        // We need to delay 1 msec prior to starting the first pair of isolation
        // reads and must wait 250usec between each subsequent pair of isolation
        // reads.  This delay gives the ISA cards time to access information from
        // possible very slow storage device.
        //

        NKSleep(1);

        memset(cardId, 0, NUMBER_CARD_ID_BYTES);
        checksum = LFSR_SEED;
        for ( j = 0; j < NUMBER_CARD_ID_BITS; j++ ) {

            //
            // Read card id bit by bit
            //

            byte1 = pnpReadData();
            byte2 = pnpReadData();
            bit = (byte1 == ISOLATION_TEST_BYTE_1) && (byte2 == ISOLATION_TEST_BYTE_2);
            cardId[j / 8] |= bit << (j % 8);
            if ( j < CHECKSUMED_BITS ) {

                //
                // Calculate checksum and only do it for the first 64 bits
                //

                bit7 = (((checksum & 2) >> 1) ^ (checksum & 1) ^ (bit)) << 7;
                checksum = (checksum >> 1) | bit7;
            }
            NKSleep(1);
        }

        //
        // Verify the card id we read is legitimate
        // First make sure checksum is valid.  Note zero checksum is considered valid.
        //

        if ( cardId[8] == 0 || checksum == cardId[8] ) {

            //
            // Next make sure cardId is not zero
            //

            byte1 = 0;
            for ( j = 0; j < NUMBER_CARD_ID_BYTES; j++ ) {
                byte1 |= cardId[j];
            }
            if ( byte1 != 0 ) {

                //
                // Make sure the vender EISA ID bytes are nonzero
                //

                if ( (cardId[0] & 0x7f) != 0 && cardId[1] != 0 ) {

                    pnpWriteAddress(SET_CSN_PORT);

                    pnpWriteData(++csn);

                    //
                    // Do Wake[CSN] command to put the newly isolated card to
                    // sleep state and other un-isolated cards to isolation
                    // state.
                    //

                    pnpWriteAddress(WAKE_CSN_PORT);
                    pnpWriteData(0);
                    continue;     // ... to isolate more cards ...
                }
            }
        }
        break;                // could not isolate more cards ...
    }

    //
    // Finaly put all cards into wait for key state.
    //

    pnpWriteAddress(CONFIG_CONTROL_PORT);
    pnpWriteData(CONTROL_WAIT_FOR_KEY);
    g_ucISANumberCSNs = (UCHAR) csn;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
VOID
ISAInitBusInfo()
{
    if ( g_bISAInitialized ) {
        return;
    }

    g_bISAInitialized = TRUE;

    InitializeCriticalSection(&g_csISAConfig);

    PnPIsolateCards();
    RETAILMSG(1, (TEXT("PnP ISA InitBusInfo : %d card(s) found\r\n"), g_ucISANumberCSNs));

    g_usISAReadPort = 0x203;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ULONG
ISAGetBusDataByOffset(
                     IN ULONG BusNumber,
                     IN ULONG SlotNumber,
                     IN PVOID Buffer,
                     IN ULONG Offset,
                     IN ULONG Length
                     )
{
    UCHAR               ucCSNumber, ucLogicalDevice;
    UCHAR               ucSerialID[9];
    UCHAR               ucResourceData[2048];
    int                 iResourceLength;
    ULONG               ulReturn;

    if ( g_bISAInitialized == FALSE )
        ISAInitBusInfo();

    if ( g_ucISANumberCSNs == 0 || g_ucISANumberCSNs == ~0U ) {
        return (0);
    }

    ucCSNumber = (UCHAR)(SlotNumber >> 8);
    ucLogicalDevice = (UCHAR)SlotNumber;

    if ( ucCSNumber > g_ucISANumberCSNs || ucCSNumber == 0 ||
         (Offset == 0 && Length != sizeof(ISA_PNP_CONFIG)) ||
         (Offset == 1 && Length != sizeof(ISA_PNP_RESOURCES)) ||
         Offset > 1 ) {
        return (0);
    }

    EnterCriticalSection(&g_csISAConfig);

    PnPSendInitiationKey(PNP_ISA_IO_BASE);

    PnPWake(PNP_ISA_IO_BASE, ucCSNumber);

    if ( Offset == 0 ) {
        PISA_PNP_CONFIG     pPnPConfig = (PISA_PNP_CONFIG)Buffer;

        PnPReadSerialId(PNP_ISA_IO_BASE, ucSerialID);

        pPnPConfig->VendorID =
        ucSerialID[0] << 24 | ucSerialID[1] << 16 | ucSerialID[2] << 8 |
        ucSerialID[3];

        pPnPConfig->SerialNumber = 
        ucSerialID[7] << 24 | ucSerialID[6] << 16 | ucSerialID[5] << 8 |
        ucSerialID[4];

        iResourceLength = PnPReadResourceData(
                                             PNP_ISA_IO_BASE, ucResourceData, sizeof(ucResourceData));

        pPnPConfig->NumberLogicalDevices = PnPGetLogicalDeviceInfo(
                                                                  ucResourceData, sizeof(ucResourceData),
                                                                  pPnPConfig->LogicalDeviceInfo);

        ulReturn = sizeof(ISA_PNP_CONFIG);
    } else {
        PISA_PNP_RESOURCES  pPnPResources = (PISA_PNP_RESOURCES)Buffer;
        int                 i;
        UCHAR               ucActive;

        PnPWriteRegister(PNP_ISA_IO_BASE, 0x07, ucLogicalDevice);

        ucActive = PnPReadRegister(PNP_ISA_IO_BASE, 0x30);

        if ( ucActive ) {
            pPnPResources->Flags = ISA_PNP_RESOURCE_FLAG_ACTIVE;

            for ( i = 0; i < 4; i++ ) {
                pPnPResources->Memory24Descriptors[i].MemoryBase =
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x40 + i * 8)) << 8;
                pPnPResources->Memory24Descriptors[i].MemoryBase |=
                (USHORT)PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x41 + i * 8));
                pPnPResources->Memory24Descriptors[i].MemoryControl =
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x42 + i * 8));
                pPnPResources->Memory24Descriptors[i].MemoryUpperLimit =
                (USHORT)(PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x43 + i * 8)) << 8);
                pPnPResources->Memory24Descriptors[i].MemoryUpperLimit |=
                (USHORT)PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x44 + i * 8));
            }

            for ( i = 0; i < 8; i++ ) {
                pPnPResources->IoPortDescriptors[i] =
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x60 + i * 2)) << 8;
                pPnPResources->IoPortDescriptors[i] |=
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x61 + i * 2));
            }

            pPnPResources->IRQDescriptors[0].IRQLevel = PnPReadRegister(PNP_ISA_IO_BASE, 0x70);
            pPnPResources->IRQDescriptors[0].IRQType = PnPReadRegister(PNP_ISA_IO_BASE, 0x71);
            pPnPResources->IRQDescriptors[1].IRQLevel = PnPReadRegister(PNP_ISA_IO_BASE, 0x72);
            pPnPResources->IRQDescriptors[1].IRQType = PnPReadRegister(PNP_ISA_IO_BASE, 0x73);

            pPnPResources->DMADescriptors[0] = PnPReadRegister(PNP_ISA_IO_BASE, 0x74);

            pPnPResources->DMADescriptors[1] = PnPReadRegister(PNP_ISA_IO_BASE, 0x75);

            for ( i = 0; i < 4; i++ ) {
                pPnPResources->Memory32Descriptors[i].MemoryBase =
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x76 + i * 16)) << 24;
                pPnPResources->Memory32Descriptors[i].MemoryBase |=
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x77 + i * 16)) << 16;
                pPnPResources->Memory32Descriptors[i].MemoryBase |=
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x78 + i * 16)) << 8;
                pPnPResources->Memory32Descriptors[i].MemoryBase |=
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x79 + i * 16));

                pPnPResources->Memory32Descriptors[i].MemoryControl =
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x7A + i * 16));

                pPnPResources->Memory32Descriptors[i].MemoryUpperLimit =
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x7B + i * 16)) << 24;
                pPnPResources->Memory32Descriptors[i].MemoryUpperLimit |=
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x7C + i * 16)) << 16;
                pPnPResources->Memory32Descriptors[i].MemoryUpperLimit |=
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x7D + i * 16)) << 8;
                pPnPResources->Memory32Descriptors[i].MemoryUpperLimit |=
                PnPReadRegister(PNP_ISA_IO_BASE, (UCHAR)(0x7E + i * 16));
            }

        } else {
            pPnPResources->Flags = 0;
        }

        ulReturn = sizeof(ISA_PNP_RESOURCES);
    }

    PnPSetWaitForKey(PNP_ISA_IO_BASE);

    LeaveCriticalSection(&g_csISAConfig);

    return (ulReturn);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ULONG
ISASetBusDataByOffset(
                     IN ULONG BusNumber,
                     IN ULONG SlotNumber,
                     IN PVOID Buffer,
                     IN ULONG Offset,
                     IN ULONG Length
                     )
{
    UCHAR               ucCSNumber, ucLogicalDevice;
    PISA_PNP_RESOURCES  pPnPResources = (PISA_PNP_RESOURCES)Buffer;
    int                 i;


    if ( g_bISAInitialized == FALSE )
        ISAInitBusInfo();

    if ( g_ucISANumberCSNs == 0 || g_ucISANumberCSNs == ~0U ) {
        return (0);
    }

    ucCSNumber = (UCHAR)(SlotNumber >> 8);
    ucLogicalDevice = (UCHAR)SlotNumber;

    if ( ucCSNumber > g_ucISANumberCSNs || ucCSNumber == 0 ||
         Offset != 1 || Length != sizeof(ISA_PNP_RESOURCES) ) {
        return (0);
    }

    EnterCriticalSection(&g_csISAConfig);

    PnPSendInitiationKey(PNP_ISA_IO_BASE);

    PnPWake(PNP_ISA_IO_BASE, ucCSNumber);

    PnPWriteRegister(PNP_ISA_IO_BASE, 0x07, ucLogicalDevice);

    if ( pPnPResources->Flags & ISA_PNP_RESOURCE_FLAG_ACTIVE ) {
        for ( i = 0; i < 4; i++ ) {
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x40 + i * 8),
                            (UCHAR)(pPnPResources->Memory24Descriptors[i].MemoryBase >> 8));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x41 + i * 8),
                            (UCHAR)(pPnPResources->Memory24Descriptors[i].MemoryBase));

            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x42 + i * 8),
                            pPnPResources->Memory24Descriptors[i].MemoryControl);

            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x43 + i * 8),
                            (UCHAR)(pPnPResources->Memory24Descriptors[i].MemoryUpperLimit >> 8));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x44 + i * 8),
                            (UCHAR)(pPnPResources->Memory24Descriptors[i].MemoryUpperLimit));
        }

        for ( i = 0; i < 8; i++ ) {
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x60 + i * 2),
                            (UCHAR)(pPnPResources->IoPortDescriptors[i] >> 8));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x61 + i * 2),
                            (UCHAR)(pPnPResources->IoPortDescriptors[i]));
        }

        PnPWriteRegister(PNP_ISA_IO_BASE, 0x70, pPnPResources->IRQDescriptors[0].IRQLevel);
        PnPWriteRegister(PNP_ISA_IO_BASE, 0x71, pPnPResources->IRQDescriptors[0].IRQType);
        PnPWriteRegister(PNP_ISA_IO_BASE, 0x72, pPnPResources->IRQDescriptors[1].IRQLevel);
        PnPWriteRegister(PNP_ISA_IO_BASE, 0x73, pPnPResources->IRQDescriptors[1].IRQType);

        PnPWriteRegister(PNP_ISA_IO_BASE, 0x74, pPnPResources->DMADescriptors[0]);
        PnPWriteRegister(PNP_ISA_IO_BASE, 0x75, pPnPResources->DMADescriptors[1]);

        for ( i = 0; i < 4; i++ ) {
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x76 + i * 16),
                            (UCHAR)(pPnPResources->Memory32Descriptors[i].MemoryBase >> 24));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x77 + i * 16),
                            (UCHAR)(pPnPResources->Memory32Descriptors[i].MemoryBase >> 16));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x78 + i * 16),
                            (UCHAR)(pPnPResources->Memory32Descriptors[i].MemoryBase >> 8));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x79 + i * 16),
                            (UCHAR)(pPnPResources->Memory32Descriptors[i].MemoryBase));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x7A + i * 16),
                            pPnPResources->Memory32Descriptors[i].MemoryControl);
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x7B + i * 16),
                            (UCHAR)(pPnPResources->Memory32Descriptors[i].MemoryUpperLimit >> 24));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x7C + i * 16),
                            (UCHAR)(pPnPResources->Memory32Descriptors[i].MemoryUpperLimit >> 16));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x7D + i * 16),
                            (UCHAR)(pPnPResources->Memory32Descriptors[i].MemoryUpperLimit >> 8));
            PnPWriteRegister(
                            PNP_ISA_IO_BASE, (UCHAR)(0x7E + i * 16),
                            (UCHAR)(pPnPResources->Memory32Descriptors[i].MemoryUpperLimit));
        }

        PnPWriteRegister(PNP_ISA_IO_BASE, 0x30, 1);
    } else {
        PnPWriteRegister(PNP_ISA_IO_BASE, 0x30, 0);
    }


    PnPSetWaitForKey(PNP_ISA_IO_BASE);

    LeaveCriticalSection(&g_csISAConfig);

    return (sizeof(ISA_PNP_RESOURCES));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
UCHAR
PnPReadRegister(
               PUCHAR pIOSpace, 
               UCHAR ucRegNo
               )
{
    _outp((USHORT) (pIOSpace + PNP_ADDRESS_PORT), (int) ucRegNo);
    return (_inp((USHORT) (pIOSpace + g_usISAReadPort)));
}    



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void
PnPWriteRegister(
                PUCHAR pIOSpace, 
                UCHAR ucRegNo, 
                UCHAR ucValue
                )
{
    _outp((USHORT) (pIOSpace + PNP_ADDRESS_PORT), (int) ucRegNo);
    _outp((USHORT) (pIOSpace + PNP_WRITE_DATA_PORT), (int) ucValue);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void
PnPSendInitiationKey(
                    PUCHAR pIOSpace
                    )
{
    int             i;
    static  UCHAR   ucInitKey[] =
    {
        0x00, 0x00,
        0x6A, 0xB5, 0xDA, 0xED, 0xF6, 0xFB, 0x7D, 0xBE,
        0xDF, 0x6F, 0x37, 0x1B, 0x0D, 0x86, 0xC3, 0x61,
        0xB0, 0x58, 0x2C, 0x16, 0x8B, 0x45, 0xA2, 0xD1,
        0xE8, 0x74, 0x3A, 0x9D, 0xCE, 0xE7, 0x73, 0x39
    };


    for ( i = 0; i < sizeof(ucInitKey); i++ ) {
        _outp((USHORT) (pIOSpace + PNP_ADDRESS_PORT), (int) ucInitKey[i]);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void
PnPWake(PUCHAR pIOSpace, UCHAR ucCSN)
{
    PnPWriteRegister(pIOSpace, 0x03, ucCSN);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void
PnPReadSerialId(PUCHAR pIOSpace, PUCHAR ucSerialId)
{
    int     i;

    for ( i = 0; i < 9; i++ ) {
        _outp((USHORT) (pIOSpace + PNP_ADDRESS_PORT), (int) 0x05);

        while ( !(_inp((USHORT) (pIOSpace + g_usISAReadPort)) & 0x01) )
            ;

        _outp((USHORT) (pIOSpace + PNP_ADDRESS_PORT), (int) 0x04);
        ucSerialId[i] = _inp((USHORT) (pIOSpace + g_usISAReadPort));
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int
PnPReadResourceData(PUCHAR pIOSpace, PUCHAR ucResourceData, int iDataSize)
{
    int     i;
    int     bSawEndTag = FALSE;
    UCHAR   ucChecksum = 0;

    for ( i = 0; i < iDataSize; i++ ) {
        _outp((USHORT) (pIOSpace + PNP_ADDRESS_PORT), (int) 0x05);

        while ( !(_inp((USHORT) (pIOSpace + g_usISAReadPort)) & 0x01) )
            ;

        _outp((USHORT) (pIOSpace + PNP_ADDRESS_PORT), (int) 0x04);
        ucResourceData[i] = _inp((USHORT) (pIOSpace + g_usISAReadPort));

        ucChecksum += ucResourceData[i];

        if ( bSawEndTag ) {
            if ( ucResourceData[i] != 0 && ucChecksum != 0 ) {
                DEBUGMSG(1, (TEXT("PnPReadResourceData : Bad resource checksum\n")));
            }

            i++;
            break;
        }

        if ( ucResourceData[i] == 0x79 ) {
            bSawEndTag = 1;
        }
    }

    return (i);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int
PnPGetLogicalDeviceInfo(
                       PUCHAR ucResourceData, int iDataSize, 
                       PISA_PNP_LOGICAL_DEVICE_INFO pDeviceInfo)
{
    PUCHAR  pCurrent;
    int     iLength, iName;
    int     i;
    int     nDevices = 0, nCompatibleIDs = 0;

    pCurrent = ucResourceData;

    memset(pDeviceInfo, 0, sizeof(ISA_PNP_LOGICAL_DEVICE_INFO) * 8);

    for ( i = 0; i < iDataSize; i++ ) {
        if ( (*pCurrent & 0x80) == 0 ) {
            //
            // Small TAG
            //

            iName = (*pCurrent >> 3) & 0x0F;
            iLength = (*pCurrent & 0x07) + 1;

            switch ( iName ) {
            case 2:     // Logical Device ID
                if ( nDevices <= 8 ) {
                    nDevices++;
                    nCompatibleIDs = 0;
                    pDeviceInfo[nDevices - 1].LogicalDeviceID =
                    pCurrent[1] << 24 | pCurrent[2] << 16 | pCurrent[3] << 8 |
                    pCurrent[4];
                }
                break;

            case 3:     // Compatible Device ID
                if ( nDevices <= 8 ) {
                    nCompatibleIDs++;
                    pDeviceInfo[nDevices - 1].CompatibleIDs[nCompatibleIDs - 1] =
                    pCurrent[1] << 24 | pCurrent[2] << 16 | pCurrent[3] << 8 |
                    pCurrent[4];
                }
                break;
            }
        } else {
            //
            // Large TAG
            //

            iName = *pCurrent & 0x7F;
            iLength = ((pCurrent[2] << 8) | pCurrent[1]) + 3;
        }

        pCurrent += iLength;
        i += iLength - 1;
    }

    return (nDevices);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void
PnPSetWaitForKey(PUCHAR pIOSpace)
{
    PnPWriteRegister(pIOSpace, 0x02, 0x02);
}
