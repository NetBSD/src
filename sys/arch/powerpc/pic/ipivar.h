/* $NetBSD: ipivar.h,v 1.3 2008/04/28 20:23:32 martin Exp $ */
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
__KERNEL_RCSID(0, "$NetBSD: ipivar.h,v 1.3 2008/04/28 20:23:32 martin Exp $");

#ifndef _IPI_VAR_H_
#define _IPI_VAR_H_

#include <machine/intr.h>

struct ipi_ops {
	void (*ppc_send_ipi)(int, u_long);
	/* type, level, arg */
	void (*ppc_establish_ipi)(int, int, void *);
	int ppc_ipi_vector;
};

/* target macros, 0+ are real cpu numbers */
#define IPI_T_ALL	-2
#define IPI_T_NOTME	-1

#define PPC_IPI_NOMESG		0x0000
#define PPC_IPI_HALT		0x0001
#define PPC_IPI_FLUSH_FPU	0x0002
#define PPC_IPI_FLUSH_VEC	0x0004

/* OpenPIC */
void setup_openpic_ipi(void);

/* IPI Handler */
int ppcipi_intr(void *);

/* convenience */
extern struct ipi_ops ipiops;

static inline void
ppc_send_ipi(int cpuid, u_long msg)
{
	ipiops.ppc_send_ipi(cpuid, msg);
}


#endif /*_IPI_VAR_H_*/
