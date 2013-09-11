// CANBench.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "sdk_can.h"

HANDLE g_hCan;


BOOL CanInit(DWORD baudrate)
{

    g_hCan = CreateFile(L"CAN1:",0,0,NULL,OPEN_EXISTING,0,NULL);

    if (g_hCan == INVALID_HANDLE_VALUE)
        return FALSE;

    IOCTL_CAN_COMMAND_IN cmdIn;
    cmdIn = STOP;
    DeviceIoControl(g_hCan,IOCTL_CAN_COMMAND,&cmdIn,sizeof(cmdIn),NULL,0,NULL,NULL);

    cmdIn = RESET;
    DeviceIoControl(g_hCan,IOCTL_CAN_COMMAND,&cmdIn,sizeof(cmdIn),NULL,0,NULL,NULL);

    IOCTL_CAN_CONFIG_IN cfgIn;
    cfgIn.cfgType = BAUDRATE_CFG;
    cfgIn.BaudRate = baudrate;
    DeviceIoControl(g_hCan,IOCTL_CAN_CONFIG,&cfgIn,sizeof(cfgIn),NULL,0,NULL,NULL);

    cmdIn = START;
    DeviceIoControl(g_hCan,IOCTL_CAN_COMMAND,&cmdIn,sizeof(cmdIn),NULL,0,NULL,NULL);

    return TRUE;
}

void CanDeinit()
{
    IOCTL_CAN_COMMAND_IN cmdIn = STOP;
    DeviceIoControl(g_hCan,IOCTL_CAN_COMMAND,&cmdIn,sizeof(cmdIn),NULL,0,NULL,NULL);

    IOCTL_CAN_STATUS_OUT statusOut;       
    DeviceIoControl(g_hCan,IOCTL_CAN_STATUS,NULL,0,&statusOut,sizeof(statusOut),NULL,NULL);


    RETAILMSG(1,(TEXT("status Out\r\n current msg tx %d %d %d\r\n current msg rx %d\r\n rx msg discarded %d\r\n msg lost %d\r\n msg filtered out %d\r\ntotal msg received %d\r\n\r\n"),
        statusOut.currentTxMsg[0],statusOut.currentTxMsg[1],statusOut.currentTxMsg[2],statusOut.currentRxMsg,statusOut.RxDiscarded,statusOut.RxLost,statusOut.FilteredOut,statusOut.TotalReceived));

    CloseHandle(g_hCan);

}
void SendCanDummyMessage(DWORD size,DWORD grp, DWORD* pNbSent)
{
    IOCTL_CAN_SEND_IN sendIn;
    CAN_MESSAGE msgArray[8];
    msgArray[0].id.u32 =0;
    msgArray[0].id.s_extended.id = 0x8721;
    msgArray[0].id.s_extended.extended = 0x1;
    msgArray[0].length = (BYTE) size;
    msgArray[0].MDH = msgArray[0].MDL = 255;

    for (DWORD i=1;i<grp;i++)
    {
        memcpy(&msgArray[i],&msgArray[0],sizeof(msgArray[0]));
    }

    sendIn.priority = TXMEDIUM;
    sendIn.timeout = 1000;
    sendIn.nbMsg = grp;
    sendIn.msgArray = msgArray;

    if (DeviceIoControl(g_hCan,IOCTL_CAN_SEND,&sendIn,sizeof(sendIn),NULL,0,NULL,NULL) == FALSE)
    {
        RETAILMSG(1,(TEXT("Send failed\r\n")));
    }
    *pNbSent = sendIn.nbMsgSent;
}

void BenchTx(DWORD duration, DWORD minsize, DWORD maxsize, DWORD mingrp, DWORD maxgrp)
{
    DWORD TotalBytesSent=0;
    DWORD TotalMsgSent=0;
    DWORD size = minsize;
    DWORD grp = mingrp;
    DWORD startDate=GetTickCount();

    if (minsize > maxsize)
    {
        RETAILMSG(1,(TEXT("invalid min/max size (min : %d / max : %d)\r\n"),minsize,maxsize));
    }

    while(GetTickCount()- startDate < duration)
    {
        SendCanDummyMessage(size,grp,&grp);
        TotalBytesSent += size*grp;
        TotalMsgSent += grp;
        size++;
        if (size > maxsize)
        {
            size = minsize;
            grp++;
            if (grp > maxgrp)
            {
                grp = mingrp;
            }
            

        }
    }
#ifndef SHIP_BUILD    
    duration = GetTickCount()- startDate;
    DWORD BytesPerSec = (TotalBytesSent*1000)/duration;
    RETAILMSG(1,(TEXT("TX results (duration %d ms, min packet size %d, max packet size %d, min burst %d, max burst %d)\t\t%u.%u kB/s (%d msg)"),
        duration,minsize,maxsize,mingrp,maxgrp,BytesPerSec /1024 , BytesPerSec % 1024,TotalMsgSent)); 
#endif    
}

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
    int rate=250;
    int duration=3000;
    int minsize=1;
    int maxsize=8;
    int mingrp=1;
    int maxgrp=8;
    UNREFERENCED_PARAMETER(envp);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(argc);
    _tprintf(_T("Can Bench TX!\n"));

    for (int i=1;i<argc;i++)
    {
        if (_memicmp(argv[i],L"-b",2*sizeof(TCHAR))== 0)
        {
            rate = _wtoi(argv[i]+2);
            if (rate == 0) return -1;
        } else if (_memicmp(argv[i],L"-d",2*sizeof(TCHAR))== 0)
        {
            duration = _wtoi(argv[i]+2);
        } else if (_memicmp(argv[i],L"-smin",5*sizeof(TCHAR))== 0)
        {
            minsize = _wtoi(argv[i]+5);
        } else if (_memicmp(argv[i],L"-smax",5*sizeof(TCHAR))== 0)
        {
            maxsize = _wtoi(argv[i]+5);
        } else if (_memicmp(argv[i],L"-gmin",5*sizeof(TCHAR))== 0)
        {
            mingrp = _wtoi(argv[i]+5);
        } else if (_memicmp(argv[i],L"-gmax",5*sizeof(TCHAR))== 0)
        {
            maxgrp = _wtoi(argv[i]+5);
        } else
        {
            _tprintf(_T("%s : unknown option. exiting ...\n"),argv[i]);
            return -1;
        }
    }
    _tprintf(_T("Bus rate is %d kBits/s\r\n"),rate);



    if (!CanInit(rate*1000))
    {
        RETAILMSG(1,(TEXT("unable to intialize the CAN driver\r\n")));
        return -1;
    }


    BenchTx(duration,minsize,maxsize,mingrp,maxgrp);
    
    Sleep(200); //Wait a bit so that the queued messages are sent before turning down the CAN transceiver

    CanDeinit();


    return 0;
}

