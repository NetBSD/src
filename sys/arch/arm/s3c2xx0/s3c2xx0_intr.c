/* $NetBSD: s3c2xx0_intr.c,v 1.4 2003/07/15 00:24:49 lukem Exp $ */

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
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
__KERNEL_RCSID(0, "$NetBSD: s3c2xx0_intr.c,v 1.4 2003/07/15 00:24:49 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/cpufunc.h>

#include <arm/s3c2xx0/s3c2xx0reg.h>
#include <arm/s3c2xx0/s3c2xx0var.h>

volatile uint32_t *s3c2xx0_intr_mask_reg;

static __inline void
__raise(int ipl)
{
	if (current_spl_level < ipl) {
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
		s3c2xx0_imask[i] &= ~mask;	/* Disable itnerrupt at upper
						 * level */

	/*
	 * Enforce a heirarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	s3c2xx0_imask[IPL_BIO] &= s3c2xx0_imask[IPL_SOFTNET];
	s3c2xx0_imask[IPL_NET] &= s3c2xx0_imask[IPL_BIO];
	s3c2xx0_imask[IPL_SOFTSERIAL] &= s3c2xx0_imask[IPL_NET];
	s3c2xx0_imask[IPL_TTY] &= s3c2xx0_imask[IPL_SOFTSERIAL];

	/*
	 * splvm() blocks all interrupts that use the kernel memory
	 * allocation facilities.
	 */
	s3c2xx0_imask[IPL_VM] &= s3c2xx0_imask[IPL_TTY];

	/*
	 * Audio devices are not allowed to perform memory allocation
	 * in their interrupt routines, and they have fairly "real-time"
	 * requirements, so give them a high interrupt priority.
	 */
	s3c2xx0_imask[IPL_AUDIO] &= s3c2xx0_imask[IPL_VM];

	/*
	 * splclock() must block anything that uses the scheduler.
	 */
	s3c2xx0_imask[IPL_CLOCK] &= s3c2xx0_imask[IPL_AUDIO];

	/*
	 * splhigh() must block "everything".
	 */
	s3c2xx0_imask[IPL_HIGH] &= s3c2xx0_imask[IPL_STATCLOCK];

	/*
	 * XXX We need serial drivers to run at the absolute highest priority
	 * in order to avoid overruns, so serial > high.
	 */
	s3c2xx0_imask[IPL_SERIAL] &= s3c2xx0_imask[IPL_HIGH];

}


static void
init_interrupt_masks(void)
{
	int i;
	s3c2xx0_imask[IPL_NONE] = 0xffffffff;

	for (i = IPL_BIO; i < IPL_SOFTSERIAL; ++i)
		s3c2xx0_imask[i] = SI_TO_IRQBIT(SI_SOFTSERIAL);
	for (; i < NIPL; ++i)
		s3c2xx0_imask[i] = 0;

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	s3c2xx0_imask[IPL_SOFT] = ~SI_TO_IRQBIT(SI_SOFT);
	s3c2xx0_imask[IPL_SOFTCLOCK] = ~SI_TO_IRQBIT(SI_SOFTCLOCK);
	s3c2xx0_imask[IPL_SOFTNET] = ~SI_TO_IRQBIT(SI_SOFTNET);

	/*
	 * splsoftclock() is the only interface that users of the
	 * generic software interrupt facility have to block their
	 * soft intrs, so splsoftclock() must also block IPL_SOFT.
	 */
	s3c2xx0_imask[IPL_SOFTCLOCK] &= s3c2xx0_imask[IPL_SOFT];

	/*
	 * splsoftnet() must also block splsoftclock(), since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	s3c2xx0_imask[IPL_SOFTNET] &= s3c2xx0_imask[IPL_SOFTCLOCK];

}


/*
 * Disable/enable interrupts.
 * This is for S3C2XX0's intergrated UART, which can't disable tx/rx
 * interrupts without disabling tx/rx by means of UART regsiters.
 */
void
s3c2xx0_mask_interrupts(int mask)
{
	int save = disable_interrupts(I32_bit);
	int i;

	for (i = 0; i < NIPL - 1; ++i)
		s3c2xx0_imask[i] &= ~mask;

	intr_mask = s3c2xx0_imask[current_spl_level];
	*s3c2xx0_intr_mask_reg = intr_mask;

	restore_interrupts(save);
}

void
s3c2xx0_unmask_interrupts(int mask)
{
	int save = disable_interrupts(I32_bit);
	int i;

	for (i = 0; i < ICU_LEN; ++i) {
		if ((mask & (1 << i)) == 0)
			continue;
		s3c2xx0_update_intr_masks(i, s3c2xx0_ilevel[i]);
	}

	intr_mask = s3c2xx0_imask[current_spl_level];
	*s3c2xx0_intr_mask_reg = intr_mask;

	restore_interrupts(save);
}

void
s3c2xx0_do_pending(void)
{
	static __cpu_simple_lock_t processing = __SIMPLELOCK_UNLOCKED;
	int oldirqstate, spl_save;

	if (__cpu_simple_lock_try(&processing) == 0)
		return;

	spl_save = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);

#define	DO_SOFTINT(si,ipl)						\
	if ((softint_pending & intr_mask) & SI_TO_IRQBIT(si)) {	\
		softint_pending &= ~SI_TO_IRQBIT(si);			\
                __raise(ipl);                                           \
		restore_interrupts(oldirqstate);			\
		softintr_dispatch(si);					\
		oldirqstate = disable_interrupts(I32_bit);		\
		s3c2xx0_setipl(spl_save);					\
	}

	do {
		DO_SOFTINT(SI_SOFTSERIAL, IPL_SOFTSERIAL);
		DO_SOFTINT(SI_SOFTNET, IPL_SOFTNET);
		DO_SOFTINT(SI_SOFTCLOCK, IPL_SOFTCLOCK);
		DO_SOFTINT(SI_SOFT, IPL_SOFT);
	} while (softint_pending & intr_mask);

	__cpu_simple_unlock(&processing);

	restore_interrupts(oldirqstate);
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
		dispatch_table[i].level = IPL_BIO;
	}

	init_interrupt_masks();

	_splraise(IPL_SERIAL);
	enable_interrupts(I32_bit);
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
#undef _setsoftintr
void
_setsoftintr(int si)
{
	return s3c2xx0_setsoftintr(si);
}
