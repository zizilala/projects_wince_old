// CanDemoEcho.cpp : Simple test app for CAN.
// - The application loops waiting for CAN messages. When one or more messages are received,
//   they are sent back to the BUS.
// - The xmit ID may be different than the rx ID so as to avoid abitration problem. (see -idrx and -idtx arguments)
// - RX and TX are synchronous
// - Dynamic baudrate change is supported (detection of change request on rx timeout).fixed baudrate is also supported (see -b argument)
// 

#include "stdafx.h"
#include "stdafx.h"
#include "sdk_can.h"

HANDLE g_hCan;

HANDLE g_hMsgQueue;
#define dimof(x) (sizeof(x)/sizeof(x[0]))
int BaudRate[] ={20000, 50000, 100000, 125000, 250000, 500000, 1000000};
int idrx=-1;
int idtx=-1;

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
    if (idrx==-1)
    {
        filterCfg.classFilter.id.s_extended.id = 123;
        filterCfg.classFilter.mask.s_extended.id = 0xF000;
    }
    else
    {
        filterCfg.classFilter.id.s_extended.id = idrx;
        filterCfg.classFilter.mask.s_extended.id = 0xFFFF;
    }
    filterCfg.classFilter.mask.s_extended.extended = 1;
    

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


RETAILMSG(1,(TEXT("status Out\r\n current msg tx %d %d %d\r\n current msg rx %d\r\n rx msg discarded %d\r\n msg lost %d\r\n msg filtered out %d\r\ntotal msg received %d\r\ntotal msg Sent %d\r\n\r\n"),
        statusOut.currentTxMsg[0],statusOut.currentTxMsg[1],statusOut.currentTxMsg[2],statusOut.currentRxMsg,statusOut.RxDiscarded,statusOut.RxLost,statusOut.FilteredOut,statusOut.TotalReceived,statusOut.TotalSent));
    CloseHandle(g_hCan);

}
BOOL ReceiveAndSendCanMessage(DWORD* pTotalSize,DWORD* pNbMsgReceived)
{
    
    IOCTL_CAN_SEND_IN sendIn;
    IOCTL_CAN_RECEIVE_OUT rcvOut;
    CAN_MESSAGE msg[20];
    rcvOut.msgArray = msg;
    rcvOut.nbMaxMsg = sizeof(msg)/sizeof(msg[0]);
    rcvOut.timeout = 1000;        
    if (DeviceIoControl(g_hCan,IOCTL_CAN_RECEIVE,NULL,0,&rcvOut,sizeof(rcvOut),NULL,NULL) == FALSE)
    {
        RETAILMSG(1,(TEXT("ReceiveAndSendCanMessage : Receive timed out\r\n")));
        return FALSE;
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
            pMsg->MDL = 0x64646464;
            pMsg->MDH = 0x64646464;
            if (idtx==-1)
            {
                pMsg->id.s_extended.id = 123;
            }
            else
            {
                pMsg->id.s_extended.id = idtx;
            }
            pMsg->id.s_extended.extended = 1;
        }

        DWORD NbMsgSent = 0;
        DWORD retry = 0;
        do 
        {
            sendIn.msgArray = rcvOut.msgArray+NbMsgSent;
            sendIn.nbMsg = rcvOut.nbMsgReceived-NbMsgSent;
            sendIn.priority = TXMEDIUM;
            sendIn.timeout = 1000;            
            if (DeviceIoControl(g_hCan,IOCTL_CAN_SEND,&sendIn,sizeof(sendIn),NULL,0,NULL,NULL) == FALSE)
            {
                RETAILMSG(1,(TEXT("ReceiveAndSendCanMessage : Send timed out\r\n")));
                retry++;
                if (retry > 5)
                {
                    RETAILMSG(1,(TEXT("ReceiveAndSendCanMessage : Send retry exceeds limit\r\n")));
                    break;
                }
            }            
            NbMsgSent += sendIn.nbMsgSent;
        } while (NbMsgSent < rcvOut.nbMsgReceived);
    }
    return TRUE;
}

