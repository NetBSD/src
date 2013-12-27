/*	$NetBSD: linux_ipccall.h,v 1.16 2013/12/27 15:10:53 njoly Exp $	*/

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

#ifndef _LINUX_IPCCALL_H
#define _LINUX_IPCCALL_H

/*
 * All linux architectures except alpha use the sys_ipc
 * syscall and need the associated defines.
 */
# if !defined(__alpha__) && !defined(__amd64__)
/* Used on: arm, i386, m68k, mips, ppc, sparc, sparc64 */
/* Not used on: alpha, amd64 */

/*
 * Defines for the numbers passes as the first argument to the
 * linux_ipc() call, and based on which the actual system calls
 * are made.
 */
#define LINUX_SYS_SEMOP		1
#define LINUX_SYS_SEMGET	2
#define LINUX_SYS_SEMCTL	3
#define LINUX_SYS_MSGSND	11
#define LINUX_SYS_MSGRCV	12
#define LINUX_SYS_MSGGET	13
#define LINUX_SYS_MSGCTL	14
#define LINUX_SYS_SHMAT		21
#define LINUX_SYS_SHMDT		22
#define LINUX_SYS_SHMGET	23
#define LINUX_SYS_SHMCTL	24


#  ifdef SYSVSEM
int linux_semop(struct lwp *, const struct linux_sys_ipc_args *,
    register_t *);
int linux_semget(struct lwp *, const struct linux_sys_ipc_args *,
    register_t *);
#  endif


#  ifdef SYSVMSG
int linux_msgsnd(struct lwp *, const struct linux_sys_ipc_args *,
    register_t *);
int linux_msgrcv(struct lwp *, const struct linux_sys_ipc_args *,
    register_t *);
int linux_msgget(struct lwp *, const struct linux_sys_ipc_args *,
    register_t *);
#  endif


#  ifdef SYSVSHM
int linux_shmdt(struct lwp *, const struct linux_sys_ipc_args *,
    register_t *);
int linux_shmget(struct lwp *, const struct linux_sys_ipc_args *,
    register_t *);
#  endif

# endif
#endif /* !_LINUX_IPCCALL_H */
