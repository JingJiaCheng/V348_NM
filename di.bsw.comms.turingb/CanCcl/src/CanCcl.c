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

/* ===========================================================================

  Name:           CanCcl.c

  Description:    CAN Communication Control Layer

                  Communication Layer to Support Signal/Message Based Interface

  Organization:   Multiplex Subsystems

 =========================================================================*/

/* ===========================================================================
  I N C L U D E   F I L E S
 =========================================================================*/

#include "can_defs.h"
#include "CanIl_Util.h"
#include "Rte_Can_Dispatcher.h"
#include "CanDisp.h"
#include "CanIl.h"
#include "CanSm.h"
#include "CanNm.h"
#include "CanTp.h"
#include "CanCcl.h"
#include "CanXcvr.h"
#include "CanCcl_Par_Cfg.h"
#include "Can_CSec.h"

/* ===========================================================================
  M A C R O S   D E F I N I T I O N S
 =========================================================================*/

#define CCL_UNUSED_VAR(X)  do { if(0 == (X)){} } while(0)


#if(CCL_USE_INIT_POINTER == STD_ON)
P2CONST(Ccl_ConfigType, CCL_VAR_ZERO_INIT, CCL_PBCFG) Ccl_ConfigDataPtr = NULL_PTR;
#endif

#define CCL_ENABLE_APP_RX_TX                (1U)
#define CCL_ENABLE_NM_RX_TX                 (2U)
#define CCL_ENABLE_APP_NM_RX_TX             (3U)

#define CCL_DISABLE_APP_RX_TX               (1U)
#define CCL_DISABLE_NM_RX_TX                (2U)
#define CCL_DISABLE_APP_NM_RX_TX            (3U)

/* ===========================================================================
  F I L E   S C O P E   V A R I A B L E   D E C L A R A T I O N S
 =========================================================================*/

static Ccl_StatusType CclStatus;

/**********************************************************************************************************************
  Com_Init
**********************************************************************************************************************/
/** \brief    This service initializes internal and external interfaces and variables of the CCL layer
              for the further processing. After calling this function the inter-ECU communication is still disabled.
 *  \param    config  NULL_PTR if CCL_USE_INIT_POINTER is STD_OFF
                      Pointer to the Ccl configuration data if CCL_USE_INIT_POINTER is STD_ON
    \return   none
    \pre      Com_InitMemory() has to be executed previously, if the startup code does not initialise variables.
    \context  The function must be called on task level and has not to interrupted
              by other administrative function calls.
    \note     The function is used by the Ecu Manager
    \warning  Com_Init shall not pre-empt any CCL function.
              The rest of the system must guarantee that Com_Init is not called in such a way.
**********************************************************************************************************************/
void Ccl_Init(P2CONST(Ccl_ConfigType, AUTOMATIC, CCL_INIT_DATA) config)
{
    CAN_UINT8 InitStatus  = E_OK;

    if(CCL_INIT == Com_GetStatus())
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
        #else
        InitStatus  = E_NOT_OK;
        #endif
    }
    
    CCL_UNUSED_VAR(config);

    if(InitStatus  != E_NOT_OK)
	{	    
	    CanEnterCriticalSection();

	    Ccl_InitMemory();

	    CanIf_Init(NULL);

	    CanIlInit(CAN_IL_NUM_OF_CHANNELS);

	    CanSm_Init();

	    CanNm_Init();

        CanTp_Init();

	    CanXcvr_Init();

	#ifdef CANUTIL_ENABLED

		CanUtil_Init();

	#endif

	    CanExitCriticalSection();
	}
}

# if (CCL_INIT_MEMORY_API == STD_ON)
/**********************************************************************************************************************
  Com_InitMemory
**********************************************************************************************************************/
/** \brief    The function initialises variables, which cannot be initialised with the startup code.
    \return   none
    \pre      Com_Init() is not called yet.
    \context  The function must be called on task level.
    \note     The function is used by the application.
**********************************************************************************************************************/
void Ccl_InitMemory(void)
{
    CclStatus = CCL_INIT;
}
# endif/* (CCL_INIT_MEMORY_API == STD_ON) */


# if (CCL_DE_INIT_API == STD_ON)
/**********************************************************************************************************************
  Com_DeInit
**********************************************************************************************************************/
/** \brief    This service stops the inter-ECU communication. All started I-PDU groups are stopped and have
              to be started again, if needed, after Com_Init is called. By a call to ComDeInit CCL is put into an
              not initialized state.
    \return   none
    \context  The function must be called on task level and has not to interrupted
              by other administrative function calls.
    \note     The function is used by the application.
    \warning  Com_DeInit shall not pre-empt any CCL function.
              The rest of the system must guarantee that Com_DeInit is not called in such a way.
**********************************************************************************************************************/
void Ccl_DeInit(void)
{
    CAN_UINT8 DeinitStatus  = E_OK;

    if(CCL_UNINIT == Com_GetStatus())
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
        #else
        DeinitStatus  = E_NOT_OK;
        #endif
    }
    
	if(DeinitStatus != E_NOT_OK)
	{
	    CanEnterCriticalSection();
	    
	    CclStatus = CCL_UNINIT;

	    CanExitCriticalSection();
	}
}
# endif


# if (CCL_IPDU_GROUP_CONTROL_API == STD_ON)
/**********************************************************************************************************************
  Com_IpduGroupControl
**********************************************************************************************************************/
/** \brief    This service starts I-PDU groups.
    \param    ipduGroupVector I-PDU group vector containing the activation state (stopped = 0/ started = 1)
                              for all I-PDU groups.
    \param    initialize      flag to request initialization of the I-PDUs which are newly started
    \return   none
    \context  The function must be called on task level.
**********************************************************************************************************************/
void Ccl_IpduGroupControl(Ccl_IpduGroupVector ipduGroupVector, boolean initialize)
{
	CAN_UINT8 ByteIndex;

    CAN_UINT8 FuncExecSt = E_OK;

    if(CCL_UNINIT == Com_GetStatus())
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
        #else
        FuncExecSt = E_NOT_OK;
        #endif
    }

    if(FuncExecSt != E_NOT_OK)
	{
	    for(ByteIndex = 0; ByteIndex < CCL_MAX_IPDUGROUPVECTOR_BYTES; ByteIndex++)
		{
		    if((ipduGroupVector[ByteIndex] & Can_Ccl_PduGrpVector[CanCclConf_IPduGroup_TuringECU_CAN_Rx]) != (uint8) 0x00U)
		    {
			    if(initialize == TRUE)
				{
			        CanIlSetIlMode( Can_ControllerId, ILSTATUS, IL_STATUS_RESUME_ALL, 0, TRUE );
					CanIlSetIlMode( Can_ControllerId, ILRXSTATUS, IL_RX_STATUS_TIMEOUT_MONITOR, Can_Ccl_Rx_Num_Messages, TRUE );
				}
				else
				{
				    CanIlSetIlMode( Can_ControllerId, ILSTATUS, IL_STATUS_SUSPEND, 0, FALSE );
				}
			}

			if((ipduGroupVector[ByteIndex] & Can_Ccl_PduGrpVector[CanCclConf_IPduGroup_TuringECU_CAN_Tx]) != (uint8) 0x00U)
			{
			    if(initialize == TRUE)
				{
				    CanIlSetIlMode( Can_ControllerId, ILSTATUS, IL_STATUS_RESUME_ALL, Can_Ccl_Tx_Num_Messages, FALSE );
                    CanIlSetIlMode( Can_ControllerId, ILTXSTATUS, 0, Can_Ccl_Tx_Num_Messages, TRUE );
				}
				else
				{
			        CanIlSetIlMode( Can_ControllerId, ILTXSTATUS, IL_STATUS_TX_SUSPEND, Can_Ccl_Tx_Num_Messages, TRUE );
				}
			}
		}
	}

}
# endif/* (CCL_IPDU_GROUP_CONTROL_API == STD_ON) */


