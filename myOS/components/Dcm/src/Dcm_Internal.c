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

typedef struct {
    uint8   session;
    uint8   testerSrcAddr;
    uint8   protocolId;
    boolean requestFullComm;
    boolean requested;
}DcmProtocolRequestType;

static DcmProtocolRequestType ProtocolRequests[DCM_NOF_REQUESTERS];
static boolean ProtocolRequestsAllowed = FALSE;

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
