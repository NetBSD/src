/*	$NetBSD: tr2a_intr.c,v 1.12.22.2 2011/04/21 01:41:02 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: tr2a_intr.c,v 1.12.22.2 2011/04/21 01:41:02 rmind Exp $");

#define __INTR_PRIVATE
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/evcnt.h>
#include <sys/lwp.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <machine/locore.h>	/* mips3_cp0* */
#include <machine/sbdvar.h>
#define	_SBD_TR2A_PRIVATE
#include <machine/sbd_tr2a.h>

SBD_DECL(tr2a);

const struct ipl_sr_map tr2a_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK,
	[IPL_VM] =		MIPS_INT_MASK & ~MIPS_INT_MASK_5,
	[IPL_SCHED] =		MIPS_INT_MASK,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

#define	NIRQ		16
enum bustype {
	ASOBUS,
	APBUS,
};

/* ASO + APbus interrupt */
struct tr2a_intr_handler {
	int (*func)(void *);
	void *arg;
	enum bustype bustype;
	uint32_t cpu_int;
	uint32_t aso_mask;
	struct evcnt evcnt;
	char evname[32];
} tr2a_intr_handler[NIRQ] = {
	[0]  = { NULL, NULL, ASOBUS, 2, 0x00000001 },	/* AM79C90 */
	[4]  = { NULL, NULL, ASOBUS, 4, 0x00300010 },	/* Z85230 (SIO) */
	[6]  = { NULL, NULL, ASOBUS, 2, 0x00000200 },	/* 53C710 SCSI-A */
	[9]  = { NULL, NULL, ASOBUS, 4, 0x00000040 },	/* Z85230 (KBMS) */
	[10] = { NULL, NULL, ASOBUS, 2, 0x00000100 },	/* 53C710 SCSI-B */
};

/* CPU interrupt */
struct tr2a_intc_handler {
	int ref_cnt;
	uint32_t mask;
} tr2a_intc_handler[6] = {
	[0] = { 0, 0x00000020 },
	[1] = { 0, 0x00000800 },
	[2] = { 0, 0x00010000 },
	[3] = { 0, 0x00200000 },
	[4] = { 0, 0x04000000 },
	[5] = { 0, 0x80000000 }
};

struct evcnt timer_tr2a_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intc", "timer");

void
tr2a_intr_init(void)
{

	/* Disable all ASObus interrupt */
#if 1
	*ASO_INT_MASK_REG &= ~TR2A_ASO_INTMASK_ALL;
#else
	/* open all interrupt for development. */
	*ASO_INT_MASK_REG |= TR2A_ASO_INTMASK_ALL;
#endif
	tr2a_wbflush();

	/* set up interval timer clock */
	*INTC_MASK_REG = 0;
	*CLOCK_REG = 0x80;
	tr2a_wbflush();
	if ((*CLOCK_REG & 0x80) != 0)
		*ASO_INT_MASK_REG |= 0x8000;	/* NMI (what UX does.) -uch */
	tr2a_wbflush();

	evcnt_attach_static(&timer_tr2a_ev);
}

void *
tr2a_intr_establish(int irq, int (*func)(void *), void *arg)
{
	struct tr2a_intr_handler *ih = &tr2a_intr_handler[irq];
	struct tr2a_intc_handler *ic = &tr2a_intc_handler[ih->cpu_int];
	int s;

	s = splhigh();
	ih->func = func;
	ih->arg  = arg;
	snprintf(ih->evname, sizeof(ih->evname), "irq %d", irq);
	evcnt_attach_dynamic(&ih->evcnt, EVCNT_TYPE_INTR,
	    NULL, "intc", ih->evname);

	if (ih->bustype == ASOBUS)
		*ASO_INT_MASK_REG |= ih->aso_mask;

	if (ic->ref_cnt++ == 0)
		*INTC_MASK_REG |= ic->mask;

	tr2a_wbflush();
	splx(s);

	return (void *)irq;
}

void
tr2a_intr_disestablish(void *arg)
{
	int s, irq = (int)arg;
	struct tr2a_intr_handler *ih = &tr2a_intr_handler[irq];
	struct tr2a_intc_handler *ic = &tr2a_intc_handler[ih->cpu_int];

	s = splhigh();
	if (ih->bustype == ASOBUS)
		*ASO_INT_MASK_REG &= ~ih->aso_mask;

	if (--ic->ref_cnt == 0)
		*INTC_MASK_REG &= ~ic->mask;

	ih->func = NULL;
	ih->arg = NULL;
	evcnt_detach(&ih->evcnt);
	splx(s);
}

