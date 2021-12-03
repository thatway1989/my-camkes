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

/*
 * NB! This file is for DCM internal use only and may only be included from DCM C-files!
 */



#ifndef DCM_INTERNAL_H_
#define DCM_INTERNAL_H_

#define DCM_DET_REPORTERROR(_api,_err)                              \
        //(void)Det_ReportError(DCM_MODULE_ID, 0, _api, _err);    \
		LOGE("Error: Dcm api=%d,err=%d", _api, _err);

#define VALIDATE(_exp,_api,_err ) \
        if(!(_exp)) { \
          DCM_DET_REPORTERROR(_api, _err); \
          return E_NOT_OK; \
        }

#define VALIDATE_RV(_exp,_api,_err,_rv ) \
        if(!(_exp)) { \
          DCM_DET_REPORTERROR(_api, _err); \
          return _rv; \
        }

#define VALIDATE_NO_RV(_exp,_api,_err ) \
        if(!(_exp) ){ \
          DCM_DET_REPORTERROR(_api, _err); \
          return; \
        }
        
#define DCM_FIRST_PERIODIC_TX_PDU 1u
#define DCM_NOF_PERIODIC_TX_PDU 3u

#define IS_PERIODIC_TX_PDU(_x) (((_x) >= DCM_FIRST_PERIODIC_TX_PDU) && ((_x) < (DCM_FIRST_PERIODIC_TX_PDU + DCM_NOF_PERIODIC_TX_PDU)))
#define TO_INTERNAL_PERIODIC_PDU(_x) ((_x) - DCM_FIRST_PERIODIC_TX_PDU)

// SID table
#define SID_DIAGNOSTIC_SESSION_CONTROL			0x10
#define SID_ECU_RESET							0x11
#define SID_CLEAR_DIAGNOSTIC_INFORMATION		0x14
#define SID_READ_DTC_INFORMATION				0x19
#define SID_READ_DATA_BY_IDENTIFIER				0x22
#define SID_READ_MEMORY_BY_ADDRESS				0x23
#define SID_READ_SCALING_DATA_BY_IDENTIFIER		0x24
#define SID_SECURITY_ACCESS						0x27
#define SID_COMMUNICATION_CONTROL               0x28
#define SID_READ_DATA_BY_PERIODIC_IDENTIFIER	0x2A
#define SID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER	0x2C
#define SID_WRITE_DATA_BY_IDENTIFIER			0x2E
#define SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER	0x2F
#define SID_ROUTINE_CONTROL						0x31
#define SID_WRITE_MEMORY_BY_ADDRESS				0x3D
#define SID_TESTER_PRESENT						0x3E
#define SID_NEGATIVE_RESPONSE					0x7F
#define SID_CONTROL_DTC_SETTING					0x85
#define SID_RESPONSE_ON_EVENT                   0x86
#define SID_LINK_CONTROL                        0x87

#define SID_REQUEST_DOWNLOAD					0x34
#define SID_REQUEST_UPLOAD						0x35
#define SID_TRANSFER_DATA						0x36
#define SID_REQUEST_TRANSFER_EXIT				0x37

//OBD SID TABLE
#define SID_REQUEST_CURRENT_POWERTRAIN_DIAGNOSTIC_DATA		0x01
#define SID_REQUEST_POWERTRAIN_FREEZE_FRAME_DATA			0x02
#define SID_CLEAR_EMISSION_RELATED_DIAGNOSTIC_INFORMATION	0x04
#define SID_REQUEST_EMISSION_RELATED_DIAGNOSTIC_TROUBLE_CODES		0x03
#define SID_REQUEST_ON_BOARD_MONITORING_TEST_RESULTS_SPECIFIC_MONITORED_SYSTEMS		0x06
#define SID_REQUEST_EMISSION_RELATED_DIAGNOSTIC_TROUBLE_CODES_DETECTED_DURING_CURRENT_OR_LAST_COMPLETED_DRIVING_CYCLE	0x07
#define SID_REQUEST_VEHICLE_INFORMATION		0x09
#define SID_REQUEST_EMISSION_RELATED_DIAGNOSTIC_TROUBLE_CODES_WITH_PERMANENT_STATUS 0x0A

// Misc definitions
#define SUPPRESS_POS_RESP_BIT		(uint8)0x80
#define SID_RESPONSE_BIT			(uint8)0x40
#define VALUE_IS_NOT_USED			(uint8)0x00

// This diag request error code is not an error and is therefore not allowed to be used by SWCs, only used internally in DCM.
// It is therefore not defined in Rte_Dcm.h
#define DCM_E_RESPONSEPENDING       ((Dcm_NegativeResponseCodeType)0x78)
#define DCM_E_FORCE_RCRRP ((Dcm_NegativeResponseCodeType)0x01)/* Not an actual response code. Used internally */

#define DID_IS_IN_DYNAMIC_RANGE(_x) (((_x) >= 0xF200) && ((_x) <= 0xF3FF))

typedef enum {
    DSD_TX_RESPONSE_READY,
    DSD_TX_RESPONSE_SUPPRESSED
} DsdProcessingDoneResultType;


typedef enum {
    DCM_READ_DID_IDLE,
    DCM_READ_DID_PENDING_COND_CHECK,
    DCM_READ_DID_PENDING_READ_DATA
} ReadDidPendingStateType;

typedef enum {
    DCM_NO_COM = 0,
    DCM_SILENT_COM,
    DCM_FULL_COM
}DcmComModeType;

typedef enum {
    DCM_REQ_DSP = 0,
#if defined(DCM_USE_SERVICE_RESPONSEONEVENT) && defined(USE_NVM)
    DCM_REQ_ROE,
#endif
    DCM_NOF_REQUESTERS
}DcmProtocolRequesterType;

void DcmSetProtocolStartRequestsAllowed(boolean allowed);

#endif /* DCM_INTERNAL_H_ */

