/* $NetBSD: ipi_hammerhead.c,v 1.2.26.1 2008/06/02 13:22:23 mjf Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: ipi_hammerhead.c,v 1.2.26.1 2008/06/02 13:22:23 mjf Exp $");

#include "opt_multiprocessor.h"
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <powerpc/atomic.h>

#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>

#ifdef MULTIPROCESSOR

extern struct ipi_ops ipiops;
extern volatile u_long IPI[CPU_MAXNUM];
static void hh_send_ipi(int, u_long);
static void hh_establish_ipi(int, int, void *);

#define HH_INTR_SECONDARY	0xf80000c0
#define HH_INTR_PRIMARY		0xf3019000
#define GC_IPI_IRQ		30

void
setup_hammerhead_ipi(void)
{
	ipiops.ppc_send_ipi = hh_send_ipi;
	ipiops.ppc_establish_ipi = hh_establish_ipi;
	ipiops.ppc_ipi_vector = GC_IPI_IRQ;
}

static void
hh_send_ipi(int target, u_long mesg)
{
	int cpu_id = target;

	if (target == IPI_T_ALL) {
		atomic_setbits_ulong(&IPI[0], mesg);
		atomic_setbits_ulong(&IPI[1], mesg);
		in32(HH_INTR_PRIMARY);
		out32(HH_INTR_SECONDARY, ~0);
		out32(HH_INTR_SECONDARY, 0);
		return;
	}

	if (target == IPI_T_NOTME) {
		switch (cpu_number()) {
		case 0:
			cpu_id = 1;
			break;
		case 1:
			cpu_id = 0;
		}
	}

	atomic_setbits_ulong(&IPI[cpu_id], mesg);
	switch (cpu_id) {
	case 0:
		in32(HH_INTR_PRIMARY);
		break;
	case 1:
		out32(HH_INTR_SECONDARY, ~0);
		out32(HH_INTR_SECONDARY, 0);
		break;
	}
}

static void
hh_establish_ipi(int type, int level, void *ih_args)
{
	intr_establish(ipiops.ppc_ipi_vector, type, level, ppcipi_intr,
	    ih_args);
}

#endif /*MULTIPROCESSOR*/
