/*	$NetBSD: darwin_audit.h,v 1.3.74.1 2008/05/18 12:33:10 yamt Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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

#ifndef	_DARWIN_AUDIT_H_
#define	_DARWIN_AUDIT_H_

typedef uid_t darwin_au_id_t;
typedef pid_t darwin_au_asid_t;

struct darwin_au_mask {
	unsigned int    am_success;
	unsigned int    am_failure;
};

struct darwin_au_tid {
	dev_t port;
	unsigned int machine;
};

struct darwin_au_tid_addr {
	dev_t at_port;
	unsigned int at_type;
	unsigned int at_addr[4];
};

struct darwin_auditinfo {
	darwin_au_id_t	ai_auid;
	struct darwin_au_mask ai_mask;
	struct darwin_au_tid ai_termid;
	darwin_au_asid_t ai_asid;
};

struct darwin_auditinfo_addr {
	darwin_au_id_t	ai_auid;
	struct darwin_au_mask ai_mask;
	struct darwin_au_tid_addr ai_termid;
	darwin_au_asid_t ai_asid;
};

#endif /* _DARWIN_AUDIT_H_ */