# if (CCL_RECEPTION_DM_CONTROL_API == STD_ON )
/**********************************************************************************************************************
  Com_ReceptionDMControl
**********************************************************************************************************************/
/** \brief    This service enables or disables I-PDU group Deadline Monitoring.
    \param    ipduGroupVector I-PDU group vector containing the requested deadline monitoring state
                              (disabled = 0/ enabled = 1) for all I-PDU groups.
    \return   none
    \context  The function must be called on task level.
**********************************************************************************************************************/
void Ccl_ReceptionDMControl(Ccl_IpduGroupVector ipduGroupVector)
{
    CAN_UINT8 ByteIndex;

	CAN_UINT8 FuncExecSt = E_OK;

    if(CCL_UNINIT == Com_GetStatus())
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
        #else
        FuncExecSt = E_NOT_OK;
        #endif
    }

    if(FuncExecSt != E_NOT_OK)
	{
	    for(ByteIndex = 0; ByteIndex < CCL_MAX_IPDUGROUPVECTOR_BYTES; ByteIndex++)
		{
		    if((ipduGroupVector[ByteIndex] & Can_Ccl_PduGrpVector[CanCclConf_IPduGroup_TuringECU_CAN_Rx]) != (uint8) 0x00U)
		    {
			    CanIlSetIlMode( Can_ControllerId, ILRXSTATUS, IL_RX_STATUS_TIMEOUT_MONITOR, Can_Ccl_Rx_Num_Messages, TRUE );
			}
			else
			{
			    CanIlSetIlMode( Can_ControllerId, ILRXSTATUS, IL_RX_STATUS_TIMEOUT_MONITOR, Can_Ccl_Rx_Num_Messages, FALSE );
			}
		}
	}
}
# endif /* (CCL_RECEPTION_DM_CONTROL_API == STD_ON ) */


/**********************************************************************************************************************
  Com_GetStatus
**********************************************************************************************************************/
/** \brief    Returns the status of the CCL module.
    \return   Com_StatusType
    \context  The function can be called on interrupt and task level.
**********************************************************************************************************************/
# if (CCL_GET_STATUS_API == STD_ON )
Ccl_StatusType Ccl_GetStatus( void )
{
    return(CclStatus);
}
# endif


# if(CCL_IPDU_GROUP_CONTROL_API == STD_ON)
/**********************************************************************************************************************
  Com_ClearIpduGroupVector
**********************************************************************************************************************/
/** \brief    This service sets all bits of the given Ccl_IpduGroupVector to 0.
    \param    ipduGroupVector I-PDU group vector to be cleared
    \return   none
    \context  The function must be called on task level.
**********************************************************************************************************************/
void Ccl_ClearIpduGroupVector(Ccl_IpduGroupVector ipduGroupVector)
{
    CAN_UINT8 ByteIndex;

	CAN_UINT8 FuncExecSt = E_OK;

    if(CCL_UNINIT == Com_GetStatus())
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
        #else
        FuncExecSt = E_NOT_OK;
        #endif
    }

    if(FuncExecSt != E_NOT_OK)
	{
	    for(ByteIndex = 0; ByteIndex < CCL_MAX_IPDUGROUPVECTOR_BYTES; ByteIndex++)
		{
	        (ipduGroupVector)[ByteIndex] = 0x00;
	    }
	}
}
# endif /* (CCL_IPDU_GROUP_CONTROL_API == STD_ON) */


# if(CCL_IPDU_GROUP_CONTROL_API == STD_ON)
/**********************************************************************************************************************
  Com_SetIpduGroup
**********************************************************************************************************************/
/** \brief    This service sets the value of a bit in an I-PDU group vector.
    \param    ipduGroupVector I-PDU group vector to be modified
    \param    ipduGroupId     ID used to identify the corresponding bit in the I-PDU group vector
    \param    bitval          New value of the corresponding bit
    \return   none
    \context  The function must be called on task level.
**********************************************************************************************************************/
void Ccl_SetIpduGroup(Ccl_IpduGroupVector ipduGroupVector,Ccl_IpduGroupIdType ipduGroupId,boolean bitval)
{
    CAN_UINT8 FuncExecSt = E_OK;

    if(CCL_UNINIT == Com_GetStatus())
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
        #else
        FuncExecSt = E_NOT_OK;
        #endif
    }
    
    if(FuncExecSt != E_NOT_OK)
	{
	    if(((ipduGroupId) >> 3) < CCL_MAX_IPDUGROUPVECTOR_BYTES)
	    {
		    if(FALSE == (bitval))
		    {
		      (ipduGroupVector)[(ipduGroupId) >> 3] &=
		        (uint8) (~((CAN_UINT8) (0x01U << ((ipduGroupId) & (Ccl_IpduGroupIdType) 0x0007U))));
		    }
		    else
		    {
		      (ipduGroupVector)[(ipduGroupId) >> 3] |= (CAN_UINT8) (0x01U << ((ipduGroupId) & (Ccl_IpduGroupIdType) 0x0007U));
		    }
		}
	}
}
# endif /* (CCL_IPDU_GROUP_CONTROL_API == STD_ON) */


