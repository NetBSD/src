/*	$NetBSD: sunos_sigcode.s,v 1.1 2001/06/08 04:49:46 mrg Exp $	*/

/*
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by Paul Kranenburg.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore.s	8.4 (Berkeley) 12/10/93
 * from: NetBSD: locore.s,v 1.140 2001/05/26 10:22:32 pk Exp
 */

#include <machine/asm.h>
#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/signal.h>
#include <sys/syscall.h>
#include <compat/sunos/sunos_syscall.h>

#include "sigcode_state.s"

	.globl	_C_LABEL(sunos_sigcode)
	.globl	_C_LABEL(sunos_esigcode)
_C_LABEL(sunos_sigcode):

	SAVE_STATE

	ldd	[%fp + 64], %o0		! sig, code
	ld	[%fp + 76], %o3		! arg3
	call	%g1			! (*sa->sa_handler)(sig,code,scp,arg3)
	 add	%fp, 64 + 16, %o2	! scp

	RESTORE_STATE

	! get registers back & set syscall #
	restore	%g0, SUNOS_SYS_sigreturn, %g1
	add	%sp, 64 + 16, %o0	! compute scp
	t	ST_SYSCALL		! sigreturn(scp)
	! sigreturn does not return unless it fails
	mov	SUNOS_SYS_exit, %g1		! exit(errno)
	t	ST_SYSCALL
_C_LABEL(sunos_esigcode):
