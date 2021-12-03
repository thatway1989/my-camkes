/*************************************************************************
    > File Name: Dcm.h
    > SPDX-License-Identifier: BSD-2-Clause
    > Author: XXX
    > Created Time: 2021年11月17日 星期三 11时08分56秒
 ************************************************************************/
#ifndef DCM_H_
#define DCM_H_

#define LOG_TAG "Dcm_service"

#include "log.h"
#include "Dcm_Cfg.h"
#include "ComStack_Types.h"
#include "Std_Types.h"
#include "Dcm_Lcfg.h"

/*! vendor and module identification */
# define DCM_VENDOR_ID			 30u
# define DCM_MODULE_ID			 0x0035u
/*! Dcm software module version */
#define DCM_SW_MAJOR_VERSION	 1u
#define DCM_SW_MINOR_VERSION	 0u
#define DCM_SW_PATCH_VERSION	 0u

#if (DCM_DEV_ERROR_DETECT == STD_ON)
// Error codes produced by this module defined by Autosar
#define DCM_E_INTERFACE_TIMEOUT             0x01u
#define DCM_E_INTERFACE_VALUE_OUT_OF_RANGE  0x02u
#define DCM_E_INTERFACE_BUFFER_OVERFLOW     0x03u
#define DCM_E_INTERFACE_PROTOCOL_MISMATCH   0x04u
#define DCM_E_UNINIT                        0x05u
#define DCM_E_PARAM                         0x06u

// Other error codes reported by this module
#define DCM_E_CONFIG_INVALID                0x40u
#define DCM_E_TP_LENGTH_MISMATCH            0x50u
#define DCM_E_UNEXPECTED_RESPONSE           0x60u
#define DCM_E_UNEXPECTED_EXECUTION          0x61u
#define DCM_E_INTEGRATION_ERROR             0x62u
#define DCM_E_WRONG_BUFFER                  0x63u
#define DCM_E_NOT_SUPPORTED                 0xfeu
#define DCM_E_NOT_IMPLEMENTED_YET           0xffu

// Service IDs in this module defined by Autosar
#define DCM_START_OF_RECEPTION_ID           0x00u
#define DCM_INIT_ID                         0x01u
#define DCM_COPY_RX_DATA_ID                 0x02u
#define DCM_TP_RX_INDICATION_ID             0x03u
#define DCM_COPY_TX_DATA_ID                 0x04u
#define DCM_TP_TX_CONFIRMATION_ID           0x05u
#define DCM_GET_SES_CTRL_TYPE_ID            0x06u
#define DCM_GET_SECURITY_LEVEL_ID           0x0du
#define DCM_GET_ACTIVE_PROTOCOL_ID          0x0fu
#define DCM_COMM_NO_COM_MODE_ENTERED_ID     0x21u
#define DCM_COMM_SILENT_COM_MODE_ENTERED_ID 0x22u
#define DCM_COMM_FULL_COM_MODE_ENTERED_ID   0x23u
#define DCM_MAIN_ID                         0x25u
#define DCM_RESETTODEFAULTSESSION_ID        0x2au
#define DCM_EXTERNALSETNEGRESPONSE_ID       0x30u
#define DCM_EXTERNALPROCESSINGDONE_ID       0x31u

// Other service IDs reported by this module
#define DCM_HANDLE_RESPONSE_TRANSMISSION_ID 0x80u
#define DCM_UDS_READ_DTC_INFO_ID            0x81u
#define DCM_UDS_RESET_ID                    0x82u
#define DCM_UDS_COMMUNICATION_CONTROL_ID    0x83u
#define DCM_CHANGE_DIAGNOSTIC_SESSION_ID    0x88u
#define DCM_GLOBAL_ID                       0xffu

#endif

void Dcm_Init( const Dcm_ConfigType *ConfigPtr ); /* @req DCM037 */

#endif /*DCM_H_*/
