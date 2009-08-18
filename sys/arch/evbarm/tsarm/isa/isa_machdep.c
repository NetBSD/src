/*	$NetBSD: isa_machdep.c,v 1.9 2009/08/18 17:02:00 dyoung Exp $	*/

/*-
 * Copyright (c) 1996-1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jesse Off
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mark Brinicombe, Charles M. Hannum and by Jason R. Thorpe of the
 * Numerical Aerospace Simulation Facility, NASA Ames Research Center.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.9 2009/08/18 17:02:00 dyoung Exp $");

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <machine/intr.h>
#include <machine/pio.h>
#include <machine/bootconfig.h>
#include <machine/isa_machdep.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <arm/ep93xx/ep93xxvar.h>
#include <uvm/uvm_extern.h>

/* prototypes */
void isa_tsarm_init(u_int, u_int);
struct arm32_isa_chipset isa_chipset_tag;

static unsigned int ep93xxirq[3] = { 20, 33, 40 };
/* only have 3 irqs connected to real lines on TS-7xxx */
static unsigned int isairq[3] = { 5, 6, 7 };
static unsigned int isairq_nhandlers[3] = { 0, 0, 0 };

int
isa_intr_alloc(isa_chipset_tag_t ic, int mask, int type, int *irq)
{
	int i, bestirq, count;

	if (type == IST_NONE)
		panic("intr_alloc: bogus type");

	bestirq = -1;
	count = -1;

	/* some interrupts should never be dynamically allocated */
	mask &= 0x00e0;

	for (i = 0; i < sizeof(isairq); i++) {
		if ((mask & (1<<isairq[i])) == 0)
			continue;
		if (isairq_nhandlers[i] < count || count == -1) {
			bestirq = isairq[i];
			count = isairq_nhandlers[i];
		}	
	}

	if (bestirq == -1)
		return (1);

	*irq = bestirq;

	return (0);
}

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{
	return NULL;
}

/*
 * Set up an interrupt handler to start being called.
 */
void *
isa_intr_establish(isa_chipset_tag_t ic, int irq, int type, int level, int (*ih_fun)(void *), void *ih_arg)
{
	int epirq = -1, i;
	/* Find real EP93XX irq number */
	for(i = 0; i < sizeof(isairq); i++) {
		if (irq == isairq[i]) epirq = ep93xxirq[i];
	}
	
	if (epirq == -1)
		return NULL;
	else
		return (ep93xx_intr_establish(epirq, level, ih_fun, ih_arg));
}

/*
 * Deregister an interrupt handler.
 */
void
isa_intr_disestablish(isa_chipset_tag_t ic, void *arg)
{
	ep93xx_intr_disestablish(arg);
}

void
isa_tsarm_init(u_int iobase16, u_int membase16)
{
        isa_io_init(iobase16, membase16);
}


/*
 * isa_intr_init()
 *
 * TODO: Set up ISA IRQ 5 on GPIO pin
 */
void
isa_intr_init(void)
{
}

void
isa_attach_hook(struct device *parent, struct device *self, struct isabus_attach_args *iba)
{
	/*
	 * Since we can only have one ISA bus, we just use a single
	 * statically allocated ISA chipset structure.  Pass it up
	 * now.
	 */
	iba->iba_ic = &isa_chipset_tag;
	printf(": PC/104 expansion bus");
}

void
isa_detach_hook(device_t self)
{
}