# if (CCL_SEND_DYN_SIGNAL_API == STD_ON)
/**********************************************************************************************************************
  Com_SendDynSignal
**********************************************************************************************************************/
/** \brief    The service Com_SendDynSignal updates the signal or group signal object identified by SignalId
              with the signal data referenced by the SignalDataPtr parameter. The Length parameter is evaluated for
              dynamic length signals.
    \param    SignalId      ID of signal or group signal to be sent.
    \param    SignalDataPtr Reference to the signal data to be transmitted.
    \param    Length        Length of the dynamic length signal.
    \return   uint8
              E_OK                        service has been accepted
              CCL_SERVICE_NOT_AVAILABLE   corresponding I-PDU group was stopped
                                          (or service failed due to development error)
              CCL_BUSY                    in case the TP-Buffer is locked for large data types handling
    \context  The function can be called on interrupt and task level and has not to be interrupted
              by other Com_SendSignal and Com_InvalidateSignal calls for the same SignalId.
    \note     The function is called by the upper layer.
    \warning  If the method is used on a microcontroller like the Tms320 DSP and the datatype uint8 is unsigned short
              the 8 MSB bits of the variable shall never be set.
              If the method is used on a microcontroller like the S12X and the datatype is uint16, sint16, uint32
              or sint32 the SignalDataPtr must be word aligned.
**********************************************************************************************************************/
uint8 Ccl_SendDynSignal(Ccl_SignalIdType SignalId, P2CONST(void, AUTOMATIC, CCL_APPL_DATA) SignalDataPtr, uint16 Length)
{
	CAN_HWINST hwInst;
    CAN_UINT8 SetSignalStatus = E_OK;
    Ccl_TxSigInfoType const * pCclTxSigInfo;
    CAN_UINT8 TxSignalData[CAN_MAX_DATA_LENGTH];
    CAN_UINT8 ByteIndex;
    CAN_UINT8 DataMatch = CAN_TRUE;
    CAN_UINT8 DataLen = 0;

    if(Can_Ccl_Tx_Signal_Num_Signals <= SignalId)
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
        #else
        SetSignalStatus = CCL_SERVICE_NOT_AVAILABLE;
        #endif
    }

    if(NULL == SignalDataPtr)
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
        #else
        SetSignalStatus = CCL_SERVICE_NOT_AVAILABLE;
        #endif
    }

    if(CCL_UNINIT == Com_GetStatus())
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
        #else
        SetSignalStatus = CCL_SERVICE_NOT_AVAILABLE;
        #endif
    }

    if(SetSignalStatus != CCL_SERVICE_NOT_AVAILABLE)
    {
	    pCclTxSigInfo = &Can_Ccl_TxSigInfo[SignalId];
	    hwInst = pCclTxSigInfo->ControllerIdOfMessage;

        if(pCclTxSigInfo->SendTypeOfTxSignal == SIG_ONWRITE)
        {
            SetSignalStatus = CanIlTxPutSignal( SignalId, (CAN_UINT8 *)SignalDataPtr, Length, hwInst );
        }
        else if(pCclTxSigInfo->SendTypeOfTxSignal == SIG_ONCHANGE)
        {
		    for(ByteIndex = 0; ByteIndex < CAN_MAX_DATA_LENGTH; ByteIndex++)
            {
                TxSignalData[ByteIndex] = 0x00; /* Coverity fix */
            }
            
		    (void)CanIlTxGetSignal( SignalId, TxSignalData, &DataLen, hwInst );
		    for(ByteIndex = 0; ByteIndex < DataLen; ByteIndex++)
			{
			    if(TxSignalData[ByteIndex] != ((CAN_UINT8 *)SignalDataPtr)[ByteIndex])
				{
				    DataMatch = CAN_FALSE;
					break;
				}
			}

		    if( DataMatch == CAN_FALSE )
			{
		        SetSignalStatus = CanIlTxPutSignal( SignalId, (CAN_UINT8 *)SignalDataPtr, Length, hwInst );
			}
        }
        else
        {
            SetSignalStatus = CanIlTxSetSignal( SignalId, SignalDataPtr, Length, hwInst );
        }
    }

    return(SetSignalStatus);
}
# endif/* (CCL_SEND_DYN_SIGNAL_API == STD_ON) */


# if (CCL_SEND_SIGNAL_API == STD_ON)
/**********************************************************************************************************************
  Com_SendSignal
**********************************************************************************************************************/
/** \brief    The service Com_SendSignal updates the signal or group signal object identified by SignalId
              with the signal data referenced by the SignalDataPtr parameter.
              For signals with a bit-size equal to 0, SignalDataPtr can be passed as NULL_PTR.
    \param    SignalId      ID of signal or group signal to be sent.
    \param    SignalDataPtr Reference to the signal data to be transmitted.
    \return   uint8
              E_OK                        service has been accepted
              CCL_SERVICE_NOT_AVAILABLE   corresponding I-PDU group was stopped
                                          (or service failed due to development error)
              CCL_BUSY                    in case the TP-Buffer is locked for large data types handling
    \context  The function can be called on interrupt and task level and has not to be interrupted
              by other Com_SendSignal and Com_InvalidateSignal calls for the same SignalId.
    \note     The function is called by the upper layer.
    \warning  If the method is used on a microcontroller like the Tms320 DSP and the datatype uint8 is unsigned short
              the 8 MSB bits of the variable shall never be set.
              If the method is used on a microcontroller like the S12X and the datatype is uint16, sint16, uint32
              or sint32 the SignalDataPtr must be word aligned.
**********************************************************************************************************************/
uint8 Ccl_SendSignal( Ccl_SignalIdType SignalId, const void* SignalDataPtr )
{
    uint8 SetSignalStatus;
    
    SetSignalStatus = Ccl_SendDynSignal( SignalId, SignalDataPtr, 0 );

    return(SetSignalStatus);
}

# endif/* (CCL_SEND_SIGNAL_API == STD_ON) */


# if (CCL_RECEIVE_DYN_SIGNAL_API == STD_ON)
/**********************************************************************************************************************
  Com_ReceiveDynSignal
**********************************************************************************************************************/
/** \brief    The service Com_ReceiveDynSignal updates the signal data referenced by SignalDataPtr
              with the data in the signal object identified by SignalId. The Length parameter indicates as
              "in parameter" the maximum length that can be received and as "out parameter" the length of the written
              dynamic length signal or group signal.
              If the signal processing of the corresponding I-Pdu is configured to DEFERRED
              the last received signal value is available not until the next call to Com_MainfunctionRx.
              If a group signal is read, the data in the shadow buffer should be updated before the call
              by a call of the service Com_ReceiveSignalGroup.
    \param    SignalId                    Id of signal or group signal to be received.
    \param    SignalDataPtr               Reference to the signal data in which to store the received data.
    \param    Length                      in: maximum length that could be received
                                          out: length of the dynamic length signal
    \return   uint8
              E_OK                        service has been accepted
              E_NOT_OK                    in case the Length (as in-parameter) is smaller than
                                          the received length of the dynamic length signal
              CCL_SERVICE_NOT_AVAILABLE   corresponding I-PDU group was stopped
                                          (or service failed due to development error)
              CCL_BUSY                    in case the TP-Buffer is locked
    \context  The function can be called on interrupt and task level.
    \note     The function is called by the upper layer.
**********************************************************************************************************************/
uint8 Ccl_ReceiveDynSignal(Ccl_SignalIdType SignalId, P2VAR(void, AUTOMATIC, CCL_APPL_VAR) SignalDataPtr, P2VAR(uint16, AUTOMATIC,CCL_APPL_VAR) Length)
{
	CAN_HWINST hwInst;

    CAN_UINT8 GetSignalStatus = E_OK;

    Ccl_RxAccessInfoType const * pCclRxAccessInfo;


    if(Can_Ccl_Rx_Num_Signals <= SignalId)
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
		#else
		GetSignalStatus = CCL_SERVICE_NOT_AVAILABLE;
        #endif
    }

    if(NULL == SignalDataPtr)
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
		#else
		GetSignalStatus = CCL_SERVICE_NOT_AVAILABLE;
        #endif
    }

    if(GetSignalStatus != CCL_SERVICE_NOT_AVAILABLE)
	{
	    pCclRxAccessInfo = &Can_Ccl_RxAccessInfo[SignalId];

	    hwInst = pCclRxAccessInfo->ControllerIdOfMessage;

	    GetSignalStatus = CanIlRxGetSignal( SignalId, (CAN_UINT8 *)SignalDataPtr, (CAN_UINT8) *Length, hwInst );

	    if(TRUE == GetSignalStatus)
	    {
	        GetSignalStatus = E_OK;
	    }
	    else
	    {
	        GetSignalStatus = CCL_SERVICE_NOT_AVAILABLE;
	    }
	}

    return(GetSignalStatus);
}
# endif/* (CCL_RECEIVE_DYN_SIGNAL_API == STD_ON) */


