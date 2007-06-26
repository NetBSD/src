/*	$NetBSD: tr2_intr.c,v 1.5 2007/06/26 13:20:19 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
__KERNEL_RCSID(0, "$NetBSD: tr2_intr.c,v 1.5 2007/06/26 13:20:19 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/intr.h>
#include <machine/locore.h>	/* mips3_cp0* */
#include <machine/sbdvar.h>
#define	_SBD_TR2_PRIVATE
#include <machine/sbd_tr2.h>

#include <mips/cpuregs.h>

SBD_DECL(tr2);

const uint32_t tr2_sr_bits[_IPL_N] = {
	[IPL_NONE] = 0,
	[IPL_SOFT] =
	    MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTCLOCK] =
	    MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_SOFTSERIAL] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_BIO] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_2,
	[IPL_NET] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0|
	    MIPS_INT_MASK_2,
	[IPL_TTY] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_4,
	[IPL_CLOCK] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
	/* !!! TEST !!! VME INTERRUPT IS NOT MASKED */
};

#define	NIRQ		8
struct tr2_intr_handler {
	int (*func)(void *);
	void *arg;
	volatile uint8_t *picnic_reg;
	uint8_t picnic_mask;
	struct evcnt evcnt;
	char evname[32];
} tr2_intr_handler[NIRQ] = {
	[0] = { NULL, NULL, PICNIC_INT4_MASK_REG, PICNIC_INT_KBMS },
	[2] = { NULL, NULL, PICNIC_INT4_MASK_REG, PICNIC_INT_SERIAL },
	[5] = { NULL, NULL, PICNIC_INT2_MASK_REG, PICNIC_INT_SCSI },
	[6] = { NULL, NULL, PICNIC_INT2_MASK_REG, PICNIC_INT_ETHER },
	[7] = { NULL, NULL, PICNIC_INT0_MASK_REG, PICNIC_INT_FDDLPT },
};

struct evcnt timer_tr2_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "picnic", "timer");

void
tr2_intr_init(void)
{
	uint32_t a = (uint32_t)ews4800mips_nmi_vec;

	/* Install dump button handler address to NVSRAM (jumped from ROM) */
	*(NVSRAM_CDUMP_ADDR + 0) = (a >> 24) & 0xff;
	*(NVSRAM_CDUMP_ADDR + 4) = (a >> 16) & 0xff;
	*(NVSRAM_CDUMP_ADDR + 8) = (a >> 8) & 0xff;
	*(NVSRAM_CDUMP_ADDR +12) = a & 0xff;

	/* Disable external interrupts */
	*PICNIC_INT0_MASK_REG = 0;
	*PICNIC_INT2_MASK_REG = 0;
	*PICNIC_INT4_MASK_REG = 0;
	*PICNIC_INT5_MASK_REG = 0;

	evcnt_attach_static(&timer_tr2_ev);
}

void *
tr2_intr_establish(int irq, int (*func)(void *), void *arg)
{
	struct tr2_intr_handler *ih = &tr2_intr_handler[irq];
	int s;

	s = splhigh();
	ih->func = func;
	ih->arg = arg;
	snprintf(ih->evname, sizeof(ih->evname), "irq %d", irq);
	evcnt_attach_dynamic(&ih->evcnt, EVCNT_TYPE_INTR, NULL,
	    "picnic", ih->evname);

	*ih->picnic_reg |= ih->picnic_mask;
	splx(s);

	return (void *)irq;
}

void
tr2_intr_disestablish(void *arg)
{
	int s, irq = (int)arg;
	struct tr2_intr_handler *ih = &tr2_intr_handler[irq];

	s = splhigh();
	*ih->picnic_reg &= ~ih->picnic_mask;
	ih->func = NULL;
	ih->arg = NULL;
	evcnt_detach(&ih->evcnt);
	splx(s);
}

void
tr2_intr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
	struct tr2_intr_handler *ih;
	struct clockframe cf;
	uint32_t r, cause0;

	cause0 = cause;

	if (ipending & MIPS_INT_MASK_5) {	/* CLOCK */
		cf.pc = pc;
		cf.sr = status;

		*PICNIC_INT5_STATUS_REG = 0;
		r = *PICNIC_INT5_STATUS_REG;

		hardclock(&cf);
		timer_tr2_ev.ev_count++;
		cause &= ~MIPS_INT_MASK_5;
	}
	_splset((status & MIPS_INT_MASK_5) | MIPS_SR_INT_IE);

	if (ipending & MIPS_INT_MASK_4) {	/* KBD, MOUSE, SERIAL */
		r = *PICNIC_INT4_STATUS_REG;

		if (r & PICNIC_INT_KBMS) {
			ih = &tr2_intr_handler[0];
			if (ih->func) {
				ih->func(ih->arg);
				ih->evcnt.ev_count++;
			}
			r &= ~PICNIC_INT_KBMS;
		}

		if (r & PICNIC_INT_SERIAL) {
#if 0
			printf("SIO interrupt\n");
#endif
			ih = &tr2_intr_handler[2];
			if (ih->func) {
				ih->func(ih->arg);
				ih->evcnt.ev_count++;
			}
			r &= ~PICNIC_INT_SERIAL;
		}

		cause &= ~MIPS_INT_MASK_4;
	}
	_splset(((status & ~cause) & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	if (ipending & MIPS_INT_MASK_3) {	/* VME */
		printf("VME interrupt\n");

		r = *(volatile uint32_t *)0xbfb00018; /* NABI? */
		if ((r & 0x10) != 0) {
			/* vme high interrupt */
		} else if ((r & 0x4) != 0) {
			/* vme lo interrupt */
		} else {
			/* error */
		}
	}

	if (ipending & MIPS_INT_MASK_2) {	/* ETHER, SCSI */
		r = *PICNIC_INT2_STATUS_REG;

		if (r & PICNIC_INT_ETHER) {
			ih = &tr2_intr_handler[6];
			if (ih->func) {
				ih->func(ih->arg);
				ih->evcnt.ev_count++;
			}
			r &= ~PICNIC_INT_ETHER;
		}

		if (r & PICNIC_INT_SCSI) {
			ih = &tr2_intr_handler[5];
			if (ih->func) {
				ih->func(ih->arg);
				ih->evcnt.ev_count++;
			}
			r &= ~PICNIC_INT_SCSI;
		}

		if ((r & PICNIC_INT_FDDLPT) &&
		    ((cause0 & status) & MIPS_INT_MASK_5)) {
#ifdef DEBUG
			printf("FDD LPT interrupt\n");
#endif
			ih = &tr2_intr_handler[7];
			if (ih->func) {
				ih->func(ih->arg);
				ih->evcnt.ev_count++;
			}
			r &= ~PICNIC_INT_FDDLPT;
		}

		cause &= ~MIPS_INT_MASK_2;
	}
	_splset(((status & ~cause) & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	if (ipending & MIPS_INT_MASK_1)
		panic("unknown interrupt INT1\n");

	if (ipending & MIPS_INT_MASK_0) {	/* FDD, PRINTER */
		printf("printer, printer interrupt\n");
		r = *PICNIC_INT0_STATUS_REG;
		if (r & PICNIC_INT_FDDLPT) {
			printf("FDD, Printer interrupt.\n");
		} else {
			printf("unknown interrupt INT0\n");
		}
		cause &= ~MIPS_INT_MASK_0;
	}
	_splset(((status & ~cause) & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);
}

void
tr2_initclocks(void)
{

	/* Enable clock interrupt */
	*PICNIC_INT5_MASK_REG |= PICNIC_INT_CLOCK;
}
