/*	$NetBSD: mach_message.h,v 1.1.4.2 2001/08/24 00:08:51 nathanw Exp $	 */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef	_MACH_MESSAGE_H_
#define	_MACH_MESSAGE_H_

typedef u_int32_t mach_msg_bits_t;
typedef u_int32_t mach_msg_size_t;
typedef u_int32_t mach_msg_id_t;
typedef u_int32_t mach_msg_timeout_t;
typedef u_int32_t mach_msg_option_t;

/*
 * Options
 */
#define MACH_MSG_OPTION_NONE	0x00000000
#define	MACH_SEND_MSG		0x00000001
#define	MACH_RCV_MSG		0x00000002
#define MACH_RCV_LARGE		0x00000004
#define MACH_SEND_TIMEOUT	0x00000010
#define MACH_SEND_INTERRUPT	0x00000040
#define MACH_SEND_CANCEL	0x00000080
#define MACH_RCV_TIMEOUT	0x00000100
#define MACH_RCV_NOTIFY		0x00000200
#define MACH_RCV_INTERRUPT	0x00000400
#define MACH_RCV_OVERWRITE	0x00001000
#define MACH_SEND_ALWAYS	0x00010000
#define MACH_SEND_TRAILER	0x00020000	

typedef	struct {
	mach_msg_bits_t	msgh_bits;
	mach_msg_size_t	msgh_size;
	mach_port_t	msgh_remote_port;
	mach_port_t	msgh_local_port;
	mach_msg_size_t msgh_reserved;
	mach_msg_id_t	msgh_id;
} mach_msg_header_t;

#endif /* !_MACH_MESSAGE_H_ */
