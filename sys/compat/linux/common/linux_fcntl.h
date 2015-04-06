/*	$NetBSD: linux_fcntl.h,v 1.16.4.1 2015/04/06 15:18:06 skrll Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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

#ifndef _LINUX_FCNTL_H
#define _LINUX_FCNTL_H

/*
 * The arguments in the flock structure have a different order from the
 * BSD structure.
 */

/* read/write mode for open(2) */
#define	LINUX_O_RDONLY		0x0000
#define LINUX_O_WRONLY		0x0001
#define LINUX_O_RDWR		0x0002
#define LINUX_O_ACCMODE		0x0003

#define LINUX_AT_FDCWD			-100
#define LINUX_AT_SYMLINK_NOFOLLOW	0x0100
#define LINUX_AT_REMOVEDIR		0x0200
#define LINUX_AT_SYMLINK_FOLLOW		0x0400
#define LINUX_AT_NO_AUTOMOUNT		0x0800
#define LINUX_AT_EMPTY_PATH		0x1000

int linux_to_bsd_ioflags(int);
int linux_to_bsd_atflags(int);

struct linux_flock {
	short       l_type;
	short       l_whence;
	linux_off_t l_start;
	linux_off_t l_len;
	linux_pid_t l_pid;
};

struct linux_flock64 {
	short       l_type;
	short       l_whence;
	off_t	    l_start;
	off_t	    l_len;
	linux_pid_t l_pid;
};

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_fcntl.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_fcntl.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_fcntl.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_fcntl.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_fcntl.h>
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_fcntl.h>
#elif defined(__amd64__)
#include <compat/linux/arch/amd64/linux_fcntl.h>
#else
#error Undefined linux_fcntl.h machine type.
#endif

/* Linux specific fcntl(2) commands */
#define	LINUX_F_SPECIFIC_BASE 	1024
#define	LINUX_F_SETLEASE 	(LINUX_F_SPECIFIC_BASE + 0)
#define	LINUX_F_GETLEASE 	(LINUX_F_SPECIFIC_BASE + 1)
#define	LINUX_F_NOTIFY 		(LINUX_F_SPECIFIC_BASE + 2)
#define	LINUX_F_CANCELLK 	(LINUX_F_SPECIFIC_BASE + 5)
#define	LINUX_F_DUPFD_CLOEXEC 	(LINUX_F_SPECIFIC_BASE + 6)
#define	LINUX_F_SETPIPE_SZ 	(LINUX_F_SPECIFIC_BASE + 7)
#define	LINUX_F_GETPIPE_SZ 	(LINUX_F_SPECIFIC_BASE + 8)

/*
 * We have to have 4 copies of the code that converts linux fcntl() file
 * locking to native form because there are 4 layouts for the structures.
 * To avoid replicating code, these defines are used.
 *
 * The next two functions take care of converting the flock
 * structure back and forth between Linux and NetBSD format.
 * The only difference in the structures is the order and size of
 * the fields, and the 'whence' value.
 */

#define conv_linux_flock(LINUX, FLOCK) \
static void \
bsd_to_##LINUX##_##FLOCK(struct LINUX##_##FLOCK *lfp, const struct flock *bfp) \
	copy_flock(lfp, LINUX_F, bfp, F) \
\
static void \
LINUX##_to_bsd_##FLOCK(struct flock *bfp, const struct LINUX##_##FLOCK *lfp) \
	copy_flock(bfp, F, lfp, LINUX_F)

#define copy_flock(dst, dst_f, src, src_f) { \
	dst->l_start = src->l_start; \
	dst->l_len = src->l_len; \
	dst->l_pid = src->l_pid; \
	dst->l_whence = src->l_whence; \
	switch (src->l_type) { \
	case src_f##_RDLCK: \
		dst->l_type = dst_f##_RDLCK; \
		break; \
	case src_f##_UNLCK: \
		dst->l_type = dst_f##_UNLCK; \
		break; \
	case src_f##_WRLCK: \
		dst->l_type = dst_f##_WRLCK; \
		break; \
	} \
    }

/*
 * These two defines do the work within the sys_fcntl() switch statement.
 */

#define do_linux_getlk(fd, cmd, arg, LINUX, FLOCK) do { \
	struct LINUX##_##FLOCK lfl; \
	struct flock bfl; \
	int fl_error; \
	if ((fl_error = copyin(arg, &lfl, sizeof lfl))) \
		return fl_error; \
	LINUX##_to_bsd_##FLOCK(&bfl, &lfl); \
	fl_error = do_fcntl_lock(fd, F_GETLK, &bfl); \
	if (fl_error) \
		return fl_error; \
	bsd_to_##LINUX##_##FLOCK(&lfl, &bfl); \
	return copyout(&lfl, arg, sizeof lfl); \
    } while (0)

#define do_linux_setlk(fd, cmd, arg, LINUX, FLOCK, setlk) do { \
	struct LINUX##_##FLOCK lfl; \
	struct flock bfl; \
	int fl_error; \
	if ((fl_error = copyin(arg, &lfl, sizeof lfl))) \
		return fl_error; \
	LINUX##_to_bsd_##FLOCK(&bfl, &lfl); \
	return do_fcntl_lock(fd, cmd == setlk ? F_SETLK : F_SETLKW, &bfl); \
    } while (0)


#endif /* !_LINUX_FCNTL_H */
