/*	$NetBSD: mach_bootstrap.h,v 1.4.74.1 2008/05/16 02:23:44 yamt Exp $ */

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

#ifndef	_MACH_BOOTSTRAP_H_
#define	_MACH_BOOTSTRAP_H_

/* bootstrap_look_up */

#define MACH_BOOTSTRAP_MAX_NAME_LEN 128
typedef char mach_name_t[MACH_BOOTSTRAP_MAX_NAME_LEN];

typedef struct {
	mach_msg_header_t req_msgh;
} mach_bootstrap_look_up_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_integer_t rep_count;
	mach_port_t rep_bootstrap_port;
	mach_name_t rep_service_name;
	mach_msg_trailer_t rep_trailer;
} mach_bootstrap_look_up_reply_t;

#endif /* _MACH_BOOTSTRAP_H_ */