# if (CCL_RECEIVE_SIGNAL_API == STD_ON)
/**********************************************************************************************************************
  Com_ReceiveSignal
**********************************************************************************************************************/
/** \brief    The service Com_ReceiveSignal updates the signal data referenced by SignalDataPtr
              with the data in the signal object identified by SignalId.
              If the signal processing of the corresponding I-Pdu is configured to DEFERRED
              the last received signal value is available not until the next call to Com_MainfunctionRx.
              If a group signal is read, the data in the shadow buffer should be updated before the call
              by a call of the service Com_ReceiveSignalGroup.
              For signals with a bit-size equal to 0, SignalDataPtr can be passed as NULL_PTR.
    \param    SignalId                    Id of signal or group signal to be received.
    \param    SignalDataPtr               Reference to the signal data in which to store the received data.
    \return   uint8
              E_OK                        service has been accepted
              CCL_SERVICE_NOT_AVAILABLE   corresponding I-PDU group was stopped
                                          (or service failed due to development error)
    \context  The function can be called on interrupt and task level.
    \note     The function is called by the upper layer.
**********************************************************************************************************************/
uint8 Ccl_ReceiveSignal( Ccl_SignalIdType SignalId, P2VAR(void, AUTOMATIC, CCL_APPL_VAR) SignalDataPtr)
{
    CAN_UINT8 GetSignalStatus = E_OK;

    CAN_UINT16 Length = 0;

    if(Can_Ccl_Rx_Num_Signals <= SignalId)
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
		#else
		GetSignalStatus = CCL_SERVICE_NOT_AVAILABLE;
        #endif
    }

    if(NULL == SignalDataPtr)
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
		#else
		GetSignalStatus = CCL_SERVICE_NOT_AVAILABLE;
        #endif
    }
	
	if(GetSignalStatus != CCL_SERVICE_NOT_AVAILABLE)
	{
	    GetSignalStatus = Ccl_ReceiveDynSignal( SignalId, SignalDataPtr, &Length );
	}

	return(GetSignalStatus);
}
# endif/* (CCL_RECEIVE_SIGNAL_API == STD_ON) */


# if (CCL_UPDATE_SHADOW_SIGNAL_API == STD_ON)
/**********************************************************************************************************************
  Com_UpdateShadowSignal
**********************************************************************************************************************/
/** \brief    The service Com_UpdateShadowSignal updates a group signal with the data, referenced by SignalDataPtr.
              The update of the group signal data is done in the shadow buffer, not in the I-PDU.
              To send out the shadow buffer, Com_SendSignalGroup has to be called.
              For signals with a bit-size equal to 0, SignalDataPtr can be passed as NULL_PTR.
    \param    SignalId      ID of group signal to be updated.
    \param    SignalDataPtr Reference to the group signal data to be updated.
    \return   void
    \context  The function can be called on interrupt and task level.
    \note     The function is called by the upper layer.
    \warning  If the method is used on a microcontroller like the S12X and the datatype is uint16, sint16, uint32
              or sint32 the SignalDataPtr must be word aligned.
    \deprecated This function is deprecated. Use 'Com_SendSignal' instead to send group signals.
**********************************************************************************************************************/
void Ccl_UpdateShadowSignal(Ccl_SignalIdType SignalId, const void* SignalDataPtr)
{

    CCL_UNUSED_VAR(SignalId);
    CCL_UNUSED_VAR(SignalDataPtr);

}
# endif/* (CCL_UPDATE_SHADOW_SIGNAL_API == STD_ON) */


# if(CCL_SEND_SIGNAL_GROUP_API == STD_ON)
/**********************************************************************************************************************
  Com_SendSignalGroup
**********************************************************************************************************************/
/** \brief    The service Com_SendSignalGroup copies the content of the associated shadow buffer to
              the associated I-PDU buffer.
              Prior to this call, all group signals should be updated in the shadow buffer
              by the call of Com_SendSignal.
    \param    SignalGroupId      ID of signal group to be send.
    \return   uint8
              E_OK                        service has been accepted
              CCL_SERVICE_NOT_AVAILABLE   corresponding I-PDU group was stopped
              (or service failed due to development error)
    \context  The function can be called on interrupt and task level and has not to be interrupted
              by other Com_SendSignalGroup calls for the same SignalGroupId.
              To guarantee data consistency of the whole signal group the complete transmission of a signal group
              (consecutive calls of 'Com_SendSignal' and 'Com_SendSignalGroup') must not be interrupted
              by another transmission request for the same signal group or by a call of 'Com_InvalidateSignalGroup'.
    \note     The function is called by the upper layer.
**********************************************************************************************************************/
uint8 Ccl_SendSignalGroup(Ccl_SignalGroupIdType SignalGroupId)
{
	uint8 retStatus = E_OK;

    CCL_UNUSED_VAR(SignalGroupId);

    return(retStatus);
}
# endif/* (CCL_SEND_SIGNAL_GROUP_API == STD_ON) */


# if (CCL_SEND_SIGNAL_GROUP_ARRAY_API == STD_ON)
/**********************************************************************************************************************
  Com_SendSignalGroupArray
**********************************************************************************************************************/
/** \brief    The service Com_SendSignalGroupArray copies the content of the associated shadow buffer to
              the associated I-PDU buffer array.
              Prior to this call, all group signals should be updated in the shadow buffer
              by the call of Com_SendSignal.
    \param    SignalGroupId                 ID of signal group to be send.
    \param    SignalGroupArrayPtr           Reference to the group signal data array in which to store the received data.
    \return   uint8
              E_OK                        service has been accepted
              CCL_SERVICE_NOT_AVAILABLE   corresponding I-PDU group was stopped
              (or service failed due to development error)
    \context  The function can be called on interrupt and task level and has not to be interrupted
              by other Com_SendSignalGroupArray calls for the same SignalGroupId.
              To guarantee data consistency of the whole signal group the complete transmission of a signal group
              (consecutive calls of 'Com_SendSignal', 'Com_SendSignalGroup' and 'Com_SendSignalGroupArray') must not be interrupted
              by another transmission request for the same signal group or by a call of 'Com_InvalidateSignalGroup'.
    \note     The function is called by the upper layer.
**********************************************************************************************************************/
uint8 Ccl_SendSignalGroupArray( Ccl_SignalGroupIdType SignalGroupId, const uint8* SignalGroupArrayPtr )
{
	uint8 retStatus = E_OK;

    CCL_UNUSED_VAR(SignalGroupId);
	CCL_UNUSED_VAR(SignalGroupArrayPtr);

    return(retStatus);
}
# endif/* (CCL_SEND_SIGNAL_GROUP_ARRAY_API == STD_ON) */


