/*	$NetBSD: netbsd32_machdep.h,v 1.1 2002/10/23 13:26:36 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The world's simplest COMPAT_NETBSD32 shim ...
 */

#ifndef _SH5_NETBSD32_MACHDEP_H
#define _SH5_NETBSD32_MACHDEP_H

#include <sys/types.h>
#include <sys/proc.h>
#include <compat/netbsd32/netbsd32.h>

typedef int netbsd32_pointer_t;
#define	NETBSD32PTR64(p32)	((void *)(long)(int)(p32))

#define	register32_t	register_t

typedef netbsd32_pointer_t netbsd32_sigcontextp_t;

#define	netbsd32_sigcontext sigcontext

#define	netbsd32_sigcode	sigcode
#define	netbsd32_esigcode	esigcode
#define	netbsd32_sendsig	sendsig
#define	netbsd32_syscall_intern	syscall_intern

extern void netbsd32_setregs(struct proc *, struct exec_package *, u_long);

#endif /* _SH5_NETBSD32_MACHDEP_H */
