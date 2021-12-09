/*-------------------------------- Arctic Core ------------------------------
 * Copyright (C) 2013, ArcCore AB, Sweden, www.arccore.com.
 * Contact: <contact@arccore.com>
 * 
 * You may ONLY use this file:
 * 1)if you have a valid commercial ArcCore license and then in accordance with  
 * the terms contained in the written license agreement between you and ArcCore, 
 * or alternatively
 * 2)if you follow the terms found in GNU General Public License version 2 as 
 * published by the Free Software Foundation and appearing in the file 
 * LICENSE.GPL included in the packaging of this file or here 
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>
 *-------------------------------- Arctic Core -----------------------------*/


//lint -w2 Static code analysis warning level

/*
 *  General requirements
 */
/* @req DCM738 DCM_E_FORCE_RCRRP_OK never used in external service processing */

#include <string.h>
#include "Dcm.h"
#include "Dcm_Internal.h"

typedef struct {
    const PduInfoType 				*pduRxData;
    PduInfoType 					*pduTxData;
    const Dcm_DsdServiceTableType	*serviceTable;
    Dcm_ProtocolAddrTypeType		addrType;
    PduIdType 						pdurTxPduId;
    PduIdType 						rxPduId;
    Dcm_ProtocolTransTypeType       txType;
    boolean                         internalRequest;
    boolean                         sendRespPendOnTransToBoot;
    boolean                         startupResponseRequest;
} MsgDataType;

// In "queue" to DSD
static boolean	dsdDslDataIndication = FALSE;
static MsgDataType msgData;

static uint8	currentSid;
static boolean	suppressPosRspMsg;

#if (DCM_MANUFACTURER_NOTIFICATION == STD_ON)
static uint16   Dcm_Dsd_sourceAddress;
static Dcm_ProtocolAddrTypeType Dcm_Dsd_requestType;
#endif

static Dcm_MsgContextType GlobalMessageContext;
typedef enum {
    DCM_SERVICE_IDLE,
    DCM_SERVICE_PENDING,
    DCM_SERVICE_WAIT_DONE,
    DCM_SERVICE_DONE,
    DCM_SERVICE_WAIT_TX_CONFIRM
}ServicePendingStateType;

typedef struct {
    Dcm_NegativeResponseCodeType NRC;
    ServicePendingStateType state;
    boolean isSubFnc;
    uint8 subFncId;
}ExternalServiceStateType;

static ExternalServiceStateType ExternalService = {.NRC = DCM_E_POSITIVERESPONSE, .state = DCM_SERVICE_IDLE, .isSubFnc = FALSE, .subFncId=0};
/*
 * Local functions
 */

#define IS_SUPRESSED_NRC_ON_FUNC_ADDR(_x) ((DCM_E_SERVICENOTSUPPORTED == (_x)) || (DCM_E_SUBFUNCTIONNOTSUPPORTED == (_x)) || (DCM_E_REQUESTOUTOFRANGE == (_x)))
static void createAndSendNcr(Dcm_NegativeResponseCodeType responseCode)
{
    PduIdType dummyId = 0;
    boolean flagdsl;

    /* Suppress reponse if DCM001 is fulfilled or if it is a type2 response and a NRC */ 
    /* @req DCM001 */

    flagdsl = DslPduRPduUsedForType2Tx(msgData.pdurTxPduId, &dummyId);
    /* Suppress reponse if DCM001 is fulfilled or if it is a type2 response and a NRC */ 
    /* @req DCM001 */
    if(((msgData.addrType == DCM_PROTOCOL_FUNCTIONAL_ADDR_TYPE)&&IS_SUPRESSED_NRC_ON_FUNC_ADDR(responseCode))||
       ((responseCode != DCM_E_POSITIVERESPONSE )&& (TRUE == flagdsl))){
        DslDsdProcessingDone(msgData.rxPduId, DSD_TX_RESPONSE_SUPPRESSED);
    }
    else {
        msgData.pduTxData->SduDataPtr[0] = SID_NEGATIVE_RESPONSE;
        msgData.pduTxData->SduDataPtr[1] = currentSid;
        msgData.pduTxData->SduDataPtr[2] = responseCode;
        msgData.pduTxData->SduLength = 3;
        DslDsdProcessingDone(msgData.rxPduId, DSD_TX_RESPONSE_READY);   
        /* @req DCM114 */ 
        /* @req DCM232 Ncr */
    }
}

