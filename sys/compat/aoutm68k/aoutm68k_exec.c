/*	$NetBSD: aoutm68k_exec.c,v 1.7 2001/09/18 19:36:38 jdolecek Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/signalvar.h>

#include <compat/aoutm68k/aoutm68k_syscall.h>
 
extern struct sysent aoutm68k_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const aoutm68k_syscallnames[];
#endif
extern char sigcode[], esigcode[];
void syscall __P((void));

struct emul emul_netbsd_aoutm68k = {
	"aoutm68k",
	"/emul/aout",
	EMUL_HAS_SYS___syscall,
	NULL,
	AOUTM68K_SYS_syscall,
	AOUTM68K_SYS_MAXSYSCALL,
	aoutm68k_sysent,
#ifdef SYSCALL_DEBUG
	aoutm68k_syscallnames,
#else
	NULL,
#endif
	sendsig,
	trapsignal,
	sigcode,
	esigcode,
	setregs,
	NULL,
	NULL,
	NULL,
	syscall
};
