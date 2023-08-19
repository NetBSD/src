/*	$NetBSD: linux_inotify.h,v 1.1 2023/08/19 17:57:54 christos Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Theodore Preduta.
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
#ifndef _LINUX_INOTIFY_H
#define	_LINUX_INOTIFY_H

#include <sys/types.h>
/* for LINUX_O_CLOEXEC, LINUX_O_NONBLOCK */
#include <compat/linux/common/linux_fcntl.h>

#define LINUX_IN_CLOEXEC	LINUX_O_CLOEXEC
#define LINUX_IN_NONBLOCK	LINUX_O_NONBLOCK

#define LINUX_IN_ALL_FLAGS	(LINUX_IN_CLOEXEC|LINUX_IN_NONBLOCK)

#define LINUX_IN_ACCESS		0x00000001
#define LINUX_IN_MODIFY		0x00000002
#define LINUX_IN_ATTRIB		0x00000004
#define LINUX_IN_CLOSE_WRITE	0x00000008
#define LINUX_IN_CLOSE_NOWRITE	0x00000010
#define LINUX_IN_OPEN		0x00000020
#define LINUX_IN_MOVED_FROM	0x00000040
#define LINUX_IN_MOVED_TO	0x00000080
#define LINUX_IN_CREATE		0x00000100
#define LINUX_IN_DELETE		0x00000200
#define LINUX_IN_DELETE_SELF	0x00000400
#define LINUX_IN_MOVE_SELF	0x00000800
#define LINUX_IN_UNMOUNT	0x00002000
#define LINUX_IN_Q_OVERFLOW	0x00004000
#define LINUX_IN_IGNORED	0x00008000

#define LINUX_IN_ONLYDIR	0x01000000
#define LINUX_IN_DONT_FOLLOW	0x02000000
#define LINUX_IN_EXCL_UNLINK	0x04000000
#define LINUX_IN_MASK_CREATE	0x10000000
#define LINUX_IN_MASK_ADD	0x20000000
#define LINUX_IN_ISDIR		0x40000000
#define LINUX_IN_ONESHOT	0x80000000

#define LINUX_IN_CLOSE		(LINUX_IN_CLOSE_WRITE|LINUX_IN_CLOSE_NOWRITE)

#define LINUX_IN_ADD_KNOWN	(LINUX_IN_ACCESS|LINUX_IN_ATTRIB \
				|LINUX_IN_CLOSE_WRITE|LINUX_IN_CLOSE_NOWRITE \
				|LINUX_IN_CREATE|LINUX_IN_DELETE \
				|LINUX_IN_DELETE_SELF|LINUX_IN_MODIFY \
				|LINUX_IN_MOVE_SELF|LINUX_IN_MOVED_FROM \
				|LINUX_IN_MOVED_TO|LINUX_IN_OPEN \
				|LINUX_IN_DONT_FOLLOW|LINUX_IN_ONLYDIR \
				|LINUX_IN_MASK_CREATE|LINUX_IN_MASK_ADD \
				|LINUX_IN_ISDIR|LINUX_IN_ONESHOT)

struct linux_inotify_event {
	int32_t		wd;
	uint32_t	mask;
	uint32_t	cookie;
	uint32_t	len;
	char		name[];
};

#ifdef _KERNEL
int linux_inotify_init(void);
int linux_inotify_fini(void);
#endif

#endif	/* !_LINUX_INOTIFY_H */
