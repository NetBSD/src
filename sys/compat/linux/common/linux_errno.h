/*	$NetBSD: linux_errno.h,v 1.13.44.1 2014/08/20 00:03:32 tls Exp $	*/

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

#ifndef _LINUX_ERRNO_H
#define _LINUX_ERRNO_H

#define LINUX_EPERM		 1
#define LINUX_ENOENT		 2
#define LINUX_ESRCH		 3
#define LINUX_EINTR		 4
#define LINUX_EIO		 5
#define LINUX_ENXIO		 6
#define LINUX_E2BIG		 7
#define LINUX_ENOEXEC		 8
#define LINUX_EBADF		 9
#define LINUX_ECHILD		10
#define LINUX_EAGAIN		11
#define LINUX_ENOMEM		12
#define LINUX_EACCES		13
#define LINUX_EFAULT		14
#define LINUX_ENOTBLK		15
#define LINUX_EBUSY		16
#define LINUX_EEXIST		17
#define LINUX_EXDEV		18
#define LINUX_ENODEV		19
#define LINUX_ENOTDIR		20
#define LINUX_EISDIR		21
#define LINUX_EINVAL		22
#define LINUX_ENFILE		23
#define LINUX_EMFILE		24
#define LINUX_ENOTTY		25
#define LINUX_ETXTBSY		26
#define LINUX_EFBIG		27
#define LINUX_ENOSPC		28
#define LINUX_ESPIPE		29
#define LINUX_EROFS		30
#define LINUX_EMLINK		31
#define LINUX_EPIPE		32
#define LINUX_EDOM		33
#define LINUX_ERANGE		34
#define LINUX_EDEADLK		35


/* Error numbers after here vary wildly    */
/* depending on the machine architechture. */
#if defined(__i386__)
#include <compat/linux/arch/i386/linux_errno.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_errno.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_errno.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_errno.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_errno.h>
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_errno.h>
#elif defined(__amd64__)
#include <compat/linux/arch/amd64/linux_errno.h>
#else
#define LINUX_SCERR_SIGN -
#include <compat/linux/common/linux_errno_generic.h>
#endif

/* Linux has no ENOTSUP error code.  */
#define LINUX_ENOTSUP		LINUX_EOPNOTSUPP

extern const int native_to_linux_errno[];

#endif /* !_LINUX_ERRNO_H */
