/* $NetBSD: ipi_openpic.c,v 1.1.2.1 2007/10/10 18:41:35 garbled Exp $ */
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipi_openpic.c,v 1.1.2.1 2007/10/10 18:41:35 garbled Exp $");

#include "opt_multiprocessor.h"
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <powerpc/openpic.h>
#include <powerpc/atomic.h>

#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>

#ifdef MULTIPROCESSOR

extern struct ipi_ops ipiops;
extern volatile u_long IPI[CPU_MAXNUM];
static void openpic_send_ipi(int, u_long);
static void openpic_establish_ipi(int, int, void *);

void
setup_openpic_ipi(void)
{
	ipiops.ppc_send_ipi = openpic_send_ipi;
	ipiops.ppc_establish_ipi = openpic_establish_ipi;
	ipiops.ppc_ipi_vector = 64;
}

static void
openpic_send_ipi(int target, u_long mesg)
{
	int cpumask=0, i;

	if (target == IPI_T_ALL) {
		for (i=0; i < ncpu; i++) {
			cpumask += 1 << i;
			atomic_setbits_ulong(&IPI[i], mesg);
		}
	} else if (target == IPI_T_NOTME) {
		for (i=0; i < ncpu; i++) {
			if (i != cpu_number())
				cpumask += 1 << i;
			atomic_setbits_ulong(&IPI[i], mesg);
		}
	} else {
		cpumask = 1 << target;
		atomic_setbits_ulong(&IPI[target], mesg);
	}

	openpic_write(OPENPIC_IPI(cpu_number(), 0), cpumask);
}

static void
openpic_establish_ipi(int type, int level, void *ih_args)
{
	intr_establish(ipiops.ppc_ipi_vector, type, level, ppcipi_intr,
	    ih_args);
}

#endif /*MULTIPROCESSOR*/