# if (CCL_RECEIVE_SIGNAL_GROUP_API == STD_ON)
/**********************************************************************************************************************
  Com_ReceiveSignalGroup
**********************************************************************************************************************/
/** \brief    The service Com_ReceiveSignalGroup copies the received signal group to the shadow buffer.
              After this call, the group signals could be copied from the shadow buffer to the upper layer by
              a call of Com_ReceiveShadowSignal.
    \param    SignalGroupId               Id of signal group to be received.
    \return   uint8
              E_OK                        service has been accepted
              CCL_SERVICE_NOT_AVAILABLE   corresponding I-PDU group was stopped
                                          (or service failed due to development error)
    \context  The function can be called on interrupt and task level.
              To guarantee data consistency of the whole signal group the complete reception of a signal group
              (consecutive calls of 'Com_ReceiveSignalGroup' and 'Com_ReceiveSignal')
              must not be interrupted by another reception request for the same signal group.
    \note     The function is called by the upper layer.
**********************************************************************************************************************/
uint8 Ccl_ReceiveSignalGroup(Ccl_SignalGroupIdType SignalGroupId)
{
	uint8 retStatus = E_OK;

    CCL_UNUSED_VAR(SignalGroupId);

    return(retStatus);
}
# endif/* (CCL_RECEIVE_SIGNAL_GROUP_API == STD_ON) */


# if (CCL_RECEIVE_SHADOW_SIGNAL_API == STD_ON)
/**********************************************************************************************************************
  Com_ReceiveShadowSignal
**********************************************************************************************************************/
/** \brief    The service Com_ReceiveShadowSignal updates the group signal data referenced by SignalDataPtr
              with the data in the shadow buffer.
              The data in the shadow buffer should be updated before the call of Com_ReceiveShadowSignal
              by a call of the service Com_ReceiveSignalGroup.
              For signals with a bit-size equal to 0, SignalDataPtr can be passed as NULL_PTR.
    \param    SignalIdType                Id of group signal to be received.
    \param    SignalDataPtr               Reference to the group signal data in which to store the received data.
    \return   void
    \context  The function can be called on interrupt and task level.
    \note     The function is called by the upper layer.
    \deprecated This function is deprecated. Use 'Com_ReceiveSignal' instead to read group signals.
**********************************************************************************************************************/
uint8 Ccl_ReceiveShadowSignal(Ccl_SignalIdType SignalId, const void* SignalDataPtr)
{
	uint8 retStatus = E_OK;

    CCL_UNUSED_VAR(SignalId);
	CCL_UNUSED_VAR(SignalDataPtr);

    return(retStatus);
}
# endif/* (CCL_RECEIVE_SHADOW_SIGNAL_API == STD_ON) */


# if (CCL_INVALIDATE_SIGNAL_API == STD_ON)
/**********************************************************************************************************************
  Com_InvalidateSignal
**********************************************************************************************************************/
/** \brief    This function invalidates the signal or group signal by calling Com_SendSignal
              with the configured invalid value.
              If this function is used to invalidate a group signal, a call to Com_SendSignalGroup is needed
              to update the signal group data.

    \param    SignalId      ID of signal or group signal to be invalidated.
    \return   uint8
              E_OK                        service has been accepted
              CCL_SERVICE_NOT_AVAILABLE   corresponding I-PDU group was stopped
                                          (or service failed due to development error)
    \context  The function can be called on interrupt and task level and has not to be interrupted
              by other Com_SendSignal and Com_InvalidateSignal calls for the same SignalId.
    \note     The function is called by the upper layer.
**********************************************************************************************************************/
uint8 Ccl_InvalidateSignal(Ccl_SignalIdType SignalId)
{
    CAN_UINT8 InvalidationSt = E_OK;

    CCL_UNUSED_VAR(SignalId);

    return(InvalidationSt);
}
# endif/* (CCL_INVALIDATE_SIGNAL_API == STD_ON) */


# if (CCL_INVALIDATE_SIGNAL_GROUP_API == STD_ON)
/**********************************************************************************************************************
  Com_InvalidateSignalGroup
**********************************************************************************************************************/
/** \brief    This function invalidates the whole signal group by calling Com_SendSignal
              with the configured invalid value for all group signals of the signal group.
              After invalidation of the current signal group data Com_SendSignalGroup is performed internally.

    \param    SignalGroupId      ID of signal group to be invalidated.
    \return   uint8
              E_OK                        service has been accepted
              CCL_SERVICE_NOT_AVAILABLE   corresponding I-PDU group was stopped
              (or service failed due to development error)
    \context  The function can be called on interrupt and task level and has not to be interrupted
              by other Com_InvalidateSignalGroup calls for the same SignalGroupId and
              by Com_SendSignal calls for a SignalId which is contained in the same signal group.
    \note     The function is called by the upper layer.
**********************************************************************************************************************/
uint8 Ccl_InvalidateSignalGroup(Ccl_SignalGroupIdType SignalGroupId)
{
	uint8 retStatus = E_OK;

    CCL_UNUSED_VAR(SignalGroupId);

    return(retStatus);
}
# endif/* (CCL_INVALIDATE_SIGNAL_GROUP_API == STD_ON) */


# if (CCL_TRIGGER_IPDU_SEND_API == STD_ON)
/**********************************************************************************************************************
  Com_TriggerIPDUSend
**********************************************************************************************************************/
/** \brief    By a call to Com_TriggerIPDUSend the I-PDU with the given ID is triggered for transmission
    \param    PduId    ID of CCL Tx I-PDU.
    \return   void
    \context  The function can be called on interrupt and task level.
    \note     The function shall only be called from within an I-PDU callout.
**********************************************************************************************************************/
void Ccl_TriggerIPDUSend(PduIdType PduId)
{
    CCL_UNUSED_VAR(PduId);
}
# endif /* (CCL_TRIGGER_IPDU_SEND_API == STD_ON) */


