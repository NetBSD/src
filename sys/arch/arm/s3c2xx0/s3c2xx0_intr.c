/* $NetBSD: s3c2xx0_intr.c,v 1.15.2.1 2012/04/17 00:06:07 yamt Exp $ */

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Common part of IRQ handlers for Samsung S3C2800/2400/2410 processors.
 * derived from i80321_icu.c
 */

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: s3c2xx0_intr.c,v 1.15.2.1 2012/04/17 00:06:07 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/s3c2xx0/s3c2xx0reg.h>
#include <arm/s3c2xx0/s3c2xx0var.h>

volatile uint32_t *s3c2xx0_intr_mask_reg;

static inline void
__raise(int ipl)
{
	if (curcpl() < ipl) {
		s3c2xx0_setipl(ipl);
	}
}
/*
 * modify interrupt mask table for SPL levels
 */
void
s3c2xx0_update_intr_masks(int irqno, int level)
{
	int mask = 1 << irqno;
	int i;


	s3c2xx0_ilevel[irqno] = level;

	for (i = 0; i < level; ++i)
		s3c2xx0_imask[i] |= mask;	/* Enable interrupt at lower
						 * level */
	for (; i < NIPL - 1; ++i)
		s3c2xx0_imask[i] &= ~mask;	/* Disable interrupt at upper
						 * level */

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	s3c2xx0_imask[IPL_VM] &= s3c2xx0_imask[IPL_SOFTSERIAL];
	s3c2xx0_imask[IPL_CLOCK] &= s3c2xx0_imask[IPL_VM];
	s3c2xx0_imask[IPL_HIGH] &= s3c2xx0_imask[IPL_CLOCK];
}

static int
stray_interrupt(void *cookie)
{
	int save;
	int irqno = (int) cookie;
	printf("stray interrupt %d\n", irqno);

	save = disable_interrupts(I32_bit);
	*s3c2xx0_intr_mask_reg &= ~(1U << irqno);
	restore_interrupts(save);

	return 0;
}
/*
 * Initialize interrupt dispatcher.
 */
void
s3c2xx0_intr_init(struct s3c2xx0_intr_dispatch * dispatch_table, int icu_len)
{
	int i;

	for (i = 0; i < icu_len; ++i) {
		dispatch_table[i].func = stray_interrupt;
		dispatch_table[i].cookie = (void *) (i);
		dispatch_table[i].level = IPL_VM;
		sprintf(dispatch_table[i].name, "irq %d", i);
		evcnt_attach_dynamic(&dispatch_table[i].ev, EVCNT_TYPE_INTR,
				     NULL, "s3c2xx0", dispatch_table[i].name);
	}

	global_intr_mask = ~0;		/* no intr is globally blocked. */

	_splraise(IPL_VM);
	enable_interrupts(I32_bit);
}

/*
 * initialize variables so that splfoo() doesn't touch illegal address.
 * called during bootstrap.
 */
void
s3c2xx0_intr_bootstrap(vaddr_t icureg)
{
	s3c2xx0_intr_mask_reg = (volatile uint32_t *)(icureg + INTCTL_INTMSK);
}



#undef splx
void
splx(int ipl)
{
	s3c2xx0_splx(ipl);
}

#undef _splraise
int
_splraise(int ipl)
{
	return s3c2xx0_splraise(ipl);
}

#undef _spllower
int
_spllower(int ipl)
{
	return s3c2xx0_spllower(ipl);
}
