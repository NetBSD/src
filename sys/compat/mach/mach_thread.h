/*	$NetBSD: mach_thread.h,v 1.7 2003/01/21 04:06:08 matt Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/* thread_policy */
typedef int mach_policy_t;

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_policy_t req_policy;
	mach_msg_type_number_t req_count;
	mach_integer_t req_base[5];
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
	mach_natural_t req_state[144];
} mach_thread_create_running_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_child_act;
	mach_msg_trailer_t rep_trailer;
} mach_thread_create_running_reply_t;
	
int mach_thread_policy(struct mach_trap_args *);
int mach_thread_create_running(struct mach_trap_args *);
void mach_create_thread_child(void *);
void mach_copy_right(struct lwp *, struct lwp *);

#endif /* _MACH_THREAD_H_ */
