/*************************************************************************
    > File Name: Dcm.c
    > SPDX-License-Identifier: BSD-2-Clause
    > Author: XXX
    > Created Time: 2021年11月17日 星期三 10时47分09秒
 ************************************************************************/

#include <stdio.h>
#include "Dcm.h"
#include "Dcm_Internal.h"
#include "ComStack_Types.h"
#include <camkes/dataport.h>

// State variable
typedef enum
{
  DCM_UNINITIALIZED = 0,
  DCM_INITIALIZED
} Dcm_StateType;

/***********************************************
 * Interface for BSW modules and SW-Cs (8.3.2) *
 ***********************************************/
BufReq_ReturnType Dcm_StartOfReception(PduIdType dcmRxPduId, PduLengthType tpSduLength, dataport_ptr_t ptr1)
{
	/* Pointers are passed through shared memory and need to be converted */
	PduLengthType *rxBufferSizePtr = (PduLengthType *)dataport_unwrap_ptr(ptr1);

    BufReq_ReturnType returnCode;

	printf("Dcm: Dcm_StartOfReception begin dcmRxPduId=%d\n", dcmRxPduId);
	printf("Dcm: Dcm_StartOfReception begin rxBufferSizePtr=%d\n", *rxBufferSizePtr);
    //VALIDATE_RV(dcmState == DCM_INITIALIZED, DCM_START_OF_RECEPTION_ID, DCM_E_UNINIT, BUFREQ_NOT_OK);
    VALIDATE_RV(dcmRxPduId < DCM_DSL_RX_PDU_ID_LIST_LENGTH, DCM_START_OF_RECEPTION_ID, DCM_E_PARAM, BUFREQ_NOT_OK);

   //returnCode = DslStartOfReception(dcmRxPduId, tpSduLength, rxBufferSizePtr, FALSE);

	returnCode = BUFREQ_OK;
	printf("Dcm: Dcm_StartOfReception end\n");
    return returnCode;
}

int run(void){
	printf("Dcm: run begin\n");

	return 0;
}
