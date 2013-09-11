// All rights reserved ADENEO EMBEDDED 2010

#ifndef __PARAMS_H
#define __PARAMS_H

#include "tvp5146.h"

#define NTSC_NUM_ACTIVE_PIXELS      (640)
#define NTSC_NUM_ACTIVE_LINES       (480)
#define PAL_NUM_ACTIVE_PIXELS       (720)
#define PAL_NUM_ACTIVE_LINES        (576)

//define the resolutions
#define X_RES                   (704) 
#define Y_RES                   (576) 

#define HORZ_INFO_SPH_VAL       (202)
#define VERT_START_VAL          (20)

#define ENABLE_BT656            //using BT656 output from video decoder
#define ENABLE_PACK8                  // Do not Define this if using older App boards - 1013690, 1014473; Define if using newer app board 1017100
//#define ENABLE_STILL_IMAGE

#ifndef ENABLE_BT656
    #define ENABLE_DEINTERLANCED
#else //ENABLE_BT656
    #undef HORZ_INFO_SPH_VAL
    #define HORZ_INFO_SPH_VAL       (60)
    #define ENABLE_DEINTERLANCED
#endif //ENABLE_BT656

#ifdef INSTANT_TVP_SETTINGS
    TVP_SETTINGS tvpSettings[]={\
        {REG_INPUT_SEL, 0x00},//Composite path
        {REG_AFE_GAIN_CTRL, 0x0F},//default
        {REG_VIDEO_STD, 0x00},//default: auto mode
        {REG_OPERATION_MODE, 0x00},//normal mode
        {REG_AUTOSWITCH_MASK, 0x3F},//autoswitch mask: enable all
        {REG_COLOR_KILLER, 0x10},//default
        {REG_LUMA_CONTROL1, 0x00},//default
        {REG_LUMA_CONTROL2, 0x00},//default
        {REG_LUMA_CONTROL3, 0x00},//optimize for NTSC/ PAL 
        {REG_BRIGHTNESS, 0x80},//default
        {REG_CONTRAST, 0x80},//default
        {REG_SATURATION, 0x80},//default
        {REG_HUE, 0x00},//default
        {REG_CHROMA_CONTROL1, 0x00},//default
        {REG_CHROMA_CONTROL2, 0x04},//default
        {REG_COMP_PR_SATURATION, 0x80},//default
        {REG_COMP_Y_CONTRAST, 0x80},//default
        {REG_COMP_PB_SATURATION, 0x80},//default
        {REG_COMP_Y_BRIGHTNESS, 0x80},//default             
        //{REG_SYNC_CONTROL, 0x0C},//FID 1st field "1", 2nd field  "0"; HS, VS active high        
#ifndef ENABLE_BT656
		{REG_OUTPUT_FORMATTER1, 0x03},//ITU601 coding range, 10bit 4:2:2 separate sync
#else
		{REG_OUTPUT_FORMATTER1, 0x00},//ITU601 coding range, 10bit 4:2:2 embedded sync
#endif 
        {REG_OUTPUT_FORMATTER2, 0x11},//Enable clk & y[9:0],c[9:0] active, DATACLK OUT "rising edge "
        {REG_OUTPUT_FORMATTER3, 0x02},//Set FID as FID output
        {REG_OUTPUT_FORMATTER4, 0xAF},//default: HS, VS is output, C1~C0 is input 
        {REG_OUTPUT_FORMATTER5, 0xFF},//default: C5~C2 is input 
        {REG_OUTPUT_FORMATTER6, 0xFF},//default: C9~C6 is input 
        //{REG_CLEAR_LOST_LOCK, 0x01}//Clear status               
};

#define NUM_TVP_SETTINGS        (sizeof(tvpSettings)/sizeof(TVP_SETTINGS))

#else
    extern TVP_SETTINGS tvpSettings[];
#endif //INSTANT_TVP_SETTINGS

#endif