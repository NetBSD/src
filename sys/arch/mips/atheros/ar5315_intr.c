/* $Id: ar5315_intr.c,v 1.1.8.4 2008/01/21 09:37:31 yamt Exp $ */
/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ar5315_intr.c,v 1.1.8.4 2008/01/21 09:37:31 yamt Exp $");

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/locore.h>
#include <mips/atheros/include/ar5315reg.h>
#include <mips/atheros/include/ar531xvar.h>

/*
 * Here's a little tidbit that can only be gleaned from Linux sources.
 *
 * IP2: (INT0) MISC
 * IP3: (INT1) WLAN0
 * IP4: (INT2) ENET0
 * IP7: (INT5) CPU CLOCK
 *
 * Only MISC interrupts are easily masked at the interrupt controller.
 * The others have to be masked at the source.
 */

#define	REGVAL(x)	*((volatile uint32_t *)(MIPS_PHYS_TO_KSEG1((x))))
#define	GETREG(x)	REGVAL((x) + AR5315_SYSREG_BASE)
#define	PUTREG(x,v)	(REGVAL((x) + AR5315_SYSREG_BASE)) = (v)

#define	NINTRS	3	/* MIPS INT2-INT4 (7 is clock interrupt) */
#define	NIRQS	9	/* bits in Miscellaneous Interrupt Status Register */

struct ar531x_intrhand {
	LIST_ENTRY(ar531x_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;
};

struct ar531x_intr {
	LIST_HEAD(, ar531x_intrhand) intr_l;
	struct evcnt	intr_count;
};

const uint32_t	ipl_sr_bits[_IPL_N] = {
	0,				/* 0: IPL_NONE */
	MIPS_SOFT_INT_MASK_0,		/* 1: IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_0,		/* 2: IPL_SOFTNET */

	MIPS_SOFT_INT_MASK_0 |
	MIPS_SOFT_INT_MASK_1 |
	MIPS_INT_MASK_0 |
	MIPS_INT_MASK_1 |
	MIPS_INT_MASK_2,		/* 3: IPL_VM */

	MIPS_INT_MASK,			/* 4: IPL_{SCHED,HIGH} */
};

static const char *ar5315_cpuintrnames[NINTRS] = {
	"int 2 (misc)",
	"int 3 (wlan)",
	"int 4 (enet)",
};

static const char *ar5315_miscintrnames[NIRQS] = {
	"misc 0 (uart)",
	"misc 1 (i2c)",
	"misc 2 (spi)",
	"misc 3 (ahb error)",
	"misc 4 (apb error)",
	"misc 5 (timer)",
	"misc 6 (gpio)",
	"misc 7 (watchdog)",
	"misc 8 (ir)"
};

static struct ar531x_intr ar5315_cpuintrs[NINTRS];
static struct ar531x_intr ar5315_miscintrs[NIRQS];

static int ar531x_miscintr(void *);

void
ar531x_intr_init(void)
{
	int	i;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&ar5315_cpuintrs[i].intr_l);
		evcnt_attach_dynamic(&ar5315_cpuintrs[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", ar5315_cpuintrnames[i]);
	}

	for (i = 0; i < NIRQS; i++) {
		LIST_INIT(&ar5315_miscintrs[i].intr_l);
		evcnt_attach_dynamic(&ar5315_miscintrs[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "ar5315", ar5315_miscintrnames[i]);
	}

	/* make sure we start without any misc interrupts enabled */
	GETREG(AR5315_SYSREG_ISR);
	PUTREG(AR5315_SYSREG_IMR, 0);

	/* make sure we register the MISC interrupt handler */
	ar531x_cpu_intr_establish(AR5315_CPU_IRQ_MISC, ar531x_miscintr, NULL);
}


void *
ar531x_cpu_intr_establish(int intr, int (*func)(void *), void *arg)
{
	struct ar531x_intrhand	*ih;
	int			s;

	if ((ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT)) == NULL)
		return NULL;

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = intr;

	if (ih == NULL)
		return NULL;

	s = splhigh();

	LIST_INSERT_HEAD(&ar5315_cpuintrs[intr].intr_l, ih, ih_q);

	/*
	 * The MIPS CPU interrupts are enabled at boot time, so they
	 * should pretty much always be ready to go.
	 */

	splx(s);
	return (ih);
}

void
ar531x_cpu_intr_disestablish(void *arg)
{
	struct ar531x_intrhand	*ih = arg;
	int			s;

	s = splhigh();

	LIST_REMOVE(ih, ih_q);

	splx(s);
	free(ih, M_DEVBUF);
}

void *
ar531x_misc_intr_establish(int irq, int (*func)(void *), void *arg)
{
	struct ar531x_intrhand	*ih;
	int			first;
	int			s;

	if ((ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT)) == NULL)
		return NULL;

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;

	if (ih == NULL)
		return NULL;

	s = splhigh();

	first = LIST_EMPTY(&ar5315_miscintrs[irq].intr_l);

	LIST_INSERT_HEAD(&ar5315_miscintrs[irq].intr_l, ih, ih_q);

	if (first) {
		uint32_t mask;
		mask = GETREG(AR5315_SYSREG_IMR);
		mask |= (1 << irq);
		PUTREG(AR5315_SYSREG_IMR, mask);
		GETREG(AR5315_SYSREG_IMR);	/* flush wbuffer */
	}

	splx(s);

	return ih;
}

void
ar531x_misc_intr_disestablish(void *arg)
{
	struct ar531x_intrhand	*ih = arg;
	int			s;

	s = splhigh();

	LIST_REMOVE(ih, ih_q);
	if (LIST_EMPTY(&ar5315_miscintrs[ih->ih_irq].intr_l)) {
		uint32_t	mask;
		mask = GETREG(AR5315_SYSREG_ISR);
		mask &= ~(1 << ih->ih_irq);
		PUTREG(AR5315_SYSREG_IMR, mask);
		GETREG(AR5315_SYSREG_IMR);	/* flush wbuffer */
	}

	splx(s);
	free(ih, M_DEVBUF);
}


int
ar531x_miscintr(void *arg)
{
	uint32_t		isr;
	int			mask;
	int			index;
	int			rv = 0;
	struct ar531x_intrhand	*ih;

	isr = GETREG(AR5315_SYSREG_ISR);
	mask = GETREG(AR5315_SYSREG_IMR);

	for (index = 0; index < NIRQS; index++) {

		if (isr & mask & (1 << index)) {
			ar5315_miscintrs[index].intr_count.ev_count++;
			LIST_FOREACH(ih, &ar5315_miscintrs[index].intr_l, ih_q)
			    rv |= (*ih->ih_func)(ih->ih_arg);
		}
	}

	return rv;
}

void
ar531x_cpuintr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
	uint32_t		mask;
	int			index;
	struct ar531x_intrhand	*ih;

	/* all others get normal handling */
	for (index = NINTRS - 1; index >= 0; index--) {
		mask = MIPS_INT_MASK_0 << index;

		if (ipending & mask) {
			ar5315_cpuintrs[index].intr_count.ev_count++;
			LIST_FOREACH(ih, &ar5315_cpuintrs[index].intr_l, ih_q)
			    (*ih->ih_func)(ih->ih_arg);
			cause &= ~mask;
		}
	}

	/* re-enable the stuff we processed */
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));
}
