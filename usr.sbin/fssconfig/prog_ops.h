/*      $NetBSD: prog_ops.h,v 1.2 2016/07/31 02:15:54 pgoyette Exp $	*/

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <fcntl.h>

#ifndef CRUNCHOPS
struct prog_ops {
	int (*op_init)(void);
	int (*op_open)(const char *, int, ...);
	int (*op_ioctl)(int, unsigned long, ...);
	int (*op_close)(int);
	ssize_t (*op_pread)(int, void *, size_t, off_t);
	int (*op_statvfs1)(const char *, struct statvfs *, int);
	int (*op_stat)(const char *, struct stat *);
	int (*op_fstat)(int, struct stat *);
};
extern const struct prog_ops prog_ops;

#define prog_init prog_ops.op_init
#define prog_open prog_ops.op_open
#define prog_ioctl prog_ops.op_ioctl
#define prog_close prog_ops.op_close
#define prog_pread prog_ops.op_pread
#define prog_stat prog_ops.op_stat
#define prog_statvfs1 prog_ops.op_statvfs1
#define prog_fstat prog_ops.op_fstat
#else
#define prog_init ((int (*)(void))NULL)
#define prog_open open
#define prog_ioctl ioctl
#define prog_close close
#define prog_pread pread
#define prog_stat stat
#define prog_statvfs1 statvfs1
#define prog_fstat fstat
#endif

#endif /* _PROG_OPS_H_ */