# if (CCL_TRIGGER_IPDU_SEND_WITH_META_DATA_API == STD_ON)
/**********************************************************************************************************************
  Com_TriggerIPDUSendWithMetaData
**********************************************************************************************************************/
/** \brief    By a call to Com_TriggerIPDUSendWithMetaData the given meta data is appended to the I-PDU and the I-PDU
              with the given ID is triggered for transmission
    \param    PduId    ID of CCL Tx I-PDU.
    \param    MetaData The Meta data that shall be added to the I-PDU before sending.
    \return   void
    \context  The function can be called on interrupt and task level.
    \note     The function shall only be called from within an I-PDU callout.
**********************************************************************************************************************/
void Ccl_TriggerIPDUSendWithMetaData(PduIdType PduId, P2CONST(uint8, AUTOMATIC, CCL_APPL_DATA) MetaData)
{
}
# endif /* (CCL_TRIGGER_IPDU_SEND_WITH_META_DATA_API == STD_ON) */

# if (CCL_SWITCH_IPDU_TX_MODE_API == STD_ON)
/**********************************************************************************************************************
  Com_SwitchIpduTxMode
**********************************************************************************************************************/
/** \brief    This method sets the TX Mode of the I-PDU referenced by PduId to Mode.
              In case the TX Mode changes the new mode is immediately effective.
              In case the requested transmission mode was already active for this I-PDU, the call will have no effect.
    \param    PduId    ID of CCL Tx I-PDU.
    \param    Mode     TX mode of the I-PDU (TRUE/FALSE)
    \return   none
    \context  The function can be called on interrupt and task level
    \note     The function is used by the BSW Manager.
**********************************************************************************************************************/
void Ccl_SwitchIpduTxMode(PduIdType PduId, boolean Mode)
{
}
# endif /* (CCL_SWITCH_IPDU_TX_MODE_API == STD_ON) */


/*********************************************************************************************************************/
/** Callback Functions & Notifications */
/*********************************************************************************************************************/

#if (CCL_TRIGGER_TRANSMIT_API == STD_ON)
/**********************************************************************************************************************
  Com_TriggerTransmit
**********************************************************************************************************************/
/** \brief    This method copies the contents of its I-PDU transmit buffer to the L-PDU buffer given by PduInfoPtr->SduDataPtr 
              and update PduInfoPtr->SduLength with length of the copied data accordingly
              The function Com_TriggerTransmit behaves independent of the configured transmission mode.
    \param    TxPduId        ID of the SDU that is requested to be transmitted.
    \param    PduInfoPtr     A pointer to a Tx buffer (SduDataPtr) to where the SDU data shall be copied.
    \return   Std_ReturnType
              E_OK           SDU has been copied and SduLength indicates the number of copied bytes. 
              E_NOT_OK       No SDU data has been copied. 
                             PduInfoPtr must not be used since it may contain a NULL pointer or point to invalid data.
    \context  The function can be called on interrupt and task level
    \note     The function is called by the upper layer.
**********************************************************************************************************************/

Std_ReturnType Ccl_TriggerTransmit(PduIdType TxPduId, P2VAR(PduInfoType, AUTOMATIC, CCL_APPL_VAR) PduInfoPtr)
{
}
# endif /* (CCL_TRIGGER_TRANSMIT_API == STD_ON) */

/**********************************************************************************************************************
  Ccl_RxIndication
**********************************************************************************************************************/
/** \brief    This function is called by the lower layer after an I-PDU has been received.
    \param    RxPduId         ID of CCL I-PDU that has been received. Identifies the data that has been received.
                              Range: 0..(maximum number of I-PDU IDs received by CCL) - 1
    \param    PduInfoPtr      Payload information of the received I-PDU (pointer to data and data length).
    \return   none
    \context  The function can be called on interrupt and task level. It is not allowed to use CAT1 interrupts with Rte (BSW00326]). 
              Due to this, the interrupt context shall be configured to a CAT2 interrupt, if an Rte is used. 
    \note     The function is called by the lower layer.
**********************************************************************************************************************/
void Ccl_RxIndication(PduIdType RxPduId)
{

    CclCbkRxAckType pCclCbkRxAckFuncPtr;

    Ccl_MessageContentInfo const * pNumOfContainedRxSignals;

    Ccl_RxNotifFuncInfo const * pCclRxNotifFuncPtr;

    CAN_UINT8 NumOfMsgSignals;

    CAN_UINT16 SignalHandleStart;

    CAN_UINT16 CanSignalIndex;

    CAN_UINT16 RxAckSignalIndex;

    CAN_UINT8 FuncExecSt = E_OK;


    if(Can_Ccl_Rx_Num_Messages <= RxPduId)
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
		#else
		FuncExecSt = E_NOT_OK;
        #endif
    }

    if(FuncExecSt != E_NOT_OK)
	{
	    pNumOfContainedRxSignals = &Can_Ccl_NumOfContainedRxSignals[RxPduId];

	    SignalHandleStart = pNumOfContainedRxSignals->SignalHandleStartIndexOfPduInfo;

	    NumOfMsgSignals   = pNumOfContainedRxSignals->SignalCountOfPduInfo;

	    for( CanSignalIndex = SignalHandleStart; CanSignalIndex < (SignalHandleStart + NumOfMsgSignals); CanSignalIndex++ )
	    {
	        pCclRxNotifFuncPtr = &Can_Ccl_RxNotifFuncPtr[CanSignalIndex];

	        if(CCL_NO_CBKRXACK != pCclRxNotifFuncPtr->RxAckIndexInComCbkInfo)
	        {
	            RxAckSignalIndex = pCclRxNotifFuncPtr->RxAckIndexInComCbkInfo;

	            pCclCbkRxAckFuncPtr = Can_Ccl_CbkRxAckFuncPtr[RxAckSignalIndex];

	            if(NULL != pCclCbkRxAckFuncPtr)
	            {
	                (pCclCbkRxAckFuncPtr)();
	            }
	        }
	        else
	        {
	        }
	    }
	}

}


/**********************************************************************************************************************
  Ccl_RxToutIndication
**********************************************************************************************************************/
/** \brief    This function is called by the lower layer after an I-PDU has been missed and timeout.
    \param    RxPduId         ID of CCL I-PDU whose reception has been timedout.
                              Range: 0..(maximum number of I-PDU IDs received by CCL) - 1
    \return   none
    \context  The function can be called on interrupt and task level. It is not allowed to use CAT1 interrupts with Rte (BSW00326]). 
              Due to this, the interrupt context shall be configured to a CAT2 interrupt, if an Rte is used. 
    \note     The function is called by the lower layer.
**********************************************************************************************************************/
void Ccl_RxToutIndication(PduIdType RxPduId)
{

    CclCbkRxTOutType pCclCbkRxTOutFuncPtr;

    Ccl_MessageContentInfo const * pNumOfContainedRxSignals;

    Ccl_RxNotifFuncInfo const * pCclRxNotifFuncPtr;

    CAN_UINT8 NumOfMsgSignals;

    CAN_UINT16 SignalHandleStart;

    CAN_UINT16 CanSignalIndex;

    CAN_UINT16 RxTOutSignalIndex;

	CAN_UINT8 FuncExecSt = E_OK;


    if(Can_Ccl_Rx_Num_Messages <= RxPduId)
    {
        #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
		#else
		FuncExecSt = E_NOT_OK;
        #endif
    }

    if(FuncExecSt != E_NOT_OK)
	{
	    pNumOfContainedRxSignals = &Can_Ccl_NumOfContainedRxSignals[RxPduId];

	    SignalHandleStart = pNumOfContainedRxSignals->SignalHandleStartIndexOfPduInfo;
	    NumOfMsgSignals   = pNumOfContainedRxSignals->SignalCountOfPduInfo;

	    for( CanSignalIndex = SignalHandleStart; CanSignalIndex < (SignalHandleStart + NumOfMsgSignals); CanSignalIndex++ )
	    {
	        pCclRxNotifFuncPtr = &Can_Ccl_RxNotifFuncPtr[CanSignalIndex];

	        if(CCL_NO_CBKRXACK != pCclRxNotifFuncPtr->RxTOutIndexInComCbkInfo)
	        {
	            RxTOutSignalIndex = pCclRxNotifFuncPtr->RxTOutIndexInComCbkInfo;

	            pCclCbkRxTOutFuncPtr = Can_Ccl_CbkRxTOutFuncPtr[RxTOutSignalIndex];

	            if(NULL != pCclCbkRxTOutFuncPtr)
	            {
	                (pCclCbkRxTOutFuncPtr)();
	            }
	        }
	        else
	        {
	        }
	    }
	}

}

