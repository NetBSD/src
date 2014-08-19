/* $NetBSD: pci_kn20aa.c,v 1.53.6.1 2014/08/20 00:02:41 tls Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_kn20aa.c,v 1.53.6.1 2014/08/20 00:02:41 tls Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

#include <alpha/pci/pci_kn20aa.h>

#include "sio.h"
#if NSIO > 0 || NPCEB > 0
#include <alpha/pci/siovar.h>
#endif

int	dec_kn20aa_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *dec_kn20aa_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *dec_kn20aa_intr_evcnt(void *, pci_intr_handle_t);
void	*dec_kn20aa_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *);
void	dec_kn20aa_intr_disestablish(void *, void *);

#define	KN20AA_PCEB_IRQ	31
#define	KN20AA_MAX_IRQ	32
#define	PCI_STRAY_MAX	5

struct alpha_shared_intr *kn20aa_pci_intr;

void	kn20aa_iointr(void *arg, unsigned long vec);
void	kn20aa_enable_intr(int irq);
void	kn20aa_disable_intr(int irq);

void
pci_kn20aa_pickintr(struct cia_config *ccp)
{
	int i;
#if NSIO > 0 || NPCEB > 0
	bus_space_tag_t iot = &ccp->cc_iot;
#endif
	pci_chipset_tag_t pc = &ccp->cc_pc;
	char *cp;

	pc->pc_intr_v = ccp;
	pc->pc_intr_map = dec_kn20aa_intr_map;
	pc->pc_intr_string = dec_kn20aa_intr_string;
	pc->pc_intr_evcnt = dec_kn20aa_intr_evcnt;
	pc->pc_intr_establish = dec_kn20aa_intr_establish;
	pc->pc_intr_disestablish = dec_kn20aa_intr_disestablish;

	/* Not supported on KN20AA. */
	pc->pc_pciide_compat_intr_establish = NULL;

#define PCI_KN20AA_IRQ_STR	8
	kn20aa_pci_intr = alpha_shared_intr_alloc(KN20AA_MAX_IRQ,
	    PCI_KN20AA_IRQ_STR);
	for (i = 0; i < KN20AA_MAX_IRQ; i++) {
		alpha_shared_intr_set_maxstrays(kn20aa_pci_intr, i,
		    PCI_STRAY_MAX);

		cp = alpha_shared_intr_string(kn20aa_pci_intr, i);
		snprintf(cp, PCI_KN20AA_IRQ_STR, "irq %d", i);
		evcnt_attach_dynamic(alpha_shared_intr_evcnt(
		    kn20aa_pci_intr, i), EVCNT_TYPE_INTR, NULL,
		    "kn20aa", cp);
	}

#if NSIO > 0 || NPCEB > 0
	sio_intr_setup(pc, iot);
	kn20aa_enable_intr(KN20AA_PCEB_IRQ);
#endif
}

int
dec_kn20aa_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device;
	int kn20aa_irq;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin > 4) {
		printf("dec_kn20aa_intr_map: bad interrupt pin %d\n", buspin);
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
		kn20aa_irq = ((device - 11) + 0) * 4;
		break;

	case 7:
		kn20aa_irq = 8;
		break;

	case 9:
		kn20aa_irq = 12;
		break;

	case 6:					/* 21040 on AlphaStation 500 */
		kn20aa_irq = 13;
		break;

	case 8:
		kn20aa_irq = 16;
		break;

	default:
	        printf("dec_kn20aa_intr_map: weird device number %d\n",
		    device);
	        return 1;
	}

	kn20aa_irq += buspin - 1;
	if (kn20aa_irq > KN20AA_MAX_IRQ)
		panic("dec_kn20aa_intr_map: kn20aa_irq too large (%d)",
		    kn20aa_irq);

	*ihp = kn20aa_irq;
	return (0);
}

const char *
dec_kn20aa_intr_string(void *ccv, pci_intr_handle_t ih, char *buf, size_t len)
{
#if 0
	struct cia_config *ccp = ccv;
#endif
	if (ih > KN20AA_MAX_IRQ)
	        panic("%s: bogus kn20aa IRQ 0x%lx", __func__, ih);

	snprintf(buf, len, "kn20aa irq %ld", ih);
	return buf;
}

const struct evcnt *
dec_kn20aa_intr_evcnt(void *ccv, pci_intr_handle_t ih)
{
#if 0
	struct cia_config *ccp = ccv;
#endif

	if (ih > KN20AA_MAX_IRQ)
		panic("%s: bogus kn20aa IRQ 0x%lx", __func__, ih);
	return (alpha_shared_intr_evcnt(kn20aa_pci_intr, ih));
}

void *
dec_kn20aa_intr_establish(
	void *ccv,
	pci_intr_handle_t ih,
	int level,
	int (*func)(void *),
	void *arg)
{
#if 0
	struct cia_config *ccp = ccv;
#endif
	void *cookie;

	if (ih > KN20AA_MAX_IRQ)
	        panic("dec_kn20aa_intr_establish: bogus kn20aa IRQ 0x%lx",
		    ih);

	cookie = alpha_shared_intr_establish(kn20aa_pci_intr, ih, IST_LEVEL,
	    level, func, arg, "kn20aa irq");

	if (cookie != NULL &&
	    alpha_shared_intr_firstactive(kn20aa_pci_intr, ih)) {
		scb_set(0x900 + SCB_IDXTOVEC(ih), kn20aa_iointr, NULL,
		    level);
		kn20aa_enable_intr(ih);
	}
	return (cookie);
}

void
dec_kn20aa_intr_disestablish(void *ccv, void *cookie)
{
#if 0
	struct cia_config *ccp = ccv;
#endif
	struct alpha_shared_intrhand *ih = cookie;
	unsigned int irq = ih->ih_num;
	int s;

	s = splhigh();

	alpha_shared_intr_disestablish(kn20aa_pci_intr, cookie,
	    "kn20aa irq");
	if (alpha_shared_intr_isactive(kn20aa_pci_intr, irq) == 0) {
		kn20aa_disable_intr(irq);
		alpha_shared_intr_set_dfltsharetype(kn20aa_pci_intr, irq,
		    IST_NONE);
		scb_free(0x900 + SCB_IDXTOVEC(irq));
	}

	splx(s);
}

void
kn20aa_iointr(void *arg, unsigned long vec)
{
	int irq;

	irq = SCB_VECTOIDX(vec - 0x900);

	if (!alpha_shared_intr_dispatch(kn20aa_pci_intr, irq)) {
		alpha_shared_intr_stray(kn20aa_pci_intr, irq,
		    "kn20aa irq");
		if (ALPHA_SHARED_INTR_DISABLE(kn20aa_pci_intr, irq))
			kn20aa_disable_intr(irq);
	} else
		alpha_shared_intr_reset_strays(kn20aa_pci_intr, irq);
}

void
kn20aa_enable_intr(int irq)
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
kn20aa_disable_intr(int irq)
{

	alpha_mb();
	REGVAL(0x8780000000L + 0x40L) &= ~(1 << irq);	/* XXX */
	alpha_mb();
}