void SynchronousEcho(DWORD duration)
{
    BOOL fNotStarted = TRUE;
    BOOL fWaitForData = TRUE;
    DWORD TotalSize=0;
    DWORD TotalMsg=0;
    int i=0;
    


    DWORD startDate=GetTickCount();

    CeSetThreadPriority(GetCurrentThread(),CeGetThreadPriority(GetCurrentThread())-2);
    
    while(fNotStarted || (duration==0) || (GetTickCount()- startDate < duration))
    {
        DWORD BytesReceived;
        DWORD MsgReceived;
        MsgReceived = 0;
        BytesReceived = 0;
        if (ReceiveAndSendCanMessage(&BytesReceived,&MsgReceived) == FALSE)
        {
            if (fNotStarted)
            {
                startDate=GetTickCount();             
            }
            else
            {
                if (fWaitForData == FALSE)
                {                    
                    fWaitForData = TRUE;
                    IOCTL_CAN_COMMAND_IN cmdIn;
                    cmdIn = STOP;
                    DeviceIoControl(g_hCan,IOCTL_CAN_COMMAND,&cmdIn,sizeof(cmdIn),NULL,0,NULL,NULL);

                    IOCTL_CAN_CONFIG_IN cfgIn;
                    cfgIn.cfgType = BAUDRATE_CFG;
                    i++;
                    if (i >= dimof(BaudRate))
                    {
                        break; //end of supported baudrate. Stop the test
                    }
                    cfgIn.BaudRate = BaudRate[i];
                    DeviceIoControl(g_hCan,IOCTL_CAN_CONFIG,&cfgIn,sizeof(cfgIn),NULL,0,NULL,NULL);

                    cmdIn = START;
                    DeviceIoControl(g_hCan,IOCTL_CAN_COMMAND,&cmdIn,sizeof(cmdIn),NULL,0,NULL,NULL);
                    _tprintf(TEXT("RX timeout detected (need to change baudrate): new baudrate is %d\r\n"),BaudRate[i]);

                    

                }
                else
                {
                    _tprintf(TEXT("RX timeout : Waiting for data\r\n"));
                }
            }

        }
        else
        {
            fNotStarted = FALSE;
            fWaitForData = FALSE;
        }
        TotalSize += BytesReceived;
        TotalMsg += MsgReceived;
    }
    duration = GetTickCount()- startDate;
    DWORD BytesPerSec = (TotalSize * 1000)/duration;
    _tprintf(TEXT("RX results (duration %d ms)\t\t%u.%u kB/s (%d msg)\r\n"),
        duration,BytesPerSec /1024 , BytesPerSec % 1024,TotalMsg);
}

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
    int rate=BaudRate[0];
    int duration=10000;
    UNREFERENCED_PARAMETER(envp);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(argc);
    _tprintf(_T("Can demo (Echo application)!\n"));

    for (int i=1;i<argc;i++)
    {
        if (_memicmp(argv[i],L"-b",2*sizeof(TCHAR))== 0)
        {
            rate = _wtoi(argv[i]+2) * 1000;
        } else if (_memicmp(argv[i],L"-d",2*sizeof(TCHAR))== 0)
        {
            duration = _wtoi(argv[i]+2);
        } else if (_memicmp(argv[i],L"-idrx",5*sizeof(TCHAR))== 0)
        {
            idrx = _wtoi(argv[i]+5);
        } else if (_memicmp(argv[i],L"-idtx",5*sizeof(TCHAR))== 0)
        {
            idtx = _wtoi(argv[i]+5);
        }else
        {
            _tprintf(_T("%s : unknown option. exiting ...\n"),argv[i]);
            return -1;
        }
    }
    _tprintf(_T("Bus rate is %d kBits/s\r\n"),rate/1000);
    _tprintf(_T("%d %d\r\n"),idtx,idrx);



    if (!CanInit(rate))
    {
        RETAILMSG(1,(TEXT("unable to intialize the CAN driver\r\n")));
        return -1;
    }

    SynchronousEcho(duration);
    
    Sleep(200); //Wait a bit so that the queued messages are sent before turning down the CAN transceiver

    CanDeinit();


    return 0;
}

