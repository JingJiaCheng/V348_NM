/* ===========================================================================

                     CONFIDENTIAL VISTEON CORPORATION

  This is an unpublished work of authorship, which contains trade secrets,
  created in 2006.  Visteon Corporation owns all rights to this work and
  intends to maintain it in confidence to preserve its trade secret status.
  Visteon Corporation reserves the right, under the copyright laws of the
  United States or those of any other country that may have jurisdiction, to
  protect this work as an unpublished work, in the event of an inadvertent
  or deliberate unauthorized publication.  Visteon Corporation also reserves
  its rights under all copyright laws to protect this work as a published
  work, when appropriate.  Those having access to this work may not copy it,
  use it, modify it or disclose the information contained in it without the
  written authorization of Visteon Corporation.

 =========================================================================*/

	#include "CanXcp_Lcfg.h"

#include "CanIf.h" /* for CanIf symbolic name value defines */

#include "CanXcp.h" 

/* -----------------------------------------------------------------------------
    &&&~ CanXcp_PduField - 
 ----------------------------------------------------------------------------- */
#define CANXCP_START_SEC_CONST_UNSPECIFIED
/* PRQA S 5087 1 */ /* MD_MSR_19.1 */
//#include "MemMap.h"

CONST(CanXcp_XcpPduType, CANXCP_PBCFG) CanXcp_PduIdField[CANXCP_NUMBER_OF_CHANNELS] =
{
  {
    Can_Ch0_Xcp_Rx_Msg_XCP_Request, 
    Can_Ch0_Xcp_Tx_Msg_TxXCP_Response_TMH
  } /*  CanXcpRxPduId CanXcpTxPduId  */ 
}; 

/* -----------------------------------------------------------------------------
    &&&~ CanXcp_Config - 
 ----------------------------------------------------------------------------- */

CONST(CanXcp_ConfigType, CANXCP_PBCFG) CanXcp_Config = 
{
  CanXcp_PduIdField,
  NULL_PTR,
  1
};

#define CANXCP_STOP_SEC_CONST_UNSPECIFIED
/* PRQA S 5087 1 */ /* MD_MSR_19.1 */
//#include "MemMap.h"

/****************************HISTORY*****************************************/
/****************************************************************************
Date              : 11/21/2016
By                : cmurali1
Change Description: Migrated to BAIC requirements
*****************************************************************************/
