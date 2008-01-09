/*	$NetBSD: linux_ipc.h,v 1.7.48.1 2008/01/09 01:51:11 matt Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _LINUX_IPC_H
#define _LINUX_IPC_H

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/ipc.h>

/*
 * Structs and values to handle the SYSV ipc/shm/msg calls implemented
 * in Linux. Most values match the NetBSD values (as they are both derived
 * from SysV values). Values that are the same may not be defined here.
 */

typedef int linux_key_t;

/*
 * The only thing different about the Linux ipc_perm structure is the
 * order of the fields.
 */
struct linux_ipc_perm {
	linux_key_t	l_key;
	ushort		l_uid;
	ushort		l_gid;
	ushort		l_cuid;
	ushort		l_cgid;
	ushort		l_mode;
	ushort		l_seq;
};

struct linux_ipc64_perm {
	linux_key_t	l_key;
	uint		l_uid;
	uint		l_gid;
	uint		l_cuid;
	uint		l_cgid;
	ushort		l_mode;
	ushort		l___pad1;
	ushort		l_seq;
	ushort		l___pad2;
	ulong		l___unused1;
	ulong		l___unused2;
};

#define LINUX_IPC_RMID	0
#define LINUX_IPC_SET	1
#define LINUX_IPC_STAT	2
#define LINUX_IPC_INFO	3

#define LINUX_IPC_64	0x100

#if defined (SYSVSEM) || defined(SYSVSHM) || defined(SYSVMSG)
#ifdef _KERNEL
__BEGIN_DECLS
void linux_to_bsd_ipc_perm(struct linux_ipc_perm *,
				       struct ipc_perm *);
void linux_to_bsd_ipc64_perm(struct linux_ipc64_perm *,
				       struct ipc_perm *);
void bsd_to_linux_ipc_perm(struct ipc_perm *,
				       struct linux_ipc_perm *);
void bsd_to_linux_ipc64_perm(struct ipc_perm *,
				       struct linux_ipc64_perm *);
__END_DECLS
#endif	/* !_KERNEL */
#endif	/* !SYSVSEM, !SYSVSHM, !SYSVMSG */

#endif /* !_LINUX_IPC_H */
