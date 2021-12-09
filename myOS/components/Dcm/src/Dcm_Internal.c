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

#include <string.h>
#include "Dcm.h"
#include "Dcm_Internal.h"

/* Disable MISRA 2004 rule 16.2, MISRA 2012 rule 17.2.
 * This because of recursive calls to readDidData.
 *  */
//lint -estring(974,*recursive*)

typedef struct {
    uint8   session;
    uint8   testerSrcAddr;
    uint8   protocolId;
    boolean requestFullComm;
    boolean requested;
}DcmProtocolRequestType;

typedef void (*Dcm_StartProtocolNotificationFncType)(boolean started);

typedef struct {
    uint8 priority;
    Dcm_StartProtocolNotificationFncType StartProtNotification;
}DcmRequesterConfigType;



static DcmProtocolRequestType ProtocolRequests[DCM_NOF_REQUESTERS];
static boolean ProtocolRequestsAllowed = FALSE;

boolean lookupNonDynamicDid(uint16 didNr, const Dcm_DspDidType **didPtr)
{
    const Dcm_DspDidType *dspDid = Dcm_ConfigPtr->Dsp->DspDid;
    boolean didFound = FALSE;

    while ((dspDid->DspDidIdentifier != didNr) &&  (FALSE == dspDid->Arc_EOL)) {
        dspDid++;
    }

    /* @req Dcm651 */
    if ((FALSE == dspDid->Arc_EOL) && (!((TRUE == dspDid->DspDidInfoRef->DspDidDynamicllyDefined) && DID_IS_IN_DYNAMIC_RANGE(didNr)) ) ) {
        didFound = TRUE;
        *didPtr = dspDid;
    }

    return didFound;
}

void DcmResetDiagnosticActivityOnSessionChange(Dcm_SesCtrlType newSession)
{
    //DspResetDiagnosticActivityOnSessionChange(newSession);
    //DsdCancelPendingExternalServices();
}

void DcmResetDiagnosticActivity(void)
{
    //DcmDspResetDiagnosticActivity();
    //DsdCancelPendingExternalServices();
}

/**
 * Function for canceling pending requests
 */
void DcmCancelPendingRequests()
{
    DspCancelPendingRequests();
    DsdCancelPendingExternalServices();
}
/**
 *
 * @param requester
 * @param session
 * @param protocolId
 * @param testerSrcAddr
 * @param requestFullComm
 * @return E_OK: Operation successful, E_NOT_OK: Operation failed
 */
Std_ReturnType DcmRequestStartProtocol(DcmProtocolRequesterType requester, uint8 session, uint8 protocolId, uint8 testerSrcAddr, boolean requestFullComm)
{
    Std_ReturnType ret = E_NOT_OK;
    if( (requester < DCM_NOF_REQUESTERS) && (TRUE == ProtocolRequestsAllowed) ) {
        ProtocolRequests[requester].requested = TRUE;
        ProtocolRequests[requester].session = session;
        ProtocolRequests[requester].testerSrcAddr = testerSrcAddr;
        ProtocolRequests[requester].protocolId = protocolId;
        ProtocolRequests[requester].requestFullComm = requestFullComm;
        ret = E_OK;
    }
    return ret;
}

/**
 * Checks for protocol start requests and tries to start the highest prioritized
 */
void DcmExecuteStartProtocolRequest(void)
{
   static const DcmRequesterConfigType DcmRequesterConfig[DCM_NOF_REQUESTERS] = {
        [DCM_REQ_DSP] = {
                .priority = 0,
                .StartProtNotification = DcmDspProtocolStartNotification
        },
    #if defined(DCM_USE_SERVICE_RESPONSEONEVENT) && defined(USE_NVM)
        [DCM_REQ_ROE] = {
                .priority = 1,
                .StartProtNotification = DcmRoeProtocolStartNotification
        },
    #endif
    };
    DcmProtocolRequesterType decidingRequester = DCM_NOF_REQUESTERS;
    boolean requestFullComm = FALSE;
    uint8 currPrio = 0xff;
    for( uint8 requester = 0; requester < (uint8)DCM_NOF_REQUESTERS; requester++ ) {
        if( TRUE == ProtocolRequests[requester].requested ) {
            if(DcmRequesterConfig[requester].priority < currPrio ) {
                currPrio = DcmRequesterConfig[requester].priority;
                decidingRequester = (DcmProtocolRequesterType)requester;
                requestFullComm = ProtocolRequests[requester].requestFullComm;
            }
        }
    }
    if(  decidingRequester != DCM_NOF_REQUESTERS ) {
        /* Find out if fullcomm should be requested */
        for( uint8 requester = 0; requester <(uint8)DCM_NOF_REQUESTERS; requester++ ) {
            if( (TRUE == ProtocolRequests[requester].requested) && (ProtocolRequests[requester].requestFullComm == TRUE) &&
                    (ProtocolRequests[requester].session == ProtocolRequests[decidingRequester].session) &&
                    (ProtocolRequests[requester].testerSrcAddr == ProtocolRequests[decidingRequester].testerSrcAddr) &&
                    (ProtocolRequests[requester].protocolId == ProtocolRequests[decidingRequester].protocolId) ) {
                requestFullComm = TRUE;
            }
        }
        Std_ReturnType ret = DslDspSilentlyStartProtocol(ProtocolRequests[decidingRequester].session, ProtocolRequests[decidingRequester].protocolId, (uint16)ProtocolRequests[decidingRequester].testerSrcAddr, requestFullComm);
        /* Notify requesters */
        boolean started;
        for( uint8 requester = 0; requester <(uint8)DCM_NOF_REQUESTERS; requester++ ) {
            if( ProtocolRequests[requester].requested == TRUE ) {
                started = FALSE;
                if( (ProtocolRequests[requester].session == ProtocolRequests[decidingRequester].session) &&
                        (ProtocolRequests[requester].testerSrcAddr == ProtocolRequests[decidingRequester].testerSrcAddr) &&
                        (ProtocolRequests[requester].protocolId == ProtocolRequests[decidingRequester].protocolId) ) {
                    started = (E_OK == ret) ? TRUE : FALSE;
                }
                if( NULL_PTR != DcmRequesterConfig[requester].StartProtNotification ) {
                    DcmRequesterConfig[requester].StartProtNotification(started);
                }
            }
        }
    }
}

/**
 * Sets allowance to request protocol start
 * @param allowed
 */
void DcmSetProtocolStartRequestsAllowed(boolean allowed)
{
    if( (TRUE == allowed) && (FALSE == ProtocolRequestsAllowed)) {
        memset(ProtocolRequests, 0, sizeof(ProtocolRequests));
    }
    ProtocolRequestsAllowed = allowed;
}
