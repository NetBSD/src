/*	$NetBSD: firepower_intr.c,v 1.6 2003/07/15 02:46:31 lukem Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
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

/*
 * Interrupt support for Firepower systems.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: firepower_intr.c,v 1.6 2003/07/15 02:46:31 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/autoconf.h>

#include <powerpc/pio.h>

#include <ofppc/firepower/firepowerreg.h>
#include <ofppc/firepower/firepowervar.h>

int	firepower_splraise(int);
int	firepower_spllower(int);
void	firepower_splx(int);
void	firepower_setsoft(int);
void	firepower_clock_return(struct clockframe *, int);
void	*firepower_intr_establish(int, int, int, int (*)(void *), void *);
void	firepower_intr_disestablish(void *);
void	firepower_do_softnet(void);

struct machvec firepower_machvec = {
	firepower_splraise,
	firepower_spllower,
	firepower_splx,
	firepower_setsoft,
	firepower_clock_return,
	firepower_intr_establish,
	firepower_intr_disestablish,
};

void	firepower_intr_calculate_masks(void);
void	firepower_extintr(void);

/* Interrupts to mask at each level. */
int imask[NIPL];

/* Current interrupt priority level. */
 __volatile int cpl;

/* Number of clock interrupts pending. */
static __volatile int clockpending;

/* Other interrupts pending. */
 __volatile int ipending;

/*
 * The Firepower's system controller provides 32 interrupt sources.
 */
#define	NIRQ		32

struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_func)(void *);		/* handler */
	void *ih_arg;			/* arg for handler */
	int ih_ipl;			/* IPL_* */
	int ih_irq;			/* IRQ number */
};

#define	IRQNAMESIZE	sizeof("irq 31")

static struct intrq {
	TAILQ_HEAD(, intrhand) iq_list;	/* handler list */
	struct evcnt iq_ev;		/* event counter */
	int iq_mask;			/* IRQs to mask while handling */
	int iq_levels;			/* IPL_*'s this IRQ has */
	int iq_ist;			/* share type */
	char iq_name[IRQNAMESIZE];	/* interrupt name */
} intrq[NIRQ];

/*
 * Unused bit in interrupt enable register; we use this to represent
 * splclock.  (Actually, it's not unused, but we special-case this
 * interrupt, so we can use it in the imask[].)
 */
#define	SPL_CLOCK	INTR_CPU_ERR

/* Shadow copy of the interrupt enable register. */
static uint32_t intr_mask;

/* Map SI_* softintr index to intr_mask bit. */
static const uint32_t softint_to_intrmask[SI_NQUEUES] = {
	INTR_SOFT(0),		/* SI_SOFT */
	INTR_SOFT(1),		/* SI_SOFTCLOCK */
	INTR_SOFT(2),		/* SI_SOFTNET */
	INTR_SOFT(3),		/* SI_SOFTSERIAL */
};

static __inline void
firepower_enable_irq(int irq)
{

	intr_mask |= (1U << irq);
	CSR_WRITE4(FPR_INTR_MASK0, intr_mask);
}

static __inline void
firepower_disable_irq(int irq)
{

	intr_mask &= ~(1U << irq);
	CSR_WRITE4(FPR_INTR_MASK0, intr_mask);
}

void
firepower_intr_init(void)
{
	struct intrq *iq;
	int i;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);
		
		sprintf(iq->iq_name, "irq %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
		    NULL, "firepower", iq->iq_name);
	}

	firepower_intr_calculate_masks();

	machine_interface = firepower_machvec;
}

void
firepower_softintr_init(void)
{

	intr_establish(16 + SI_SOFTNET, INTR_SOFT(SI_SOFTNET),
	    IST_LEVEL, (void *)firepower_do_softnet, NULL);
}

/*
 * NOTE: This routine must be called with interrupts disabled in the MSR.
 */
