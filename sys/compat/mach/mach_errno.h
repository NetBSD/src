/*	$NetBSD: mach_errno.h,v 1.7 2003/02/09 22:13:46 manu Exp $ */

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

#ifndef _MACH_ERRNO_H_
#define _MACH_ERRNO_H_

extern int native_to_mach_errno[];

#define MACH_KERN_SUCCESS			0
#define MACH_KERN_INVALID_ADDRESS		1
#define MACH_KERN_PROTECTION_FAILURE		2
#define MACH_KERN_NO_SPACE			3
#define MACH_KERN_INVALID_ARGUMENT		4
#define MACH_KERN_FAILURE			5
#define MACH_KERN_RESOURCE_SHORTAGE		6
#define MACH_KERN_NOT_RECEIVER			7
#define MACH_KERN_NO_ACCESS			8
#define MACH_KERN_MEMORY_FAILURE		9
#define MACH_KERN_MEMORY_ERROR			10
#define	MACH_KERN_ALREADY_IN_SET		11
#define MACH_KERN_NOT_IN_SET			12
#define MACH_KERN_NAME_EXISTS			13
#define MACH_KERN_ABORTED			14
#define MACH_KERN_INVALID_NAME			15
#define	MACH_KERN_INVALID_TASK			16
#define MACH_KERN_INVALID_RIGHT			17
#define MACH_KERN_INVALID_VALUE			18
#define	MACH_KERN_UREFS_OVERFLOW		19
#define	MACH_KERN_INVALID_CAPABILITY		20
#define MACH_KERN_RIGHT_EXISTS			21
#define	MACH_KERN_INVALID_HOST			22
#define MACH_KERN_MEMORY_PRESENT		23
#define MACH_KERN_MEMORY_DATA_MOVED		24
#define MACH_KERN_MEMORY_RESTART_COPY		25
#define MACH_KERN_INVALID_PROCESSOR_SET		26
#define MACH_KERN_POLICY_LIMIT			27
#define MACH_KERN_INVALID_POLICY		28
#define MACH_KERN_INVALID_OBJECT		29
#define MACH_KERN_ALREADY_WAITING		30
#define MACH_KERN_DEFAULT_SET			31
#define MACH_KERN_EXCEPTION_PROTECTED		32
#define MACH_KERN_INVALID_LEDGER		33
#define MACH_KERN_INVALID_MEMORY_CONTROL	34
#define MACH_KERN_INVALID_SECURITY		35
#define MACH_KERN_NOT_DEPRESSED			36
#define MACH_KERN_TERMINATED			37
#define MACH_KERN_LOCK_SET_DESTROYED		38
#define MACH_KERN_LOCK_UNSTABLE			39
#define MACH_KERN_LOCK_OWNED			40
#define MACH_KERN_LOCK_OWNED_SELF		41
#define MACH_KERN_SEMAPHORE_DESTROYED		42
#define MACH_KERN_RPC_SERVER_TERMINATED		43
#define MACH_KERN_RPC_TERMINATE_ORPHAN		44
#define MACH_KERN_RPC_CONTINUE_ORPHAN		45
#define	MACH_KERN_NOT_SUPPORTED			46
#define	MACH_KERN_NODE_DOWN			47
#define MACH_KERN_NOT_WAITING			48
#define	MACH_KERN_OPERATION_TIMED_OUT		49

/* IOKit errors */
#define MACH_IOKIT_ENOMEM       0xe00002bd
#define MACH_IOKIT_ENODEV       0xe00002c0
#define MACH_IOKIT_EAGAIN       0xe00002be
#define MACH_IOKIT_EPERM        0xe00002c1
#define MACH_IOKIT_EINVAL       0xe00002c2

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_error_reply_t;

int mach_msg_error(struct mach_trap_args *, int);
int mach_iokit_error(struct mach_trap_args *, int);

#endif	/* _MACH_ERRNO_H_ */
