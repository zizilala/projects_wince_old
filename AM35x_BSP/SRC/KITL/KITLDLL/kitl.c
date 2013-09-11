// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File:  kitl.c
//
#include "bsp.h"
#include <kitlprot.h>
#include <halether.h>
#include <oal.h>
#include <oalex.h>
//#include <bus.h>
#include "oemkitl.h"

extern DBGPARAM dpCurSettings;
//------------------------------------------------------------------------------
//
//  Global:  g_oalKitlBuffer
//
//  This global variable is intended to be used by KITL protocol implementation.
//
UINT8 g_oalKitlBuffer[OAL_KITL_BUFFER_SIZE];

//------------------------------------------------------------------------------
//
//  Global:  g_kitlState
//
//  This global static variable store common KITL state information.
//
static struct {
    CHAR deviceId[OAL_KITL_ID_SIZE];
    OAL_KITL_ARGS args;
    OAL_KITL_DEVICE *pDevice;
} g_kitlState;

//------------------------------------------------------------------------------
//  External functions
//
BOOL OALKitlSerialInit(
    LPSTR deviceId, OAL_KITL_DEVICE *pDevice, OAL_KITL_ARGS *pArgs, 
    KITLTRANSPORT *pKitl
);

BOOL OALKitlEthInit(
    LPSTR deviceId, OAL_KITL_DEVICE *pDevice, OAL_KITL_ARGS *pArgs, 
    KITLTRANSPORT *pKitl
);


