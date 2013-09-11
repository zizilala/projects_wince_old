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
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:
    oaldma.h

Abstract:
    Contains definitions for DMA API functions.

Notes:

--*/

#ifndef   __FDMA_H__
#define   __FDMA_H__


//
// Helper macros.
//

#define READ_FDMA_REG(A)     READ_REGISTER_ULONG((PULONG)(A))
#define WRITE_FDMA_REG(A, B) WRITE_REGISTER_ULONG((PULONG)(A), (B))
#define FDMA_CHAN_SHIFT(Channel) (Channel * 2)

//
// FDMA register base addresses
//

#define FDMA_BASE_ADDRESS      (0xB9220000)
#define FDMA_INTERFACE_ADDRESS (FDMA_BASE_ADDRESS)
#define FDMA_CHANNEL_ADDRESS   (FDMA_BASE_ADDRESS + 0x8000)
#define FDMA_MAILBOX_ADDRESS   (FDMA_BASE_ADDRESS + 0xBFC0)


//
// Maximum number of FDMA adatpers
//

#define FDMA_MAX_ADAPTERS 1


//
// Maximum number of FDMA channels and IRQ lines
//

#define FDMA_MAX_CHANNELS 16
#define FDMA_MAX_IRQS     30


//
// FDMA channel defines
//

#define FDMA_CHANNEL_STAT_IDLE    0x0
#define FDMA_CHANNEL_STAT_RUNNING 0x2
#define FDMA_CHANNEL_STAT_PAUSED  0x3

#define FDMA_CHANNEL_INT_MSG      0x1
#define FDMA_CHANNEL_INT_ERR      0x2
#define FDMA_CHANNEL_INT_MASK     0x3

#define FDMA_CHANNEL_CMD_MASK     0x3
#define FDMA_CHANNEL_CMD_RESTART  0x0
#define FDMA_CHANNEL_CMD_START    0x1
#define FDMA_CHANNEL_CMD_PAUSE    0x2
#define FDMA_CHANNEL_CMD_FLUSH    0x3


//
// FDMA alignment requirements
//

#define FDMA_NODE_ALIGNMENT       32
#define FDMA_NODE_ALIGNMENT_SHIFT 5


//
// Maximum number of map registers per channel
//

#define FDMA_MAX_MAP_REGISTERS 256


//
// Maximum number of bytes per map register transfer
//

#define FDMA_MAX_TRANSFER_SIZE 0xFFFFFFFF


//
// FDMA_EN
//

typedef struct _FDMA_EN {

  ULONG En: 1;
  ULONG Reserved: 31;

} FDMA_EN, *PFDMA_EN;


//
// FDMA_REQ_CTRL
//

typedef struct _FDMA_CMD_STAT {

  union {

    //
    // Read status register
    //

    struct {

      ULONG Sta:2;
      ULONG Err: 3;
      ULONG Data: 27;

    } Read;

    //
    // Write command register
    //

    struct {

      ULONG Cmd:5;
      ULONG Data: 27;

    } Write;

    ULONG All;
  };
} FDMA_CMD_STAT, *PFDMA_CMD_STAT;


//
// FDMA_PTR and FDMA_CNT
//

typedef struct _FDMA_NODE_PARAM {

  struct _NODE_REGISTERS *NextNode;
  ULONG Reserved;
  ULONG Cnt;
  ULONG Saddr;
  ULONG Daddr;
  ULONG Reserved1[11];

} FDMA_NODE_PARAM, *PFDMA_NODE_PARAM;


//
// FDMA_REQ_CTRL
//

typedef struct _FDMA_REQ_CTRL {

  ULONG HoldOff:4;
  ULONG OpCode:4;
  ULONG Reserved: 6;
  ULONG Wnr: 1;
  ULONG Reserved1: 7;
  ULONG SrcDestNum: 2;
  ULONG NumOps: 5;
  ULONG IncrAddr: 1;
  ULONG Reserved2: 2;

} FDMA_REQ_CTRL, *PFDMA_REQ_CTRL;


//
// FDMA registers layout
//

typedef struct _FDMA_INTERFACE_REGS {

  ULONG Id;
  ULONG Ver;
  FDMA_EN En;

} FDMA_INTERFACE_REGS, *PFDMA_INTERFACE_REGS;

typedef struct _FDMA_CHANNEL_REGS {

  ULONG RevId;
  ULONG Reserved[15];
  FDMA_CMD_STAT ChanCmdStat[FDMA_MAX_CHANNELS];
  ULONG Reserved1[1088];
  FDMA_NODE_PARAM NodeParam[FDMA_MAX_CHANNELS];
  ULONG Reserved2[128];
  FDMA_REQ_CTRL ReqCtrl[FDMA_MAX_IRQS];

} FDMA_CHANNEL_REGS, *PFDMA_CHANNEL_REGS;

