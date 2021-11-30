/*************************************************************************
    > File Name: Dcm.h
    > SPDX-License-Identifier: BSD-2-Clause
    > Author: XXX
    > Created Time: 2021年11月17日 星期三 11时08分56秒
 ************************************************************************/

// Service IDs in this module defined by Autosar
#define DCM_START_OF_RECEPTION_ID			0x00u
#define DCM_INIT_ID							0x01u
#define DCM_COPY_RX_DATA_ID					0x02u
#define DCM_TP_RX_INDICATION_ID				0x03u
#define DCM_COPY_TX_DATA_ID					0x04u
#define DCM_TP_TX_CONFIRMATION_ID			0x05u
#define DCM_GET_SES_CTRL_TYPE_ID			0x06u
#define DCM_GET_SECURITY_LEVEL_ID			0x0du
#define DCM_GET_ACTIVE_PROTOCOL_ID			0x0fu
#define DCM_COMM_NO_COM_MODE_ENTERED_ID		0x21u
#define DCM_COMM_SILENT_COM_MODE_ENTERED_ID	0x22u
#define DCM_COMM_FULL_COM_MODE_ENTERED_ID	0x23u
#define DCM_MAIN_ID							0x25u
#define DCM_RESETTODEFAULTSESSION_ID        0x2au
#define DCM_EXTERNALSETNEGRESPONSE_ID       0x30u
#define DCM_EXTERNALPROCESSINGDONE_ID       0x31u

// Error codes produced by this module defined by Autosar
#define DCM_E_INTERFACE_TIMEOUT             0x01u
#define DCM_E_INTERFACE_VALUE_OUT_OF_RANGE  0x02u
#define DCM_E_INTERFACE_BUFFER_OVERFLOW     0x03u
#define DCM_E_INTERFACE_PROTOCOL_MISMATCH   0x04u
#define DCM_E_UNINIT                        0x05u
#define DCM_E_PARAM                         0x06u

#define DCM_DSL_RX_PDU_ID_LIST_LENGTH 12343
