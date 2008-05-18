/*	$NetBSD: mach_thread.h,v 1.18.72.1 2008/05/18 12:33:23 yamt Exp $ */

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

#ifndef	_MACH_THREAD_H_
#define	_MACH_THREAD_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/proc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>

/* For mach_create_thread_child() */
struct mach_create_thread_child_args {
	struct lwp *mctc_lwp;
	struct lwp *mctc_oldlwp;
	mach_natural_t *mctc_state;
	int mctc_flavor;
	int mctc_child_done;
};

/* For mach_sys_syscall_thread_switch() */
#define MACH_SWITCH_OPTION_NONE	0
#define MACH_SWITCH_OPTION_DEPRESS	1
#define MACH_SWITCH_OPTION_WAIT	2
#define MACH_SWITCH_OPTION_IDLE	3

/* For mach_thread_info */
#define MACH_THREAD_BASIC_INFO 3
struct mach_thread_basic_info {
	mach_time_value_t	user_time;
	mach_time_value_t	system_time;
	mach_integer_t	cpu_usage;
	mach_policy_t	policy;
	mach_integer_t	run_state;
	mach_integer_t	flags;
	mach_integer_t	suspend_count;
	mach_integer_t	sleep_time;
};
#define MACH_TH_STATE_RUNNING		1
#define MACH_TH_STATE_STOPPED		2
#define MACH_TH_STATE_WAITING		3
#define MACH_TH_STATE_UNINTERRUPTIBLE	4
#define MACH_TH_STATE_HALTED		5

#define MACH_TH_FLAGS_SWAPPED	1
#define MACH_TH_FLAGS_IDLE	2

#define MACH_THREAD_SCHED_TIMESHARE_INFO 10
struct mach_policy_timeshare_info {
	mach_integer_t max_priority;
	mach_integer_t base_priority;
	mach_integer_t cur_priority;
	mach_boolean_t depressed;
	mach_integer_t depress_priority;
};

#define MACH_THREAD_SCHED_RR_INFO	11
#define MACH_THREAD_SCHED_FIFO_INFO	12

/* For mach_policy_t */
#define MACH_THREAD_STANDARD_POLICY	1
#define MACH_THREAD_TIME_CONSTRAINT_POLICY 2
#define MACH_THREAD_PRECEDENCE_POLICY	3

/* thread_policy */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_policy_t req_policy;
	mach_msg_type_number_t req_count;
	mach_integer_t req_base[0];
	mach_boolean_t req_setlimit;
} mach_thread_policy_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_thread_policy_reply_t;

/* mach_thread_create_running */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_thread_state_flavor_t req_flavor;
	mach_msg_type_number_t req_count;
	mach_natural_t req_state[0];
} mach_thread_create_running_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_child_act;
	mach_msg_trailer_t rep_trailer;
} mach_thread_create_running_reply_t;

/* mach_thread_info */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_thread_flavor_t req_flavor;
	mach_msg_type_number_t req_count;
} mach_thread_info_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_count;
	mach_integer_t rep_out[12];
	mach_msg_trailer_t rep_trailer;
} mach_thread_info_reply_t;

/* thread_get_state */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_thread_state_flavor_t req_flavor;
	mach_msg_type_number_t req_count;
} mach_thread_get_state_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_count;
	mach_integer_t rep_state[144];
	mach_msg_trailer_t rep_trailer;
} mach_thread_get_state_reply_t;

/* mach_thread_set_state */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_thread_state_flavor_t req_flavor;
	mach_msg_type_number_t req_count;
	mach_integer_t req_state[0];
} mach_thread_set_state_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_thread_set_state_reply_t;

/* thread_suspend */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_thread_suspend_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_thread_suspend_reply_t;

/* thread_resume */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_thread_resume_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_thread_resume_reply_t;

/* thread_abort */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_thread_abort_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_thread_abort_reply_t;

/* thread_set_policy */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_pset;
	mach_ndr_record_t req_ndr;
	mach_policy_t req_policy;
	mach_msg_type_number_t req_base_count;
	mach_integer_t req_base[0];
	mach_msg_type_number_t req_limit_count;
	mach_integer_t req_limit[0];
} mach_thread_set_policy_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_thread_set_policy_reply_t;

/* These are machine dependent functions */
int mach_thread_get_state_machdep(struct lwp *, int, void *, int *);
int mach_thread_set_state_machdep(struct lwp *, int, void *);
void mach_create_thread_child(void *);

#endif /* _MACH_THREAD_H_ */
