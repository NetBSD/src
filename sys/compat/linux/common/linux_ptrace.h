/*	$NetBSD: linux_ptrace.h,v 1.4 2001/05/22 21:11:54 manu Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Scheler.
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

#ifndef _LINUX_PTRACE_H
#define _LINUX_PTRACE_H

#define LINUX_PTRACE_TRACEME		 0
#define LINUX_PTRACE_PEEKTEXT		 1
#define LINUX_PTRACE_PEEKDATA		 2
#define LINUX_PTRACE_PEEKUSR		 3
#define LINUX_PTRACE_POKETEXT		 4
#define LINUX_PTRACE_POKEDATA		 5
#define LINUX_PTRACE_POKEUSR		 6
#define LINUX_PTRACE_CONT		 7
#define LINUX_PTRACE_KILL		 8
#define LINUX_PTRACE_SINGLESTEP		 9
#define LINUX_PTRACE_GETREGS		12
#define LINUX_PTRACE_SETREGS		13
#define LINUX_PTRACE_GETFPREGS		14
#define LINUX_PTRACE_SETFPREGS		15
#define LINUX_PTRACE_ATTACH		16
#define LINUX_PTRACE_DETACH		17
#define LINUX_PTRACE_SYSCALL		24

#if defined(__i386__) || defined (__powerpc__)
int linux_sys_ptrace_arch __P((struct proc *, void *, register_t *));

#define LINUX_SYS_PTRACE_ARCH(p,v,r)	linux_sys_ptrace_arch((p),(v),(r))
#else 
#define LINUX_SYS_PTRACE_ARCH(p,v,r)	EIO
#endif

#endif /* !_LINUX_PTRACE_H */
