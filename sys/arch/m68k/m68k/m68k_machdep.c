/*	$NetBSD: m68k_machdep.c,v 1.6.78.2 2010/08/11 22:52:19 yamt Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: m68k_machdep.c,v 1.6.78.2 2010/08/11 22:52:19 yamt Exp $");

#include <sys/param.h>
#include <m68k/m68k.h>
#include <m68k/frame.h>

/* the following is used externally (sysctl_hw) */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

extern char ucas_32_ras_start[];
extern char ucas_32_ras_end[];
extern short exframesize[];

bool
ucas_ras_check(struct trapframe *v)
{
	struct frame *f = (void *)v;

	if (f->f_pc <= (vaddr_t)ucas_32_ras_start ||
	    f->f_pc >= (vaddr_t)ucas_32_ras_end) {
		return false;
	}
	f->f_pc = (vaddr_t)ucas_32_ras_start;
	f->f_stackadj = exframesize[f->f_format];
	f->f_format = f->f_vector = 0;
	return true;
}
