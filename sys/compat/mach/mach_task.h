/*	$NetBSD: mach_task.h,v 1.1.2.2 2002/11/11 22:07:28 nathanw Exp $ */

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

#ifndef	_MACH_TASK_H_
#define	_MACH_TASK_H_

/* task_get_special_port */ 

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

int mach_task_get_special_port __P((struct proc *, mach_msg_header_t *));
int mach_ports_lookup __P((struct proc *, mach_msg_header_t *));

#endif /* _MACH_TASK_H_ */