void
firepower_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		firepower_disable_irq(irq);
		TAILQ_FOREACH(ih, &iq->iq_list, ih_list)
			levels |= (1U << ih->ih_ipl);
		iq->iq_levels = levels;
	}

	/* Next, figure out which IRQs are used by each IPL. */
	for (ipl = 0; ipl < NIPL; ipl++) {
		int irqs = 0;
		for (irq = 0; irq < NIRQ; irq++) {
			if (intrq[irq].iq_levels & (1U << ipl))
				irqs |= (1U << irq);
		}
		imask[ipl] = irqs;
	}

	imask[IPL_NONE] = 0;

	/*
	 * set up software interrupts.
	 */
	imask[IPL_SOFT] =	INTR_SOFT(SI_SOFT);
	imask[IPL_SOFTCLOCK] =	INTR_SOFT(SI_SOFTCLOCK);
	imask[IPL_SOFTNET] =	INTR_SOFT(SI_SOFTNET);
	imask[IPL_SOFTSERIAL] =	INTR_SOFT(SI_SOFTSERIAL);

	/*
	 * splsoftclock() is the only interface that users of the
	 * generic software interrupt facility have to block their
	 * soft intrs, so splsoftclock() must also block IPL_SOFT.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_SOFT];

	/*
	 * splsoftnet() must also block IPL_SOFTCLOCK, since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];

	/*
	 * Enfore a heirarchy that gives "slow" devices (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/*
	 * splvm() blocks all interrupts that use the kernel memory
	 * allocation facilities.
	 */
	imask[IPL_VM] |= imask[IPL_TTY];

	/*
	 * Audio devices are not allowed to perform memory allocation
	 * in their interrupt routines, and they have fairly "real-time"
	 * requirements, so give them a high interrupt priority.
	 */
	imask[IPL_AUDIO] |= imask[IPL_VM];

	/*
	 * splclock() must block anything that uses the scheduler.
	 */
	imask[IPL_CLOCK] |= SPL_CLOCK;		/* block the clock */
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];

	/*
	 * splhigh() must block "everything".
	 */
	imask[IPL_HIGH] |= imask[IPL_CLOCK];

	/*
	 * XXX We need serial drivers to run at the absolute highest priority
	 * in order to avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];

	/*
	 * Now compute which IRQs must be blocked when servicing any
	 * given IRQ.
	 */
	for (irq = 0; irq < NIRQ; irq++) {
		int irqs = (1U << irq);
		iq = &intrq[irq];
		if (TAILQ_FIRST(&iq->iq_list) != NULL)
			firepower_enable_irq(irq);
		TAILQ_FOREACH(ih, &iq->iq_list, ih_list)
			irqs |= imask[ih->ih_ipl];
		iq->iq_mask = irqs;
	}
}

static void
do_pending_int(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int emsr, dmsr;
	int new, irq, hwpend;
	static __volatile int processing;

	if (processing)
		return;
	processing = 1;

	emsr = mfmsr();
	dmsr = emsr & ~PSL_EE;

	new = cpl;

	for (;;) {
		cpl = new;

		mtmsr(dmsr);

		/*
		 * First check for missed clock interrupts.  We
		 * only process one of these here.
		 */
		if (clockpending && (cpl & SPL_CLOCK) == 0) {
			struct clockframe frame;

			cpl |= imask[IPL_CLOCK];
			clockpending--;
			mtmsr(emsr);

			/*
			 * Fake a clock interrupt frame
			 */
			frame.pri = new;
			frame.depth = curcpu()->ci_intrdepth + 1;
			frame.srr1 = 0;
			frame.srr0 = (int)firepower_splx;
			/*
			 * Do standard timer interrupt stuff
			 */
			hardclock(&frame);
			continue;
		}

		/*
		 * Note: we don't have to special-case any software
		 * interrupts, here, since we have hardware-assisted
		 * software interrupts.
		 */

		hwpend = ipending & ~cpl;
		if (hwpend) {
			ipending &= ~hwpend;
			while (hwpend) {
				irq = ffs(hwpend) - 1;
				hwpend &= ~(1U << irq);
				iq = &intrq[irq];
				iq->iq_ev.ev_count++;
				uvmexp.intrs++;
				cpl |= iq->iq_mask;
				mtmsr(emsr);
				TAILQ_FOREACH(ih, &iq->iq_list, ih_list) {
					(void) (*ih->ih_func)(ih->ih_arg);
				}
				mtmsr(dmsr);
				firepower_enable_irq(irq);
			}
			/*
			 * No point in re-enabling interrupts here, since
			 * we disable them again at the top of the loop.
			 */
			continue;
		}
		break;
	}

	mtmsr(emsr);
	processing = 0;
}

int
firepower_splraise(int ipl)
{
	int old;

	__asm __volatile("eieio; sync");	/* reorder protect */

	old = cpl;
	cpl |= ipl;

	__asm __volatile("eieio; sync");	/* reorder protect */
	return (old);
}

