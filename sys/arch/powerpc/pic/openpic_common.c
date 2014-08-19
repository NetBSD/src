/*	$NetBSD: openpic_common.c,v 1.6.6.1 2014/08/20 00:03:20 tls Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: openpic_common.c,v 1.6.6.1 2014/08/20 00:03:20 tls Exp $");

#include "opt_openpic.h"
#include "opt_interrupt.h"

#include <sys/param.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <powerpc/openpic.h>

#include <powerpc/pic/picvar.h>

volatile unsigned char *openpic_base;

void
opic_finish_setup(struct pic_ops *pic)
{
	uint32_t cpumask = 0;
	int i;

#ifdef OPENPIC_DISTRIBUTE
	for (i = 0; i < ncpu; i++)
		cpumask |= (1 << cpu_info[i].ci_index);
#else
	cpumask = 1;
#endif
	for (i = 0; i < pic->pic_numintrs; i++) {
		/* send all interrupts to all active CPUs */
		openpic_write(OPENPIC_IDEST(i), cpumask);
	}
}

void
openpic_set_priority(int cpu, int pri)
{
	u_int x;

	x = openpic_read(OPENPIC_CPU_PRIORITY(cpu));
	x &= ~OPENPIC_CPU_PRIORITY_MASK;
	x |= pri;
	openpic_write(OPENPIC_CPU_PRIORITY(cpu), x);
}

int
opic_get_irq(struct pic_ops *pic, int mode)
{

	return openpic_read_irq(curcpu()->ci_index);
}

void
opic_ack_irq(struct pic_ops *pic, int irq)
{

	openpic_eoi(curcpu()->ci_index);
}