typedef struct _FDMA_MAILBOX_REGS {

  ULONG CmdStat;
  ULONG CmdSet;
  ULONG CmdClr;
  ULONG CmdMask;
  ULONG IntStat;
  ULONG IntSet;
  ULONG IntClr;
  ULONG IntMask;

} FDMA_MAILBOX_REGS, *PFDMA_MAILBOX_REGS;


//
// Enums for FDMA_REQ_CTRL
//

typedef enum {

  AddressConstant = 1,
  AddressIncrementing

} ADDRESS_INC_TYPE;


//
// FDMA_REQ_CTRL
//

typedef struct _NODE_CTRL {

  ULONG ReqMap:5;
  ADDRESS_INC_TYPE SrcInc:2;
  ADDRESS_INC_TYPE DstInc: 2;
  ULONG Reserved: 21;
  ULONG PauseEnb: 1;
  ULONG IntEnb: 1;

} NODE_CTRL, *PNODE_CTRL;


//
// Memory-to-memory and paced transfer node registers
//

typedef struct _NODE_REGISTERS {

  struct _NODE_REGISTERS *NextPtr;
  NODE_CTRL Ctrl;
  ULONG Nbytes;
  ULONG Saddr;
  ULONG Daddr;
  ULONG Length;
  ULONG Sstride;
  ULONG Dstride;

} NODE_REGISTERS, *PNODE_REGISTERS;


//
// Node queue
//

typedef struct _NODE_QUEUE {

  PNODE_REGISTERS Head;
  PNODE_REGISTERS Tail;
  ULONG Count;

} NODE_QUEUE, *PNODE_QUEUE;


//
// Node queue manipulation
//

#define VirtToPhy(_Node, _BaseVirt, _BasePhy) ((ULONG)_BasePhy.LowPart + ((ULONG)_Node - (ULONG)_BaseVirt))


#define PhyToVirt(_Node, _BaseVirt, _BasePhy) ((ULONG)_BaseVirt + ((ULONG)_Node - (ULONG)_BasePhy.LowPart))


#define IsQueueEmpty(_Queue) ((_Queue)->Count == 0)


#define GetQueueCount(_Queue) ((_Queue)->Count)


#define InitQueue(_Queue)                           \
{                                                   \
    (_Queue)->Head = NULL;                          \
    (_Queue)->Tail = NULL;                          \
    (_Queue)->Count = 0;                            \
}


#define EnqueueNode(_Queue, _Node)                  \
{                                                   \
    if (!((_Queue)->Head))                          \
    {                                               \
        (_Queue)->Head = _Node;                     \
    }                                               \
    else                                            \
    {                                               \
        (_Queue)->Tail->NextPtr = _Node;            \
    }                                               \
                                                    \
    (_Node)->NextPtr = NULL;                        \
    (_Queue)->Tail = _Node;                         \
    (_Queue)->Count++;                              \
}


#define DequeueNode(_Queue, _Node)                  \
{                                                   \
    if ((_Queue)->Count)                            \
    {                                               \
        _Node = (_Queue)->Head;                     \
        if (!((_Queue)->Head->NextPtr))             \
        {                                           \
            (_Queue)->Tail = NULL;                  \
        }                                           \
        (_Queue)->Head = (_Queue)->Head->NextPtr;   \
        (_Queue)->Count--;                          \
    }                                               \
    else                                            \
    {                                               \
        _Node = NULL;                               \
    }                                               \
}


#define MoveQueue(_Queue1, _Queue2)                 \
{                                                   \
    (_Queue2)->Head  = (_Queue1)->Head;             \
    (_Queue2)->Tail  = (_Queue1)->Tail;             \
    (_Queue2)->Count = (_Queue1)->Count;            \
                                                    \
    InitQueue(_Queue1);                             \
}


//
// Channel pdd data structure.
//

typedef struct _PDD_CHANNEL {

  PULONG pCmdSet;
  PULONG pChanCmd;
  PULONG pNextNode;
  PULONG pSaddr;
  PULONG pDaddr;
  PULONG pChanCnt;
  PULONG pChanReqCtl;

  PVOID NodeVirtBase;
  PHYSICAL_ADDRESS NodePhyBase;
  NODE_QUEUE NodeFree;
  NODE_QUEUE NodePend;
  NODE_QUEUE NodeTransfer;
  NODE_QUEUE NodeDone;

} PDD_CHANNEL_OBJ, *PPDD_CHANNEL_OBJ;


//
// Adapter pdd data structure.
//

typedef struct _PDD_ADAPTER {

  PFDMA_INTERFACE_REGS pInterfRegs;
  PFDMA_CHANNEL_REGS pChanRegs;
  PFDMA_MAILBOX_REGS pMBRegs;

} PDD_ADAPTER_OBJ, *PPDD_ADAPTER_OBJ;

#endif __FDMA_H__