static void setCurrentMessageContext(boolean isSubFunction)
{
    uint8 offset = (TRUE == isSubFunction) ? 2u:1u;
    GlobalMessageContext.dcmRxPduId = msgData.rxPduId;
    GlobalMessageContext.idContext = (uint8)msgData.rxPduId;
    GlobalMessageContext.msgAddInfo.reqType = (msgData.addrType == DCM_PROTOCOL_FUNCTIONAL_ADDR_TYPE)? TRUE: FALSE;
    GlobalMessageContext.msgAddInfo.suppressPosResponse = suppressPosRspMsg;
    GlobalMessageContext.reqData = &msgData.pduRxData->SduDataPtr[offset];
    GlobalMessageContext.reqDataLen = (Dcm_MsgLenType)(msgData.pduRxData->SduLength - offset);
    GlobalMessageContext.resData = &msgData.pduTxData->SduDataPtr[offset];
    GlobalMessageContext.resDataLen = 0;
    GlobalMessageContext.resMaxDataLen = (Dcm_MsgLenType)(msgData.pduTxData->SduLength - offset);
}

/**
 * Starts processing an external sub service
 * @param SidCfgPtr
 */
static void startExternalSubServiceProcessing(const Dcm_DsdSubServiceType *SubCfgPtr)
{
    setCurrentMessageContext(TRUE);
    ExternalService.NRC = DCM_E_POSITIVERESPONSE;
    ExternalService.state = DCM_SERVICE_WAIT_DONE;
    ExternalService.isSubFnc = TRUE;
    ExternalService.subFncId = SubCfgPtr->DsdSubServiceId;
    if( (E_PENDING == SubCfgPtr->DsdSubServiceFnc(DCM_INITIAL, &GlobalMessageContext)) && (DCM_SERVICE_WAIT_DONE == ExternalService.state) ) {
        ExternalService.state = DCM_SERVICE_PENDING;
    }
}

/**
 * Starts processing an external service
 * @param SidCfgPtr
 */
static void startExternalServiceProcessing(const Dcm_DsdServiceType *SidCfgPtr)
{
    setCurrentMessageContext(FALSE);
    ExternalService.NRC = DCM_E_POSITIVERESPONSE;
    ExternalService.state = DCM_SERVICE_WAIT_DONE;
    ExternalService.isSubFnc = FALSE;
    /* @req DCM732 */
    /* @req DCM760 */
    if( (E_PENDING == SidCfgPtr->DspSidTabFnc(DCM_INITIAL, &GlobalMessageContext)) && (DCM_SERVICE_WAIT_DONE == ExternalService.state) ) {
        ExternalService.state = DCM_SERVICE_PENDING;
    }
}

static void selectServiceFunction(const Dcm_DsdServiceType *sidConfPtr)
{
    /* !req DCM442 Services missing */
    /* Check if subfunction supported */
    boolean processService = TRUE;
    const Dcm_DsdSubServiceType *subServicePtr;
    Dcm_NegativeResponseCodeType respCode;
    if( sidConfPtr->DsdSidTabSubfuncAvail == TRUE ) {
        if( msgData.pduRxData->SduLength > 1 ) { /* @req DCM696 */
            if( NULL_PTR == sidConfPtr->DsdSubServiceList ) {
                /* This to be backwards compatible. If no sub-function configured,
                 * behave as all are supported. */
            } else if( TRUE == DsdLookupSubService(currentSid, sidConfPtr, msgData.pduRxData->SduDataPtr[1], &subServicePtr, &respCode) ) {
                if( DCM_E_POSITIVERESPONSE == respCode ) {
                    if( NULL_PTR != subServicePtr->DsdSubServiceFnc ) {
                        startExternalSubServiceProcessing(subServicePtr);
                        processService = FALSE;
                    }
                } else {
                    /* Sub-function supported but currently not available */
                    createAndSendNcr(respCode);
                    processService = FALSE;
                }
            } else {
                /* Sub-function not supported */
                createAndSendNcr(DCM_E_SUBFUNCTIONNOTSUPPORTED);
                processService = FALSE;
            }
        } else {
            /* Not enough data for Sub-function */
            createAndSendNcr(DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT);
            processService = FALSE;
        }
    }
    if(TRUE == processService) {
        /* @req DCM221 */
        //runInternalService(sidConfPtr->DsdSidTabServiceId);
    }
}

