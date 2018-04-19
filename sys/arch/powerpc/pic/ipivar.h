/* $NetBSD: ipivar.h,v 1.9 2018/04/19 21:50:07 christos Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: ipivar.h,v 1.9 2018/04/19 21:50:07 christos Exp $");

#ifndef _IPI_VAR_H_
#define _IPI_VAR_H_

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#if defined(_KERNEL) && !defined(PPC_BOOKE)
struct ipi_ops {
	void (*ppc_send_ipi)(cpuid_t, uint32_t);
	/* type, level, arg */
	void (*ppc_establish_ipi)(int, int, void *);
	int ppc_ipi_vector;
};

/* target macros, 0+ are real cpu numbers */
#define IPI_DST_ALL	((cpuid_t) -2)
#define IPI_DST_NOTME	((cpuid_t) -1)

#define IPI_NOMESG		0x0000
#define IPI_HALT		0x0001
#define IPI_XCALL		0x0002
#define IPI_KPREEMPT		0x0004
#define IPI_GENERIC		0x0008
#define IPI_SUSPEND		0x0010

/* OpenPIC */
void setup_openpic_ipi(void);

/* IPI Handler */
int ipi_intr(void *);

/* convenience */
extern struct ipi_ops ipiops;

static __inline void
cpu_send_ipi(cpuid_t cpuid, uint32_t msg)
{
	(*ipiops.ppc_send_ipi)(cpuid, msg);
}
#endif

#endif /*_IPI_VAR_H_*/
