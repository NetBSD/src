/*	$NetBSD: linux_ipccall.h,v 1.7 1999/01/10 15:05:36 tron Exp $	*/

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

#ifndef _LINUX_IPCCALL_H
#define _LINUX_IPCCALL_H

/*
 * All linux architectures except alpha use the sys_ipc
 * syscall and need the associated defines.
 */
# if !defined(__alpha__)
/* Used on: arm, i386, m68k, mips, ppc, sparc, sparc64 */
/* Not used on: alpha */

/*
 * Defines for the numbers passes as the first argument to the
 * linux_ipc() call, and based on which the actual system calls
 * are made.
 */
#define LINUX_SYS_semop		1
#define LINUX_SYS_semget	2
#define LINUX_SYS_semctl	3
#define LINUX_SYS_msgsnd	11
#define LINUX_SYS_msgrcv	12
#define LINUX_SYS_msgget	13
#define LINUX_SYS_msgctl	14
#define LINUX_SYS_shmat		21
#define LINUX_SYS_shmdt		22
#define LINUX_SYS_shmget	23
#define LINUX_SYS_shmctl	24


#  ifdef SYSVSEM
inline int linux_semop __P((struct proc *, struct linux_sys_ipc_args *,
    register_t *));
inline int linux_semget __P((struct proc *, struct linux_sys_ipc_args *,
    register_t *));
#  endif


#  ifdef SYSVMSG
inline int linux_msgsnd __P((struct proc *, struct linux_sys_ipc_args *,
    register_t *));
inline int linux_msgrcv __P((struct proc *, struct linux_sys_ipc_args *,
    register_t *));
inline int linux_msgget __P((struct proc *, struct linux_sys_ipc_args *,
    register_t *));
#  endif


#  ifdef SYSVSHM
inline int linux_shmdt __P((struct proc *, struct linux_sys_ipc_args *,
    register_t *));
inline int linux_shmget __P((struct proc *, struct linux_sys_ipc_args *,
    register_t *));
#  endif

# endif
#endif /* !_LINUX_IPCCALL_H */