/**
 * Checks if a service is supported
 * @param sid
 * @param sidPtr
 * @return TRUE: Service supported, FALSE: Service not supported
 */
static boolean lookupSid(uint8 sid, const Dcm_DsdServiceType **sidPtr)
{
    boolean returnStatus = FALSE;
    *sidPtr = NULL_PTR;
    const Dcm_DsdServiceType *service;

    if(NULL_PTR != msgData.serviceTable) {
        service = msgData.serviceTable->DsdService;
        while ((service->DsdSidTabServiceId != sid) && (FALSE == service->Arc_EOL)) {
            service++;
        }

        if (FALSE == service->Arc_EOL) {
            *sidPtr = service;
            returnStatus = TRUE;
        }
    }

    return returnStatus;
}

/**
 * Runs an external service
 * @param opStatus
 * @return
 */
static Std_ReturnType DslRunExternalServices(Dcm_OpStatusType opStatus)
{
    Std_ReturnType ret = E_NOT_OK;
    const Dcm_DsdServiceType *sidCfgPtr = NULL_PTR;
    if(TRUE == lookupSid(currentSid, &sidCfgPtr)) {
        if( TRUE == ExternalService.isSubFnc ) {
            const Dcm_DsdSubServiceType *subServicePtr;
            Dcm_NegativeResponseCodeType resp;
            if(( TRUE == DsdLookupSubService(currentSid, sidCfgPtr, ExternalService.subFncId, &subServicePtr, &resp)) && (NULL_PTR != subServicePtr->DsdSubServiceFnc)) {
                ret = subServicePtr->DsdSubServiceFnc(opStatus, NULL_PTR);
            }
        } else {
            if( NULL_PTR != sidCfgPtr->DspSidTabFnc ) {
                ret = sidCfgPtr->DspSidTabFnc(opStatus, NULL_PTR);
            }
        }
    }
    return ret;
}

/**
 * Main function for external service processing.
 */
static void externalServiceMainFunction()
{
    /* @req DCM760 */
    if( DCM_SERVICE_IDLE != ExternalService.state ) {
        if( DCM_SERVICE_PENDING == ExternalService.state ) {
            if((E_PENDING != DslRunExternalServices(DCM_PENDING)) && (DCM_SERVICE_PENDING == ExternalService.state)) {
                /* Service did not return pending and was not finished within the context of
                 * service function. Wait for it to complete */
                ExternalService.state = DCM_SERVICE_WAIT_DONE;
            }
        }
        if( DCM_SERVICE_DONE == ExternalService.state ) {
            if( DCM_E_POSITIVERESPONSE != ExternalService.NRC ) {
                DsdDspProcessingDone(ExternalService.NRC);
            } else {
                /* Create a positive response */
                msgData.pduTxData->SduLength++;
                DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
            }
            ExternalService.state = DCM_SERVICE_WAIT_TX_CONFIRM;
        }
    }
}
/*
 * Exported functions
 */
/**
 * Cancels a pending external service
 */
void DsdCancelPendingExternalServices(void)
{
    if( (DCM_SERVICE_PENDING == ExternalService.state) ||
            (DCM_SERVICE_WAIT_DONE == ExternalService.state) ) {
        /* @req DCM735 */
        (void)DslRunExternalServices(DCM_CANCEL);
    }
    ExternalService.state = DCM_SERVICE_IDLE;
}

/**
 * Sets negative response of external service
 * @param pMsgContext
 * @param ErrorCode
 */
void DsdExternalSetNegResponse(const Dcm_MsgContextType* pMsgContext, Dcm_NegativeResponseCodeType ErrorCode)
{
    if(pMsgContext->dcmRxPduId == GlobalMessageContext.dcmRxPduId) {
        ExternalService.NRC = ErrorCode;
    }
}

