/*	$NetBSD: syslog_private.h,v 1.4 2022/04/19 20:32:15 rillig Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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

#define DEC()							\
	do {							\
		if (prlen >= tbuf_left)				\
			prlen = tbuf_left - 1;			\
		p += prlen;					\
		tbuf_left -= prlen;				\
	} while (0)

struct syslog_fun {
	size_t (*timefun)(char *, size_t);
	int  (*errfun)(int, char *, size_t);
	int __sysloglike(3, 0) (*prfun)(char *, size_t, const char *, va_list);
	int (*lock)(const struct syslog_data *);
	int (*unlock)(const struct syslog_data *);
};

void _vxsyslogp_r(int, struct syslog_fun *, struct syslog_data *,
    const char *, const char *, const char *, va_list) __sysloglike(6, 0);
void _openlog_unlocked_r(const char *, int, int, struct syslog_data *);
void _closelog_unlocked_r(struct syslog_data *);

extern struct syslog_fun _syslog_ss_fun;