void
tr2a_intr(int ppl, vaddr_t pc, uint32_t status)
{
	struct tr2a_intr_handler *ih;
	struct clockframe cf;
	uint32_t r, intc_cause, ipending;
	int ipl;

	intc_cause = *INTC_STATUS_REG & *INTC_MASK_REG;

	while (ppl < (ipl = splintr(&ipending))) {
		if ((ipending & MIPS_INT_MASK_5) && (intc_cause & INTC_INT5)) {
			cf.pc = pc;
			cf.sr = status;
			cf.intr = (curcpu()->ci_idepth > 1);
			tr2a_wbflush();
			*INTC_CLEAR_REG = 0x7c;
			*INTC_STATUS_REG;

			hardclock(&cf);
			timer_tr2a_ev.ev_count++;
		}

		if ((ipending & MIPS_INT_MASK_4) && (intc_cause & INTC_INT4)) {
			/* KBD, MOUSE, SERIAL */
			r = *ASO_INT_STATUS_REG;
			if (r & 0x300010) {
				ih = &tr2a_intr_handler[4];
				if (ih->func) {
					ih->func(ih->arg);
					ih->evcnt.ev_count++;
				}
			} else if (r & 0x40) {
				/* kbms */
				ih = &tr2a_intr_handler[9];
				if (ih->func) {
					ih->func(ih->arg);
					ih->evcnt.ev_count++;
				}
			} else if (r & 0x20) {
				printf("INT4 (1)\n");
			} else if (r & 0x00800000) {
				printf("INT4 (2)\n");
			} else if (r & 0x00400000) {
				printf("INT4 (3)\n");
			} else if (r != 0) {
				printf("not for INT4 %x\n", r);
			}

			tr2a_wbflush();
			*INTC_CLEAR_REG = 0x68;
			*INTC_STATUS_REG;
		}

		if ((ipending & MIPS_INT_MASK_3) && (intc_cause & INTC_INT3)) {
			/* APbus HI */
			printf("APbus HI\n");
			tr2a_wbflush();
			*INTC_CLEAR_REG = 0x54;
			*INTC_STATUS_REG;
		}

		if ((ipending & MIPS_INT_MASK_2) && (intc_cause & INTC_INT2)) {
			/* SCSI, ETHER */
			r = *ASO_INT_STATUS_REG;
			if (r & 0x100) {	/* SCSI-A */
				ih = &tr2a_intr_handler[6];
				if (ih->func) {
					ih->func(ih->arg);
					ih->evcnt.ev_count++;
				}
			} else if (r & 0x200) {	/* SCSI-B */
				ih = &tr2a_intr_handler[10];
				if (ih->func) {
					ih->func(ih->arg);
					ih->evcnt.ev_count++;
				}
			} else if (r & 0x1) {	/* LANCE */
				ih = &tr2a_intr_handler[0];
				if (ih->func) {
					ih->func(ih->arg);
					ih->evcnt.ev_count++;
				}
			} else if (r != 0) {
				printf("not for INT2 %x %x\n", r,
				    *ASO_DMAINT_STATUS_REG);
			}

			tr2a_wbflush();
			*INTC_CLEAR_REG = 0x40;
			*INTC_STATUS_REG;
		}

		if ((ipending & MIPS_INT_MASK_1) && (intc_cause & INTC_INT1)) {
			/* APbus LO */
			printf("APbus LO\n");
			tr2a_wbflush();
			*INTC_CLEAR_REG = 0x2c;
			*INTC_STATUS_REG;
		}

		if ((ipending & MIPS_INT_MASK_0) && (intc_cause & INTC_INT0)) {
			/* NMI etc. */
			r = *ASO_INT_STATUS_REG;
			printf("INT0 %08x\n", r);
			if (r & 0x8000) {
				printf("INT0(1) NMI\n");
			} else if (r & 0x8) {
				printf("INT0(2)\n");
			} else if (r & 0x4) {
				printf("INT0(3)\n");
			} else if (r != 0) {
				printf("not for INT0 %x\n", r);
			}
			tr2a_wbflush();
			*INTC_CLEAR_REG = 0x14;
			*INTC_STATUS_REG;
		}
		intc_cause = *INTC_STATUS_REG & *INTC_MASK_REG;
	}
}

void
tr2a_initclocks(void)
{

	/* Enable INT5 */
	*INTC_MASK_REG |= INTC_INT5;
	tr2a_wbflush();
}