/**
 * Sets external service processing to done
 * @param pMsgContext
 */
void DsdExternalProcessingDone(const Dcm_MsgContextType* pMsgContext)
{
    if(pMsgContext->dcmRxPduId == GlobalMessageContext.dcmRxPduId) {
        ExternalService.state = DCM_SERVICE_DONE;
        if( DCM_E_POSITIVERESPONSE == ExternalService.NRC ) {
            if(TRUE == ExternalService.isSubFnc) {
                memcpy(&msgData.pduTxData->SduDataPtr[2], pMsgContext->resData, pMsgContext->resDataLen);
                msgData.pduTxData->SduLength = (PduLengthType)pMsgContext->resDataLen;
                msgData.pduTxData->SduDataPtr[1] = ExternalService.subFncId;
                msgData.pduTxData->SduLength++;
            } else {
                memcpy(&msgData.pduTxData->SduDataPtr[1], pMsgContext->resData, pMsgContext->resDataLen);
                msgData.pduTxData->SduLength =(PduLengthType)pMsgContext->resDataLen;
            }
        }
    }
}

/**
 * Confirms transmission of response to externally processed service
 * @param result
 */
static void DcmConfirmation(NotifResultType result)
{
    Dcm_ConfirmationStatusType status;
    if( DCM_SERVICE_WAIT_TX_CONFIRM == ExternalService.state) {
        if( NTFRSLT_OK == result ) {
            status = (DCM_E_POSITIVERESPONSE == ExternalService.NRC) ? DCM_RES_POS_OK:DCM_RES_NEG_OK;
        } else {
            status = (DCM_E_POSITIVERESPONSE == ExternalService.NRC) ? DCM_RES_POS_NOT_OK:DCM_RES_NEG_NOT_OK;
        }
        Dcm_Confirmation(GlobalMessageContext.idContext, GlobalMessageContext.dcmRxPduId, status);
        ExternalService.state = DCM_SERVICE_IDLE;
    }
}

void DsdInit(void)
{

}


void DsdMain(void)
{
	if (TRUE == dsdDslDataIndication) {
		dsdDslDataIndication = FALSE;
		DsdHandleRequest();
	}
	/* @req DCM734 */
	externalServiceMainFunction();
}

/**
 * Checks if a sub-function for a service is supported and available in the current session and security level
 * @param sidConfPtr
 * @param subService
 * @param subServicePtr
 * @param respCode
 * @return TRUE: Sub-function supported, FALSE: Sub-function not supported
 */
#define ROE_STORAGE_STATE (1u<<6u)
boolean DsdLookupSubService(uint8 SID, const Dcm_DsdServiceType *sidConfPtr, uint8 subService, const Dcm_DsdSubServiceType **subServicePtr, Dcm_NegativeResponseCodeType *respCode)
{
    boolean found = FALSE;
    *respCode = DCM_E_POSITIVERESPONSE;
    if( (NULL_PTR != sidConfPtr) && (TRUE == sidConfPtr->DsdSidTabSubfuncAvail) && (NULL_PTR != sidConfPtr->DsdSubServiceList) ) {
        uint8 indx = 0;
        while( FALSE == sidConfPtr->DsdSubServiceList[indx].Arc_EOL ) {
            if( (subService == sidConfPtr->DsdSubServiceList[indx].DsdSubServiceId) ||
                    ((SID_RESPONSE_ON_EVENT == SID) && (subService == (sidConfPtr->DsdSubServiceList[indx].DsdSubServiceId | ROE_STORAGE_STATE))) ) {
                *subServicePtr = &sidConfPtr->DsdSubServiceList[indx];
                if(TRUE == DspCheckSessionLevel(sidConfPtr->DsdSubServiceList[indx].DsdSubServiceSessionLevelRef)) {
                    if(FALSE == DspCheckSecurityLevel(sidConfPtr->DsdSubServiceList[indx].DsdSubServiceSecurityLevelRef) ) {
                        /* @req DCM617 */
                        *respCode = DCM_E_SECURITYACCESSDENIED;
                    }
                } else {
                    *respCode = DCM_E_SUBFUNCTIONNOTSUPPORTEDINACTIVESESSION;
                }
                found = TRUE;
            }
            indx++;
        }
    }
    return found;
}


