/*      $NetBSD: prog_ops.h,v 1.1.56.1 2024/06/20 18:20:56 martin Exp $ */

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
#include <sys/event.h>
#include <sys/time.h>

#ifndef CRUNCHOPS

struct prog_ops {
	int (*op_init)(void);

	int (*op_open)(const char *, int, ...);
	int (*op_close)(int);
	ssize_t (*op_read)(int, void *, size_t);
	int (*op_fcntl)(int, int, ...);
	int (*op_ioctl)(int, unsigned long, ...);
	int (*op_kqueue)(void);
	int (*op_kevent)(int, const struct kevent *, size_t, struct kevent *,
			 size_t, const struct timespec *);
};
extern const struct prog_ops prog_ops;

#define prog_init prog_ops.op_init
#define prog_open prog_ops.op_open
#define prog_close prog_ops.op_close
#define prog_read prog_ops.op_read
#define prog_kevent prog_ops.op_kevent
#define prog_ioctl prog_ops.op_ioctl
#define prog_fcntl prog_ops.op_fcntl
#define prog_kqueue prog_ops.op_kqueue

#else

#define prog_init ((int (*)(void))NULL)
#define prog_open open
#define prog_close close
#define prog_read read
#define prog_kevent kevent
#define prog_ioctl ioctl
#define prog_fcntl fcntl
#define prog_kqueue kqueue

#endif /* CRUNCHOPS */

#endif /* _PROG_OPS_H_ */
