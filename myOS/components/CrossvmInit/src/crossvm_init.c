/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <camkes.h>
#include "ComStack_Types.h"
#include <camkes/dataport.h>

void my_cust_fun(char *arg)
{
	BufReq_ReturnType retVal = BUFREQ_NOT_OK;
	PduLengthType bufSize = 10;
	PduLengthType* bufferSizePtr = &bufSize;

	printf("CrossvmInit: my_cust_fun arg:%s\n", arg);

	/* Copy the pointer to shared memory and pass the shared memory pointer */
	memcpy(dataport_ptr1, bufferSizePtr, sizeof(dataport_ptr1 - 1));

	retVal = Dcm_StartOfReception(1, 10, dataport_wrap_ptr((void *)dataport_ptr1));
	printf("CrossvmInit: Dcm_StartOfReception\n");
	printf("CrossvmInit:Dcm_StartOfReception retVal=%d", retVal);
}

void my_cust_timer()
{
	memcpy(dataport_ptr1, dest, sizeof(dataport_ptr1 - 1));

	printf("time is %d\n", atoi((char*)dest));
	printf("time is %d\n", atoi((char*)dataport_ptr1));
	TimerTest_emit();
}

int run(void)
{
    memset(dest, '\0', 4096);
    strcpy(dest, "This is a crossvm dataport test string\n");

    while (1) {
        ready_wait();
        printf("CrossvmInit:Got an event, and the dest is:%s\n", (char *)dest);

		//my_cust_fun((char *)dest);
		my_cust_timer();
		printf("CrossvmInit: my cust fun end\n");
        //done_emit_underlying();
    }

    return 0;
}