int
firepower_spllower(int ipl)
{
	int old = cpl;

	__asm __volatile("eieio; sync");	/* reorder protect */

	splx(ipl);
	return (old);
}

void
firepower_splx(int new)
{

	__asm __volatile("eieio; sync");	/* reorder protect */
	cpl = new;
	if (ipending & ~new)
		do_pending_int();
	__asm __volatile("eieio; sync");	/* reorder protect */
}

void
firepower_setsoft(int ipl)
{
	int bit;
	int msr;

	switch (ipl) {
	case IPL_SOFT:
		bit = SI_SOFT;
		break;

	case IPL_SOFTCLOCK:
		bit = SI_SOFTCLOCK;
		break;

	case IPL_SOFTNET:
		bit = SI_SOFTNET;
		break;

	case IPL_SOFTSERIAL:
		bit = SI_SOFTSERIAL;
		break;

	default:
		panic("firepower_setsoft: unknown soft IPL %d", ipl);
	}

#if 0
	/*
	 * XXX
	 * we'd like to use the hardware support for softints,
	 * but I haven't been able to get it to work so far.
	 */

	CSR_WRITE4(FPR_INTR_REQUEST_SET, softint_to_intrmask[bit]);
	msr = 0;
#else
	msr = mfmsr();
	mtmsr(msr & ~PSL_EE);
	ipending |= softint_to_intrmask[bit];
	mtmsr(msr);

	/* Check for pendings. */
	if (ipending & ~cpl) {
		do_pending_int();
	}

#endif
}

void *
firepower_intr_establish(int irq, int ipl, int ist,
    int (*func)(void *), void *arg)
{
	struct intrq *iq;
	struct intrhand *ih;
	int msr;

#ifdef DEBUG
	/* Filter out interrupts which are invalid. */
	switch (irq) {
	case 20:
	case 21:
	case 24:
		panic("firepower_intr_establish: reserved IRQ %d", irq);
	}
	if (irq < 0 || irq > NIRQ)
		panic("firepower_intr_establish: IRQ %d out of range", irq);

	/* All Firepower interrupts are level-triggered. */
	if (ist != IST_LEVEL)
		panic("firepower_intr_establish: not level-triggered");
#endif

	iq = &intrq[irq];

	ih = malloc(sizeof *ih, M_DEVBUF, M_NOWAIT);
	if (ih == NULL) {
		printf("firepower_intr_establish: unable to allocate "
		    "interrupt cookie\n");
		return (NULL);
	}

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_ipl = ipl;
	ih->ih_irq = irq;

	iq->iq_ist = ist;

	msr = mfmsr();
	mtmsr(msr & ~PSL_EE);
	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);
	firepower_intr_calculate_masks();
	mtmsr(msr);

	return (ih);
}

void
firepower_intr_disestablish(void *cookie)
{
	struct intrhand *ih = cookie;
	struct intrq *iq = &intrq[ih->ih_irq];
	int msr;

	msr = mfmsr();
	mtmsr(msr & ~PSL_EE);
	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);
	firepower_intr_calculate_masks();
	mtmsr(msr);

	free(ih, M_DEVBUF);
}

void
firepower_clock_return(struct clockframe *frame, int nticks)
{
	int pri, msr;

	pri = cpl;

	if (pri & SPL_CLOCK)
		clockpending += nticks;
	else {
		cpl = pri | imask[IPL_CLOCK];

		/* Reenable interrupts. */
		msr = mfmsr();
		mtmsr(msr | PSL_EE);

		/*
		 * Do standard timer interrupt stuff.  Do softclock stuff
		 * only on the last iteration.
		 */
		frame->pri = pri | softint_to_intrmask[SI_SOFTCLOCK];
		while (--nticks > 0)
			hardclock(frame);
		frame->pri = pri;
		hardclock(frame);

		/* Disable interrupts again. */
		mtmsr(msr);
		cpl = pri;
	}
}

/*
 * firepower_extintr:
 *
 *	Main interrupt handler routine, called from the EXT_INTR vector.
 */
