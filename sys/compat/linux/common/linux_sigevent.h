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

#ifndef _LINUX_SIGEVENT_H
#define _LINUX_SIGEVENT_H

#define LINUX_SIGEV_SIGNAL    0       /* notify via signal */
#define LINUX_SIGEV_NONE      1       /* other notification: meaningless */
#define LINUX_SIGEV_THREAD    2       /* deliver via thread creation */
#define LINUX_SIGEV_THREAD_ID 4       /* deliver to thread */

#define LINUX_SIGEV_MAX  64
#ifndef LINUX_SIGEV_PAD
#define LINUX_SIGEV_PAD  ((LINUX_SIGEV_MAX/sizeof(int)) - 3)
#endif

typedef struct linux_sigevent {
	sigval_t sigev_value;
	int sigev_signo;
	int sigev_notify;
	union {
		int pad[LINUX_SIGEV_PAD];
		int tid;
		struct {
			void (*func)(sigval_t);
			void *attr;
		} _sigev_thread;
	} _sigev_un;
} linux_sigevent_t;

#endif /* _LINUX_SIGEVENT_H */
