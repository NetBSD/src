/* $NetBSD: machdep.h,v 1.8 2012/01/14 17:31:09 reinoud Exp $ */

/*-
 * Copyright (c) 2011 Reinoud Zandijk <reinoud@netbsd.org>
 * All rights reserved.
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

int	md_syscall_check_opcode(ucontext_t *ucp);
void	md_syscall_get_opcode(ucontext_t *ucp, uint32_t *opcode);
void	md_syscall_get_syscallnumber(ucontext_t *ucp, uint32_t *code);
int	md_syscall_getargs(lwp_t *l, ucontext_t *ucp, int nargs, int argsize,
		register_t *args);
void	md_syscall_set_returnargs(lwp_t *l, ucontext_t *ucp,
		int error, register_t *rval);
void	md_syscall_inc_pc(ucontext_t *ucp, uint32_t opcode);
void	md_syscall_dec_pc(ucontext_t *ucp, uint32_t opcode);
register_t md_get_pc(ucontext_t *ucp);
register_t md_get_sp(ucontext_t *ucp);

/* handlers */
void	syscall(void);
