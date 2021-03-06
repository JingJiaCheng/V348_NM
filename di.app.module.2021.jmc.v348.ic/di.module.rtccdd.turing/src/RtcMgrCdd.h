//---------------------------------------------------------------------------------------------------------------------
//
// VISTEON CORPORATION CONFIDENTIAL
// ________________________________
//
// [2016] Visteon Corporation
// All Rights Reserved.
//
// NOTICE: This is an unpublished work of authorship, which contains trade secrets.
// Visteon Corporation owns all rights to this work and intends to maintain it in confidence to preserve
// its trade secret status. Visteon Corporation reserves the right, under the copyright laws of the United States
// or those of any other country that may have jurisdiction, to protect this work as an unpublished work,
// in the event of an inadvertent or deliberate unauthorized publication. Visteon Corporation also reserves its rights
// under all copyright laws to protect this work as a published work, when appropriate.
// Those having access to this work may not copy it, use it, modify it, or disclose the information contained in it
// without the written authorization of Visteon Corporation.
//
//---------------------------------------------------------------------------------------------------------------------

#ifndef RTCMGRCDD_H
#define RTCMGRCDD_H

#include "RtcIfCdd.h"
//=====================================================================================================================
//  CONSTANTS & TYPES
//=====================================================================================================================

//=====================================================================================================================
//  FORWARD DECLARATIONS
//=====================================================================================================================

//=====================================================================================================================
//  PRIVATE
//=====================================================================================================================

//=====================================================================================================================
//  PUBLIC
//=====================================================================================================================
extern Std_ReturnType RtcMgrCdd_Init(void);

extern Std_ReturnType RtcMgrCdd_DeInit(void);

extern Std_ReturnType RtcMgrCdd_Activate(void);

extern Std_ReturnType RtcMgrCdd_DeActivate(void);

extern Std_ReturnType RtcMgrCdd_SetTime(Rtc_TimeType Time);

extern Std_ReturnType RtcMgrCdd_GetTime(Rtc_TimeType* p_Time);

extern void 		  RtcMgrCdd_MainFunction(void);

#endif