//------------------------------------------------------------------------------
//
//  Function:  OALKitlGetDevLoc
//
//  This function allows other module to obtain KITL device location.
//
BOOL OALKitlGetDevLoc(DEVICE_LOCATION *pDevLoc)
{
    if(pDevLoc)
    {
        memcpy(pDevLoc, &g_kitlState.args.devLoc, sizeof(*pDevLoc));
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//------------------------------------------------------------------------------
//
//  Function:  OALKitlGetFlags
//
//  This function allows other modules to obtain KITL flags.
//
BOOL OALKitlGetFlags(UINT32 *pFlags)
{
    if(pFlags)
    {
        memcpy(pFlags, &g_kitlState.args.flags, sizeof(*pFlags));
    }
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  OALLogSetZones
//
//  Updates the current zone mask.
//
VOID KitlLogSetZones(UINT32 newMask)
{
    // Update dpCurSettings mask which actually controls the zones
    dpCurSettings.ulZoneMask = newMask;

    OALMSG(OAL_INFO, (
        L"INFO:OALLogSetZones: dpCurSettings.ulZoneMask: 0x%x\r\n",
        dpCurSettings.ulZoneMask
    ));
}
//------------------------------------------------------------------------------
//
//  Function:  OEMKitlInit
//
//  This function is called from KitlInit to initialize KITL device and
//  KITLTRANSPORT structure. Implementation verify boot args structure validity
//  and call KITL device class init function.
//
BOOL OEMKitlInit(PKITLTRANSPORT pKitl)
{
    BOOL rc = FALSE;
    
  //  KitlLogSetZones(0xFFFF);

    KITL_RETAILMSG(ZONE_KITL_OAL, ("+OEMKitlInit(0x%08x)\r\n", pKitl));

    switch (g_kitlState.pDevice->type) {
#ifdef KITL_ETHER             
    case OAL_KITL_TYPE_ETH:
        rc = OALKitlEthInit(
            g_kitlState.deviceId, g_kitlState.pDevice,
            &g_kitlState.args, pKitl
        );
        break;
#endif
#ifdef KITL_SERIAL
    case OAL_KITL_TYPE_SERIAL:
        rc = OALKitlSerialInit(
            g_kitlState.deviceId, g_kitlState.pDevice,
            &g_kitlState.args, pKitl
        );
        break;
#endif
    }
    if (rc) {
        pKitl->pfnPowerOn  = OALKitlPowerOn;
        pKitl->pfnPowerOff = OALKitlPowerOff;
    } else {
        pKitl->pfnPowerOn  = NULL;
        pKitl->pfnPowerOff = NULL;
    }

    KITL_RETAILMSG(ZONE_KITL_OAL, ("-OEMKitlInit(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  OALKitlInit
//
//  This function is called from OEMInit to initialize KITL. It typically
//  calls initialization routine for device KITL class and then KitlInit.
//  The kernel/KITL then call back OEMKitlInit at moment when debug/KITL
//  connection should be initialized.
//
BOOL OALKitlInit(
    LPCSTR deviceId, OAL_KITL_ARGS *pArgs, OAL_KITL_DEVICE *pDevice
) {
    BOOL rc = FALSE;

    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "+OALKitlInit('%hs', 0x%08x - %d/%d/0x%08x, 0x%08x)\r\n", deviceId, 
        pArgs->flags, pArgs->devLoc.IfcType, pArgs->devLoc.BusNumber, 
        pArgs->devLoc.LogicalLoc, pDevice
    ));

    // Display KITL parameters
    KITL_RETAILMSG(ZONE_INIT, (
        "DeviceId................. %s\r\n", deviceId
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pArgs->flags............. 0x%x\r\n", pArgs->flags
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pArgs->devLoc.IfcType.... %d\r\n",   pArgs->devLoc.IfcType
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pArgs->devLoc.LogicalLoc. 0x%x\r\n", pArgs->devLoc.LogicalLoc
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pArgs->devLoc.PhysicalLoc 0x%x\r\n", pArgs->devLoc.PhysicalLoc
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pArgs->devLoc.Pin........ %d\r\n",   pArgs->devLoc.Pin
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pArgs->ip4address........ %s\r\n",   OALKitlIPtoString(pArgs->ipAddress)
    ));
   KITL_RETAILMSG(ZONE_INIT, (
        "pArgs->mac............... 0x%x 0x%x 0x%x\r\n", pArgs->mac[0], pArgs->mac[1], pArgs->mac[2]
    ));

    KITL_RETAILMSG(ZONE_INIT, (
        "pDevice->Name............ %hs\r\n",   pDevice->name
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pDevice->ifcType......... %d\r\n",   pDevice->ifcType
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pDevice->id.............. 0x%x\r\n", pDevice->id
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pDevice->resource........ %d\r\n",   pDevice->resource
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pDevice->type............ %d\r\n",   pDevice->type
    ));
    KITL_RETAILMSG(ZONE_INIT, (
        "pDevice->pDriver......... 0x%x\r\n", pDevice->pDriver
    ));

    // If KITL is disabled simply return
    if ((pArgs->flags & OAL_KITL_FLAGS_ENABLED) == 0) {
        KITL_RETAILMSG(ZONE_WARNING, ("WARN: OALKitlInit: KITL Disabled\r\n"));
        rc = TRUE;
        goto cleanUp;
    }

    // Find if we support device on given location
    g_kitlState.pDevice = OALKitlFindDevice(&pArgs->devLoc, pDevice);
    if (g_kitlState.pDevice == NULL) {
        KITL_RETAILMSG(ZONE_ERROR, (
            "ERROR: OALKitlInit: No supported KITL device at interface %d "
            "bus %d location 0x%x\r\n", pArgs->devLoc.IfcType,
            pArgs->devLoc.BusNumber, pArgs->devLoc.LogicalLoc
        ));
        goto cleanUp;
    }

    // RNDIS_MDD (public code) attempts to map devLoc.PhysicalLoc with
    // NKCreateStaticMapping.  NKCreateStaticMapping requires a true
    // physical address.  OALKitlFindDevice fills in devLoc.PhysicalLoc
    // with the kernel mode virtual address which causes NKCreateStaticMapping
    // to fail.
    // Overwrite devLoc.PhysicalLoc with the actual physical address so 
    // this function succeeds.  Note that all kitl transports need to 
    // handle a true physical address in this location.
    pArgs->devLoc.PhysicalLoc = (PVOID)OALVAtoPA(pArgs->devLoc.PhysicalLoc);
    
    // Save KITL configuration 
    memcpy(g_kitlState.deviceId, deviceId, sizeof(g_kitlState.deviceId));
    memcpy(&g_kitlState.args, pArgs, sizeof(g_kitlState.args));
            
    // Start KITL in desired mode
    if (!KitlInit((pArgs->flags & OAL_KITL_FLAGS_PASSIVE) == 0)) {
        KITL_RETAILMSG(ZONE_ERROR, ("ERROR: OALKitlInit: KitlInit failed\r\n"));
        goto cleanUp;
    }
    
    rc = TRUE;
    
cleanUp:
    KITL_RETAILMSG(ZONE_KITL_OAL, ("-OALKitlInit(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  OALKitlPowerOff
//
//  This function is called as part of OEMPowerOff implementation. It should
//  save all information about KITL device and put it to power off mode.
//
VOID OALKitlPowerOff()
{
    KITL_RETAILMSG(ZONE_KITL_OAL, ("+OALKitlPowerOff\r\n"));
    
    switch (g_kitlState.pDevice->type) {
#ifdef KITL_ETHER
    case OAL_KITL_TYPE_ETH:
        {
            OAL_KITL_ETH_DRIVER *pDriver;
            pDriver = (OAL_KITL_ETH_DRIVER*)g_kitlState.pDevice->pDriver;
            if (pDriver && pDriver->pfnPowerOff != NULL) pDriver->pfnPowerOff();
        }            
        break;
#endif
#ifdef KITL_SERIAL
     case OAL_KITL_TYPE_SERIAL:
        {
            OAL_KITL_SERIAL_DRIVER *pDriver;
            pDriver = (OAL_KITL_SERIAL_DRIVER*)g_kitlState.pDevice->pDriver;
            if (pDriver && pDriver->pfnPowerOff != NULL) pDriver->pfnPowerOff();
        }
        break;
#endif
    }        

    OEMKitlEnable(FALSE);    
    KITL_RETAILMSG(ZONE_KITL_OAL, ("-OALKitlPowerOff\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  OALKitlPowerOn
//
//  This function is called as part of OEMPowerOff implementation. It should
//  restore KITL device back to working state.
//
VOID OALKitlPowerOn()
{
    KITL_RETAILMSG(ZONE_KITL_OAL, ("+OALKitlPowerOn\r\n"));


    OEMKitlEnable(TRUE);
  
    switch (g_kitlState.pDevice->type) {
#ifdef KITL_ETHER
    case OAL_KITL_TYPE_ETH:
        {
            OAL_KITL_ETH_DRIVER *pDriver;
            pDriver = (OAL_KITL_ETH_DRIVER*)g_kitlState.pDevice->pDriver;
            if (pDriver->pfnPowerOn != NULL) pDriver->pfnPowerOn();
        }            
        break;
#endif
#ifdef KITL_SERIAL
     case OAL_KITL_TYPE_SERIAL:
        {
            OAL_KITL_SERIAL_DRIVER *pDriver;
            pDriver = (OAL_KITL_SERIAL_DRIVER*)g_kitlState.pDevice->pDriver;
            if (pDriver->pfnPowerOn != NULL) pDriver->pfnPowerOn();
        }
        break;
#endif
    }        

    KITL_RETAILMSG(ZONE_KITL_OAL, ("-OALKitlPowerOn\r\n"));
}


#if 0
void 
memdump(unsigned char * data,   unsigned short num_bytes, unsigned short offset    )
{    
#if 1
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(num_bytes);
    UNREFERENCED_PARAMETER(offset);
    
#else
    unsigned short i,j,l;
    unsigned char tmp_str[100];
    unsigned char tmp_str1[10];
    for (i = 0; i < num_bytes; i += 16){
        unsigned short n ;
        tmp_str[0]='\0';
        n = i+offset ;
        for (j=0; j<4; j++) {
            l=n%16;
            if (l>=10)
            tmp_str[3-j]=(unsigned char)('A'+l-10);
            else
            tmp_str[3-j]=(unsigned char)(l+'0');
            n >>= 4 ;
        }
        tmp_str[4]='\0';
        strcat ( (char *)tmp_str, "  ");
        /*          Output the hex bytes        */
        for (j = i; j < (i+16); j++) {
            int m ;
            if (j < num_bytes)  {
                m=((unsigned int)((unsigned char)*(data+j)))/16 ;
                if (m>=10)
                    tmp_str1[0]='A'+(unsigned char)m-10;
                else
                    tmp_str1[0]=(unsigned char)m+'0';
                m=((unsigned int)((unsigned char)*(data+j)))%16 ;
                if (m>=10)
                    tmp_str1[1]='A'+(unsigned char)m-10;
                else
                    tmp_str1[1]=(unsigned char)m+'0';
                tmp_str1[2]='\0';
                strcat ((char *)tmp_str, (char *)tmp_str1);
                strcat ((char *)tmp_str, " ");
            }
            else {
                strcat((char *)tmp_str,"   ");
            }
        }
        strcat((char *)tmp_str, "  ");
        l=(unsigned short)strlen((char *)tmp_str);

        /*         * Output the ASCII bytes        */
        for (j = i; j < (i+16); j++){
            if (j < num_bytes){
                char c = *(data+j);
                if (c < ' ' || c > 'z')
                    c = '.';
                tmp_str[l++]=c;
            }
            else
                tmp_str[l++]=' ';
        }
        tmp_str[l++]='\r';        tmp_str[l++]='\n';        tmp_str[l++]='\0';
        KITLOutputDebugString("%s", tmp_str);    
    }
#endif
}
#endif
//------------------------------------------------------------------------------
