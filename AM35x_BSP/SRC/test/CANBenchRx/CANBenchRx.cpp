// CANBenchRx.cpp : Defines the entry point for the console application.
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


    IOCTL_CAN_CLASS_FILTER_CONFIG_IN filterCfg;
    memset(&filterCfg,0,sizeof(filterCfg));

    
    filterCfg.cfgType = CREATE_CLASS_FILTER_CFG;
    filterCfg.fEnabled = TRUE;
    filterCfg.rxPriority = RXCRITICAL;
    filterCfg.classFilter.id.s_extended.extended = 1;
    filterCfg.classFilter.id.s_extended.id = 123;
    filterCfg.classFilter.mask.s_extended.extended = 1;
    filterCfg.classFilter.mask.s_extended.id = 0xFFF;

    DeviceIoControl(g_hCan,IOCTL_CAN_FILTER_CONFIG,&filterCfg,sizeof(filterCfg),NULL,0,NULL,NULL);

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
void ReceiveCanMessage(DWORD* pTotalSize,DWORD* pNbMsgReceived)
{
    IOCTL_CAN_RECEIVE_OUT rcvOut;
    CAN_MESSAGE msg[20];
    rcvOut.msgArray = msg;
    rcvOut.nbMaxMsg = sizeof(msg)/sizeof(msg[0]);
    rcvOut.timeout = 1000;        
    if (DeviceIoControl(g_hCan,IOCTL_CAN_RECEIVE,NULL,0,&rcvOut,sizeof(rcvOut),NULL,NULL) == FALSE)
    {
        RETAILMSG(1,(TEXT("Receive timed out\r\n")));
    }
    else
    {
        DWORD i;
        CAN_MESSAGE* pMsg;

        *pNbMsgReceived = rcvOut.nbMsgReceived;
        for (i=0;i<rcvOut.nbMsgReceived;i++)
        {
            pMsg = &rcvOut.msgArray[i];
            *pTotalSize += pMsg->length;
        }
    }
}

void BenchRx(DWORD duration)
{
    DWORD TotalSize=0;
    DWORD TotalMsg=0;

    DWORD startDate=GetTickCount();

    
    while(GetTickCount()- startDate < duration)
    {
        DWORD BytesReceived;
        DWORD MsgReceived;
        MsgReceived = 0;
        BytesReceived = 0;
        ReceiveCanMessage(&BytesReceived,&MsgReceived);
        
        TotalSize += BytesReceived;
        TotalMsg += MsgReceived;
    }
#ifndef SHIP_BUILD    
    duration = GetTickCount()- startDate;
    DWORD BytesPerSec = (TotalSize * 1000)/duration;
    RETAILMSG(1,(TEXT("RX results (duration %d ms)\t\t%u.%u kB/s (%d msg)"),
        duration,BytesPerSec /1024 , BytesPerSec % 1024,TotalMsg)); 
#endif    
}

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
    int rate=250;
    int duration=3000;
    UNREFERENCED_PARAMETER(envp);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(argc);
    _tprintf(_T("Can Bench RX!\n"));

    for (int i=1;i<argc;i++)
    {
        if (_memicmp(argv[i],L"-b",2*sizeof(TCHAR))== 0)
        {
            rate = _wtoi(argv[i]+2);
        } else if (_memicmp(argv[i],L"-d",2*sizeof(TCHAR))== 0)
        {
            duration = _wtoi(argv[i]+2);
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


    BenchRx(duration);
    
//    Sleep(200); //Wait a bit so that the queued messages are sent before turning down the CAN transceiver

    CanDeinit();


    return 0;
}

