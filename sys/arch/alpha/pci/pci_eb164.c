/*	$NetBSD: pci_eb164.c,v 1.1 1996/11/11 21:08:12 cgd Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <vm/vm.h>

#include <machine/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

#include <alpha/pci/pci_eb164.h>

#ifndef EVCNT_COUNTERS
#include <machine/intrcnt.h>
#endif

#include "sio.h"
#if NSIO
#include <alpha/pci/siovar.h>
#endif

int	dec_eb164_intr_map __P((void *, pcitag_t, int, int,
	    pci_intr_handle_t *));
const char *dec_eb164_intr_string __P((void *, pci_intr_handle_t));
void	*dec_eb164_intr_establish __P((void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *));
void	dec_eb164_intr_disestablish __P((void *, void *));

#if 0 /* XXX */
#define	KN20AA_PCEB_IRQ	31
#define	KN20AA_MAX_IRQ	32
#define	PCI_STRAY_MAX	5

struct eb164_intrhand {
	TAILQ_ENTRY(eb164_intrhand) ih_q;
        int     (*ih_fun)();
        void    *ih_arg;
        u_long  ih_count;
        int     ih_level;
};
TAILQ_HEAD(eb164_intrchain, eb164_intrhand);

struct eb164_intrchain eb164_pci_intrs[KN20AA_MAX_IRQ];
int	eb164_pci_strayintrcnt[KN20AA_MAX_IRQ];
#ifdef EVCNT_COUNTERS
struct evcnt eb164_intr_evcnt;
#endif
#endif /* 0 XXX */

void	eb164_pci_strayintr __P((int irq));
void	eb164_iointr __P((void *framep, unsigned long vec));
#if 0 /* XXX */
void	eb164_enable_intr __P((int irq));
void	eb164_disable_intr __P((int irq));
struct eb164_intrhand *eb164_attach_intr __P((struct eb164_intrchain *,
			    int, int (*) (void *), void *));
#endif /* 0 XXX */

void
pci_eb164_pickintr(ccp)
	struct cia_config *ccp;
{
	int i;
#if 0 /* XXX */
	struct eb164_intrhand *nintrhand;
#endif /* 0 XXX */
	bus_space_tag_t iot = ccp->cc_iot;
	pci_chipset_tag_t pc = &ccp->cc_pc;

#if 0 /* XXX */
	for (i = 0; i < KN20AA_MAX_IRQ; i++)
		TAILQ_INIT(&eb164_pci_intrs[i]);
#endif /* 0 XXX */

        pc->pc_intr_v = ccp;
        pc->pc_intr_map = dec_eb164_intr_map;
        pc->pc_intr_string = dec_eb164_intr_string;
        pc->pc_intr_establish = dec_eb164_intr_establish;
        pc->pc_intr_disestablish = dec_eb164_intr_disestablish;

#if NSIO
	sio_intr_setup(iot);
#endif

	set_iointr(eb164_iointr);

#if 0 /* XXX */
#if NSIO
	eb164_enable_intr(KN20AA_PCEB_IRQ);
#if 0 /* XXX init PCEB interrupt handler? */
	eb164_attach_intr(&eb164_pci_intrs[KN20AA_PCEB_IRQ], ???, ???, ???);
#endif
#endif
#endif /* 0 XXX */
}

int     
dec_eb164_intr_map(ccv, bustag, buspin, line, ihp)
        void *ccv;
        pcitag_t bustag; 
        int buspin, line;
        pci_intr_handle_t *ihp;
{

	printf("dec_eb164_intr_map(0x%lx, %d, %d)\n", bustag, buspin, line);
	return 0;

#if 0
	struct cia_config *ccp = ccv;
	pci_chipset_tag_t pc = &ccp->cc_pc;
	int device;
	int eb164_irq;
	void *ih;

        if (buspin == 0) {
                /* No IRQ used. */
                return 1;
        }
        if (buspin > 4) {
                printf("pci_map_int: bad interrupt pin %d\n", buspin);
                return 1;
        }

	/*
	 * Slot->interrupt translation.  Appears to work, though it
	 * may not hold up forever.
	 *
	 * The DEC engineers who did this hardware obviously engaged
	 * in random drug testing.
	 */
	pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	switch (device) {
	case 11:
	case 12:
		eb164_irq = ((device - 11) + 0) * 4;
		break;

	case 7:
		eb164_irq = 8;
		break;

	case 9:
		eb164_irq = 12;
		break;

	case 6:					/* 21040 on AlphaStation 500 */
		eb164_irq = 13;
		break;

	case 8:
		eb164_irq = 16;
		break;

	default:
#ifdef KN20AA_BOGUS_IRQ_FROB
		*ihp = 0xdeadbeef;
		printf("\n\n BOGUS INTERRUPT MAPPING: dev %d, pin %d\n",
		    device, buspin);
		return (0);
#endif
		panic("pci_eb164_map_int: invalid device number %d\n",
		    device);
	}

	eb164_irq += buspin - 1;
	if (eb164_irq > KN20AA_MAX_IRQ)
		panic("pci_eb164_map_int: eb164_irq too large (%d)\n",
		    eb164_irq);

	*ihp = eb164_irq;
	return (0);
#endif
}

