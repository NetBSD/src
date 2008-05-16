/*	$NetBSD: mach_task.h,v 1.15.74.1 2008/05/16 02:23:44 yamt Exp $ */

/*-
 * Copyright (c) 2002-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_MACH_TASK_H_
#define	_MACH_TASK_H_

/* task_get_special_port */

#define MACH_TASK_KERNEL_PORT		1
#define MACH_TASK_HOST_PORT		2
#define MACH_TASK_BOOTSTRAP_PORT	4
#define MACH_TASK_WIRED_LEDGER_PORT	5
#define MACH_TASK_PAGED_LEDGER_PORT	6

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	int req_which_port;
} mach_task_get_special_port_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_msgh_body;
	mach_msg_port_descriptor_t rep_special_port;
	mach_msg_trailer_t rep_trailer;
} mach_task_get_special_port_reply_t;

/* mach_ports_lookup */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_ports_lookup_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_msgh_body;
	mach_msg_ool_ports_descriptor_t rep_init_port_set;
	mach_ndr_record_t rep_ndr;
	mach_msg_type_number_t rep_init_port_set_count;
	mach_msg_trailer_t rep_trailer;
} mach_ports_lookup_reply_t;

/* mach_set_special_port */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_msgh_body;
	mach_msg_port_descriptor_t req_special_port;
	mach_ndr_record_t req_ndr;
	int req_which_port;
} mach_task_set_special_port_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_task_set_special_port_reply_t;

/* task_threads */

typedef struct {
	 mach_msg_header_t req_msgh;
} mach_task_threads_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_ool_ports_descriptor_t rep_list;
	mach_ndr_record_t rep_ndr;
	mach_msg_type_number_t rep_count;
	mach_msg_trailer_t rep_trailer;
} mach_task_threads_reply_t;

/* task_get_exception_ports */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_exception_mask_t req_mask;
} mach_task_get_exception_ports_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_old_handler[32];
	mach_ndr_record_t rep_ndr;
	mach_msg_type_number_t rep_masks_count;
	mach_exception_mask_t rep_masks[32];
	mach_exception_behavior_t rep_old_behaviors[32];
	mach_thread_state_flavor_t rep_old_flavors[32];
	mach_msg_trailer_t rep_trailer;
} mach_task_get_exception_ports_reply_t;

/* task_set_exception_ports */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_new_port;
	mach_ndr_record_t req_ndr;
	mach_exception_mask_t req_mask;
	mach_exception_behavior_t req_behavior;
	mach_thread_state_flavor_t req_flavor;
} mach_task_set_exception_ports_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_task_set_exception_ports_reply_t;

/* task_info */

#define MACH_TASK_BASIC_INFO 4
struct mach_task_basic_info {
	mach_integer_t mtbi_suspend_count;
	mach_vm_size_t mtbi_virtual_size;
	mach_vm_size_t mtbi_resident_size;
	mach_time_value_t mtbi_user_time;
	mach_time_value_t mtbi_system_time;
	mach_policy_t mtbi_policy;
};

#define MACH_TASK_EVENTS_INFO 2
struct mach_task_events_info {
	mach_integer_t mtei_faults;
	mach_integer_t mtei_pageins;
	mach_integer_t mtei_cow_faults;
	mach_integer_t mtei_message_sent;
	mach_integer_t mtei_message_received;
	mach_integer_t mtei_syscalls_mach;
	mach_integer_t mtei_syscalls_unix;
	mach_integer_t mtei_csw;
};

#define MACH_TASK_THREAD_TIMES_INFO 3
struct mach_task_thread_times_info {
	mach_time_value_t mttti_user_time;
	mach_time_value_t mttti_system_time;
};

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_task_flavor_t req_flavor;
	mach_msg_type_number_t req_count;
} mach_task_info_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_count;
	mach_integer_t rep_info[8];
	mach_msg_trailer_t rep_trailer;
} mach_task_info_reply_t;

/* task_suspend */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_task_suspend_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_task_suspend_reply_t;

/* task_resume */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_task_resume_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_task_resume_reply_t;

/* task_terminate */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_task_terminate_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_task_terminate_reply_t;

#endif /* _MACH_TASK_H_ */
