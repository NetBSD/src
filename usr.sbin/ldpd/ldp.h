/* $NetBSD: ldp.h,v 1.6 2013/01/26 19:44:52 kefren Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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

#ifndef _LDP_H_
#define _LDP_H_

#include <sys/types.h>
#include <netinet/in.h>

/* RFC5036 */
#define	LDP_PORT		646

#define	LDP_COMMAND_PORT	2626

#define	LDPD_VER		"0.4.0"

#define CONFFILE		"/etc/ldpd.conf"

extern char     my_ldp_id[20];

#define LDP_ID my_ldp_id

/* LDP Messages */
#define	LDP_NOTIFICATION	0x0001
#define	LDP_HELLO		0x0100
#define	LDP_INITIALIZE		0x0200
#define	LDP_KEEPALIVE		0x0201
#define	LDP_ADDRESS		0x0300
#define	LDP_ADDRESS_WITHDRAW	0x0301
#define	LDP_LABEL_MAPPING	0x0400
#define	LDP_LABEL_REQUEST	0x0401
#define	LDP_LABEL_WITHDRAW	0x0402
#define	LDP_LABEL_RELEASE	0x0403
#define	LDP_LABEL_ABORT		0x0404

/* Protocol version */
#define	LDP_VERSION		1

/* Various timers */
#define	LDP_HELLO_TIME 5
#define	LDP_HELLO_KEEP 15
#define	LDP_THELLO_KEEP 45
#define	LDP_KEEPALIVE_TIME 4
#define	LDP_HOLDTIME 15

#define	MIN_LABEL		16
#define	MAX_LABEL		1048576

#define	ROUTE_LOOKUP_LOOP	6
#define	REPLAY_MAX		100
#define	MAX_POLL_FDS		200

void	print_usage(char*);

#endif	/* !_LDP_H_ */
