/*	$NetBSD: bl.h,v 1.4 2015/01/19 18:52:55 christos Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
#ifndef _BL_H
#define _BL_H

#include "blacklist.h"

struct sockcred;

typedef struct {
	bl_type_t bi_type;
	int *bi_fd;
	struct sockcred *bi_cred;
	char bi_msg[1024];
} bl_info_t;

typedef struct blacklist *bl_t;

__BEGIN_DECLS
bl_t bl_create2(bool, const char *, void (*)(int, const char *, ...));
bl_info_t *bl_recv(bl_t);
__END_DECLS

#define _PATH_BLSOCK "/tmp/blsock"
#define _PATH_BLCONF "/etc/blacklistd/conf"

#endif /* _BL_H */
typedef struct {
	uint32_t bl_len;
	uint32_t bl_version;
	uint32_t bl_type;
	char bl_data[];
} bl_message_t;

struct blacklist {
	int b_fd;
	int b_connected;
	const char *b_path;
	void (*b_fun)(int, const char *, ...);
	bl_info_t b_info;
};
#define BL_VERSION	1