void DsdHandleRequest(void)
{
    Std_ReturnType result = E_OK;
    const Dcm_DsdServiceType *sidConfPtr = NULL_PTR;
    Dcm_NegativeResponseCodeType errorCode ;
    errorCode= DCM_E_POSITIVERESPONSE;

    if( msgData.pduRxData->SduLength > 0 ) {
        currentSid = msgData.pduRxData->SduDataPtr[0];	
        /* @req DCM198 */

#if (DCM_MANUFACTURER_NOTIFICATION == STD_ON)  
        /* @req DCM218 */
        Dcm_ProtocolType dummyProtocolId;
        if( E_OK == Arc_DslGetRxConnectionParams(msgData.rxPduId, &Dcm_Dsd_sourceAddress, &Dcm_Dsd_requestType, &dummyProtocolId) ) {
            result = ServiceIndication(currentSid, msgData.pduRxData->SduDataPtr, msgData.pduRxData->SduLength,
                                   Dcm_Dsd_requestType, Dcm_Dsd_sourceAddress, &errorCode);
        }

#endif
    }

    if (( 0 == msgData.pduRxData->SduLength) || (E_REQUEST_NOT_ACCEPTED == result)) {  /*lint !e845 !e774 result value updated under DCM_MANUFACTURER_NOTIFICATION */
        /* Special case with sdu length 0, No service id so we cannot send response. */
        /* @req DCM677 */
        /* @req DCM462 */
        // Do not send any response     
        /* @req DCM462 */ 
        /* @req DCM677 */
        DslDsdProcessingDone(msgData.rxPduId, DSD_TX_RESPONSE_SUPPRESSED);
    } else if (result != E_OK) {  /*lint !e774 result value updated under DCM_MANUFACTURER_NOTIFICATION */
        /* @req DCM678 */ 
        /* @req DCM463 */
        createAndSendNcr(errorCode);
        /* @req DCM463 */
    }
    /* @req DCM178 */
    //lint --e(506, 774)	PC-Lint exception Misra 13.7, 14.1 Allow configuration variables in boolean expression
    else if ((DCM_RESPOND_ALL_REQUEST == STD_ON) || ((currentSid & 0x7Fu) < 0x40)) {
        /* @req DCM084 */
        if (TRUE == lookupSid(currentSid, &sidConfPtr)) {
            /* @req DCM192 */
            /* @req DCM193 */
            // SID found!
            /* Skip checks if service is supported in current session and security level if it is a startup response request
             * and the request should be handled internally (as the service is performed by bootloader and we should only respond) */
            boolean skipSesAndSecChecks = (msgData.startupResponseRequest && (NULL_PTR == sidConfPtr->DspSidTabFnc))? TRUE: FALSE;
            if ((TRUE == skipSesAndSecChecks) || (TRUE == DspCheckSessionLevel(sidConfPtr->DsdSidTabSessionLevelRef))) {
                /* @req DCM211 */
                if ((TRUE == skipSesAndSecChecks) || (TRUE == DspCheckSecurityLevel(sidConfPtr->DsdSidTabSecurityLevelRef))) {
                    /* @req DCM217 */
                    //lint --e(506, 774)	PC-Lint exception Misra 13.7, 14.1 Allow configuration variables in boolean expression
                    //lint --e(506, 774)	PC-Lint exception Misra 13.7, 14.1 Allow configuration variables in boolean expression
                    if ( (TRUE == sidConfPtr->DsdSidTabSubfuncAvail) && (SUPPRESS_POS_RESP_BIT == (msgData.pduRxData->SduDataPtr[1] & SUPPRESS_POS_RESP_BIT) )) {	
                        /* @req DCM204 */
                        suppressPosRspMsg = TRUE;	/* @req DCM202 */
                        msgData.pduRxData->SduDataPtr[1] &= ~SUPPRESS_POS_RESP_BIT;	/* @req DCM201 */
                    } else {
                        suppressPosRspMsg = FALSE;	/* @req DCM202 */
                    }
#if (DCM_USE_SPLIT_TASK_CONCEPT == STD_ON)
                    DslSetDspProcessingActive(msgData.rxPduId, TRUE);
#endif
                    if( NULL_PTR != sidConfPtr->DspSidTabFnc ) {
                        /* @req DCM196 */
                        startExternalServiceProcessing(sidConfPtr);
                    } else {
                        selectServiceFunction(sidConfPtr);
                    }
#if (DCM_USE_SPLIT_TASK_CONCEPT == STD_ON)
                    DslSetDspProcessingActive(msgData.rxPduId, FALSE);
#endif
                } else {
                    createAndSendNcr(DCM_E_SECURITYACCESSDENIED);	/* @req DCM217 */
                }
            } else {
                createAndSendNcr(DCM_E_SERVICENOTSUPPORTEDINACTIVESESSION);	/* @req DCM211 */
            }
        } else {
            createAndSendNcr(DCM_E_SERVICENOTSUPPORTED);	/* @req DCM197 */
        }
    } else {
        // Inform DSL that message has been discard
        DslDsdProcessingDone(msgData.rxPduId, DSD_TX_RESPONSE_SUPPRESSED);
    }
 /*lint -e{438} errorCode last value assigned not used because value updated under DCM_MANUFACTURER_NOTIFICATION */
}

