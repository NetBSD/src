/*	$NetBSD: compat_linux.h,v 1.1.6.2 2009/05/13 17:18:54 jym Exp $	*/
/*	$OpenBSD: compat_linux.h,v 1.8 2007/06/16 00:26:33 deraadt Exp $	*/

/*
 * Copyright (c) 2005 Uwe Stuehler <uwe@bsdx.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* This file must be included late, for redefinitions to take effect. */

#ifndef _LOCORE

#include <sys/param.h>
#include <sys/termios.h>
#include <sys/fd_set.h>
struct lwp;
struct vnode;

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_errno.h>

#define	INT_LIMIT(x)		(~((x)1 << (sizeof(x) * 8 - 1)))
#define	OFFSET_MAX		INT_LIMIT(long long)
#define	OFFT_OFFSET_MAX		INT_LIMIT(long)

#undef	SEEK_SET
#undef	SEEK_CUR
#define SEEK_SET		0
#define SEEK_CUR		1

#define	LINUX_EOVERFLOW		75

#endif /* !_LOCORE */

#include <compat/linux/linux_syscall.h>

/* linux/asm/unistd.h */
#define __NR_SYSCALL_BASE	0x900000
#define __NR_exit		(__NR_SYSCALL_BASE + LINUX_SYS_exit)
#define __NR_read		(__NR_SYSCALL_BASE + LINUX_SYS_read)
#define __NR_write		(__NR_SYSCALL_BASE + LINUX_SYS_write)
#define __NR_open		(__NR_SYSCALL_BASE + LINUX_SYS_open)
#define __NR_close		(__NR_SYSCALL_BASE + LINUX_SYS_close)
#define __NR_time		(__NR_SYSCALL_BASE + LINUX_SYS_time)
#define __NR_lseek32		(__NR_SYSCALL_BASE + LINUX_SYS_lseek)
#define __NR_ioctl		(__NR_SYSCALL_BASE + LINUX_SYS_ioctl)
#define __NR_select		(__NR_SYSCALL_BASE + LINUX_SYS_select)
#define __NR_stat		(__NR_SYSCALL_BASE + LINUX_SYS_stat)
#define __NR_syscall		(__NR_SYSCALL_BASE + 113)

#undef	SYS_select
#define SYS_select		__NR_select
