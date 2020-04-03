/*      $NetBSD: prog_ops.h,v 1.5 2020/04/03 16:20:52 martin Exp $	*/

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#ifndef _PROG_OPS_H_
#define _PROG_OPS_H_

#include <sys/types.h>
#include <sys/sysctl.h>

#ifndef CRUNCHOPS
/*
 * This is shared between netstat and route (as they share some code)
 */
struct sysctlnode;
struct prog_ops {
	int (*op_init)(void);

	int (*op_socket)(int, int, int);
	int (*op_setsockopt)(int, int, int, const void *, socklen_t);


	int (*op_open)(const char *, int, ...);
	pid_t (*op_getpid)(void);

	ssize_t (*op_read)(int, void *, size_t);
	ssize_t (*op_write)(int, const void *, size_t);

	int (*op_shutdown)(int, int);

	int (*op_sysctl)(const int *, u_int, void *, size_t *,
			 const void *, size_t);

	int (*op_sysctlbyname)(const char *, void *, size_t *,
			 const void *, size_t);

	int (*op_sysctlgetmibinfo)(const char *, int *, u_int *,
			 char *, size_t *, struct sysctlnode **, int);

	int (*op_sysctlnametomib)(const char *, int *, size_t *);
};
extern const struct prog_ops prog_ops;

#define prog_init prog_ops.op_init

#define prog_socket prog_ops.op_socket
#define prog_setsockopt prog_ops.op_setsockopt

#define prog_open prog_ops.op_open
#define prog_getpid prog_ops.op_getpid

#define prog_read prog_ops.op_read
#define prog_write prog_ops.op_write

#define prog_shutdown prog_ops.op_shutdown

#define prog_sysctl prog_ops.op_sysctl

#define prog_sysctlbyname prog_ops.op_sysctlbyname
#define prog_sysctlgetmibinfo prog_ops.op_sysctlgetmibinfo
#define prog_sysctlnametomib prog_ops.op_sysctlnametomib

#else
#define prog_init ((int (*)(void))NULL)

#define prog_socket socket
#define prog_setsockopt setsockopt

#define prog_open open
#define prog_getpid getpid

#define prog_read read
#define prog_write write

#define prog_shutdown shutdown

#define prog_sysctl sysctl
#define prog_sysctlbyname sysctlbyname
#define prog_sysctlgetmibinfo sysctlgetmibinfo
#define prog_sysctlnametomib sysctlnametomib

#endif

#endif /* _PROG_OPS_H_ */