void DsdDspProcessingDone(Dcm_NegativeResponseCodeType responseCode)
{
    PduIdType dummyId = 0;
    if (responseCode == DCM_E_POSITIVERESPONSE) {
        if (suppressPosRspMsg == FALSE) {
            /* @req DCM200 */
            /* @req DCM231 */
            /* @req DCM222 */
            if( FALSE == DslPduRPduUsedForType2Tx(msgData.pdurTxPduId, &dummyId) ) {
                /* @req DCM223 */
                /* @req DCM224 */
                msgData.pduTxData->SduDataPtr[0] = currentSid | SID_RESPONSE_BIT;	
            }
            /* @req DCM114 */
            /* @req DCM225 */
            /* @req DCM232 Ok */
            DslDsdProcessingDone(msgData.rxPduId, DSD_TX_RESPONSE_READY);
        } else {
            /* @req DCM236 */
            /* @req DCM240 */
            DspDcmConfirmation(msgData.pdurTxPduId, NTFRSLT_OK);
            DslDsdProcessingDone(msgData.rxPduId, DSD_TX_RESPONSE_SUPPRESSED);
            DcmConfirmation(NTFRSLT_OK);
        }
    } else {
        /* @req DCM228 */
        createAndSendNcr(responseCode);
    }

}

void DsdDataConfirmation(PduIdType confirmPduId, NotifResultType result)
{
    DspDcmConfirmation(confirmPduId, result);	/* @req DCM236 */

#if (DCM_MANUFACTURER_NOTIFICATION == STD_ON)  /* @req DCM742  */
    ServiceConfirmation(currentSid, Dcm_Dsd_requestType, Dcm_Dsd_sourceAddress, result);
#endif
    DcmConfirmation(result);
}

void DsdDslDataIndication(const PduInfoType *pduRxData,
        const Dcm_DsdServiceTableType *protocolSIDTable,
        Dcm_ProtocolAddrTypeType addrType,
        PduIdType txPduId,
        PduInfoType *pduTxData,
        PduIdType rxContextPduId,
        Dcm_ProtocolTransTypeType txType,
        boolean internalRequest,
        boolean sendRespPendOnTransToBoot,
        boolean startupResponseRequest)
{
    msgData.rxPduId = rxContextPduId;
    msgData.pdurTxPduId = txPduId;
    msgData.pduRxData = pduRxData;
    msgData.pduTxData = pduTxData;
    msgData.addrType = addrType;
    msgData.serviceTable = protocolSIDTable; /* @req DCM195 */
    msgData.txType = txType;
    msgData.internalRequest = internalRequest;
    msgData.sendRespPendOnTransToBoot = sendRespPendOnTransToBoot;
    msgData.startupResponseRequest = startupResponseRequest;
    dsdDslDataIndication = TRUE;
}

/**
 * Clears suppress positive message bit
 */
void DsdClearSuppressPosRspMsg(void)
{
    suppressPosRspMsg = FALSE;
}
