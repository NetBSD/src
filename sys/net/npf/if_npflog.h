/*	$NetBSD: if_npflog.h,v 1.1.18.2 2017/12/03 11:39:03 jdolecek Exp $	*/

/*
 * Copyright 2001 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _NET_NPF_IF_NPFLOG_H_
#define _NET_NPF_IF_NPFLOG_H_

#ifndef _KERNEL
#error "not supposed to be exposed to userland"
#endif

#define NPFLOG_RULESET_NAME_SIZE	16

/*
 * For now, we use a header compatible with pflog.
 * This will be improved in the future.
 */

struct npfloghdr {
	uint8_t		length;
	sa_family_t	af;
	uint8_t		action;
	uint8_t		reason;
	char		ifname[IFNAMSIZ];
	char		ruleset[NPFLOG_RULESET_NAME_SIZE];
	uint32_t	rulenr;
	uint32_t	subrulenr;
	uint32_t	uid;
	uint32_t	pid;
	uint32_t	rule_uid;
	uint32_t	rule_pid;
	uint8_t		dir;
	uint8_t		pad[3];
};

#define DLT_NPFLOG	DLT_PFLOG

#define NPFLOG_HDRLEN		sizeof(struct npfloghdr)
#define NPFLOG_REAL_HDRLEN	offsetof(struct npfloghdr, pad)

#endif /* _NET_NPF_IF_NPFLOG_H_ */
