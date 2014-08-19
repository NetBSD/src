/*	$NetBSD: process_machdep.c,v 1.5.22.1 2014/08/20 00:03:07 tls Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 *
 * Author: 
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.5.22.1 2014/08/20 00:03:07 tls Exp $");

#include <sys/param.h>
#include <sys/ptrace.h>

#include <machine/reg.h>
int
process_read_regs(struct lwp *l, struct reg *regs)
{
printf("%s: not yet\n", __func__);
	return 0;
}


int
process_write_regs(struct lwp *l, const struct reg *regs)
{
printf("%s: not yet\n", __func__);
	return 0;
}


int
process_read_fpregs(struct lwp *l, struct fpreg *fpregs, size_t *sz)
{
printf("%s: not yet\n", __func__);
	return 0;
}


int
process_write_fpregs(struct lwp *l, const struct fpreg *fpregs, size_t sz)
{
printf("%s: not yet\n", __func__);
	return 0;
}

/*
 * Set the process's program counter.
 */
int
process_set_pc(struct lwp *l, void *addr)
{
printf("%s: not yet\n", __func__);
	return 0;
}


int
process_sstep(struct lwp *l, int sstep)
{
printf("%s: not yet\n", __func__);
	return 0;
}

