/*	$NetBSD: mach_notify.h,v 1.8 2003/12/09 12:13:44 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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

#ifndef _MACH_NOTIFICATION_H_
#define _MACH_NOTIFICATION_H_

#define MACH_NOTIFY_DELETED_MSGID 65
#define MACH_NOTIFY_DESTROYED_MSGID 69
#define MACH_NOTIFY_NO_SENDERS_MSGID 70
#define MACH_NOTIFY_SEND_ONCE_MSGID 71
#define MACH_NOTIFY_DEAD_NAME_MSGID 72

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_name_t req_name;
	mach_msg_trailer_t req_trailer;
} mach_notify_port_deleted_request_t;

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_rights;
	mach_msg_trailer_t req_trailer;
} mach_notify_port_destroyed_request_t;

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_mscount_t req_mscount;
	mach_msg_trailer_t req_trailer;
} mach_notify_port_no_senders_request_t;

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_trailer_t req_trailer;
} mach_notify_send_once_request_t;

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_port_name_t req_name;
	mach_msg_trailer_t req_trailer;
} mach_notify_port_dead_name_request_t;

void mach_notify_port_destroyed(struct lwp *, struct mach_right *);
void mach_notify_port_no_senders(struct lwp *, struct mach_right *);
void mach_notify_port_dead_name(struct lwp *, struct mach_right *);

#endif /* _MACH_NOTIFICATION_H_ */

