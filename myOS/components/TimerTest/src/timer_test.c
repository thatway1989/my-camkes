/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <camkes.h>
#include <stdio.h>
#include <autoconf.h>

int g_ns;

/* This is need hardware, you can refer to camkes code projects/camkes/apps/timeserver example learning.
void periodic_test_case()
{
	seL4_CPtr notification = timeout_notification();

	timeout_periodic(0, g_ns);
	for (int i = 0; i < 3; i++) {
		seL4_Wait(notification, NULL);
		printf("%s:ns is %d\n.",get_instance_name(), g_ns);
		timeout_completed();
	}
	timeout_stop(0);
}
*/

static void event_callback(void *_ UNUSED)
{
	printf("%s:buf is %s\n.",get_instance_name(), (char *)buf);
	g_ns = atoi((char *)buf) * 100000000;
	//periodic_test_case();
}

int run(void)
{
	printf("timer test: %s run!\n",get_instance_name());
	TimerTest_reg_callback(event_callback, NULL);

    return 0;
}
