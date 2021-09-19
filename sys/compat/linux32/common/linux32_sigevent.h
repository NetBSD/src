/*	$NetBSD: linux32_sigevent.h,v 1.2 2021/09/19 22:30:28 thorpej Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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

#ifndef _LINUX32_SIGEVENT_H
#define _LINUX32_SIGEVENT_H

typedef union linux32_sigval {
	int		sival_int;
	netbsd32_voidp	sival_ptr;
} linux32_sigval_t;

#define LINUX32_SIGEV_MAX  64
#ifndef LINUX32_SIGEV_PAD
#define LINUX32_SIGEV_PAD ((LINUX32_SIGEV_MAX -				\
			    (sizeof(linux32_sigval_t) + (sizeof(int) * 2))) / \
			   sizeof(int))
#endif

typedef struct linux32_sigevent {
	linux32_sigval_t sigev_value;	/* sizeof(pointer) */
	int sigev_signo;
	int sigev_notify;
	/* guaranteed to have natural pointer alignment */
	union {
		int pad[LINUX32_SIGEV_PAD];
		int tid;
		struct {
			void (*func)(linux32_sigval_t);
			void *attr;
		} _sigev_thread;
	} _sigev_un;
} linux32_sigevent_t;

int	linux32_to_native_sigevent(struct sigevent *,
	    const struct linux32_sigevent *);
int	linux32_sigevent_copyin(const void *, void *, size_t);

#endif /* _LINUX32_SIGEVENT_H */