void
firepower_extintr(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int msr, pcpl, irq, ibit, hwpend;

	pcpl = cpl;
	msr = mfmsr();

	for (hwpend = CSR_READ4(FPR_INTR_REQUEST); hwpend != 0;) {
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		hwpend &= ~ibit;

		/*
		 * First, disable the IRQ.  This allows us to
		 * put off a masked interrupt, and also to
		 * re-enable interrupts while we're processing
		 * the IRQ.
		 */
		firepower_disable_irq(irq);

		if (pcpl & ibit) {

			/*
			 * IRQ is masked; mark it as pending for processing
			 * when the IRQ becomes unblocked.
			 */
			ipending |= ibit;
			continue;
		}

		iq = &intrq[irq];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		cpl |= iq->iq_mask;
		mtmsr(msr | PSL_EE);
		TAILQ_FOREACH(ih, &iq->iq_list, ih_list) {
			(void) (*ih->ih_func)(ih->ih_arg);
		}
		mtmsr(msr);
		firepower_enable_irq(irq);

		cpl = pcpl;
	}

	/* Check for pendings. */
	if (ipending & ~cpl) {
		mtmsr(msr | PSL_EE);
		do_pending_int();
		mtmsr(msr);
	}
}

/****************************************************************************
 * PCI interrupt support
 ****************************************************************************/

int	firepower_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *firepower_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *firepower_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*firepower_pci_intr_establish(void *, pci_intr_handle_t,
	    int, int (*)(void *), void *);
void	firepower_pci_intr_disestablish(void *, void *);

void	*firepower_pciide_compat_intr_establish(void *, struct device *,
	    struct pci_attach_args *, int, int (*)(void *), void *);

void
firepower_pci_intr_init(pci_chipset_tag_t pc, void *v)
{

	pc->pc_intr_v = v;
	pc->pc_intr_map = firepower_pci_intr_map;
	pc->pc_intr_string = firepower_pci_intr_string;
	pc->pc_intr_evcnt = firepower_pci_intr_evcnt;
	pc->pc_intr_establish = firepower_pci_intr_establish;
	pc->pc_intr_disestablish = firepower_pci_intr_disestablish;

	pc->pc_pciide_compat_intr_establish =
	    firepower_pciide_compat_intr_establish;
}

int
firepower_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	int buspin = pa->pa_intrpin;
	int bustag = pa->pa_intrtag;
	int line = pa->pa_intrline;
	int bus, device, function;

	if (buspin == 0) {
		/* No IRQ used. */
		return (1);
	}
	if (buspin > 4) {
		printf("firepower_pci_intr_map: bad interrupt pin %d\n",
		    buspin);
		return (1);
	}

	pci_decompose_tag(pc, bustag, &bus, &device, &function);

	/*
	 * The OpenFirmware has placed the interrupt mapping in the "line"
	 * value.  A value of (char)-1 indicates there is no mapping.
	 */
	if (line == 0xff) {
		printf("firepower_pci_intr_map: no mapping for %d/%d/%d\n",
		    bus, device, function);
		return (1);
	}

	if (line < 16) {
		printf("firepower_pci_intr_map: %d/%d/%d at ISA IRQ %d ??\n",
		    bus, device, function, line);
		return (1);
	}

	if (line < 20) {
		printf("firepower_pci_intr_map: %d/%d/%d at soft IRQ %d ??\n",
		    bus, device, function, line - 16);
		return (1);
	}

	if (line > 26) {
		printf("firepower_pci_intr_map: %d/%d/%d IRQ %d out of range\n",
		    bus, device, function, line);
		return (1);
	}

	*ihp = line;
	return (0);
}

const char *
firepower_pci_intr_string(void *v, pci_intr_handle_t ih)
{
	static char intrstring[sizeof("firepower irq 31")];

	sprintf(intrstring, "firepower irq %lu", ih);
	return (intrstring);
}

const struct evcnt *
firepower_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	struct intrq *iq;

	iq = &intrq[ih];
	return (&iq->iq_ev);
}

void *
firepower_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	return (firepower_intr_establish(ih, level, IST_LEVEL, func, arg));
}

void
firepower_pci_intr_disestablish(void *v, void *cookie)
{

	firepower_intr_disestablish(cookie);
}

void *
firepower_pciide_compat_intr_establish(void *v, struct device *dev,
    struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{

	/* XXX for now */
	return (NULL);
}

void
firepower_do_softnet()
{
	int pisr, s;

	s = splsoftnet();
	pisr = netisr;
	netisr = 0;
	softnet(pisr);
	splx(s);
}
