/*	$NetBSD: netbsd32_machdep.h,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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

#ifndef _MACHINE_NETBSD32_H_
#define _MACHINE_NETBSD32_H_

#include <sys/types.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/exec.h>

/*
 * On MIPS, pointers are signed.
 */
#define	NETBSD32_POINTER_TYPE			int32_t
typedef struct { NETBSD32_POINTER_TYPE i32; }	netbsd32_pointer_t;

#define NETBSD32_INT64_ALIGN

typedef netbsd32_pointer_t			netbsd32_sigcontextp_t;

/*
 * The sigcode is ABI neutral.
 */
#define	netbsd32_sigcode			sigcode
#define	netbsd32_esigcode			esigcode

/*
 * Note: syscall_intern does not care about COMPAT_NETBSD32.
 */
#define	netbsd32_syscall_intern			syscall_intern
void	netbsd32_setregs(struct lwp *, struct exec_package *, vaddr_t);

#endif /* _MACHINE_NETBSD32_H_ */
