/* $NetBSD: linux_mmap_powerpc.c,v 1.1 2001/01/19 01:36:51 manu Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 * This product includes software developed by the NetBSD
 * Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs.h>

#include <compat/linux/arch/powerpc/linux_types.h>
#include <compat/linux/arch/powerpc/linux_signal.h>
#include <compat/linux/arch/powerpc/linux_syscallargs.h>

#include <compat/linux/common/linux_mmap.h>

/*
 * This wraps linux_sys_powerpc_mmap() to linux_sys_mmap(). We do this
 * because the offset agrument has no the same length (see linux_mmap.h)
 *
 * XXX This should die. We should use a linux_off_t in the 
 * common linux_sys_mmap() function
 */
int linux_sys_powerpc_mmap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_powerpc_mmap_args /* {
		syscallarg(unsigned long) addr;
		syscallarg(size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(linux_off_t) offset;
	} */ *uap = v;
	struct linux_sys_mmap_args cma;

	SCARG(&cma,addr) = SCARG(uap, addr);
	SCARG(&cma,len) = SCARG(uap, len);
	SCARG(&cma,prot) = SCARG(uap, prot);
	SCARG(&cma,flags) = SCARG(uap, flags);
	SCARG(&cma,fd) = SCARG(uap, fd);
	SCARG(&cma,offset) = (off_t) SCARG(uap, offset);

	return linux_sys_mmap(p, &cma, retval);
}