const char *
dec_eb164_intr_string(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{
	struct cia_config *ccp = ccv;
        static char irqstr[15];          /* 11 + 2 + NULL + sanity */

	sprintf(irqstr, "BOGUS");
	return (irqstr);

#if 0 /* XXX */
#ifdef KN20AA_BOGUS_IRQ_FROB
	if (ih == 0xdeadbeef) {
		sprintf(irqstr, "BOGUS");
		return (irqstr);
	}
#endif
        if (ih > KN20AA_MAX_IRQ)
                panic("dec_eb164_a50_intr_string: bogus eb164 IRQ 0x%x\n",
		    ih);

        sprintf(irqstr, "eb164 irq %d", ih);
        return (irqstr);
#endif /* 0 XXX */
}

void *
dec_eb164_intr_establish(ccv, ih, level, func, arg)
        void *ccv, *arg;
        pci_intr_handle_t ih;
        int level;
        int (*func) __P((void *));
{

	printf("dec_eb164_intr_establish(0x%lx, %d, %p, %p)\n", ih, level, func, arg);
	return ((void *)0xbabefacedeadbeef);

#if 0 /* XXX */
        struct cia_config *ccp = ccv;
	void *cookie;

#ifdef KN20AA_BOGUS_IRQ_FROB
	if (ih == 0xdeadbeef) {
		int i;
		char chars[10];

		printf("dec_eb164_intr_establish: BOGUS IRQ\n");
		do {
			printf("IRQ to enable? ");
			getstr(chars, 10);
			i = atoi(chars);
		} while (i < 0 || i > 32);
		printf("ENABLING IRQ %d\n", i);
		eb164_enable_intr(i);
		return ((void *)0xbabefacedeadbeef);
	}
#endif
        if (ih > KN20AA_MAX_IRQ)
                panic("dec_eb164_intr_establish: bogus eb164 IRQ 0x%x\n",
		    ih);

	cookie = eb164_attach_intr(&eb164_pci_intrs[ih], level, func, arg);
	eb164_enable_intr(ih);
	return (cookie);
#endif /* XXX */
}

void    
dec_eb164_intr_disestablish(ccv, cookie)
        void *ccv, *cookie;
{
	struct cia_config *ccp = ccv;

	panic("dec_eb164_intr_disestablish not implemented"); /* XXX */
}

/*
 * caught a stray interrupt; notify if not too many seen already.
 */
void
eb164_pci_strayintr(irq)
	int irq;
{

	log(LOG_ERR, "stray eb164 irq %d\n", irq);

#if 0 /* XXX */
	eb164_pci_strayintrcnt[irq]++;
	if (eb164_pci_strayintrcnt[irq] == PCI_STRAY_MAX)
		eb164_disable_intr(irq);

	log(LOG_ERR, "stray eb164 irq %d\n", irq);
	if (eb164_pci_strayintrcnt[irq] == PCI_STRAY_MAX)
		log(LOG_ERR, "disabling interrupts on eb164 irq %d\n", irq);
#endif /* 0 XXX */
}

void
eb164_iointr(framep, vec)
	void *framep;
	unsigned long vec;
{
	struct eb164_intrhand *ih;
	int irq, handled;

#if 0 /* XXX */
	if (vec >= 0x900) {
		if (vec >= 0x900 + (KN20AA_MAX_IRQ << 4))
			panic("eb164_iointr: vec 0x%x out of range\n", vec);
		irq = (vec - 0x900) >> 4;

#ifdef EVCNT_COUNTERS
		eb164_intr_evcnt.ev_count++;
#else
		if (KN20AA_MAX_IRQ != INTRCNT_KN20AA_IRQ_LEN)
			panic("eb164 interrupt counter sizes inconsistent");
		intrcnt[INTRCNT_KN20AA_IRQ + irq]++;
#endif

		for (ih = eb164_pci_intrs[irq].tqh_first, handled = 0;
		    ih != NULL; ih = ih->ih_q.tqe_next) {
			int rv;

			rv = (*ih->ih_fun)(ih->ih_arg);

			ih->ih_count++;
			handled = handled || (rv != 0);
		}
		if (!handled)
			eb164_pci_strayintr(irq);
		return;
	}
	if (vec >= 0x800) {
#if NSIO
		sio_iointr(framep, vec);
#endif
		return;
	} 
#endif /* 0 XXX */
	panic("eb164_iointr: weird vec 0x%x\n", vec);
}

#if 0 /* XXX */
void
eb164_enable_intr(irq)
	int irq;
{

	/*
	 * From disassembling small bits of the OSF/1 kernel:
	 * the following appears to enable a given interrupt request.
	 * "blech."  I'd give valuable body parts for better docs or
	 * for a good decompiler.
	 */
	alpha_mb();
	REGVAL(0x8780000000L + 0x40L) |= (1 << irq);	/* XXX */
	alpha_mb();
}

void
eb164_disable_intr(irq)
	int irq;
{

	alpha_mb();
	REGVAL(0x8780000000L + 0x40L) &= ~(1 << irq);	/* XXX */
	alpha_mb();
}

struct eb164_intrhand *
eb164_attach_intr(chain, level, func, arg)
	struct eb164_intrchain *chain;
	int level;
	int (*func) __P((void *));
	void *arg;
{
	struct eb164_intrhand *nintrhand;

	nintrhand = (struct eb164_intrhand *)
	    malloc(sizeof *nintrhand, M_DEVBUF, M_WAITOK);

        nintrhand->ih_fun = func;
        nintrhand->ih_arg = arg;
        nintrhand->ih_count = 0;
        nintrhand->ih_level = level;
	TAILQ_INSERT_TAIL(chain, nintrhand, ih_q);
	
	return (nintrhand);
}
#endif /* 0 XXX */
