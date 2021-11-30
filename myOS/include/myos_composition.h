/*************************************************************************
    > File Name: myos_composition.h
    > SPDX-License-Identifier: BSD-2-Clause
    > Author: XXX
    > Created Time: 2021年11月17日 星期三 14时11分06秒
 ************************************************************************/

#define VM_COMFIGURATION_EXTRA_DEF() \
        vm0.num_extra_frame_caps = 0; \
        vm0.extra_frame_map_address = 0; \
        vm0.cnode_size_bits = 23; \
        vm0.simple_untyped24_pool = 12; \
        vm0.crossvm_dp_0_id = 0; \
        vm0.crossvm_dp_0_size = 4096; \
        vm0.crossvm_dp_1_id = 0; \
        vm0.crossvm_dp_1_size = 4096;

#define CROSSVM_INIT_COMPOSITION_DEF() \
        component CrossvmInit crossvm_init; \
        connection seL4Notification event_conn_0(from vm0.ready, to crossvm_init.ready); \
        connection seL4GlobalAsynch event_conn_1(from crossvm_init.done, to vm0.done); \
        connection seL4SharedDataWithCaps cross_vm_conn_0(from crossvm_init.dest, to vm0.crossvm_dp_0); \
        connection seL4SharedDataWithCaps cross_vm_conn_1(from crossvm_init.src, to vm0.crossvm_dp_1); \
        connection seL4VMDTBPassthrough vm_dtb(from vm0.dtb_self, to vm0.dtb);

#define DCM_COMPOSITION_DEF() \
	component Dcm dcm; \
	connection seL4RPCCall crossvm_dcm_RPC(from crossvm_init.Dcm, to dcm.Dcm); \
	connection seL4SharedData crossvm_dcm_dataport(from crossvm_init.dataport_ptr1, to dcm.dataport_ptr1);

#define TIMERTEST_COMPOSITION_DEF() \
	component TimerTest timer1; \
	component TimerTest timer2; \
	connection seL4Notification crossvm_timertest_Event(from crossvm_init.TimerTest, to timer1.TimerTest, to timer2.TimerTest); \
	connection seL4SharedData crossvm_timertest_dataport(from crossvm_init.dataport_ptr1, to timer1.buf, to timer2.buf);