# if (CCL_TP_RXINDICATION_API == STD_ON)
/**********************************************************************************************************************
  Ccl_TpRxIndication
**********************************************************************************************************************/
/** \brief    This function is called after an I-PDU has been received via the TP API.
    \param    PduId         Identification of the received I-PDU.
    \param    Result        Result of the reception.
    \return   none
    \context  The function can be called on interrupt and task level. It is not allowed to use CAT1 interrupts with Rte (BSW00326]). 
              Due to this, the interrupt context shall be configured to a CAT2 interrupt, if an Rte is used. 
    \note     The function is called by the lower layer.
**********************************************************************************************************************/
void Ccl_TpRxIndication(PduIdType PduId, Std_ReturnType Result)
{
}
# endif /* (CCL_TP_RXINDICATION_API == STD_ON) */

/**********************************************************************************************************************
  Ccl_TxConfirmation
**********************************************************************************************************************/
/** \brief    This function is called by the lower layer after the PDU has been transmitted on the network.
              A confirmation that is received for an I-PDU that does not require a confirmation is silently discarded.
    \param    TxPduId    ID of CCL I-PDU that has been transmitted.
                         Range: 0..(maximum number of I-PDU IDs transmitted by CCL) - 1
    \return   none
    \context  The function can be called on interrupt and task level. It is not allowed to use CAT1 interrupts with Rte (BSW00326]). 
              Due to this, the interrupt context shall be configured to a CAT2 interrupt if an Rte is used. 
    \note     The function is called by the lower layer.
**********************************************************************************************************************/
void Ccl_TxConfirmation(PduIdType TxPduId)
{
}

#if (CCL_TX_TP_CONFIRMATION_API == STD_ON)
/**********************************************************************************************************************
  Ccl_TpRxIndication
**********************************************************************************************************************/
/** \brief    This function is called after an I-PDU has been transmitted via the TP API.
    \param    PduId         Identification of the transmitted I-PDU.
    \param    Result        Result of the transmission.
    \return   none
    \context  The function can be called on interrupt and task level. It is not allowed to use CAT1 interrupts with Rte (BSW00326]). 
              Due to this, the interrupt context shall be configured to a CAT2 interrupt, if an Rte is used. 
    \note     The function is called by the lower layer.
**********************************************************************************************************************/
void Ccl_TpTxConfirmation(PduIdType PduId, Std_ReturnType Result)
{
}
# endif /* (CCL_TX_TP_CONFIRMATION_API == STD_ON) */

# if (CCL_START_OF_RECEPTION_API == STD_ON)
/**********************************************************************************************************************
  Com_StartOfReception
**********************************************************************************************************************/
/** \brief    This function is called at the start of receiving an N-SDU.
              The N-SDU might be fragmented into multiple N-PDUs (FF with one or more following CFs) or might consist of a single N-PDU (SF).
    \param    CclRxPduId          Identification of the I-PDU.
    \param    TpSduInfoPtr        Pointer to a PduInfoType structure containing the payload data (without protocol information) and 
                                  payload length of the first frame or single frame of a transport protocol I-PDU reception.
    \param    RxBufferSizePtr     Available receive buffer in the receiving module. 
                                  This parameter will be used to compute the Block Size (BS) in the transport protocol module
    \return   BufReq_ReturnType
              BUFREQ_OK           Buffer request accomplished successful.
              BUFREQ_E_NOT_OK     Buffer request not successful. Buffer cannot be accessed.
              BUFREQ_E_BUSY       Temporarily no buffer available. It's up the requestor to retry request for a certain time.
              BUFREQ_E_OVFL       No Buffer of the required length can be provided.
    \context  The function can be called on interrupt and task level. It is not allowed to use CAT1 interrupts with Rte (BSW00326]). 
              Due to this, the interrupt context shall be configured to a CAT2 interrupt, if an Rte is used. 
    \note     The function is called by the lower layer.
**********************************************************************************************************************/
BufReq_ReturnType Ccl_StartOfReception(PduIdType CclRxPduId, P2VAR(PduInfoType, AUTOMATIC, CCL_APPL_VAR) TpSduInfoPtr,
                                                              PduLengthType TpSduLength,
                                                              P2VAR(PduLengthType, AUTOMATIC,CCL_APPL_VAR) RxBufferSizePtr)
{
}
                                                              
# endif /* (CCL_START_OF_RECEPTION_API == STD_ON) */

# if (CCL_COPY_RX_DATA_API == STD_ON)
/**********************************************************************************************************************
  Com_CopyRxData
**********************************************************************************************************************/
/** \brief    This function is called to provide the received data of an I-PDU segment (N-PDU) to the upper layer.
              Each call to this function provides the next part of the I-PDU data.
              The size of the remaining data is written to the position indicated by bufferSizePtr
    \param    PduId               Identification of the received I-PDU.
    \param    PduInfoPointer      Provides the source buffer (SduDataPtr) and the number of bytes to be copied (SduLength). 
                                  An SduLength of 0 can be used to query the current amount of available buffer in the upper layer module.
                                  In this case, the SduDataPtr may be a NULL_PTR.
    \param    RxBufferSizePtr     Available receive buffer after data has been copied.
    \return   BufReq_ReturnType
              BUFREQ_OK           Buffer request accomplished successful.
              BUFREQ_E_NOT_OK     Buffer request not successful. Buffer cannot be accessed.
              BUFREQ_E_BUSY       Temporarily no buffer available. It's up the requestor to retry request for a certain time.
              BUFREQ_E_OVFL       No Buffer of the required length can be provided.
    \context  The function can be called on interrupt and task level. It is not allowed to use CAT1 interrupts with Rte (BSW00326]). 
              Due to this, the interrupt context shall be configured to a CAT2 interrupt, if an Rte is used. 
    \note     The function is called by the upper layer.
**********************************************************************************************************************/
BufReq_ReturnType Ccl_CopyRxData(PduIdType PduId, CONSTP2VAR(PduInfoType, AUTOMATIC, CCL_APPL_DATA) PduInfoPointer,
                                                        P2VAR(PduLengthType, AUTOMATIC, CCL_APPL_VAR) RxBufferSizePtr)
{
}
# endif /* (CCL_COPY_RX_DATA_API == STD_ON) */


