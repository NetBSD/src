/*	$NetBSD: rump_syscalls_compat.h,v 1.3.4.3 2010/10/22 07:22:47 uebayasi Exp $	*/

/*-
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RUMP_RUMP_SYSCALLS_COMPAT_H_
#define _RUMP_RUMP_SYSCALLS_COMPAT_H_

#ifndef _KERNEL
/*
 * Compat calls.  They're manual now.  Note the slightly non-standard
 * naming.  This is because we cannot exploit __RENAME() the same way
 * normal builds exploit it -- we want to build *new* files linked
 * against these symbols.  Note that the defines don't allow calling
 * the current ones from a old userland, should that be desired for
 * whatever reason.  
 */
#ifdef __NetBSD__
#include <sys/param.h>
#if !__NetBSD_Prereq__(5,99,7)
#define rump_sys_stat(a,b) rump_sys_nb5_stat(a,b)
#define rump_sys_lstat(a,b) rump_sys_nb5_lstat(a,b)
#define rump_sys_fstat(a,b) rump_sys_nb5_fstat(a,b)
#define rump_sys_pollts(a,b,c,d) rump_sys_nb5_pollts(a,b,c,d)
#define rump_sys_select(a,b,c,d,e) rump_sys_nb5_select(a,b,c,d,e)
#endif /* __NetBSD_Prereq */
#endif /* __NetBSD__ */
#endif /* _KERNEL */

#ifdef _BEGIN_DECLS
_BEGIN_DECLS  
#endif

struct stat;
struct pollfd;
struct timespec;
int rump_sys_nb5_stat(const char *, struct stat *);
int rump_sys_nb5_lstat(const char *, struct stat *);
int rump_sys_nb5_fstat(int, struct stat *);
int rump_sys_nb5_pollts(struct pollfd *, size_t,
			const struct timespec *, const void *);
int rump_sys_nb5_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

#ifdef _END_DECLS
_END_DECLS
#endif

#endif /* _RUMP_RUMP_SYSCALLS_COMPAT_H_ */