#if (CCL_COPY_TX_DATA_API == STD_ON)
/**********************************************************************************************************************
  Com_CopyTxData
**********************************************************************************************************************/
/** \brief    This function is called to acquire the transmit data of an I-PDU segment (N-PDU). 
              Each call to this function provides the next part of the I-PDU data unless retry->TpDataState is TP_DATARETRY.
              In this case, the function restarts to copy the data beginning at the offset from the current position indicated by retry->TxTpDataCnt.
              The size of the remaining data is written to the position indicated by availableDataPtr.
    \param    PduId               Identification of the transmitted I-PDU.
    \param    PduInfoPtr          Provides the destination buffer (SduDataPtr) and the number of bytes to be copied (SduLength).
                                  If not enough transmit data is available, no data is copied by the upper layer module and BUFREQ_E_BUSY is returned.
                                  The lower layer module may retry the call.
                                  An SduLength of 0 can be used to indicate state changes in the retry parameter or to query the current amount of available data in the upper layer module.
                                  In this case, the SduDataPtr may be a NULL_PTR.
    \param    RetryInfoPtr        Used to acknowledge transmitted data or to retransmit data after transmission problems.
                                  If retry parameter is a NULL_PTR, it indicates that the transmit data can be removed from the buffer immediately after it has been copied.
                                  Otherwise, the retry parameter must point to a valid RetryInfoType element.
    \param    TxDataCntPtr        Indicates the remaining number of bytes that are available in the upper layer module's Tx buffer. 
                                  availableDataPtr can be used by TP modules that support dynamic payload lengths (e.g. FrIsoTp) to determine the size of the following CFs
    \return   BufReq_ReturnType
              BUFREQ_OK           Buffer request accomplished successful.
              BUFREQ_E_NOT_OK     Buffer request not successful. Buffer cannot be accessed.
              BUFREQ_E_BUSY       Temporarily no buffer available. It's up the requestor to retry request for a certain time.
              BUFREQ_E_OVFL       No Buffer of the required length can be provided.
    \context  The function can be called on interrupt and task level. It is not allowed to use CAT1 interrupts with Rte (BSW00326]). 
              Due to this, the interrupt context shall be configured to a CAT2 interrupt, if an Rte is used. 
    \note     The function is called by the upper layer.
**********************************************************************************************************************/
BufReq_ReturnType Ccl_CopyTxData(PduIdType PduId,
                                                        P2VAR(PduInfoType, AUTOMATIC, CCL_APPL_VAR) PduInfoPtr,
                                                        P2VAR(RetryInfoType, AUTOMATIC, CCL_APPL_VAR) RetryInfoPtr,
                                                        P2VAR(PduLengthType, AUTOMATIC, CCL_APPL_VAR) TxDataCntPtr)
{
}
# endif /* (CCL_COPY_TX_DATA_API == STD_ON) */


/**********************************************************************************************************************
  CanCcl_MainFunction
**********************************************************************************************************************/
void CanCcl_MainFunction( void )
{
    CAN_UINT8 ChannelIndex;

    CanNm_MainFunction(); /*TG1_TC2_SC1:Powerup or wakeup(local or remote), the first IC output msg should be NM Alive. NM handled before IL.*/
    for(ChannelIndex = 0; ChannelIndex < CAN_NUMBER_OF_CHANNELS; ChannelIndex++)
	{


        CanSm_MainFunction(ChannelIndex);
	}
	CanIlRxTask(0/*ChannelIndex*/); /*no IL msg in ch1*/
	CanIlTxTask(0/*ChannelIndex*/); /*no IL msg in ch1*/

    CanTp_MainFunction();
    
    //Nm_MainFunction();
    
    
}

/**********************************************************************************************************************
  PduR_DcmTransmit - For Dcm interface to transmit the frame data
**********************************************************************************************************************/
Std_ReturnType PduR_DcmTransmit(PduIdType TxPduId, PduInfoType const * PduInfoPtr)
{
    Std_ReturnType ReturnStatus = E_OK;


    if( (CAN_UINT8) Can_Ch0_Tp_Tx_Msg_IC_DiagResp_TMH != TxPduId )
	{
	    #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
		#else
		ReturnStatus = E_NOT_OK;
		#endif
	}

    if( NULL == PduInfoPtr )
	{
	    #ifdef CANUTIL_ENABLED
        CanUtil_ReportError();
		#else
		ReturnStatus = E_NOT_OK;
		#endif
	}

	if(E_NOT_OK != ReturnStatus)
	{
        ReturnStatus = CanTp_Transmit( 0, TxPduId, PduInfoPtr );
	}

    return(ReturnStatus);
}

void Ccl_Dcm_CommCtrl_Sf00(CAN_UINT8 CommCtrlType)
{
    switch(CommCtrlType)
    {
        case CCL_ENABLE_APP_RX_TX:
            CanIlSetIlMode( Can_ControllerId, ILSTATUS, (IL_STATUS_SUSPEND|IL_STATUS_TX_SUSPEND), 0, FALSE );
            break;
        case CCL_ENABLE_NM_RX_TX:
            CanNmSetNmMode(Can_ControllerId, NM_STATUS_RESUME);
            break;
        case CCL_ENABLE_APP_NM_RX_TX:
            CanIlSetIlMode( Can_ControllerId, ILSTATUS, (IL_STATUS_SUSPEND|IL_STATUS_TX_SUSPEND), 0, FALSE );
            CanNmSetNmMode(Can_ControllerId, NM_STATUS_RESUME);
            break;
        default:
            break;
    }
}

void Ccl_Dcm_CommCtrl_Sf03(CAN_UINT8 CommCtrlType)
{
    switch(CommCtrlType)
    {
        case CCL_DISABLE_APP_RX_TX:
            CanIlSetIlMode( Can_ControllerId, ILSTATUS, IL_STATUS_SUSPEND, 0, TRUE );
            break;
        case CCL_DISABLE_NM_RX_TX:
            CanNmSetNmMode(Can_ControllerId, NM_STATUS_SUSPEND);
            break;
        case CCL_DISABLE_APP_NM_RX_TX:
            CanIlSetIlMode( Can_ControllerId, ILSTATUS, IL_STATUS_SUSPEND, 0, TRUE );
            CanNmSetNmMode(Can_ControllerId, NM_STATUS_SUSPEND);
            break;
        default:
            break;
    }
}

/****************************HISTORY*****************************************/
/****************************************************************************
Date              : 11/21/2016
By                : cmurali1
Change Description: Migrated to BAIC requirements
*****************************************************************************/

